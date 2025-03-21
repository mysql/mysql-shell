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

#include "mysqlshdk/include/scripting/types/unpacker.h"

#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

Option_unpacker::Option_unpacker(const Dictionary_t &options) {
  set_options(options);
}

void Option_unpacker::set_options(const Dictionary_t &options) {
  m_options = options;
  m_unknown.clear();
  m_missing.clear();
  if (m_options) {
    for (const auto &opt : *m_options) m_unknown.insert(opt.first);
  }
}

Value Option_unpacker::get_required(const char *name, Value_type type) {
  if (!m_options || is_ignored(name)) {
    m_missing.insert(name);
    return {};
  }

  auto opt = m_options->find(name);
  if (opt == m_options->end()) {
    m_missing.insert(name);
    return {};
  }

  m_unknown.erase(name);

  if (type != Undefined && !is_compatible_type(opt->second.get_type(), type)) {
    throw Exception::type_error(str_format(
        "Option '%s' is expected to be of type %s, but is %s", name,
        type_name(type).c_str(), type_name(opt->second.get_type()).c_str()));
  }

  return opt->second;
}

Value Option_unpacker::get_optional(const char *name, Value_type type,
                                    bool case_insensitive) {
  if (!m_options || is_ignored(name)) return {};

  auto opt = m_options->find(name);

  if (case_insensitive && opt == m_options->end()) {
    for (auto it = m_options->begin(); it != m_options->end(); ++it) {
      if (!str_caseeq(it->first.c_str(), name)) continue;
      name = it->first.c_str();
      opt = it;
      break;
    }
  }

  if (opt == m_options->end()) return {};

  m_unknown.erase(name);

  if (type != Undefined && !is_compatible_type(opt->second.get_type(), type)) {
    throw Exception::type_error(str_format(
        "Option '%s' is expected to be of type %s, but is %s", name,
        type_name(type).c_str(), type_name(opt->second.get_type()).c_str()));
  }

  return opt->second;
}

Value Option_unpacker::get_optional_exact(const char *name, Value_type type,
                                          bool case_insensitive) {
  if (!m_options || is_ignored(name)) return {};

  auto opt = m_options->find(name);

  if (case_insensitive && opt == m_options->end()) {
    for (auto it = m_options->begin(); it != m_options->end(); ++it) {
      if (!str_caseeq(it->first.c_str(), name)) continue;
      name = it->first.c_str();
      opt = it;
      break;
    }
  }

  if (opt == m_options->end()) return {};

  m_unknown.erase(name);

  auto opt_type = opt->second.get_type();

  if (type != Undefined &&
      ((opt_type == String && !is_compatible_type(String, type)) ||
       (opt_type != String && opt_type != type))) {
    throw Exception::type_error(
        str_format("Option '%s' is expected to be of type %s, but is %s", name,
                   type_name(type).c_str(), type_name(opt_type).c_str()));
  }

  return opt->second;
}

void Option_unpacker::end(std::string_view context) { validate(context); }

void Option_unpacker::validate(std::string_view context) {
  std::string msg;
  if (!m_unknown.empty() && !m_missing.empty()) {
    msg.append("Invalid and missing options ");
    if (!context.empty()) msg.append(context).append(" ");
    msg.append("(invalid: ").append(str_join(m_unknown, ", "));
    msg.append("), (missing: ").append(str_join(m_missing, ", "));
    msg.append(")");
  } else if (!m_unknown.empty()) {
    msg.append("Invalid options");
    if (!context.empty()) msg.append(" ").append(context);
    msg.append(": ");
    msg.append(str_join(m_unknown, ", "));
  } else if (!m_missing.empty()) {
    msg.append("Missing required options");
    if (!context.empty()) msg.append(" ").append(context);
    msg.append(": ");
    msg.append(str_join(m_missing, ", "));
  }
  if (!msg.empty()) throw Exception::argument_error(msg);
}

}  // namespace shcore
