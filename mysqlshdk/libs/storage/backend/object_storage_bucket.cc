/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/object_storage_bucket.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/fault_injection.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

using rest::Headers;
using rest::Response;
using rest::Response_error;

namespace {

constexpr size_t MAX_LIST_OBJECTS_LIMIT = 1000;

FI_DEFINE(os_put_object, ([](const mysqlshdk::utils::FI::Args &args) {
            throw Response_error(
                static_cast<Response::Status_code>(args.get_int("code")),
                args.get_string("msg"));
          }));

}  // namespace

Bucket::Bucket(const Config_ptr &config) : m_config(config) {
  assert(m_config && m_config->valid());
}

std::vector<Object_details> Bucket::list_objects(
    const std::string &prefix, size_t limit, bool recursive,
    const Object_details::Fields_mask &fields,
    std::unordered_set<std::string> *out_prefixes) {
  bool done = false;
  std::vector<Object_details> result;
  std::string next_start;
  auto remaining = limit;

  while (!done) {
    // Only sets the limit when the request will be satisfied in one call,
    // otherwise the limit will be set after the necessary calls to fulfil the
    // limit request
    auto request = list_objects_request(
        prefix, remaining < MAX_LIST_OBJECTS_LIMIT ? remaining : 0, recursive,
        fields, next_start);
    rest::String_response response;

    try {
      ensure_connection()->get(&request, &response);
    } catch (const Response_error &error) {
      throw Response_error(error.code(),
                           "Failed to list objects using prefix '" + prefix +
                               "': " + error.what());
    }

    next_start.clear();

    try {
      auto list =
          parse_list_objects(response.buffer, &next_start, out_prefixes);

      if (remaining) {
        remaining -= result.size();
      }

      std::move(list.begin(), list.end(), std::back_inserter(result));
    } catch (const shcore::Exception &error) {
      const auto msg = "Failed to parse 'list objects' (with prefix '" +
                       prefix + "') response: " + error.what();
      const auto &raw_data = response.buffer;

      log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
                 raw_data.data());

      throw shcore::Exception::runtime_error(msg);
    }

    if (next_start.empty() || result.size() == limit) {
      done = true;
    }
  }

  return result;
}

size_t Bucket::head_object(const std::string &object_name) {
  auto request = head_object_request(object_name);
  Response response;

  try {
    ensure_connection()->head(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), "Failed to get summary for object '" +
                                           object_name + "': " + error.what());
  }

  return std::stoul(response.headers.at("content-length"));
}

void Bucket::delete_object(const std::string &object_name) {
  auto request = delete_object_request(object_name);

  try {
    ensure_connection()->delete_(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), "Failed to delete object '" +
                                           object_name + "': " + error.what());
  }
}

void Bucket::rename_object(const std::string &src_name,
                           const std::string &new_name) {
  try {
    execute_rename_object(ensure_connection(), src_name, new_name);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), "Failed to rename object '" + src_name +
                                           "' to '" + new_name +
                                           "': " + error.what());
  }
}

void Bucket::put_object(const std::string &object_name, const char *data,
                        size_t size) {
  Headers headers{{"content-type", "application/octet-stream"}};

  auto request = put_object_request(object_name, std::move(headers));
  request.body = data;
  request.size = size;

  try {
    FI_TRIGGER_TRAP(os_put_object, mysqlshdk::utils::FI::Trigger_options(
                                       {{"name", object_name}}));

    ensure_connection()->put(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(), "Failed to put object '" + object_name +
                                           "': " + error.what());
  }
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer,
                          const std::optional<size_t> &from_byte,
                          const std::optional<size_t> &to_byte) {
  Headers headers;
  std::string range;

  if (from_byte || to_byte) {
    range = shcore::str_format(
        "bytes=%s-%s", (!from_byte ? "" : std::to_string(*from_byte).c_str()),
        (!to_byte ? "" : std::to_string(*to_byte).c_str()));
    headers["range"] = range;
  }

  auto request = get_object_request(object_name, std::move(headers));
  Response response;
  response.body = buffer;

  try {
    ensure_connection()->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to get object '" + object_name + "'" +
                             (range.empty() ? "" : " [" + range + "]") + ": " +
                             error.what());
  }

  return buffer->size();
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer,
                          size_t from_byte, size_t to_byte) {
  return get_object(object_name, buffer, std::optional<size_t>{from_byte},
                    std::optional<size_t>{to_byte});
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer,
                          size_t from_byte) {
  return get_object(object_name, buffer, std::optional<size_t>{from_byte},
                    std::optional<size_t>{});
}

