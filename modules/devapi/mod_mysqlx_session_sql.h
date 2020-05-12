/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/mod_mysqlx_resultset.h"

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
  SqlExecute bind(Value data);
  SqlResult execute();
#elif DOXYGEN_PY
  SqlExecute sql(str statement);
  SqlExecute bind(Value data);
  SqlResult execute();
#endif
  explicit SqlExecute(std::shared_ptr<Session> owner);
  std::string class_name() const override { return "SqlExecute"; }
  std::shared_ptr<SqlExecute> sql(const std::string &statement);
  inline void set_sql(const std::string &sql) { _sql = sql; };
  std::shared_ptr<SqlExecute> bind(const shcore::Value &data);
  inline void add_bind(const shcore::Value &value) {
    _parameters->push_back(value);
  }
  std::shared_ptr<SqlResult> execute();

 private:
  std::weak_ptr<Session> _session;
  std::string _sql;
  shcore::Array_t _parameters = shcore::make_array();
  Mysqlx::Prepare::Prepare m_prep_stmt;
  uint64_t m_execution_count;

  std::shared_ptr<SqlResult> execute_sql(
      const std::shared_ptr<mysqlsh::mysqlx::Session> &session);

  struct F {
    static constexpr Allowed_function_mask sql = 1 << 0;
    static constexpr Allowed_function_mask bind = 1 << 1;
    static constexpr Allowed_function_mask execute = 1 << 2;
  };

  Allowed_function_mask function_name_to_bitmask(
      const std::string &s) const override {
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
