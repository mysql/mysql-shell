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

#include "mysqlshdk/libs/azure/signer.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace azure {
namespace {
const std::string k_authorization_header = "authorization";
const std::string k_date_header = "x-ms-date";
const std::string k_version_header = "x-ms-version";
const std::string k_service_version = "2021-08-06";

std::string to_string(const Services &types) {
  std::string value;
  if (types.is_set(Allowed_services::BLOBS)) value += 'b';
  if (types.is_set(Allowed_services::QUEUE)) value += 'q';
  if (types.is_set(Allowed_services::FILES)) value += 'f';

  if (value.empty()) {
    throw std::logic_error("No service types have been specified.");
  }

  return value;
}

std::string to_string(const Resource_types &types) {
  std::string value;
  if (types.is_set(Allowed_resource_types::SERVICE)) value += 's';
  if (types.is_set(Allowed_resource_types::CONTAINER)) value += 'c';
  if (types.is_set(Allowed_resource_types::OBJECT)) value += 'o';

  if (value.empty()) {
    throw std::logic_error("No resource types have been specified.");
  }

  return value;
}

std::string to_string(const Permissions &permissions) {
  std::string value;
  if (permissions.is_set(Allowed_permissions::READ)) value += 'r';
  if (permissions.is_set(Allowed_permissions::WRITE)) value += 'w';
  if (permissions.is_set(Allowed_permissions::DELETE)) value += 'd';
  if (permissions.is_set(Allowed_permissions::LIST)) value += 'l';
  if (permissions.is_set(Allowed_permissions::ADD)) value += 'a';
  if (permissions.is_set(Allowed_permissions::CREATE)) value += 'c';
  if (permissions.is_set(Allowed_permissions::PROCESS)) value += 'p';
  if (permissions.is_set(Allowed_permissions::TAG)) value += 't';
  if (permissions.is_set(Allowed_permissions::FILTER)) value += 'f';
  if (permissions.is_set(Allowed_permissions::IMMUTABLE_STORAGE)) value += 'i';
  if (permissions.is_set(Allowed_permissions::DELETE_VERSION)) value += 'x';
  if (permissions.is_set(Allowed_permissions::PERMANENT_DELETE)) value += 'y';

  if (value.empty()) {
    throw std::logic_error("No permissions have been specified.");
  }

  return value;
}

}  // namespace

Signer::Signer(const Blob_storage_config &config)
    : m_account_name(config.m_account_name),
      m_account_key(config.m_account_key),
      m_using_sas_token(!config.m_sas_token_data.empty()) {
  if (!m_account_key.empty()) {
    std::string decoded_key;
    shcore::decode_base64(m_account_key, &decoded_key);

    set_secret_access_key(decoded_key);
  }
}

bool Signer::should_sign_request(const rest::Signed_request *) const {
  return !m_account_key.empty() || m_using_sas_token;
}

rest::Headers Signer::get_required_headers(const rest::Signed_request *request,
                                           time_t now) const {
  using mysqlshdk::utils::fmttime;
  using mysqlshdk::utils::Time_type;

  // Gets the headers coming in the request
  rest::Headers result = request->unsigned_headers();

  // Adds additional required headers
  result[k_date_header] =
      fmttime("%a, %d %b %Y %H:%M:%S GMT", Time_type::GMT, &now);
  result[k_version_header] = k_service_version;

  if (request->size) {
    result["Content-Length"] = std::to_string(request->size);
  }

  return result;
}

std::string Signer::get_canonical_headers(const rest::Headers &headers) const {
  // ------------------------------
  // ---- CanonicalizedHeaders ----
  // ------------------------------
  std::map<std::string, std::string, shcore::Lexicographical_comparator>
      canonical_headers;

  // Get the headers to be used in the signature
  size_t total_size = 0;
  for (const auto &header : headers) {
    if (shcore::str_ibeginswith(header.first, "x-ms-")) {
      std::string value = header.second;
      auto vsize = value.size();
      if (vsize > 0 && (value[0] != '"' || value[vsize - 1] != '"')) {
        std::replace(value.begin(), value.end(), '\r', ' ');
        std::replace(value.begin(), value.end(), '\n', ' ');
        std::replace(value.begin(), value.end(), '\t', ' ');
      }
      total_size += header.first.size() + value.size() + 2;
      canonical_headers[shcore::str_lower(header.first)] = std::move(value);
    }
  }

  std::string canonicalized_headers;
  canonicalized_headers.reserve(total_size);
  for (auto header : canonical_headers) {
    canonicalized_headers += header.first;
    canonicalized_headers += ':';
    canonicalized_headers += header.second;
    canonicalized_headers += '\n';
  }
  return canonicalized_headers;
}

std::string Signer::get_canonical_resource(
    const rest::Signed_request *request) const {
  // ------------------------------
  // ---- CanonicalizedResource ---
  // ------------------------------

  // 1) Beginning with an empty string (""), append a forward slash (/),
  // followed by the name of the account that owns the resource being accessed.
  std::string canonicalized_resource = "/" + m_account_name;

  // 2) Append the resource's encoded URI path, without any query parameters.
  canonicalized_resource += request->path().real();

  // 3) Retrieve all query parameters on the resource URI, including the comp
  // parameter if it exists.
  // 4) Convert all parameter names to lowercase.
  // 5) Sort the query parameters lexicographically by parameter name, in
  // ascending order.
  std::map<std::string, std::string, shcore::Lexicographical_comparator>
      canonical_query;

  for (const auto &item : request->query()) {
    canonical_query[shcore::str_lower(item.first)] = item.second.value_or("");
  }

  // 6) URL-decode each query parameter name and value.
  // 7) Include a new-line character (\n) before each name-value pair.
  // 8) Append each query parameter name and value to the string in the
  // following format, making sure to include the colon (:) between the name
  // and the value: parameter-name:parameter-value
  for (const auto &item : canonical_query) {
    canonicalized_resource += '\n';
    canonicalized_resource += item.first;
    canonicalized_resource += ':';
    canonicalized_resource += item.second;
  }

  return canonicalized_resource;
}

