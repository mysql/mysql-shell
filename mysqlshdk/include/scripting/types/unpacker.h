/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_UNPACKER_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_UNPACKER_H_

#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types.h"

namespace shcore {

// Extract option values from an options dictionary, with validations
// Replaces Argument_map
class Option_unpacker {
 public:
  Option_unpacker() = default;
  explicit Option_unpacker(const Dictionary_t &options);

  virtual ~Option_unpacker() = default;

  // For external unpacker objects
  template <typename T>
  Option_unpacker &unpack(T *extra) {
    extra->unpack(this);
    return *this;
  }

  // Extract required option
  template <typename T>
  Option_unpacker &required(const char *name, T *out_value) {
    extract_value<T>(name, out_value, get_required(name, Type_info<T>::vtype));
    return *this;
  }

  // Extract optional option
  template <typename T>
  Option_unpacker &optional(const char *name, T *out_value) {
    extract_value<T>(name, out_value, get_optional(name, Type_info<T>::vtype));
    return *this;
  }

  // Extract optional option with exact type (no conversions)
  template <typename T>
  Option_unpacker &optional_exact(const char *name, T *out_value) {
    extract_value<T>(name, out_value,
                     get_optional_exact(name, Type_info<T>::vtype));
    return *this;
  }

  // Case insensitive

  template <typename T>
  Option_unpacker &optional_ci(const char *name, T *out_value) {
    extract_value<T>(name, out_value,
                     get_optional(name, Type_info<T>::vtype, true));
    return *this;
  }

  void end(std::string_view context = "");

  void set_options(const shcore::Dictionary_t &options);

  void set_ignored(std::unordered_set<std::string_view> ignored) {
    m_ignored = std::move(ignored);
  }

  bool is_ignored(std::string_view option) const {
    return m_ignored.count(option) > 0;
  }

  const shcore::Dictionary_t &options() const { return m_options; }

 protected:
  Dictionary_t m_options;
  std::set<std::string> m_unknown;
  std::set<std::string> m_missing;
  std::unordered_set<std::string_view> m_ignored;

  template <typename T, typename S>
  void extract_value(const char *name, S *out_value, const Value &value) {
    if (value) {
      try {
        *out_value = Type_info<T>::to_native(value);
      } catch (const std::exception &e) {
        std::string msg = e.what();
        if (msg.compare(0, 18, "Invalid typecast: ") == 0) msg = msg.substr(18);
        throw Exception::type_error(std::string("Option '") + name + "' " +
                                    msg);
      }
    }
  }

  Value get_required(const char *name, Value_type type);
  Value get_optional(const char *name, Value_type type,
                     bool case_insensitive = false);
  Value get_optional_exact(const char *name, Value_type type,
                           bool case_insensitive = false);

  void validate(std::string_view context = {});
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_UNPACKER_H_
