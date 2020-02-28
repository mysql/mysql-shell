/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/oci/oci_bucket.h"
#include "mysqlshdk/libs/db/uri_encoder.h"

namespace mysqlshdk {
namespace oci {

const size_t MAX_LIST_OBJECTS_LIMIT = 1000;

namespace {
std::string encode_path(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_path_segment(data);
}

std::string encode_query(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_query(data);
}

}  // namespace

Bucket::Bucket(const Oci_options &options)
    : m_options(options),
      kBucketPath{shcore::str_format(
          "/n/%s/b/%s", encode_path(*m_options.os_namespace).c_str(),
          encode_path(*m_options.os_bucket_name).c_str())},
      kListObjectsPath{kBucketPath + "/o"},
      kObjectActionFormat{kBucketPath + "/o/%s"},
      kRenameObjectPath{kBucketPath + "/actions/renameObject"},
      kMultipartActionPath{kBucketPath + "/u"},
      kUploadPartFormat{kBucketPath + "/u/%s?uploadId=%s&uploadPartNum=%zi"},
      kMultipartActionFormat{kBucketPath + "/u/%s?uploadId=%s"} {
  // Now update the REST service to point to the Object Store end point
  m_rest_service =
      std::make_shared<Oci_rest_service>(Oci_service::OBJECT_STORAGE, options);

  std::string raw_data;
  m_rest_service->get(kBucketPath, {}, &raw_data);
  auto data = shcore::Value::parse(raw_data).as_map();

  m_compartment_id = data->get_string("compartmentId");
  m_etag = data->get_string("etag");
  m_is_read_only = data->get_bool("isReadOnly");
  m_access_type = data->get_string("publicAccessType");
}

std::vector<Object_details> Bucket::list_objects(
    const std::string &prefix, const std::string &start, const std::string &end,
    size_t limit, bool recursive, const Object_fields_mask &fields,
    std::vector<std::string> *out_prefixes, std::string *out_nextStart) {
  const std::string start_place_holder{"\nstart\nplace\nholder\n"};

  // If not all fields are included then it's needed to send the field list
  std::vector<std::string> include_fields;
  if (fields != object_fields::kName) {
    if (fields.is_set(Object_fields::NAME)) include_fields.push_back("name");
    if (fields.is_set(Object_fields::MD5)) include_fields.push_back("md5");
    if (fields.is_set(Object_fields::ETAG)) include_fields.push_back("etag");
    if (fields.is_set(Object_fields::SIZE)) include_fields.push_back("size");
    if (fields.is_set(Object_fields::TIME_CREATED))
      include_fields.push_back("timeCreated");
  }

  std::vector<std::string> parameters;

  if (!prefix.empty())
    parameters.emplace_back("prefix=" + encode_query(prefix));
  bool has_start = false;
  std::string next_start;
  if (!start.empty()) {
    parameters.emplace_back("start=" + start_place_holder);
    has_start = true;
    next_start = start;
  }
  if (!end.empty()) parameters.emplace_back("end=" + encode_query(end));

  // Only sets the limit when the request will be satisfied in one call,
  // otherwise the limit will be set after the necessary calls to fulfull the
  // limit request
  bool has_limit = false;
  if (limit && limit <= MAX_LIST_OBJECTS_LIMIT) {
    parameters.emplace_back("limit=" + std::to_string(limit));
    has_limit = true;
  }

  if (!recursive) parameters.emplace_back("delimiter=/");
  if (!fields.empty())
    parameters.emplace_back("fields=" + shcore::str_join(include_fields, ","));

  auto path = kListObjectsPath;
  path.append("?" + shcore::str_join(parameters, "&"));

  bool done = false;

  std::vector<Object_details> list;

  while (!done) {
    std::string real_path = has_start
                                ? shcore::str_replace(path, start_place_holder,
                                                      encode_query(next_start))
                                : path;

    std::string raw_data;
    m_rest_service->get(real_path, {}, &raw_data);
    auto data = shcore::Value::parse(raw_data).as_map();

    auto objects = data->get_array("objects");
    for (auto &value : *objects) {
      Object_details details;
      auto object = value.as_map();

      if (fields.is_set(Object_fields::NAME))
        details.name = object->get_string("name");
      if (fields.is_set(Object_fields::MD5))
        details.md5 = object->get_string("md5");
      if (fields.is_set(Object_fields::ETAG))
        details.etag = object->get_string("etag");
      if (fields.is_set(Object_fields::SIZE))
        details.size = static_cast<size_t>(object->get_uint("size"));
      if (fields.is_set(Object_fields::TIME_CREATED))
        details.time_created = object->get_string("timeCreated");

      list.push_back(details);
    }

    if (out_prefixes && data->has_key("prefixes")) {
      auto prefixes = data->get_array("prefixes");
      for (const auto &aprefix : *prefixes) {
        out_prefixes->push_back(aprefix.as_string());
      }
    }

    // The presense of nextStartWith indicates there are still more
    // object names to retrieve
    if (data->has_key("nextStartWith")) {
      next_start = data->get_string("nextStartWith");
      if (out_nextStart) *out_nextStart = next_start;
      if (!has_start) {
        if (parameters.empty() && !has_limit) {
          path += "?start=" + start_place_holder;
        } else {
          path += "&start=" + start_place_holder;
        }
      }

      if (limit) {
        size_t remaining = limit - list.size();
        if (remaining == 0) {
          done = true;
        } else {
          if (remaining <= MAX_LIST_OBJECTS_LIMIT) {
            if (parameters.empty() && !has_start) {
              path += "?limit=" + std::to_string(remaining);
            } else {
              path += "&limit=" + std::to_string(remaining);
            }
          }
        }
      }
    } else {
      if (out_nextStart) *out_nextStart = "";
      done = true;
    }
  }

  return list;
}

void Bucket::put_object(const std::string &objectName, const char *body,
                        size_t size, bool override) {
  Headers headers{{"content-type", "application/octet-stream"}};

  if (!override) {
    headers["if-none-match"] = "*";
  }

  m_rest_service->put(shcore::str_format(kObjectActionFormat.c_str(),
                                         encode_path(objectName).c_str()),
                      body, size, headers);
}

void Bucket::delete_object(const std::string &objectName) {
  m_rest_service->delete_(shcore::str_format(kObjectActionFormat.c_str(),
                                             encode_path(objectName).c_str()),
                          nullptr, 0L, {{"accept", "*/*"}});
}

size_t Bucket::head_object(const std::string &objectName) {
  Headers response_headers;
  m_rest_service->head(shcore::str_format(kObjectActionFormat.c_str(),
                                          encode_path(objectName).c_str()),
                       {}, nullptr, &response_headers);

  return std::stoul(response_headers.at("content-length"));
}

size_t Bucket::get_object(const std::string &objectName, void *buffer,
                          const mysqlshdk::utils::nullable<size_t> &from_byte,
                          const mysqlshdk::utils::nullable<size_t> &to_byte) {
  Headers headers;
  size_t read_size = 0;
  if (!from_byte.is_null() || !to_byte.is_null()) {
    headers["range"] = shcore::str_format(
        "bytes=%s-%s",
        (from_byte.is_null() ? "" : std::to_string(*from_byte).c_str()),
        (to_byte.is_null() ? "" : std::to_string(*to_byte).c_str()));
  }

  std::string content;
  m_rest_service->get(shcore::str_format(kObjectActionFormat.c_str(),
                                         encode_path(objectName).c_str()),
                      headers, &content);

  read_size = content.size();
  std::copy(content.data(), content.data() + content.size(),
            reinterpret_cast<uint8_t *>(buffer));

  return read_size;
}

void Bucket::rename_object(const std::string &sourceName,
                           const std::string &newName) {
  std::string fmt =
      "{"
      "\"sourceName\":\"%s\","
      "\"newName\":\"%s\""
      "}";

  // Rename Object Details
  std::string rod =
      shcore::str_format(fmt.c_str(), sourceName.c_str(), newName.c_str());

  m_rest_service->post(kRenameObjectPath, rod.data(), rod.size());
}

std::vector<Multipart_object> Bucket::list_multipart_uploads(size_t limit) {
  std::vector<Multipart_object> objects;

  auto path = kMultipartActionPath;
  if (limit) path.append("?limit=" + std::to_string(limit));

  std::string raw_data;
  m_rest_service->get(path, {}, &raw_data);

  auto data = shcore::Value::parse(raw_data).as_array();
  for (const auto &value : *data) {
    const auto &object(value.as_map());
    objects.push_back(
        {object->get_string("object"), object->get_string("uploadId")});
  }

  return objects;
}

std::vector<Multipart_object_part> Bucket::list_multipart_upload_parts(
    const Multipart_object &object, size_t limit) {
  std::vector<Multipart_object_part> parts;

  auto path = kMultipartActionPath + "/" + encode_path(object.name);
  path.append("?uploadId=" + object.upload_id);
  if (limit) path.append("&limit=" + std::to_string(limit));

  std::string raw_data;
  m_rest_service->get(path, {}, &raw_data);

  auto data = shcore::Value::parse(raw_data).as_array();
  for (const auto &value : *data) {
    const auto &part(value.as_map());
    parts.push_back({static_cast<size_t>(part->get_uint("partNumber")),
                     part->get_string("etag"),
                     static_cast<size_t>(part->get_uint("size"))});
  }

  return parts;
}

Multipart_object Bucket::create_multipart_upload(
    const std::string &objectName) {
  std::string upload_id;
  std::string body{"{\"object\":\"" + objectName + "\"}"};

  std::string data;
  m_rest_service->post(kMultipartActionPath, body.c_str(), body.size(), {},
                       &data);

  auto map = shcore::Value::parse(data).as_map();
  upload_id = map->get_string("uploadId");

  return {objectName, upload_id};
}

Multipart_object_part Bucket::upload_part(const Multipart_object &object,
                                          size_t partNum, const char *body,
                                          size_t size) {
  auto path = shcore::str_format(kUploadPartFormat.c_str(),
                                 encode_path(object.name).c_str(),
                                 object.upload_id.c_str(), partNum);

  Headers response_headers;
  m_rest_service->put(path, body, size, {}, nullptr, &response_headers);

  return {partNum, response_headers.at("ETag"), size};
}

void Bucket::commit_multipart_upload(
    const Multipart_object &object,
    const std::vector<Multipart_object_part> &parts) {
  std::vector<std::string> etags;
  for (const auto &part : parts) {
    etags.push_back("{\"partNum\":" + std::to_string(part.part_num) +
                    ",\"etag\":\"" + part.etag + "\"}");
  }

  std::string body =
      "{\"partsToCommit\":[" + shcore::str_join(etags, ",") + "]}";

  auto path = shcore::str_format(kMultipartActionFormat.c_str(),
                                 object.name.c_str(), object.upload_id.c_str());
  m_rest_service->post(path, body.c_str(), body.size());
}

void Bucket::abort_multipart_upload(const Multipart_object &object) {
  auto path = shcore::str_format(kMultipartActionFormat.c_str(),
                                 encode_path(object.name).c_str(),
                                 object.upload_id.c_str());
  m_rest_service->delete_(path, nullptr, 0L);
}
}  // namespace oci
}  // namespace mysqlshdk
