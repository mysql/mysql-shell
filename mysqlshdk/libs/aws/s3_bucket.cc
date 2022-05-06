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

#include "mysqlshdk/libs/aws/s3_bucket.h"

#include <tinyxml2.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

using rest::Base_response_buffer;
using rest::Headers;
using rest::Response;
using rest::Response_error;
using rest::Signed_request;
using rest::Signed_rest_service;
using rest::String_response;

namespace {

constexpr std::size_t k_min_part_size = 5242880;  // 5 MiB
// 5 GiB or architecture limit, whichever is smaller
constexpr std::size_t k_max_part_size = std::min<uint64_t>(
    UINT64_C(5368709120), std::numeric_limits<std::size_t>::max());

inline std::string encode_path(const std::string &data) {
  // signer expects that path segments are URL-encoded, slashes should not be
  // encoded
  return shcore::pctencode_path(data);
}

inline std::string encode_query(const std::string &data) {
  // signer expects that query parameters are URL-encoded
  return shcore::pctencode(data);
}

std::unique_ptr<tinyxml2::XMLDocument> parse_xml(
    const rest::Base_response_buffer &buffer) {
  auto xml = std::make_unique<tinyxml2::XMLDocument>();

  if (tinyxml2::XMLError::XML_SUCCESS !=
      xml->Parse(buffer.data(), buffer.size())) {
    throw shcore::Exception::runtime_error("Failed to parse AWS response: " +
                                           std::string{xml->ErrorStr()});
  }

  return xml;
}

}  // namespace

S3_bucket::S3_bucket(const S3_bucket_config_ptr &config)
    : storage::backend::object_storage::Bucket(config),
      m_config(config),
      // bucket-related requests, path-style: /bucket, virtual-style: /
      m_bucket_path("/" + (config->path_style_access()
                               ? encode_path(config->bucket_name())
                               : "")),
      // object-related requests, path-style: /bucket/key, virtual-style: /key
      m_object_path_prefix(m_bucket_path +
                           (config->path_style_access() ? "/" : "")) {}

rest::Signed_request S3_bucket::list_objects_request(
    const std::string &prefix, size_t limit, bool recursive,
    const Object_details::Fields_mask &, const std::string &start_from) {
  // ListObjectsV2
  Query query = {{"list-type", "2"}};

  if (!prefix.empty()) {
    query.emplace("prefix", encode_query(prefix));
  }

  if (limit) {
    query.emplace("max-keys", std::to_string(limit));
  }

  if (!recursive) {
    // delimiter is '/'
    query.emplace("delimiter", "%2F");
  }

  // AWS S3 does not allow to select fields, it always returns all of them

  if (!start_from.empty()) {
    query.emplace("continuation-token", encode_query(start_from));
  }

  return create_bucket_request(query);
}

std::vector<Object_details> S3_bucket::parse_list_objects(
    const rest::Base_response_buffer &buffer, std::string *next_start_from,
    std::unordered_set<std::string> *out_prefixes) {
  const auto xml = parse_xml(buffer);
  const auto root = xml->FirstChildElement("ListBucketResult");

  if (!root) {
    throw shcore::Exception::runtime_error(
        "Expected root element called 'ListBucketResult'");
  }

  std::vector<Object_details> objects;

  for (auto contents = root->FirstChildElement("Contents"); contents;
       contents = contents->NextSiblingElement("Contents")) {
    Object_details details;

    if (const auto name = contents->FirstChildElement("Key")) {
      details.name = name->GetText();
    }

    if (const auto size = contents->FirstChildElement("Size")) {
      details.size = shcore::lexical_cast<std::size_t>(size->GetText());
    }

    if (const auto etag = contents->FirstChildElement("ETag")) {
      details.etag = etag->GetText();
    }

    if (const auto created = contents->FirstChildElement("LastModified")) {
      details.time_created = created->GetText();
    }

    objects.emplace_back(std::move(details));
  }

  if (next_start_from) {
    if (const auto token = root->FirstChildElement("NextContinuationToken")) {
      *next_start_from = token->GetText();
    }
  }

  if (out_prefixes) {
    for (auto prefixes = root->FirstChildElement("CommonPrefixes"); prefixes;
         prefixes = prefixes->NextSiblingElement("CommonPrefixes")) {
      if (const auto prefix = prefixes->FirstChildElement("Prefix")) {
        out_prefixes->emplace(prefix->GetText());
      }
    }
  }

  return objects;
}

