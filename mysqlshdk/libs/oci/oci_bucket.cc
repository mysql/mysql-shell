/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include <algorithm>
#include <regex>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlshdk {
namespace oci {

const size_t MAX_LIST_OBJECTS_LIMIT = 1000;

FI_DEFINE(oci_put_object, ([](const mysqlshdk::utils::FI::Args &args) {
            throw mysqlshdk::oci::Response_error(
                static_cast<mysqlshdk::oci::Response::Status_code>(
                    args.get_int("code")),
                args.get_string("msg"));
          }));

namespace {
std::string encode_path(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_path_segment(data, false);
}

std::string encode_query(const std::string &data) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return encoder.encode_query(data);
}

/**
 * Returns true if the given url is a pre-authenticated request
 */
const std::regex k_par_regex("^\\/p\\/.+\\/n\\/.+\\/b\\/.+\\/o\\/.+$");
bool is_par(const std::string &url) {
  return std::regex_match(url, k_par_regex);
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

const std::string &object_name_for_log(const std::string &object_name,
                                       const Oci_request &request) {
  return request.is_par_request ? request.path().masked() : object_name;
}

}  // namespace

std::string to_string(PAR_access_type access_type) {
  std::string access_type_str;

  switch (access_type) {
    case PAR_access_type::OBJECT_READ:
      access_type_str = "ObjectRead";
      break;
    case PAR_access_type::OBJECT_WRITE:
      access_type_str = "ObjectWrite";
      break;
    case PAR_access_type::OBJECT_READ_WRITE:
      access_type_str = "ObjectReadWrite";
      break;
    case PAR_access_type::ANY_OBJECT_READ:
      access_type_str = "AnyObjectRead";
      break;
    case PAR_access_type::ANY_OBJECT_WRITE:
      access_type_str = "AnyObjectWrite";
      break;
    case PAR_access_type::ANY_OBJECT_READ_WRITE:
      access_type_str = "AnyObjectReadWrite";
      break;
  }

  return access_type_str;
}

std::string to_string(PAR_list_action list_action) {
  std::string par_list_action_str;
  switch (list_action) {
    case PAR_list_action::DENY:
      par_list_action_str = "Deny";
      break;
    case PAR_list_action::LIST_OBJECTS:
      par_list_action_str = "ListObjects";
      break;
  }

  return par_list_action_str;
}

std::string hide_par_secret(const std::string &par, std::size_t start_at) {
  const auto p = par.find("/p/", start_at);
  // if p is std::string::npos, this will simply return std::string::npos
  const auto n = par.find("/n/", p);

  if (std::string::npos == p || std::string::npos == n) {
    throw std::logic_error("This is not a PAR: " + par);
  }

  return par.substr(0, p + 3) + "<secret>" + par.substr(n);
}

Bucket::Bucket(const Oci_options &options)
    : m_options(options),
      kNamespacePath{shcore::str_format(
          "/n/%s/b/", encode_path(*m_options.os_namespace).c_str())},
      kBucketPath{kNamespacePath + encode_path(*m_options.os_bucket_name) +
                  "/"},
      kListObjectsPath{kBucketPath + "o"},
      kObjectActionFormat{kBucketPath + "o/%s"},
      kRenameObjectPath{kBucketPath + "actions/renameObject"},
      kMultipartActionPath{kBucketPath + "u"},
      kUploadPartFormat{kBucketPath + "u/%s?uploadId=%s&uploadPartNum=%zi"},
      kMultipartActionFormat{kBucketPath + "u/%s?uploadId=%s"},
      kParActionPath{kBucketPath + "p/"} {}

void Bucket::ensure_connection() {
  static thread_local std::unordered_map<std::string,
                                         std::unique_ptr<Oci_rest_service>>
      services;

  if (!m_rest_service) {
    auto &service = services[m_options.get_hash()];

    if (!service) {
      service = std::make_unique<Oci_rest_service>(Oci_service::OBJECT_STORAGE,
                                                   m_options);
    }

    m_rest_service = service.get();
  }
}

std::vector<Object_details> Bucket::list_objects(
    const std::string &prefix, const std::string &start, const std::string &end,
    size_t limit, bool recursive, const Object_fields_mask &fields,
    std::vector<std::string> *out_prefixes, std::string *out_nextStart) {
  // Ensures the REST connection is established
  ensure_connection();

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
  // otherwise the limit will be set after the necessary calls to fulfill the
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

  std::string msg("Failed to get object list");
  if (!prefix.empty()) msg.append(" using prefix '" + prefix + "'");

  while (!done) {
    std::string real_path = has_start
                                ? shcore::str_replace(path, start_place_holder,
                                                      encode_query(next_start))
                                : path;
    rest::String_response response;
    const auto &raw_data = response.buffer;

    try {
      auto request = Oci_request(real_path);
      m_rest_service->get(&request, &response);
    } catch (const Response_error &error) {
      throw Response_error(error.code(), msg + ": " + error.what());
    }

    shcore::Dictionary_t data;
    try {
      data = shcore::Value::parse(raw_data.data(), raw_data.size()).as_map();
    } catch (const shcore::Exception &error) {
      msg.append(": ").append(error.what());

      log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
                 raw_data.data());

      throw shcore::Exception::runtime_error(msg);
    }

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

      list.emplace_back(std::move(details));
    }

