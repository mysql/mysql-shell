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

#include "mysqlshdk/include/scripting/types/validator.h"

#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/include/scripting/types/parameter.h"
#include "mysqlshdk/include/scripting/types/unpacker.h"

namespace shcore {

bool Parameter_validator::valid_type(const Parameter &param,
                                     Value_type type) const {
  return is_compatible_type(type, param.type());
}

void Parameter_validator::validate(const Parameter &param, const Value &data,
                                   Parameter_context *context) const {
  if (!m_enabled) return;
  try {
    // NOTE: Skipping validation for Undefined parameters that are optional as
    // they will use the default value
    if (param.flag == Param_flag::Optional &&
        data.get_type() == Value_type::Undefined)
      return;

    if (!valid_type(param, data.get_type()))
      throw std::invalid_argument("Invalid type");

    // The call to check_type only verifies the kConvertible matrix there:
    // - A string is convertible to integer, bool and float
    // - A Null us convertible to object, array and dictionary
    // We do this validation to make sure the conversion is really valid
    if (data.get_type() == shcore::String) {
      if (param.type() == shcore::Integer)
        data.as_int();
      else if (param.type() == shcore::Bool)
        data.as_bool();
      else if (param.type() == shcore::Float)
        data.as_double();
    }
  } catch (...) {
    auto error =
        shcore::str_format("%s is expected to be %s", context->str().c_str(),
                           shcore::type_description(param.type()).c_str());
    throw shcore::Exception::type_error(error);
  }
}

void Object_validator::validate(const Parameter &param, const Value &data,
                                Parameter_context *context) const {
  if (!m_enabled) return;
  // NOTE: Skipping validation for Undefined parameters that are optional as
  // they will use the default value
  if (param.flag == Param_flag::Optional &&
      data.get_type() == Value_type::Undefined)
    return;
  Parameter_validator::validate(param, data, context);

  if (m_allowed_ref->empty()) return;

  const auto object = data.as_object();

  if (!object) {
    throw shcore::Exception::type_error(shcore::str_format(
        "%s is expected to be an object", context->str().c_str()));
  }

  if (std::find(std::begin(m_allowed), std::end(m_allowed),
                object->class_name()) == std::end(m_allowed)) {
    auto allowed_str = shcore::str_join(m_allowed, ", ");

    if (m_allowed.size() == 1)
      throw shcore::Exception::type_error(
          shcore::str_format("%s is expected to be a '%s' object",
                             context->str().c_str(), allowed_str.c_str()));

    throw shcore::Exception::argument_error(
        shcore::str_format("%s is expected to be one of '%s'",
                           context->str().c_str(), allowed_str.c_str()));
  }
}

void String_validator::validate(const Parameter &param, const Value &data,
                                Parameter_context *context) const {
  if (!m_enabled) return;
  // NOTE: Skipping validation for Undefined parameters that are optional as
  // they will use the default value
  if (param.flag == Param_flag::Optional &&
      data.get_type() == Value_type::Undefined)
    return;
  Parameter_validator::validate(param, data, context);

  if (m_allowed_ref->empty()) return;

  if (std::find(std::begin(*m_allowed_ref), std::end(*m_allowed_ref),
                data.as_string()) == std::end(*m_allowed_ref)) {
    auto error = shcore::str_format(
        "%s only accepts the following values: %s.", context->str().c_str(),
        shcore::str_join(*m_allowed_ref, ", ").c_str());
    throw shcore::Exception::argument_error(error);
  }
}

void Option_validator::validate(const Parameter &param, const Value &data,
                                Parameter_context *context) const {
  if (!m_enabled) return;
  // NOTE: Skipping validation for Undefined parameters that are optional as
  // they will use the default value
  if (param.flag == Param_flag::Optional &&
      data.get_type() == Value_type::Undefined)
    return;
  Parameter_validator::validate(param, data, context);

  if (m_allowed_ref->empty()) return;

  Option_unpacker unpacker(data.as_map());

  for (const auto &item : *m_allowed_ref) {
    shcore::Value value;
    if (item->flag == Param_flag::Mandatory) {
      unpacker.required(item->name.c_str(), &value);
    } else {
      unpacker.optional(item->name.c_str(), &value);
    }

    if (value) {
      context->levels.push_back({"option '" + item->name + "'", {}});
      item->validate(value, context);
      context->levels.pop_back();
    }
  }

  unpacker.end("at " + context->str());
}

bool Nullable_validator::valid_type(const Parameter &param,
                                    Value_type type) const {
  return (Value_type::Null == type) ||
         Parameter_validator::valid_type(param, type);
}

}  // namespace shcore
