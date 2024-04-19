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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SQL_HANDLER_REGISTRY_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SQL_HANDLER_REGISTRY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/string_list_matcher.h"

namespace shcore {
class Sql_handler;
using Sql_handler_ptr = std::shared_ptr<Sql_handler>;
using Sql_handler_vector = std::vector<Sql_handler_ptr>;

/**
 * Registry to store custom SQL Handlers through shell.registerSqlHandler
 */
class Sql_handler_registry {
 public:
  Sql_handler_registry() = default;

  Sql_handler_registry(const Sql_handler_registry &) = delete;
  Sql_handler_registry(Sql_handler_registry &&) = delete;

  Sql_handler_registry &operator=(const Sql_handler_registry &) = delete;
  Sql_handler_registry &operator=(Sql_handler_registry &&) = delete;

  ~Sql_handler_registry() { clear(); }

  void register_sql_handler(const std::string &name,
                            const std::string &description,
                            const std::vector<std::string> &prefixes,
                            shcore::Function_base_ref callback);

  bool empty() const { return m_sql_handlers.empty(); }

  void clear() { m_sql_handlers.clear(); }

  /**
   * Returns the list of SQL handlers that should be activated for a given SQL
   * statement.
   */
  Sql_handler_ptr get_handler_for_sql(std::string_view sql) const;

  const std::map<std::string, Sql_handler_ptr> &get_sql_handlers() const {
    return m_sql_handlers;
  }

 private:
  mysqlshdk::String_prefix_matcher m_sql_handler_matcher;
  std::map<std::string, Sql_handler_ptr> m_sql_handlers;
};

// implemented in scoped_contexts.cc
std::shared_ptr<shcore::Sql_handler_registry> current_sql_handler_registry(
    bool allow_empty = false);

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SQL_HANDLER_REGISTRY_H_