size_t Bucket::get_object(const std::string &object_name,
                          mysqlshdk::rest::Base_response_buffer *buffer) {
  return get_object(object_name, buffer, std::optional<size_t>{},
                    std::optional<size_t>{});
}

std::vector<Multipart_object> Bucket::list_multipart_uploads(size_t limit) {
  auto request = list_multipart_uploads_request(limit);
  rest::String_response response;

  try {
    ensure_connection()->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(
        error.code(),
        std::string{"Failed to list multipart uploads: "} + error.what());
  }

  try {
    return parse_list_multipart_uploads(response.buffer);
  } catch (const shcore::Exception &error) {
    const auto msg =
        std::string{"Failed to parse 'list multipart uploads' response: "} +
        error.what();
    const auto &raw_data = response.buffer;

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
               raw_data.data());

    throw shcore::Exception::runtime_error(msg);
  }
}

std::vector<Multipart_object_part> Bucket::list_multipart_uploaded_parts(
    const Multipart_object &object, size_t limit) {
  auto request = list_multipart_uploaded_parts_request(object, limit);
  rest::String_response response;

  try {
    ensure_connection()->get(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to list uploaded parts for object '" +
                             object.name + "': " + error.what());
  }

  try {
    return parse_list_multipart_uploaded_parts(response.buffer);
  } catch (const shcore::Exception &error) {
    const auto msg =
        "Failed to parse 'list uploaded parts' response for object '" +
        object.name + "': " + error.what();
    const auto &raw_data = response.buffer;

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
               raw_data.data());

    throw shcore::Exception::runtime_error(msg);
  }
}

Multipart_object Bucket::create_multipart_upload(
    const std::string &object_name) {
  std::string body;
  auto request = create_multipart_upload_request(object_name, &body);
  request.body = body.c_str();
  request.size = body.size();

  rest::String_response response;

  try {
    ensure_connection()->post(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to create a multipart upload for object '" +
                             object_name + "': " + error.what());
  }

  try {
    return {object_name, parse_create_multipart_upload(response.buffer)};
  } catch (const shcore::Exception &error) {
    const auto msg =
        "Failed to parse 'create multipart upload' response for object '" +
        object_name + "': " + error.what();
    const auto &raw_data = response.buffer;

    log_debug2("%s\n%.*s", msg.c_str(), static_cast<int>(raw_data.size()),
               raw_data.data());

    throw shcore::Exception::runtime_error(msg);
  }
}

Multipart_object_part Bucket::upload_part(const Multipart_object &object,
                                          size_t part_num, const char *body,
                                          size_t size) {
  auto request = upload_part_request(object, part_num);
  request.body = body;
  request.size = size;
  Response response;

  try {
    ensure_connection()->put(&request, &response);
  } catch (const Response_error &error) {
    throw Response_error(
        error.code(),
        shcore::str_format("Failed to upload part %zu for object '%s': %s",
                           part_num, object.name.c_str(), error.what()));
  }

  return {part_num, response.headers.at("ETag"), size};
}

void Bucket::commit_multipart_upload(
    const Multipart_object &object,
    const std::vector<Multipart_object_part> &parts) {
  std::string body;
  auto request = commit_multipart_upload_request(object, parts, &body);
  request.body = body.c_str();
  request.size = body.size();

  try {
    ensure_connection()->post(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to commit multipart upload for object '" +
                             object.name + "': " + error.what());
  }
}

void Bucket::abort_multipart_upload(const Multipart_object &object) {
  auto request = abort_multipart_upload_request(object);

  try {
    ensure_connection()->delete_(&request);
  } catch (const Response_error &error) {
    throw Response_error(error.code(),
                         "Failed to abort multipart upload for object '" +
                             object.name + "': " + error.what());
  }
}

rest::Signed_rest_service *Bucket::ensure_connection() {
  static thread_local std::unordered_map<
      std::string, std::unique_ptr<rest::Signed_rest_service>>
      services;

  // need to detect when this instance was passed to another thread and replace
  // the REST service, or we may end up sharing it across multiple threads
  const auto current_thread = std::this_thread::get_id();

  if (current_thread != m_rest_service_thread) {
    m_rest_service = nullptr;
  }

  if (!m_rest_service) {
    auto &service = services[m_config->hash()];

    if (!service) {
      service = std::make_unique<rest::Signed_rest_service>(*m_config);
    }

    m_rest_service = service.get();
    m_rest_service_thread = current_thread;
  }

  return m_rest_service;
}

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
