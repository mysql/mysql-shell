/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/sql_handler_registry.h"

#include "mysqlshdk/include/shellcore/sql_handler.h"

namespace shcore {
void Sql_handler_registry::register_sql_handler(
    const std::string &name, const std::string &description,
    const std::vector<std::string> &prefixes,
    shcore::Function_base_ref callback) {
  if (m_sql_handlers.find(name) != m_sql_handlers.end()) {
    throw std::runtime_error(shcore::str_format(
        "An SQL Handler named '%s' already exists.", name.c_str()));
  }

  if (name.empty()) {
    throw std::runtime_error("An empty name is not allowed.");
  }

  if (description.empty()) {
    throw std::runtime_error("An empty description is not allowed.");
  }

  if (prefixes.empty()) {
    throw std::runtime_error("At least one prefix must be specified.");
  } else {
    for (auto prefix : prefixes) {
      if (shcore::str_strip(prefix).empty()) {
        throw std::runtime_error("Empty or blank prefixes are not allowed.");
      }
    }
  }

  if (!callback) {
    throw std::runtime_error("The callback function must be defined.");
  }

  std::vector<std::string> overlaps;
  for (const auto &prefix : prefixes) {
    try {
      m_sql_handler_matcher.check_for_overlaps(prefix);
    } catch (const mysqlshdk::Prefix_overlap_error &error) {
      overlaps.push_back(
          shcore::str_format("%s defined at the '%s' SQL Handler.",
                             error.what(), error.tag().c_str()));
    }
  }

  if (!overlaps.empty()) {
    throw std::runtime_error(shcore::str_format(
        "The following prefixes overlap with existing prefixes: \n%s",
        shcore::str_join(overlaps, "\n - ").c_str()));
  }

  for (const auto &prefix : prefixes) {
    m_sql_handler_matcher.add_string(prefix, name);
  }

  m_sql_handlers[name] = std::make_shared<Sql_handler>(description, callback);
}

Sql_handler_ptr Sql_handler_registry::get_handler_for_sql(
    std::string_view sql) const {
  auto match = m_sql_handler_matcher.matches(std::string(sql));
  if (match.has_value()) {
    return m_sql_handlers.at(*match);
  }

  return {};
}

}  // namespace shcore
