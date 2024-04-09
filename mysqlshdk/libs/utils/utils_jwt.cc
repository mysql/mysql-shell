/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_jwt.h"

#include <rapidjson/document.h>

#include <cassert>
#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

Jwt::Object::Object(Object_type type)
    : m_type(type), m_object(rapidjson::Type::kObjectType) {}

Jwt::Object Jwt::Object::from_string(std::string_view object,
                                     Object_type type) {
  std::string json;
  Object o{type};

  if (!decode_base64url(object, &json)) {
    throw std::runtime_error(
        str_format("Could not base64url-decode the JWT's %s", o.type_str()));
  }

  try {
    o.m_object = json::parse_object_or_throw(json);
  } catch (const std::exception &e) {
    throw std::runtime_error(
        str_format("Could not parse the JWT's %s: %s", o.type_str(), e.what()));
  }

  return o;
}

void Jwt::Object::add(std::string_view name, std::string_view value) {
  auto &a = m_object.GetAllocator();
  m_object.AddMember(
      {name.data(), static_cast<rapidjson::SizeType>(name.length()), a},
      {value.data(), static_cast<rapidjson::SizeType>(value.length()), a}, a);
}

void Jwt::Object::add(std::string_view name, int64_t value) {
  auto &a = m_object.GetAllocator();
  m_object.AddMember(
      {name.data(), static_cast<rapidjson::SizeType>(name.length()), a},
      rapidjson::Value{value}, a);
}

void Jwt::Object::add(std::string_view name, uint64_t value) {
  auto &a = m_object.GetAllocator();
  m_object.AddMember(
      {name.data(), static_cast<rapidjson::SizeType>(name.length()), a},
      rapidjson::Value{value}, a);
}

bool Jwt::Object::has(std::string_view name) const {
  return m_object.HasMember(rapidjson::StringRef(name.data(), name.length()));
}

std::string_view Jwt::Object::get_string(std::string_view name) const {
  const auto it =
      m_object.FindMember(rapidjson::StringRef(name.data(), name.length()));

  if (m_object.MemberEnd() == it) {
    throw std::runtime_error(
        str_format("JWT %s: a string entry '%.*s' not found", type_str(),
                   static_cast<int>(name.length()), name.data()));
  }

  if (!it->value.IsString()) {
    throw std::runtime_error(
        str_format("JWT %s: entry '%.*s' is not a string", type_str(),
                   static_cast<int>(name.length()), name.data()));
  }

  return {it->value.GetString(), it->value.GetStringLength()};
}

int64_t Jwt::Object::get_int(std::string_view name) const {
  const auto it =
      m_object.FindMember(rapidjson::StringRef(name.data(), name.length()));

  if (m_object.MemberEnd() == it) {
    throw std::runtime_error(
        str_format("JWT %s: an int entry '%.*s' not found", type_str(),
                   static_cast<int>(name.length()), name.data()));
  }

  if (!it->value.IsInt64()) {
    throw std::runtime_error(
        str_format("JWT %s: entry '%.*s' is not an int", type_str(),
                   static_cast<int>(name.length()), name.data()));
  }

  return it->value.GetInt64();
}

uint64_t Jwt::Object::get_uint(std::string_view name) const {
  const auto it =
      m_object.FindMember(rapidjson::StringRef(name.data(), name.length()));

  if (m_object.MemberEnd() == it) {
    throw std::runtime_error(
        str_format("JWT %s: an unsigned int entry '%.*s' not found", type_str(),
                   static_cast<int>(name.length()), name.data()));
  }

  if (!it->value.IsUint64()) {
    throw std::runtime_error(
        str_format("JWT %s: entry '%.*s' is not an unsigned int", type_str(),
                   static_cast<int>(name.length()), name.data()));
  }

  return it->value.GetUint64();
}

std::string Jwt::Object::to_string() const {
  std::string result;

  if (!encode_base64url(json::to_string(m_object), &result)) {
    throw std::runtime_error(
        str_format("Could not base64url-encode the JWT's %s", type_str()));
  }

  return result;
}

const char *Jwt::Object::type_str() const noexcept {
  switch (m_type) {
    case Object_type::HEADER:
      return "header";

    case Object_type::PAYLOAD:
      return "payload";
  }

  // should not happen
  assert(false);
  return "";
}

Jwt Jwt::from_string(std::string_view jwt) {
  const auto first = jwt.find('.');

  if (std::string_view::npos == first) {
    throw std::runtime_error(
        "Malformed JWT, expected: header.payload.signature");
  }

  const auto second = jwt.find('.', first + 1);

  if (std::string_view::npos == second) {
    throw std::runtime_error(
        "Malformed JWT, expected: header.payload.signature");
  }

  Jwt result;

  result.m_header =
      Object::from_string(jwt.substr(0, first), Object_type::HEADER);
  result.m_payload = Object::from_string(
      jwt.substr(first + 1, second - first - 1), Object_type::PAYLOAD);

  result.m_data = jwt.substr(0, second);

  if (!decode_base64url(jwt.substr(second + 1), &result.m_signature)) {
    throw std::runtime_error("Could not base64url-decode the JWT's signature");
  }

  return result;
}

}  // namespace shcore