rest::Signed_request S3_bucket::head_object_request(
    const std::string &object_name) {
  // HeadObject
  return create_object_request(object_name);
}

rest::Signed_request S3_bucket::delete_object_request(
    const std::string &object_name) {
  // DeleteObject
  return create_object_request(object_name);
}

rest::Signed_request S3_bucket::put_object_request(
    const std::string &object_name, rest::Headers headers) {
  // PutObject
  return create_object_request(object_name, std::move(headers));
}

rest::Signed_request S3_bucket::get_object_request(
    const std::string &object_name, rest::Headers headers) {
  // GetObject
  return create_object_request(object_name, std::move(headers));
}

void S3_bucket::execute_rename_object(Signed_rest_service *,
                                      const std::string &source_name,
                                      const std::string &new_name) {
  const auto total_size = head_object(source_name);

  if (total_size <= k_max_part_size) {
    // objects up to 5GiB can be copied normally
    copy_object(source_name, new_name);
  } else {
    // objects bigger than that need to be copied using parts
    copy_object_multipart(source_name, new_name, total_size, k_max_part_size);
  }

  delete_object(source_name);
}

rest::Signed_request S3_bucket::list_multipart_uploads_request(size_t limit) {
  // ListMultipartUploads
  Query query = {{"uploads", {}}};

  if (limit) {
    query.emplace("max-uploads", std::to_string(limit));
  }

  return create_bucket_request(query);
}

std::vector<Multipart_object> S3_bucket::parse_list_multipart_uploads(
    const rest::Base_response_buffer &buffer) {
  const auto xml = parse_xml(buffer);
  const auto root = xml->FirstChildElement("ListMultipartUploadsResult");

  if (!root) {
    throw shcore::Exception::runtime_error(
        "Expected root element called 'ListMultipartUploadsResult'");
  }

  std::vector<Multipart_object> uploads;

  for (auto upload = root->FirstChildElement("Upload"); upload;
       upload = upload->NextSiblingElement("Upload")) {
    Multipart_object details;

    if (const auto name = upload->FirstChildElement("Key")) {
      details.name = name->GetText();
    } else {
      throw shcore::Exception::runtime_error(
          "'ListMultipartUploadsResult'.'Upload' does not have a 'Key' "
          "element");
    }

    if (const auto id = upload->FirstChildElement("UploadId")) {
      details.upload_id = id->GetText();
    } else {
      throw shcore::Exception::runtime_error(
          "'ListMultipartUploadsResult'.'Upload' does not have an 'UploadId' "
          "element");
    }

    uploads.emplace_back(std::move(details));
  }

  return uploads;
}

rest::Signed_request S3_bucket::list_multipart_uploaded_parts_request(
    const Multipart_object &object, size_t limit) {
  // ListParts
  Query query = {{"uploadId", object.upload_id}};

  if (limit) {
    query.emplace("max-parts", std::to_string(limit));
  }

  return create_object_request(object.name, query);
}

std::vector<Multipart_object_part>
S3_bucket::parse_list_multipart_uploaded_parts(
    const rest::Base_response_buffer &buffer) {
  const auto xml = parse_xml(buffer);
  const auto root = xml->FirstChildElement("ListPartsResult");

  if (!root) {
    throw shcore::Exception::runtime_error(
        "Expected root element called 'ListPartsResult'");
  }

  std::vector<Multipart_object_part> parts;

  for (auto part = root->FirstChildElement("Part"); part;
       part = part->NextSiblingElement("Part")) {
    Multipart_object_part details;

    if (const auto number = part->FirstChildElement("PartNumber")) {
      details.part_num = shcore::lexical_cast<std::size_t>(number->GetText());
    } else {
      throw shcore::Exception::runtime_error(
          "'ListPartsResult'.'Part' does not have a 'PartNumber' element");
    }

    if (const auto etag = part->FirstChildElement("ETag")) {
      details.etag = etag->GetText();
    } else {
      throw shcore::Exception::runtime_error(
          "'ListPartsResult'.'Part' does not have an 'ETag' element");
    }

    if (const auto size = part->FirstChildElement("Size")) {
      details.size = shcore::lexical_cast<std::size_t>(size->GetText());
    } else {
      throw shcore::Exception::runtime_error(
          "'ListPartsResult'.'Part' does not have a 'Size' element");
    }

    parts.emplace_back(std::move(details));
  }

  return parts;
}