    if (out_prefixes && data->has_key("prefixes")) {
      auto prefixes = data->get_array("prefixes");
      for (const auto &aprefix : *prefixes) {
        out_prefixes->push_back(aprefix.as_string());
      }
    }

    // The presence of nextStartWith indicates there are still more object names
    // to retrieve
    if (data->has_key("nextStartWith")) {
      next_start = data->get_string("nextStartWith");
      if (out_nextStart) *out_nextStart = next_start;
      if (!has_start) {
        if (parameters.empty() && !has_limit) {
          path += "?start=" + start_place_holder;
        } else {
          path += "&start=" + start_place_holder;
        }
        has_start = true;
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

void Bucket::put_object(const std::string &object_name, const char *body,
                        size_t size, bool override) {
  // Ensures the REST connection is established
  ensure_connection();

  Headers headers{{"content-type", "application/octet-stream"}};

  if (!override) {
    headers["if-none-match"] = "*";
  }

  auto request = create_request(object_name, std::move(headers));
  request.body = body;
  request.size = size;

  try {
    FI_TRIGGER_TRAP(oci_put_object, mysqlshdk::utils::FI::Trigger_options(
                                        {{"name", object_name}}));

    m_rest_service->put(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to put object '" +
                             object_name_for_log(object_name, request) +
                             "': " + error.what());
  }
}

void Bucket::delete_object(const std::string &object_name) {
  // Ensures the REST connection is established
  ensure_connection();

  Headers headers;
  const auto par = is_par(object_name);

  if (par) {
    headers.emplace("content-type", "application/octet-stream");
  } else {
    headers.emplace("accept", "*/*");
  }

  auto request = create_request(object_name, std::move(headers));

  try {
    if (par) {
      // truncate the file, a PAR URL does not support DELETE
      char body = 0;
      request.body = &body;
      request.size = 0;

      m_rest_service->put(&request);
    } else {
      m_rest_service->delete_(&request);
    }
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to delete object '" +
                             object_name_for_log(object_name, request) +
                             "': " + error.what());
  }
}

size_t Bucket::head_object(const std::string &object_name) {
  // Ensures the REST connection is established
  ensure_connection();

  auto request = create_request(object_name);
  Response response;

  try {
    m_rest_service->head(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to get summary for object '" +
                             object_name_for_log(object_name, request) +
                             "': " + error.what());
  }

  return std::stoul(response.headers.at("content-length"));
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer,
                          const mysqlshdk::utils::nullable<size_t> &from_byte,
                          const mysqlshdk::utils::nullable<size_t> &to_byte) {
  // Ensures the REST connection is established
  ensure_connection();

  Headers headers;
  std::string range;

  if (!from_byte.is_null() || !to_byte.is_null()) {
    range = shcore::str_format(
        "bytes=%s-%s",
        (from_byte.is_null() ? "" : std::to_string(*from_byte).c_str()),
        (to_byte.is_null() ? "" : std::to_string(*to_byte).c_str()));
    headers["range"] = range;
  }

  auto request = create_request(object_name, std::move(headers));

  try {
    Response response;
    response.body = buffer;
    m_rest_service->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to get object '" +
                             object_name_for_log(object_name, request) + "'" +
                             (range.empty() ? "" : " [" + range + "]") + ": " +
                             error.what());
  }

  return buffer->size();
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer,
                          size_t from_byte, size_t to_byte) {
  return get_object(object_name, buffer,
                    mysqlshdk::utils::nullable<size_t>{from_byte},
                    mysqlshdk::utils::nullable<size_t>{to_byte});
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer,
                          size_t from_byte) {
  return get_object(object_name, buffer,
                    mysqlshdk::utils::nullable<size_t>{from_byte},
                    mysqlshdk::utils::nullable<size_t>{});
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer) {
  return get_object(object_name, buffer, mysqlshdk::utils::nullable<size_t>{},
                    mysqlshdk::utils::nullable<size_t>{});
}

void Bucket::rename_object(const std::string &source_name,
                           const std::string &new_name) {
  // Ensures the REST connection is established
  ensure_connection();

  // Rename Object Details
  std::string rod = encode_json("sourceName", source_name, "newName", new_name);

  try {
    auto request = Oci_request(kRenameObjectPath);
    request.body = rod.data();
    request.size = rod.size();
    m_rest_service->post(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), "Failed to rename object '" +
                                           source_name + "' to '" + new_name +
                                           "': " + error.what());
  }
}

