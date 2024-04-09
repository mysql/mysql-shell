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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_JWT_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_JWT_H_

#include <cinttypes>
#include <string>
#include <string_view>

#include "mysqlshdk/libs/utils/utils_json.h"

namespace shcore {

/**
 * JSON Web Token (JWT) - RFC 7519
 */
class Jwt final {
 public:
  enum class Object_type {
    HEADER,
    PAYLOAD,
  };

  class Object {
   public:
    Object() = delete;

    explicit Object(Object_type type);

    Object(const Object &) = delete;
    Object(Object &&) = default;

    Object &operator=(const Object &) = delete;
    Object &operator=(Object &&) = default;

    static Object from_string(std::string_view object, Object_type type);

    void add(std::string_view name, std::string_view value);
    void add(std::string_view name, int64_t value);
    void add(std::string_view name, uint64_t value);

    bool has(std::string_view name) const;

    std::string_view get_string(std::string_view name) const;
    int64_t get_int(std::string_view name) const;
    uint64_t get_uint(std::string_view name) const;

    std::string to_string() const;

    inline Object_type type() const noexcept { return m_type; }

    const char *type_str() const noexcept;

   private:
    Object_type m_type;
    json::JSON m_object;
  };

  Jwt() = default;

  Jwt(const Jwt &) = delete;
  Jwt(Jwt &&) = default;

  Jwt &operator=(const Jwt &) = delete;
  Jwt &operator=(Jwt &&) = default;

  static Jwt from_string(std::string_view jwt);

  inline Object &header() noexcept { return m_header; }

  inline const Object &header() const noexcept { return m_header; }

  inline Object &payload() noexcept { return m_payload; }

  inline const Object &payload() const noexcept { return m_payload; }

 private:
  Object m_header{Object_type::HEADER};
  Object m_payload{Object_type::PAYLOAD};

  std::string m_data;
  std::string m_signature;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_JWT_H_