rest::Signed_request S3_bucket::create_multipart_upload_request(
    const std::string &object_name, std::string *request_body) {
  // CreateMultipartUpload
  // no body
  *request_body = "";
  Query query = {{"uploads", {}}};
  return create_object_request(object_name, query);
}

std::string S3_bucket::parse_create_multipart_upload(
    const rest::Base_response_buffer &buffer) {
  const auto xml = parse_xml(buffer);
  const auto root = xml->FirstChildElement("InitiateMultipartUploadResult");

  if (!root) {
    throw shcore::Exception::runtime_error(
        "Expected root element called 'InitiateMultipartUploadResult'");
  }

  if (const auto id = root->FirstChildElement("UploadId")) {
    return id->GetText();
  } else {
    throw shcore::Exception::runtime_error(
        "'InitiateMultipartUploadResult' does not have an 'UploadId' element");
  }
}

rest::Signed_request S3_bucket::upload_part_request(
    const Multipart_object &object, size_t part_num) {
  // UploadPart
  Query query = {{"partNumber", std::to_string(part_num)},
                 {"uploadId", object.upload_id}};
  return create_object_request(object.name, query);
}

rest::Signed_request S3_bucket::commit_multipart_upload_request(
    const Multipart_object &object,
    const std::vector<Multipart_object_part> &parts,
    std::string *request_body) {
  // CompleteMultipartUpload
  Query query = {{"uploadId", object.upload_id}};

  // create the request, using compact mode to save some bytes
  tinyxml2::XMLPrinter printer;

  printer.PushDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
  printer.OpenElement("CompleteMultipartUpload", true);
  printer.PushAttribute("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");

  for (const auto &part : parts) {
    printer.OpenElement("Part", true);

    printer.OpenElement("ETag", true);
    printer.PushText(part.etag.c_str());
    printer.CloseElement(true);  // ETag

    printer.OpenElement("PartNumber", true);
    printer.PushText(static_cast<uint64_t>(part.part_num));
    printer.CloseElement(true);  // PartNumber

    printer.CloseElement(true);  // Part
  }

  printer.CloseElement(true);  // CompleteMultipartUpload

  *request_body = printer.CStr();

  return create_object_request(object.name, query);
}

rest::Signed_request S3_bucket::abort_multipart_upload_request(
    const Multipart_object &object) {
  // AbortMultipartUpload
  Query query = {{"uploadId", object.upload_id}};
  return create_object_request(object.name, query);
}

