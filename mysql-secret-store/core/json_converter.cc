/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-secret-store/core/json_converter.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "mysqlshdk/libs/db/connection_options.h"

using mysql::secret_store::common::Helper_exception;

namespace mysql {
namespace secret_store {
namespace core {
namespace converter {

namespace {

constexpr auto k_secret = "Secret";
constexpr auto k_secret_type = "SecretType";
constexpr auto k_server_url = "ServerURL";

std::string to_string(rapidjson::Document *doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc->Accept(writer);
  return std::string{buffer.GetString(), buffer.GetSize()};
}

rapidjson::Document parse(const std::string &json) {
  rapidjson::Document doc;

  doc.Parse(json.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error{std::string{"Failed to parse JSON: "} +
                             rapidjson::GetParseError_En(doc.GetParseError())};
  }

  return doc;
}

rapidjson::Document to_object(
    const common::Secret_id &secret_id,
    rapidjson::Document::AllocatorType *allocator = nullptr) {
  rapidjson::Document doc{rapidjson::Type::kObjectType, allocator};
  auto &a = doc.GetAllocator();

  doc.AddMember(rapidjson::StringRef(k_secret_type),
                rapidjson::StringRef(secret_id.secret_type.c_str(),
                                     secret_id.secret_type.length()),
                a);
  doc.AddMember(
      rapidjson::StringRef(k_server_url),
      rapidjson::StringRef(secret_id.url.c_str(), secret_id.url.length()), a);

  return doc;
}

void validate(const rapidjson::Value &v,
              const std::vector<std::string> &required) {
  for (const auto &r : required) {
    if (!v.HasMember(r.c_str())) {
      throw Helper_exception{"\"" + r + "\" is missing"};
    }
    if (!v[r.c_str()].IsString()) {
      throw Helper_exception{"\"" + r + "\" should be a string"};
    }
  }
}

common::Secret_id to_secret_id(const rapidjson::Value &secret_id) {
  validate(secret_id, {k_secret_type, k_server_url});
  return {secret_id[k_secret_type].GetString(),
          secret_id[k_server_url].GetString()};
}

}  // namespace

common::Secret to_secret(const std::string &secret) {
  auto doc = parse(secret);
  validate(doc, {k_secret});
  if (doc.MemberCount() > 3) {
    throw Helper_exception{"JSON object contains unknown member"};
  }
  return {doc[k_secret].GetString(), to_secret_id(doc)};
}

common::Secret_id to_secret_id(const std::string &secret_id) {
  auto doc = parse(secret_id);
  if (doc.MemberCount() > 2) {
    throw Helper_exception{"JSON object contains unknown member"};
  }
  return to_secret_id(doc);
}

std::string to_string(const common::Secret &secret) {
  auto doc = to_object(secret.id);
  auto &allocator = doc.GetAllocator();

  doc.AddMember(
      rapidjson::StringRef(k_secret),
      rapidjson::StringRef(secret.secret.c_str(), secret.secret.length()),
      allocator);

  return to_string(&doc);
}

std::string to_string(const common::Secret_id &secret_id) {
  auto doc = to_object(secret_id);
  return to_string(&doc);
}

std::string to_string(const std::vector<common::Secret_id> &secrets) {
  rapidjson::Document doc{rapidjson::Type::kArrayType};
  auto &allocator = doc.GetAllocator();

  for (const auto &s : secrets) {
    doc.PushBack(to_object(s, &allocator), allocator);
  }

  return to_string(&doc);
}

}  // namespace converter
}  // namespace core
}  // namespace secret_store
}  // namespace mysql
