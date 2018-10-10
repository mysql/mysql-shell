/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_MOD_MYSQLX_SESSION_SQL_H_
#define MODULES_DEVAPI_MOD_MYSQLX_SESSION_SQL_H_

#include <memory>
#include <string>
#include "db/mysqlx/mysqlxclient_clean.h"
#include "modules/devapi/dynamic_object.h"

namespace mysqlsh {
namespace mysqlx {
class Session;
/**
 * \ingroup XDevAPI
 * $(SQLEXECUTE_BRIEF)
 *
 * $(SQLEXECUTE_DETAIL)
 * \sa Session
 */
class SqlExecute : public Dynamic_object,
                   public std::enable_shared_from_this<SqlExecute> {
 public:
#if DOXYGEN_JS
  SqlExecute sql(String statement);
  SqlExecute bind(Value value);
  SqlExecute bind(List values);
  SqlResult execute();
#elif DOXYGEN_PY
  SqlExecute sql(str statement);
  SqlExecute bind(Value value);
  SqlExecute bind(list values);
  SqlResult execute();
#endif
  explicit SqlExecute(std::shared_ptr<Session> owner);
  std::string class_name() const override { return "SqlExecute"; }
  shcore::Value sql(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);
  virtual shcore::Value execute(const shcore::Argument_list &args);

 private:
  std::weak_ptr<Session> _session;
  std::string _sql;
  shcore::Argument_list _parameters;
  Mysqlx::Prepare::Prepare m_prep_stmt;
  uint64_t m_execution_count;

  shcore::Value execute_sql(
      const std::shared_ptr<mysqlsh::mysqlx::Session> &session);

  struct F {
    static constexpr Allowed_function_mask __shell_hook__ = 1 << 0;
    static constexpr Allowed_function_mask sql = 1 << 1;
    static constexpr Allowed_function_mask bind = 1 << 2;
    static constexpr Allowed_function_mask execute = 1 << 3;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
    if ("__shell_hook__" == s) {
      return F::__shell_hook__;
    }
    if ("sql" == s) {
      return F::sql;
    }
    if ("bind" == s) {
      return F::bind;
    }
    if ("execute" == s) {
      return F::execute;
    }
    if ("help" == s) {
      return enabled_functions_;
    }
    return 0;
  }
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_SESSION_SQL_H_