std::string Signer::get_string_to_sign(const rest::Signed_request *request,
                                       const rest::Headers &headers) const {
  // 9) If a query parameter has more than one value, sort all values
  // lexicographically, then include them in a comma-separated list:
  // parameter-name:parameter-value-1,parameter-value-2,parameter-value-n
  // NOTE: This seems to be not needed, if at any point signing fails
  // because of this, keep in mind that a nice workaround is to simply
  // define the query values in lexicographical order

  // ---------------------
  // ---- StringToSign ---
  // ---------------------
  // StringToSign = VERB + "\n" +
  //                Content-Encoding + "\n" +
  //                Content-Language + "\n" +
  //                Content-Length + "\n" +
  //                Content-MD5 + "\n" +
  //                Content-Type + "\n" +
  //                Date + "\n" +
  //                If-Modified-Since + "\n" +
  //                If-Match + "\n" +
  //                If-None-Match + "\n" +
  //                If-Unmodified-Since + "\n" +
  //                Range + "\n" +
  //                CanonicalizedHeaders +
  //                CanonicalizedResource;

  std::string string_to_sign;
  string_to_sign.reserve(512);

  // HTTPMethod
  string_to_sign += shcore::str_upper(rest::type_name(request->type));
  string_to_sign += '\n';
  // Content-Encoding  +
  string_to_sign += '\n';
  // Content-Language +
  string_to_sign += '\n';
  // Content-Length +
  auto content_length = headers.find("content-length");
  if (content_length != headers.end() &&
      std::atoi(content_length->second.c_str()) != 0) {
    string_to_sign += content_length->second;
  }
  string_to_sign += '\n';
  // Content-MD5 +
  string_to_sign += '\n';
  // Content-Type +

  auto content_type = headers.find("content-type");
  if (content_type != headers.end()) {
    string_to_sign += content_type->second;
  } else if (request->type == rest::Type::PUT ||
             request->type == rest::Type::DELETE) {
    // By default curl is setting this when no content type is specified
    string_to_sign += "application/x-www-form-urlencoded";
  }

  string_to_sign += '\n';
  // Date +
  string_to_sign += '\n';
  // If-Modified-Since +
  string_to_sign += '\n';
  // If-Match +
  string_to_sign += '\n';
  // If-None-Match +
  string_to_sign += '\n';
  // If-Unmodified-Since +
  string_to_sign += '\n';
  // Range +
  auto range = headers.find("range");
  if (range != headers.end()) {
    string_to_sign += range->second;
  }
  string_to_sign += '\n';
  // CanonicalizedHeaders
  string_to_sign += get_canonical_headers(headers);

  // CanonicalizedResource;
  string_to_sign += get_canonical_resource(request);

  return string_to_sign;
}

rest::Headers Signer::sign_request(const rest::Signed_request *request,
                                   time_t now) const {
  auto result = get_required_headers(request, now);

  // NOTE: if using a SAS Token, we don't really need to sign the request as the
  // SAS Token data was already as part of the request query in Blob_container
  if (m_using_sas_token) return result;

  auto string_to_sign = get_string_to_sign(request, result);

  auto encoded_string =
      shcore::ssl::hmac_sha256(m_secret_access_key, string_to_sign);
  std::string signature;
  shcore::encode_base64(encoded_string.data(), encoded_string.size(),
                        &signature);

  // Authorization="[SharedKey|SharedKeyLite] <AccountName>:<Signature>"
  result[k_authorization_header] =
      "SharedKey " + m_account_name + ":" + signature;

  return result;
}

void Signer::set_secret_access_key(const std::string &key) {
  m_secret_access_key.resize(key.size());

  memcpy(m_secret_access_key.data(), key.c_str(), key.size());
}

std::string Signer::create_account_sas_token(
    const Permissions &permissions, const Resource_types &resource_types,
    const Services &services, int expiry_hours) const {
  auto start = shcore::current_time_rfc3339();
  auto expiry = shcore::future_time_rfc3339(std::chrono::hours(expiry_hours));

  auto str_permissions = to_string(permissions);
  auto str_resource_types = to_string(resource_types);
  auto str_services = to_string(services);

  std::string string_to_sign = m_account_name + "\n" +   // Account
                               str_permissions + "\n" +  // Signed Permissions
                               str_services + "\n" +     // Signed Service
                               str_resource_types +
                               "\n" +           // Signed Resource Type
                               start + "\n" +   // Signed Start
                               expiry + "\n" +  // Signed Expiry
                               "\n" +           // Signed IP Address
                               "https\n" +      // Signed Protocol
                               k_service_version + "\n" +  // Signed Version
                               "\n";                       // Encryption Scope

  auto encoded_string =
      shcore::ssl::hmac_sha256(m_secret_access_key, string_to_sign);
  std::string signature;
  signature.reserve(encoded_string.size());

  shcore::encode_base64(encoded_string.data(), encoded_string.size(),
                        &signature);
  return shcore::str_format(
      "sv=%s&ss=%s&srt=%s&sp=%s&se=%s&st=%s&spr=https&sig=%s",
      k_service_version.c_str(), str_services.c_str(),
      str_resource_types.c_str(), str_permissions.c_str(), expiry.c_str(),
      start.c_str(), shcore::pctencode(signature).c_str());
}

}  // namespace azure
}  // namespace mysqlshdk
