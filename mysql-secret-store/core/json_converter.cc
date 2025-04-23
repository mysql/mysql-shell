/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#include "mysql-secret-store/core/json_converter.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <utility>

using mysql::secret_store::common::Helper_exception;

namespace mysql {
namespace secret_store {
namespace core {
namespace converter {

namespace {

constexpr auto k_secret = "Secret";
constexpr auto k_secret_type = "SecretType";
constexpr auto k_secret_id = "SecretID";
// TODO(pawel): this is deprecated, remove it at some point
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

  if (!doc.IsObject()) {
    std::runtime_error{"Failed to parse JSON: expected object"};
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
      rapidjson::StringRef(k_secret_id),
      rapidjson::StringRef(secret_id.id.c_str(), secret_id.id.length()), a);
  doc.AddMember(
      rapidjson::StringRef(k_server_url),
      rapidjson::StringRef(secret_id.id.c_str(), secret_id.id.length()), a);

  return doc;
}

std::string required(const rapidjson::Value &object, const char *name) {
  const auto it = object.FindMember(name);

  if (object.MemberEnd() == it) {
    throw Helper_exception{"\"" + std::string(name) + "\" is missing"};
  }

  if (!it->value.IsString()) {
    throw Helper_exception{"\"" + std::string(name) + "\" should be a string"};
  }

  return std::string{it->value.GetString(), it->value.GetStringLength()};
}

std::string to_secret_type(const rapidjson::Value &secret_type) {
  return required(secret_type, k_secret_type);
}

common::Secret_id to_secret_id(const rapidjson::Value &secret_id) {
  std::string id;

  // we first check for the deprecated key, to have a meaningful message if the
  // new key is missing
  try {
    id = required(secret_id, k_server_url);
  } catch (...) {
    id = required(secret_id, k_secret_id);
  }

  return {to_secret_type(secret_id), std::move(id)};
}

}  // namespace

common::Secret to_secret(const std::string &secret) {
  const auto doc = parse(secret);
  return {required(doc, k_secret), to_secret_id(doc)};
}

common::Secret_id to_secret_id(const std::string &secret_id) {
  const auto doc = parse(secret_id);
  return to_secret_id(doc);
}

std::string to_secret_type(const std::string &secret_type) {
  const auto doc = parse(secret_type);
  return to_secret_type(doc);
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