bool S3_bucket::exists() {
  // HeadBucket
  const auto service = ensure_connection();

  try {
    auto request = create_bucket_request();
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

void S3_bucket::create() {
  // CreateBucket
  const auto service = ensure_connection();
  auto request = create_bucket_request();
  service->put(&request);
}

void S3_bucket::delete_() {
  // DeleteBucket
  const auto service = ensure_connection();
  auto request = create_bucket_request();
  service->delete_(&request);
}

void S3_bucket::copy_object(const std::string &src_name,
                            const std::string &new_name) {
  // CopyObject
  const auto service = ensure_connection();
  Headers headers = {{"x-amz-copy-source", copy_source_header(src_name)}};
  auto request = create_object_request(new_name, std::move(headers));
  service->put(&request);
}

void S3_bucket::copy_object_multipart(const std::string &src_name,
                                      const std::string &new_name,
                                      std::size_t total_size,
                                      std::size_t part_size) {
  if (part_size > total_size) {
    throw shcore::Exception::runtime_error(
        "Total size has to be greater than part size");
  }

  if (part_size < k_min_part_size || part_size > k_max_part_size) {
    throw shcore::Exception::runtime_error(
        "Part size has to be a value between " +
        std::to_string(k_min_part_size) + " and " +
        std::to_string(k_max_part_size));
  }

  const auto upload = create_multipart_upload(new_name);
  std::vector<Multipart_object_part> parts;
  std::size_t start = 0;
  std::size_t id = 0;
  const auto service = ensure_connection();

  // UploadPartCopy
  Headers headers = {{"x-amz-copy-source", copy_source_header(src_name)}};
  auto &range = headers["x-amz-copy-source-range"];

  Query query = {{"uploadId", upload.upload_id}};
  auto &part_number = query["partNumber"];

  while (start < total_size) {
    const auto end = std::min(start + part_size, total_size) - 1;
    range = "bytes=" + std::to_string(start) + '-' + std::to_string(end);
    part_number = std::to_string(++id);

    // copy headers
    auto request = Signed_request(
        add_query_parameters(object_path(new_name), query), headers);
    rest::String_response response;
    service->put(&request, &response);

    {
      const auto xml = parse_xml(response.buffer);
      const auto root = xml->FirstChildElement("CopyPartResult");

      if (!root) {
        throw shcore::Exception::runtime_error(
            "Expected root element called 'CopyPartResult'");
      }

      if (const auto etag = root->FirstChildElement("ETag")) {
        Multipart_object_part part{id, etag->GetText(), part_size};
        parts.emplace_back(std::move(part));
      } else {
        throw shcore::Exception::runtime_error(
            "'CopyPartResult' does not have an 'ETag' element");
      }
    }

    start = end + 1;
  }

  commit_multipart_upload(upload, parts);
}

void S3_bucket::delete_objects(const std::vector<Object_details> &list) {
  delete_objects<Object_details>(
      list, [](const Object_details &item) -> const std::string & {
        return item.name;
      });
}

void S3_bucket::delete_objects(const std::vector<std::string> &list) {
  delete_objects<std::string>(
      list,
      [](const std::string &item) -> const std::string & { return item; });
}

rest::Signed_request S3_bucket::create_bucket_request(
    const Query &query) const {
  return Signed_request(add_query_parameters(m_bucket_path, query));
}

rest::Signed_request S3_bucket::create_object_request(
    const std::string &name, rest::Headers headers) const {
  return Signed_request(object_path(name), std::move(headers));
}

rest::Signed_request S3_bucket::create_object_request(
    const std::string &name, const Query &query) const {
  return Signed_request(add_query_parameters(object_path(name), query));
}

std::string S3_bucket::add_query_parameters(const std::string &path,
                                            const Query &query) const {
  std::string p = path;

  if (!query.empty()) {
    p.reserve(256);
    p += '?';

    for (const auto &param : query) {
      p += param.first;

      p += '=';
      p += param.second.value_or("");

      p += '&';
    }

    // remove the last ampersand
    p.pop_back();
  }

  return p;
}

std::string S3_bucket::object_path(const std::string &name) const {
  return m_object_path_prefix + encode_path(name);
}

std::string S3_bucket::copy_source_header(const std::string &name) const {
  return encode_query(m_config->bucket_name() + "/" + name);
}

template <typename T>
void S3_bucket::delete_objects(
    const std::vector<T> &list,
    const std::function<const std::string &(const T &)> &get_name) {
  static constexpr std::size_t max_items = 1000;

  const auto size = list.size();
  const auto service = ensure_connection();

  // DeleteObjects
  Headers headers;
  auto &md5 = headers["Content-MD5"];
  Query query = {{"delete", {}}};

  for (std::size_t i = 0; i < size; i += max_items) {
    tinyxml2::XMLPrinter printer;

    printer.PushDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
    printer.OpenElement("Delete", true);
    printer.PushAttribute("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");

    for (std::size_t j = 0; j < max_items && i + j < size; ++j) {
      printer.OpenElement("Object", true);

      printer.OpenElement("Key", true);
      printer.PushText(get_name(list[i + j]).c_str());
      printer.CloseElement(true);  // Key

      printer.CloseElement(true);  // Object
    }

    printer.OpenElement("Quiet", true);
    printer.PushText("true");
    printer.CloseElement(true);  // Quiet

    printer.CloseElement(true);  // Delete

    // CStrSize() includes the terminating null, subtract it from length
    const auto body_size = printer.CStrSize() - 1;
    const auto hash = shcore::ssl::md5(printer.CStr(), body_size);
    shcore::encode_base64(hash.data(), hash.size(), &md5);

    auto request =
        Signed_request(add_query_parameters(m_bucket_path, query), headers);
    request.body = printer.CStr();
    request.size = body_size;

    service->post(&request);
  }
}

}  // namespace aws
}  // namespace mysqlshdk
