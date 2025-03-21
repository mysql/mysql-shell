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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_VALIDATOR_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_VALIDATOR_H_

#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"

namespace shcore {

struct Parameter;
struct Parameter_context;

struct Parameter_validator {
 public:
  virtual ~Parameter_validator() = default;
  virtual bool valid_type(const Parameter &param, Value_type type) const;
  virtual void validate(const Parameter &param, const Value &data,
                        Parameter_context *context) const;
  void set_enabled(bool enabled) { m_enabled = enabled; }

 protected:
  // Parameter validators were initially created to validate data provided on
  // functions in EXTENSION OBJECTS: when a function is an API extension object
  // is called the parameter validators are executed, this way we ensure the
  // same validation scheme is used in all extension objects.
  //
  // The parameter validators contain metadata that is useful in CLI integration
  // so we are overloading its use, not only for extension objects but also for
  // any API exposed from C++. This is: we require the metadata for proper CLI
  // integration but we do not need the validators to be executed, for this
  // reason this flag is required.
  bool m_enabled = true;
};

/**
 * Holds the element types to be allowed, Undefined for ANY
 */
struct List_validator : public Parameter_validator {
 public:
  void set_element_type(shcore::Value_type type) { m_element_type = type; }
  shcore::Value_type element_type() const { return m_element_type; }

 protected:
  shcore::Value_type m_element_type = shcore::Undefined;
};

/**
 * This is used as a base class for validators that enable defining specific set
 * of allowed data:
 *
 * - String validator could define set of allowed values.
 * - Option validator could define set of allowed options.
 * - List validator could define set of allowed items.
 *
 * Also enables referring to the allowed list either internally or externally.
 */
template <typename T>
struct Parameter_validator_with_allowed : public Parameter_validator {
 public:
  Parameter_validator_with_allowed() : m_allowed_ref(&m_allowed) {}
  void set_allowed(std::vector<T> &&allowed) {
    m_allowed = std::move(allowed);
    set_allowed(&m_allowed);
  }

  void set_allowed(const std::vector<T> *allowed) { m_allowed_ref = allowed; }

  const std::vector<T> &allowed() const { return *m_allowed_ref; }

 protected:
  std::vector<T> m_allowed;
  const std::vector<T> *m_allowed_ref;
};

struct Object_validator : public Parameter_validator_with_allowed<std::string> {
  void validate(const Parameter &param, const Value &data,
                Parameter_context *context) const override;
};

struct String_validator : public Parameter_validator_with_allowed<std::string> {
  void validate(const Parameter &param, const Value &data,
                Parameter_context *context) const override;
};

struct Option_validator
    : public Parameter_validator_with_allowed<std::shared_ptr<Parameter>> {
  void validate(const Parameter &param, const Value &data,
                Parameter_context *context) const override;
};

struct Nullable_validator : public Parameter_validator {
 public:
  bool valid_type(const Parameter &param, Value_type type) const override;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_VALIDATOR_H_
