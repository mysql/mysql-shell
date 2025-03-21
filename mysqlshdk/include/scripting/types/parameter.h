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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_PARAMETER_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_PARAMETER_H_

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types/validator.h"

namespace shcore {

/**
 * The parameter classes below, can be used to define both
 * parameters and options.
 *
 * This helper class will be used on the proper generation of
 * error messages both on parameter definition and validation.
 */
struct Parameter_context {
  std::string title;

  struct Context_definition {
    std::string name;
    std::optional<int> position;
  };

  std::vector<Context_definition> levels;

  std::string str() const;
};

enum class Param_flag { Mandatory, Optional };

struct Parameter final {
  Parameter() = default;

  Parameter(std::string n, Value_type t, Param_flag f, std::string sname = "",
            bool cmd_line = true, bool deprecated = false)
      : name(std::move(n)),
        flag(f),
        short_name(std::move(sname)),
        cmd_line_enabled(cmd_line),
        is_deprecated(deprecated) {
    set_type(t);
  }

  std::string name;
  Param_flag flag;
  Value def_value;

  // Supports defining a shortname, i.e. for options in CMD line args
  std::string short_name;

  // Supports disabling a specific option for the CLI integration.
  // Scenario: same option being valid in 2 different parameters even both of
  // them are used for the same thing, it makes no sense to allow both in CLI so
  // we can disable one to avoid unnecessary inconsistency.
  // i.e. Sandbox Operations support password option both in instance and
  // options param.
  bool cmd_line_enabled;

  bool is_deprecated;

  bool operator==(const Parameter &r) const {
    return name == r.name && m_type == r.m_type && flag == r.flag &&
           short_name == r.short_name &&
           cmd_line_enabled == r.cmd_line_enabled &&
           is_deprecated == r.is_deprecated;
  }

  bool operator!=(const Parameter &r) const { return !operator==(r); }

  void validate(const Value &data, Parameter_context *context) const;

  bool valid_type(Value_type type) const;

  void set_type(Value_type type) {
    m_type = type;

    switch (m_type) {
      case Value_type::Object:
        set_validator(std::make_unique<Object_validator>());
        break;

      case Value_type::String:
        set_validator(std::make_unique<String_validator>());
        break;

      case Value_type::Map:
        set_validator(std::make_unique<Option_validator>());
        break;

      case Value_type::Array:
        set_validator(std::make_unique<List_validator>());
        break;
      default:
        // no validator in the default case
        break;
    }
  }

  Value_type type() const { return m_type; }

  void set_validator(std::unique_ptr<Parameter_validator> v) {
    m_validator = std::move(v);
  }

  template <typename T>
    requires std::is_base_of_v<Parameter_validator, T>
  T *validator() const {
    return dynamic_cast<T *>(m_validator.get());
  }

 private:
  Value_type m_type;
  std::unique_ptr<Parameter_validator> m_validator;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_PARAMETER_H_
