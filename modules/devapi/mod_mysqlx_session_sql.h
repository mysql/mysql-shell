/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_MOD_MYSQLX_SESSION_SQL_H_
#define MODULES_DEVAPI_MOD_MYSQLX_SESSION_SQL_H_

#include <memory>
#include <string>
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
  virtual std::string class_name() const { return "SqlExecute"; }
  shcore::Value sql(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);
  virtual shcore::Value execute(const shcore::Argument_list &args);

 private:
  std::weak_ptr<Session> _session;
  std::string _sql;
  shcore::Argument_list _parameters;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_SESSION_SQL_H_
