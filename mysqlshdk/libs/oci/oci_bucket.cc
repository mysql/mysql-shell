/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include <unordered_map>
#include <vector>

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlshdk {
namespace oci {

using rest::Base_response_buffer;
using rest::Headers;
using rest::Response;
using rest::Response_error;
using rest::Signed_request;
using rest::Signed_rest_service;
using rest::String_response;

const size_t MAX_LIST_OBJECTS_LIMIT = 1000;

namespace {
std::string encode_path(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_path_segment(data, false);
}

std::string encode_query(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_query(data);
}

namespace detail {

template <typename K, typename V>
void encode_json_helper(shcore::JSON_dumper *json, K &&key, V &&value) {
  json->append(std::forward<K>(key), std::forward<V>(value));
}

template <typename K, typename V, typename... Args>
void encode_json_helper(shcore::JSON_dumper *json, K &&key, V &&value,
                        Args &&... args) {
  encode_json_helper(json, std::forward<K>(key), std::forward<V>(value));
  encode_json_helper(json, std::forward<Args>(args)...);
}

}  // namespace detail

template <typename... Args>
std::string encode_json(Args &&... args) {
  shcore::JSON_dumper json;
  json.start_object();
  detail::encode_json_helper(&json, std::forward<Args>(args)...);
  json.end_object();
  return json.str();
}

}  // namespace

Oci_bucket::Oci_bucket(const Oci_bucket_config_ptr &config)
    : storage::backend::object_storage::Bucket(config),
      m_config(config),
      kNamespacePath{shcore::str_format(
          "/n/%s/b/", encode_path(config->oci_namespace()).c_str())},
      kBucketPath{kNamespacePath + encode_path(config->bucket_name()) + "/"},
      kListObjectsPath{kBucketPath + "o"},
      kObjectActionFormat{kBucketPath + "o/%s"},
      kRenameObjectPath{kBucketPath + "actions/renameObject"},
      kMultipartActionPath{kBucketPath + "u"},
      kUploadPartFormat{kBucketPath + "u/%s?uploadId=%s&uploadPartNum=%zi"},
      kMultipartActionFormat{kBucketPath + "u/%s?uploadId=%s"},
      kParActionPath{kBucketPath + "p/"} {}

rest::Signed_request Oci_bucket::list_objects_request(
    const std::string &prefix, size_t limit, bool recursive,
    const Object_details::Fields_mask &fields, const std::string &start_from) {
  std::vector<std::string> parameters;

  if (!prefix.empty()) {
    parameters.emplace_back("prefix=" + encode_query(prefix));
  }

  if (limit) {
    parameters.emplace_back("limit=" + std::to_string(limit));
  }

  if (!recursive) parameters.emplace_back("delimiter=/");

  if (Object_details::NAME != fields) {
    // If more fields than 'name' are included then it's needed to send the
    // field list
    std::vector<std::string> include_fields;

    if (fields.is_set(Object_details::Fields::NAME)) {
      include_fields.emplace_back("name");
    }

    if (fields.is_set(Object_details::Fields::SIZE)) {
      include_fields.emplace_back("size");
    }

    if (fields.is_set(Object_details::Fields::ETAG)) {
      include_fields.emplace_back("etag");
    }

    if (fields.is_set(Object_details::Fields::TIME_CREATED)) {
      include_fields.emplace_back("timeCreated");
    }

    parameters.emplace_back("fields=" + shcore::str_join(include_fields, ","));
  }

  if (!start_from.empty()) {
    parameters.emplace_back("start=" + encode_query(start_from));
  }

  auto path = kListObjectsPath;

  if (!parameters.empty()) {
    path += '?';
    path += shcore::str_join(parameters, "&");
  }

  return Signed_request(std::move(path));
}

std::vector<Object_details> Oci_bucket::parse_list_objects(
    const rest::Base_response_buffer &buffer, std::string *next_start_from,
    std::unordered_set<std::string> *out_prefixes) {
  std::vector<Object_details> objects;
  const auto data = shcore::Value::parse(buffer.data(), buffer.size()).as_map();

  for (const auto &value : *data->get_array("objects")) {
    const auto object = value.as_map();
    Object_details details;

    details.name = object->get_string("name");
    details.size = static_cast<size_t>(object->get_uint("size"));
    details.etag = object->get_string("etag");
    details.time_created = object->get_string("timeCreated");

    objects.emplace_back(std::move(details));
  }

  *next_start_from = data->get_string("nextStartWith");

  if (out_prefixes) {
    const auto prefixes = data->get_array("prefixes");

    if (prefixes) {
      for (const auto &prefix : *prefixes) {
        out_prefixes->emplace(prefix.as_string());
      }
    }
  }

  return objects;
}

rest::Signed_request Oci_bucket::head_object_request(
    const std::string &object_name) {
  return create_request(object_name);
}

rest::Signed_request Oci_bucket::delete_object_request(
    const std::string &object_name) {
  return create_request(object_name, {{"accept", "*/*"}});
}

rest::Signed_request Oci_bucket::put_object_request(
    const std::string &object_name, rest::Headers headers) {
  return create_request(object_name, std::move(headers));
}

rest::Signed_request Oci_bucket::get_object_request(
    const std::string &object_name, rest::Headers headers) {
  return create_request(object_name, std::move(headers));
}

void Oci_bucket::execute_rename_object(Signed_rest_service *service,
                                       const std::string &source_name,
                                       const std::string &new_name) {
  // Rename Object Details
  std::string rod = encode_json("sourceName", source_name, "newName", new_name);

  auto request = Signed_request(kRenameObjectPath);
  request.body = rod.data();
  request.size = rod.size();
  service->post(&request);
}

rest::Signed_request Oci_bucket::list_multipart_uploads_request(size_t limit) {
  auto path = kMultipartActionPath;
  if (limit) path.append("?limit=" + std::to_string(limit));
  return Signed_request(std::move(path));
}

std::vector<Multipart_object> Oci_bucket::parse_list_multipart_uploads(
    const mysqlshdk::rest::Base_response_buffer &buffer) {
  std::vector<Multipart_object> objects;
  const auto uploads =
      shcore::Value::parse(buffer.data(), buffer.size()).as_array();

  for (const auto &value : *uploads) {
    const auto object = value.as_map();
    objects.push_back(
        {object->get_string("object"), object->get_string("uploadId")});
  }

  return objects;
}

rest::Signed_request Oci_bucket::list_multipart_uploaded_parts_request(
    const Multipart_object &object, size_t limit) {
  auto path = kMultipartActionPath + "/" + encode_path(object.name);
  path.append("?uploadId=" + object.upload_id);
  if (limit) path.append("&limit=" + std::to_string(limit));
  return Signed_request(std::move(path));
}

std::vector<Multipart_object_part>
Oci_bucket::parse_list_multipart_uploaded_parts(
    const rest::Base_response_buffer &buffer) {
  std::vector<Multipart_object_part> parts;
  const auto uploads =
      shcore::Value::parse(buffer.data(), buffer.size()).as_array();

  for (const auto &value : *uploads) {
    const auto part = value.as_map();
    parts.push_back({static_cast<size_t>(part->get_uint("partNumber")),
                     part->get_string("etag"),
                     static_cast<size_t>(part->get_uint("size"))});
  }

  return parts;
}

rest::Signed_request Oci_bucket::create_multipart_upload_request(
    const std::string &object_name, std::string *request_body) {
  *request_body = encode_json("object", object_name);
  return Signed_request(kMultipartActionPath);
}

std::string Oci_bucket::parse_create_multipart_upload(
    const rest::Base_response_buffer &buffer) {
  return shcore::Value::parse(buffer.data(), buffer.size())
      .as_map()
      ->get_string("uploadId");
}

rest::Signed_request Oci_bucket::upload_part_request(
    const Multipart_object &object, size_t part_num) {
  return Signed_request{shcore::str_format(kUploadPartFormat.c_str(),
                                           encode_path(object.name).c_str(),
                                           object.upload_id.c_str(), part_num)};
}

rest::Signed_request Oci_bucket::commit_multipart_upload_request(
    const Multipart_object &object,
    const std::vector<Multipart_object_part> &parts,
    std::string *request_body) {
  shcore::Array_t etags = shcore::make_array();

  for (const auto &part : parts) {
    etags->emplace_back(shcore::make_dict(
        "partNum", static_cast<uint64_t>(part.part_num), "etag", part.etag));
  }

  *request_body = encode_json("partsToCommit", std::move(etags));

  return Signed_request{shcore::str_format(kMultipartActionFormat.c_str(),
                                           encode_path(object.name).c_str(),
                                           object.upload_id.c_str())};
}

rest::Signed_request Oci_bucket::abort_multipart_upload_request(
    const Multipart_object &object) {
  return Signed_request{shcore::str_format(kMultipartActionFormat.c_str(),
                                           encode_path(object.name).c_str(),
                                           object.upload_id.c_str())};
}

PAR Oci_bucket::create_pre_authenticated_request(
    PAR_access_type access_type, const std::string &expiration_time,
    const std::string &par_name, const std::string &object_name,
    std::optional<PAR_list_action> list_action) {
  // Ensures the REST connection is established
  const auto service = ensure_connection();

  std::string access_type_str = to_string(access_type);

  std::string par_list_action_str;
  if (list_action.has_value()) {
    par_list_action_str = to_string(list_action.value());
  }

  // TODO(rennox): Formatting of the request bodies on all the functions should
  // be updated to use the shcore::Json_dumper
  std::string body = "{\"accessType\":\"" + access_type_str +
                     "\", \"name\":\"" + par_name + "\"";

  // For the OBJECT* access types, the name is mandatory and it is set as
  // provided, for the ANY_OBJECT* access types, it is optional and it is set
  // only if not empty
  bool set_object_name = access_type == PAR_access_type::OBJECT_READ ||
                         access_type == PAR_access_type::OBJECT_WRITE ||
                         access_type == PAR_access_type::OBJECT_READ_WRITE ||
                         !object_name.empty();

  if (set_object_name) {
    body.append(", \"objectName\":\"" + object_name + "\"");
  }

  if (!par_list_action_str.empty()) {
    body.append(", \"bucketListingAction\":\"" + par_list_action_str + "\"");
  }

  body.append(", \"timeExpires\":\"" + expiration_time + "\"}");

  rest::String_response response;
  const auto &data = response.buffer;

  std::string target;
  if (object_name.empty()) {
    target = "for bucket " + m_config->bucket_name();
  } else {
    target = "for object " + object_name;
  }

  std::string error_msg = shcore::str_format(
      "Failed to create %s PAR %s: ", access_type_str.c_str(), target.c_str());
  try {
    auto request = Signed_request(kParActionPath);
    request.body = body.data();
    request.size = body.size();
    service->post(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), error_msg + error.what());
  }

  shcore::Dictionary_t map;
  try {
    map = shcore::Value::parse(data.data(), data.size()).as_map();
  } catch (const shcore::Exception &error) {
    error_msg += error.what();

    log_debug2("%s\n%.*s", error_msg.c_str(), static_cast<int>(data.size()),
               data.data());

    throw shcore::Exception::runtime_error(error_msg);
  }

  return {map->get_string("id"),
          map->get_string("name"),
          map->get_string("accessUri"),
          map->get_string("accessType"),
          set_object_name ? map->get_string("objectName") : "",
          map->get_string("timeCreated"),
          map->get_string("timeExpires"),
          map->is_null("bucketListingAction")
              ? ""
              : map->get_string("bucketListingAction"),
          0};
}

void Oci_bucket::delete_pre_authenticated_request(const std::string &id) {
  // Ensures the REST connection is established
  const auto service = ensure_connection();

  try {
    auto request = Signed_request(kParActionPath + id);
    service->delete_(&request);
  } catch (const Response_error &error) {
    throw Response_error(
        error.code(),
        shcore::str_format("Failed to delete PAR: %s", error.what()));
  }
}

std::vector<PAR> Oci_bucket::list_preauthenticated_requests(
    const std::string &prefix, size_t limit, const std::string &page) {
  // Ensures the REST connection is established
  const auto service = ensure_connection();

  std::vector<std::string> parameters;

  if (!prefix.empty())
    parameters.emplace_back("prefix=" + encode_query(prefix));

  // Only sets the limit when the request will be satisfied in one call,
  // otherwise the limit will be set after the necessary calls to fulfill the
  // limit request
  if (limit && limit <= MAX_LIST_OBJECTS_LIMIT) {
    parameters.emplace_back("limit=" + std::to_string(limit));
  }

  if (!page.empty()) parameters.emplace_back("page=" + encode_query(page));

  auto path = kParActionPath;
  if (!parameters.empty()) {
    path.append("?" + shcore::str_join(parameters, "&"));
  }

  std::vector<PAR> list;

  std::string msg("Failed to get preauthenticated request list");
  if (!prefix.empty()) msg.append(" using prefix '" + prefix + "'");

  // TODO(rennox): Pagiing loop?
  //  while (!done) {
  rest::String_response response;
  const auto &raw_data = response.buffer;

  try {
    auto request = Signed_request(path);
    service->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), msg + ": " + error.what());
  }

  shcore::Array_t data;
  try {
    data = shcore::Value::parse(raw_data.data(), raw_data.size()).as_array();
  } catch (const shcore::Exception &error) {
    msg.append(": ").append(error.what());

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
               raw_data.data());

    throw shcore::Exception::runtime_error(msg);
  }

  for (auto &value : *data) {
    PAR details;
    auto object = value.as_map();

    details.access_type = object->get_string("accessType");
    details.id = object->get_string("id");
    details.name = object->get_string("name");
    if (!object->is_null("objectName"))
      details.object_name = object->get_string("objectName");
    details.time_created = object->get_string("timeCreated");
    details.time_expires = object->get_string("timeExpires");

    list.push_back(details);
  }

  return list;
}