std::vector<Multipart_object> Bucket::list_multipart_uploads(size_t limit) {
  // Ensures the REST connection is established
  ensure_connection();

  std::vector<Multipart_object> objects;

  auto path = kMultipartActionPath;
  if (limit) path.append("?limit=" + std::to_string(limit));

  rest::String_response response;
  const auto &raw_data = response.buffer;

  std::string msg("Failed to list multipart uploads: ");
  try {
    auto request = Oci_request(path);
    m_rest_service->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), msg + error.what());
  }

  shcore::Array_t data;
  try {
    data = shcore::Value::parse(raw_data.data(), raw_data.size()).as_array();
  } catch (const shcore::Exception &error) {
    msg.append(error.what());

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
               raw_data.data());

    throw shcore::Exception::runtime_error(msg);
  }

  for (const auto &value : *data) {
    const auto &object(value.as_map());
    objects.push_back(
        {object->get_string("object"), object->get_string("uploadId")});
  }

  return objects;
}

std::vector<Multipart_object_part> Bucket::list_multipart_upload_parts(
    const Multipart_object &object, size_t limit) {
  // Ensures the REST connection is established
  ensure_connection();

  std::vector<Multipart_object_part> parts;

  auto path = kMultipartActionPath + "/" + encode_path(object.name);
  path.append("?uploadId=" + object.upload_id);
  if (limit) path.append("&limit=" + std::to_string(limit));

  rest::String_response response;
  const auto &raw_data = response.buffer;

  std::string msg("Failed to list uploaded parts for object '" + object.name +
                  "': ");
  try {
    auto request = Oci_request(path);
    m_rest_service->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), msg + error.what());
  }

  shcore::Array_t data;
  try {
    data = shcore::Value::parse(raw_data.data(), raw_data.size()).as_array();
  } catch (const shcore::Exception &error) {
    msg.append(error.what());

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
               raw_data.data());

    throw shcore::Exception::runtime_error(msg);
  }

  for (const auto &value : *data) {
    const auto &part(value.as_map());
    parts.push_back({static_cast<size_t>(part->get_uint("partNumber")),
                     part->get_string("etag"),
                     static_cast<size_t>(part->get_uint("size"))});
  }

  return parts;
}

Multipart_object Bucket::create_multipart_upload(
    const std::string &object_name) {
  // Ensures the REST connection is established
  ensure_connection();

  std::string body = encode_json("object", object_name);
  std::string upload_id;

  rest::String_response response;
  const auto &data = response.buffer;
  std::string msg("Failed to create a multipart upload for object '" +
                  object_name + "': ");
  try {
    auto request = Oci_request(kMultipartActionPath);
    request.body = body.c_str();
    request.size = body.size();
    m_rest_service->post(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), msg + error.what());
  }

  shcore::Dictionary_t map;
  try {
    map = shcore::Value::parse(data.data(), data.size()).as_map();
  } catch (const shcore::Exception &error) {
    msg.append(error.what());

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(data.size()),
               data.data());

    throw shcore::Exception::runtime_error(msg);
  }

  upload_id = map->get_string("uploadId");

  return {object_name, upload_id};
}

