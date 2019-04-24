/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/nullable_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace utils {

using nullable_options::Comparison_mode;
using nullable_options::Set_mode;

Nullable_options::Nullable_options(Comparison_mode mode,
                                   const std::string &context)
    : _ctx(context),
      _mode(mode),
      _options(_mode == Comparison_mode::CASE_SENSITIVE
                   ? Nullable_options::comp
                   : Nullable_options::icomp),
      _defaults(_mode == Comparison_mode::CASE_SENSITIVE
                    ? Nullable_options::comp
                    : Nullable_options::icomp) {
  if (!_ctx.empty() && _ctx[_ctx.size() - 1] != ' ') _ctx.append(" ");
}

int Nullable_options::compare(const std::string &lhs,
                              const std::string &rhs) const {
  if (_mode == Comparison_mode::CASE_SENSITIVE)
    return lhs.compare(rhs);
  else
    return shcore::str_casecmp(lhs.c_str(), rhs.c_str());
}

bool Nullable_options::comp(const std::string &lhs, const std::string &rhs) {
  return lhs.compare(rhs) < 0;
}

bool Nullable_options::icomp(const std::string &lhs, const std::string &rhs) {
  return shcore::str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

bool Nullable_options::has(const std::string &name) const {
  return _options.find(name) != _options.end();
}

bool Nullable_options::has_value(const std::string &name) const {
  if (!has(name)) throw_invalid_option(name);

  return !_options.at(name).is_null();
}

void Nullable_options::set_unchecked(const std::string &name,
                                     const char *value) {
  _options[name] = value;
}

void Nullable_options::set(const std::string &name, const std::string &value,
                           Set_mode mode) {
  // The option is meant to be created
  if (mode == Set_mode::CREATE && has(name)) throw_already_defined_option(name);

  // The option is meant to be updated (So must exist)
  if (mode == Set_mode::UPDATE_ONLY && !has(name)) throw_invalid_option(name);

  // The option is meant to be updated if null (So must exist and be empty)
  if (mode == Set_mode::UPDATE_NULL) {
    if (!has(name))
      throw_invalid_option(name);
    else if (has_value(name))
      throw_already_defined_option(name);
  }
  _options[name] = value;
}

void Nullable_options::set(const std::string &name, const char *value,
                           Set_mode mode) {
  // The option is meant to be created
  if (mode == Set_mode::CREATE && has(name)) throw_already_defined_option(name);

  // The option is meant to be updated (So must exist)
  if (mode == Set_mode::UPDATE_ONLY && !has(name)) throw_invalid_option(name);

  // The option is meant to be updated if null (So must exist and be empty)
  if (mode == Set_mode::UPDATE_NULL) {
    if (!has(name))
      throw_invalid_option(name);
    else if (has_value(name))
      throw_already_defined_option(name);
  }

  if (value)
    _options[name] = value;
  else
    _options[name] = nullable<std::string>();
}

void Nullable_options::set_default(const std::string &name, const char *value) {
  if (value)
    _defaults[name] = value;
  else {
    if (_defaults.find(name) != _defaults.end()) {
      _defaults.erase(name);
    }
  }
}

bool Nullable_options::has_default(const std::string &name) const {
  auto pos = _defaults.find(name);
  return pos != _defaults.end();
}

std::string Nullable_options::get_default(const std::string &name) const {
  if (has_default(name)) return *_defaults.at(name);

  return "";
}

const std::string &Nullable_options::get_value(const std::string &name) const {
  //  if (!has(name)) throw_invalid_option(name);

  if (!has_value(name)) throw_no_value(name);

  return *_options.at(name);
}

void Nullable_options::clear_value(const std::string &name) {
  if (!has(name)) throw_invalid_option(name);

  _options.at(name).reset();
}

void Nullable_options::remove(const std::string &name) {
  if (!has(name)) throw_invalid_option(name);

  _options.erase(name);
}

void Nullable_options::throw_invalid_option(const std::string &name) const {
  throw std::invalid_argument("Invalid " + _ctx + "option '" + name + "'.");
}

void Nullable_options::throw_no_value(const std::string &name) const {
  throw std::invalid_argument("The " + _ctx + "option '" + name +
                              "' has no "
                              "value.");
}

void Nullable_options::throw_already_defined_option(
    const std::string &name) const {
  std::string value;
  if (has_value(name)) value = " as '" + get_value(name) + "'";

  throw std::invalid_argument("The " + _ctx + "option '" + name +
                              "' is "
                              "already defined" +
                              value + ".");
}

bool Nullable_options::operator==(const Nullable_options &other) const {
  // Tests for different case sensitiveness
  if (_mode != other._mode) return false;

  // Tests for different context
  if (_ctx != other._ctx) return false;

  // Tests for different number of options
  if (size() != other.size()) return false;

  for (auto o : _options) {
    // Tests for options being contained on both
    if (!other.has(o.first)) {
      return false;
    } else {
      bool _has_value = has_value(o.first);
      // tests for options havin or not value on both
      if (_has_value != other.has_value(o.first))
        return false;
      else if (_has_value)
        // tests for options having the same value on both
        // NOTE(rennox): Value comparison is Case Sensitive in all cases
        return *o.second == other.get_value(o.first);
    }
  }

  return true;
}

bool Nullable_options::operator!=(const Nullable_options &other) const {
  return !(*this == other);
}

}  // namespace utils
}  // namespace mysqlshdk