bool Oci_bucket::exists() {
  // Ensures the REST connection is established
  const auto service = ensure_connection();

  try {
    auto request = Signed_request(kBucketPath);
    service->head(&request);
    return true;
  } catch (const Response_error &error) {
    if (rest::Response::Status_code::NOT_FOUND == error.code()) {
      return false;
    } else {
      throw;
    }
  }
}

void Oci_bucket::create(const std::string &compartment_id) {
  // Ensures the REST connection is established
  const auto service = ensure_connection();

  // CreateBucketDetails
  const auto body = encode_json("compartmentId", compartment_id, "name",
                                encode_path(m_config->bucket_name()));

  auto request = Signed_request(kNamespacePath);
  request.body = body.c_str();
  request.size = body.size();
  service->post(&request);
}

void Oci_bucket::delete_() {
  // Ensures the REST connection is established
  const auto service = ensure_connection();

  auto request = Signed_request(kBucketPath);
  request.body = "";
  request.size = 0;
  service->delete_(&request);
}

Signed_request Oci_bucket::create_request(const std::string &object_name,
                                          Headers headers) const {
  return Signed_request(shcore::str_format(kObjectActionFormat.c_str(),
                                           encode_path(object_name).c_str()),
                        std::move(headers));
}

}  // namespace oci
}  // namespace mysqlshdk
