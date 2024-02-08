/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/azure/blob_storage_container.h"

#include <tinyxml2.h>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/rest/rest_utils.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/uuid_gen.h"

namespace mysqlshdk {
namespace azure {

using rest::Base_response_buffer;
using rest::parse_xml;
using rest::Response;
using rest::Response_error;

namespace {
std::string part_id(size_t part_num) {
  static std::mutex part_id_mutex;
  std::lock_guard<std::mutex> lock(part_id_mutex);
  static std::unordered_map<size_t, std::string> encoded_ids;

  if (encoded_ids.find(part_num) == encoded_ids.end()) {
    // This needs to be a unique identifier within the Blob, it is being
    // simply created as 5 digit 0 padded values, this, because the max limit
    // of uncommitted Blocks associated to a Blob is 100K
    std::string uuid = shcore::str_format("%05zi", part_num);

    std::string base64_encoded;
    shcore::encode_base64(static_cast<const unsigned char *>(
                              static_cast<const void *>(uuid.c_str())),
                          uuid.size(), &base64_encoded);

    encoded_ids[part_num] = std::move(base64_encoded);
  }

  return encoded_ids[part_num];
}

Query list_objects_request_query(const std::string &prefix, size_t limit,
                                 bool recursive, const std::string &start_from,
                                 bool uncommitted) {
  Query query = {{"restype", "container"}, {"comp", "list"}};

  if (!prefix.empty()) {
    query.emplace("prefix", prefix);
  }

  if (!recursive) {
    query.emplace("delimiter", "/");
  }

  if (limit) {
    query.emplace("maxresults", std::to_string(limit));
  }

  // Azure does not allow to select fields, it always returns all of them

  if (!start_from.empty()) {
    query.emplace("marker", start_from);
  }

  if (uncommitted) {
    query.emplace("include", "metadata,uncommittedblobs");
  }

  return query;
}

const tinyxml2::XMLElement *get_child(const tinyxml2::XMLNode *node,
                                      const char *name, bool optional = false) {
  if (auto child = node->FirstChildElement(name)) {
    return child;
  }

  if (!optional) {
    std::string parent{"The XML document"};
    if (const auto value = node->Value()) {
      parent = shcore::str_format("A '%s' node", value);
    }

    throw std::runtime_error(shcore::str_format("%s is missing a %s element.",
                                                parent.c_str(), name));
  }

  return nullptr;
}

const tinyxml2::XMLElement *get_optional_child(const tinyxml2::XMLNode *node,
                                               const char *name) {
  return get_child(node, name, true);
}

const char *get_child_text(const tinyxml2::XMLNode *node, const char *name) {
  const auto child = get_child(node, name);

  if (const auto text = child->GetText()) {
    return text;
  }
  throw std::runtime_error(shcore::str_format("A '%s' node is empty.", name));
}

const char *get_optional_child_text(const tinyxml2::XMLNode *node,
                                    const char *name) {
  if (const auto child = get_optional_child(node, name)) {
    if (const auto text = child->GetText()) {
      return text;
    }
  }
  return nullptr;
}

}  // namespace

using mysqlshdk::db::uri::pctencode_path;

Blob_container::Blob_container(const Storage_config_ptr &config)
    : Container(config),
      m_storage_config(config),
      m_blob_container_path(pctencode_path(config->m_container_name)) {}

std::string Blob_container::blob_container_path() const {
  std::string endpoint_path;

  if (!m_storage_config->m_endpoint_path.empty()) {
    endpoint_path = "/" + pctencode_path(m_storage_config->m_account_name);
  }

  return endpoint_path + "/" + m_blob_container_path;
}

std::string Blob_container::object_path(const std::string &name) const {
  return blob_container_path() + "/" + pctencode_path(name);
}

void Blob_container::add_sas_token(mysqlshdk::rest::Query *query) const {
  assert(query);
  // Adds the SAS URL Token
  for (const auto &item : m_storage_config->m_sas_token_data) {
    (*query)[item.first] = item.second;
  }
}

Signed_request Blob_container::create_blob_container_request(
    Headers headers, Query query) const {
  add_sas_token(&query);

  return Signed_request(blob_container_path(), std::move(headers),
                        std::move(query));
}

Signed_request Blob_container::create_blob_request(const std::string &name,
                                                   Headers headers,
                                                   Query query) const {
  add_sas_token(&query);

  return Signed_request(object_path(name), std::move(headers),
                        std::move(query));
}

bool Blob_container::exists() {
  const auto service = ensure_connection();

  try {
    auto request =
        create_blob_container_request({}, {{"restype", "container"}});
    service->head(&request);
    return true;
  } catch (const Response_error &error) {
    if (Response::Status_code::NOT_FOUND == error.status_code()) {
      return false;
    } else {
      throw;
    }
  }
}

void Blob_container::create() {
  const auto service = ensure_connection();
  auto request = create_blob_container_request({}, {{"restype", "container"}});
  service->put(&request);
}

void Blob_container::delete_() {
  const auto service = ensure_connection();
  auto request = create_blob_container_request({}, {{"restype", "container"}});
  service->delete_(&request);
}

Signed_request Blob_container::list_objects_request(
    const std::string &prefix, size_t limit, bool recursive,
    const Object_details::Fields_mask &, const std::string &start_from) {
  return create_blob_container_request(
      {},
      list_objects_request_query(prefix, limit, recursive, start_from, false));
}

std::vector<Object_details> Blob_container::parse_list_objects(
    const Base_response_buffer &buffer, std::string *next_start_from,
    std::unordered_set<std::string> *out_prefixes) {
  try {
    return parse_list_objects(buffer, next_start_from, out_prefixes, false);
  } catch (const std::exception &err) {
    log_debug3("Failed Parsing Container.ListObjects From:\n%s", buffer.data());
    throw std::runtime_error(shcore::str_format(
        "Error parsing result for Container.ListObjects: %s", err.what()));
  }
}

std::vector<Object_details> Blob_container::parse_list_objects(
    const Base_response_buffer &buffer, std::string *next_start_from,
    std::unordered_set<std::string> *out_prefixes,
    bool only_multipart_uploads) {
  std::vector<Object_details> objects;
  if (buffer.size() == 0) return objects;

  const auto xml = parse_xml(buffer);
  const auto root = get_optional_child(xml.get(), "EnumerationResults");
  if (!root) return objects;

  const auto blobs = get_optional_child(root, "Blobs");
  if (!blobs) return objects;

  for (auto blob = get_optional_child(blobs, "Blob"); blob;
       blob = blob->NextSiblingElement("Blob")) {
    Object_details details;

    details.name = get_child_text(blob, "Name");

    if (const auto properties = get_child(blob, "Properties")) {
      if (const auto size =
              get_optional_child_text(properties, "Content-Length")) {
        details.size = shcore::lexical_cast<std::size_t>(size);
      }

      // NOTE: Multipart uploads are listed as:
      // - Objects with length 0 and no Metadata Element
      // - Objects ActiveMultipart Metadata
      if (only_multipart_uploads) {
        auto metadata = get_optional_child(blob, "Metadata");
        bool is_active_multipart = (details.size == 0 && metadata == nullptr);
        if (!is_active_multipart) {
          if (metadata != nullptr &&
              get_optional_child(metadata, "ActiveMultipart") != nullptr) {
            is_active_multipart = true;
          }

          if (!is_active_multipart) continue;
        }
      }

      if (const auto etag = get_optional_child_text(properties, "Etag")) {
        details.etag.assign(etag);
      }

      if (const auto created =
              get_optional_child_text(properties, "Creation-Time")) {
        details.time_created.assign(created);
      }
    }

    if (out_prefixes) {
      if (const auto prefix = get_optional_child(blobs, "BlobPrefix")) {
        if (const auto prefix_name = get_optional_child_text(prefix, "Name")) {
          out_prefixes->emplace(prefix_name);
        }
      }
    }

    objects.emplace_back(std::move(details));
  }

  if (next_start_from) {
    if (const auto next_marker = get_optional_child_text(root, "NextMarker")) {
      next_start_from->assign(next_marker);
    }
  }

  return objects;
}

Signed_request Blob_container::head_object_request(
    const std::string &object_name) {
  // HeadObject
  return create_blob_request(object_name);
}

Signed_request Blob_container::delete_object_request(
    const std::string &object_name) {
  // DeleteObject
  return create_blob_request(object_name);
}

Signed_request Blob_container::put_object_request(
    const std::string &object_name, Headers headers) {
  Headers custom_headers = headers;
  custom_headers["x-ms-blob-type"] = "BlockBlob";
  // PutObject
  return create_blob_request(object_name, std::move(custom_headers));
}

Signed_request Blob_container::get_object_request(
    const std::string &object_name, Headers headers) {
  // GetObject
  return create_blob_request(object_name, std::move(headers));
}

Signed_request Blob_container::list_multipart_uploads_request(size_t limit) {
  // TODO(rennox): This is not handling pagination...
  return create_blob_container_request(
      {}, list_objects_request_query("", limit, "", "", true));
}

std::vector<Multipart_object> Blob_container::parse_list_multipart_uploads(
    const Base_response_buffer &buffer) {
  std::vector<Multipart_object> uploads;
  try {
    std::string next_start_from;

    // TODO(rennox): Add support for paged results
    auto objects = parse_list_objects(buffer, &next_start_from, nullptr, true);

    for (const auto &object : objects) {
      uploads.push_back(Multipart_object{object.name, ""});
    }

  } catch (const std::exception &err) {
    log_debug3("Failed Parsing Container.ListMultipartUploads From: %s\n",
               buffer.data());
    throw std::runtime_error(shcore::str_format(
        "Error parsing result for Container.ListMultipartUploads: %s",
        err.what()));
  }
  return uploads;
}

Signed_request Blob_container::list_multipart_uploaded_parts_request(
    const Multipart_object &object, size_t /*limit*/) {
  return create_blob_request(
      object.name, {},
      {{"comp", "blocklist"}, {"blocklisttype", "uncommitted"}});
}

std::vector<Multipart_object_part>
Blob_container::parse_list_multipart_uploaded_parts(
    const Base_response_buffer &buffer) {
  std::vector<Multipart_object_part> parts;
  try {
    const auto xml = parse_xml(buffer);

    const auto root = get_child(xml.get(), "BlockList");
    const auto uncommitted = get_child(root, "UncommittedBlocks");

    for (auto part = get_optional_child(uncommitted, "Block"); part;
         part = part->NextSiblingElement("Block")) {
      Multipart_object_part details;

      // Name is base64 encoded id, should be turned into sequence
      details.etag.assign(get_child_text(part, "Name"));
      std::string decoded_id;
      shcore::decode_base64(details.etag, &decoded_id);
      details.part_num = shcore::lexical_cast<std::size_t>(decoded_id);

      details.size =
          shcore::lexical_cast<std::size_t>(get_child_text(part, "Size"));

      // Skips the first block as it is just a placeholder
      if (details.part_num != 0) parts.emplace_back(std::move(details));
    }
  } catch (const std::exception &err) {
    log_debug3("Failed Parsing Container.ListMultipartUploadedParts From: %s\n",
               buffer.data());
    throw std::runtime_error(shcore::str_format(
        "Error parsing result for Container.ListMultipartUploadedParts: %s",
        err.what()));
  }

  return parts;
}

Signed_request Blob_container::create_multipart_upload_request(
    const std::string &object_name, std::string *request_body) {
  // In Azure there's no specific API to create a multipart upload, it will be
  // implicitly created as an uncommitted blob when the first block is
  // uploaded.
  return upload_part_request({object_name, ""}, 0, request_body->size());
}

void Blob_container::on_multipart_upload_created(
    const std::string &object_name) {
  // Multipart upload has 2 behaviors
  // - 1: New Object: Uncommitted blob is created and can be identified as in
  //      progress multipart upload
  // - 2: Existing Object: The existing object is tagged with an ActiveMultipart
  //      metadata entry to make it identifiable as in progress multipart upload
  //
  // For case 1, setting the ActiveMultipart metadata entry will fail WITH
  // NOT_FOUND as the object is not considered to exist yet, so it is OK
  try {
    auto request =
        create_blob_request(object_name, {{"x-ms-meta-ActiveMultipart", "1"}},
                            {{"comp", "metadata"}});
    ensure_connection()->put(&request);
  } catch (const mysqlshdk::rest::Response_error &error) {
    if (error.status_code() !=
        mysqlshdk::rest::Response::Status_code::NOT_FOUND)
      throw;
  }
}

std::string Blob_container::parse_create_multipart_upload(
    const rest::String_response & /*response*/) {
  // The Block ID of the first part will be used as the Multipart Object ID,
  // for later inclusion on the commit.
  return "";
}

Signed_request Blob_container::upload_part_request(
    const Multipart_object &object, size_t part_num, size_t size) {
  return create_blob_request(
      object.name,
      {{"content-length", std::to_string(size)},
       {"content-type", "application/octet-stream"}},
      {{"comp", "block"}, {"blockid", part_id(part_num)}});
}

std::string Blob_container::parse_multipart_upload(
    const rest::Response & /*response*/) {
  return "";
}

Signed_request Blob_container::commit_multipart_upload_request(
    const Multipart_object &object,
    const std::vector<Multipart_object_part> &parts,
    std::string *request_body) {
  // CompleteMultipartUpload
  Query query = {{"comp", "blocklist"}};

  // create the request, using compact mode to save some bytes
  tinyxml2::XMLPrinter printer;

  printer.PushDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
  printer.OpenElement("BlockList", true);

  for (const auto &part : parts) {
    printer.OpenElement("Latest", true);

    printer.PushText(part_id(part.part_num).c_str());

    printer.CloseElement(true);  // Latest
  }

  printer.CloseElement(true);  // BlockList

  *request_body = printer.CStr();

  return create_blob_request(object.name, {{"Content-Type", "text/plain"}},
                             std::move(query));
}

void Blob_container::abort_multipart_upload(const Multipart_object &object) {
  // Multipart upload has 2 behaviors
  // - 1: New Object: Uncommitted blob is created and can be identified as in
  //      progress multipart upload
  // - 2: Existing Object: The existing object is tagged with an ActiveMultipart
  //      metadata entry to make it identifiable as in progress multipart upload
  //
  // The abort multipart upload will work this way:
  // - Attempts to remove the ActiveMultipart metadata entry
  // - If fails with NOT_FOUND, it means it was a new object, so we commit with
  // no blocks and then delete it

  try {
    auto request = create_blob_request(object.name, {}, {{"comp", "metadata"}});
    ensure_connection()->put(&request);
  } catch (const mysqlshdk::rest::Response_error &error) {
    // NOT_FOUND indicates it is a new Blob, The tag operation will fail, but it
    // is OK
    if (error.status_code() ==
        mysqlshdk::rest::Response::Status_code::NOT_FOUND) {
      commit_multipart_upload(object, {});
      delete_object(object.name);
    } else {
      throw;
    }
  }
}

}  // namespace azure
}  // namespace mysqlshdk