Multipart_object_part Bucket::upload_part(const Multipart_object &object,
                                          size_t partNum, const char *body,
                                          size_t size) {
  // Ensures the REST connection is established
  ensure_connection();

  auto path = shcore::str_format(kUploadPartFormat.c_str(),
                                 encode_path(object.name).c_str(),
                                 object.upload_id.c_str(), partNum);

  Response response;

  try {
    auto request = Oci_request(path);
    request.body = body;
    request.size = size;
    m_rest_service->put(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(
        error.code(),
        shcore::str_format("Failed to upload part %zu for object '%s': %s",
                           partNum, object.name.c_str(), error.what()));
  }

  return {partNum, response.headers.at("Etag"), size};
}

void Bucket::commit_multipart_upload(
    const Multipart_object &object,
    const std::vector<Multipart_object_part> &parts) {
  // Ensures the REST connection is established
  ensure_connection();

  shcore::Array_t etags = shcore::make_array();

  for (const auto &part : parts) {
    etags->emplace_back(shcore::make_dict(
        "partNum", static_cast<uint64_t>(part.part_num), "etag", part.etag));
  }

  std::string body = encode_json("partsToCommit", std::move(etags));

  auto path = shcore::str_format(kMultipartActionFormat.c_str(),
                                 encode_path(object.name).c_str(),
                                 object.upload_id.c_str());

  try {
    auto request = Oci_request(path);
    request.body = body.c_str();
    request.size = body.size();
    m_rest_service->post(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to commit multipart upload for object '" +
                             object.name + "': " + error.what());
  }
}

void Bucket::abort_multipart_upload(const Multipart_object &object) {
  // Ensures the REST connection is established
  ensure_connection();

  auto path = shcore::str_format(kMultipartActionFormat.c_str(),
                                 encode_path(object.name).c_str(),
                                 object.upload_id.c_str());
  try {
    auto request = Oci_request(path);
    m_rest_service->delete_(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to abort multipart upload for object '" +
                             object.name + "': " + error.what());
  }
}

PAR Bucket::create_pre_authenticated_request(
    PAR_access_type access_type, const std::string &expiration_time,
    const std::string &par_name, const std::string &object_name,
    std::optional<PAR_list_action> list_action) {
  // Ensures the REST connection is established
  ensure_connection();

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
    target = "for bucket " + get_name();
  } else {
    target = "for object " + object_name;
  }

  std::string error_msg = shcore::str_format(
      "Failed to create %s PAR %s: ", access_type_str.c_str(), target.c_str());
  try {
    auto request = Oci_request(kParActionPath);
    request.body = body.data();
    request.size = body.size();
    m_rest_service->post(&request, &response);
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

void Bucket::delete_pre_authenticated_request(const std::string &id) {
  // Ensures the REST connection is established
  ensure_connection();

  try {
    auto request = Oci_request(kParActionPath + id);
    m_rest_service->delete_(&request);
  } catch (const Response_error &error) {
    throw Response_error(
        error.code(),
        shcore::str_format("Failed to delete PAR: %s", error.what()));
  }
}

std::vector<PAR> Bucket::list_preauthenticated_requests(
    const std::string &prefix, size_t limit, const std::string &page) {
  // Ensures the REST connection is established
  ensure_connection();

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
    auto request = Oci_request(path);
    m_rest_service->get(&request, &response);
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

bool Bucket::exists() {
  // Ensures the REST connection is established
  ensure_connection();

  try {
    auto request = Oci_request(kBucketPath);
    m_rest_service->head(&request);
    return true;
  } catch (const Response_error &error) {
    if (rest::Response::Status_code::NOT_FOUND == error.code()) {
      return false;
    } else {
      throw;
    }
  }
}

void Bucket::create(const std::string &compartment_id) {
  // Ensures the REST connection is established
  ensure_connection();

  // CreateBucketDetails
  const auto body = encode_json("compartmentId", compartment_id, "name",
                                encode_path(*m_options.os_bucket_name));

  auto request = Oci_request(kNamespacePath);
  request.body = body.c_str();
  request.size = body.size();
  m_rest_service->post(&request);
}

void Bucket::delete_() {
  // Ensures the REST connection is established
  ensure_connection();

  auto request = Oci_request(kBucketPath);
  request.body = "";
  request.size = 0;
  m_rest_service->delete_(&request);
}

Oci_request Bucket::create_request(const std::string &object_name,
                                   Headers headers) const {
  const auto par = is_par(object_name);

  return Oci_request(par ? anonymize_par(object_name)
                         : shcore::str_format(kObjectActionFormat.c_str(),
                                              encode_path(object_name).c_str()),
                     std::move(headers), !par, par);
}

}  // namespace oci
}  // namespace mysqlshdk
