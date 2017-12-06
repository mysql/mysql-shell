/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_CRUD_DEFINITION_H_
#define MODULES_DEVAPI_CRUD_DEFINITION_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "modules/devapi/dynamic_object.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "scripting/common.h"
#include "db/mysqlx/mysqlxclient_clean.h"

namespace mysqlsh {
class DatabaseObject;
namespace mysqlx {

#if DOXYGEN_CPP
/**
 * Base class for CRUD operations.
 *
 * The CRUD operations will use "dynamic" functions to control the method
 * chaining.
 * A dynamic function is one that will be enabled/disabled based on the method
 * chain sequence.
 */
#endif
class Crud_definition : public Dynamic_object {
 public:
  explicit Crud_definition(std::shared_ptr<DatabaseObject> owner);

  // The last step on CRUD operations
  virtual shcore::Value execute(const shcore::Argument_list &args) = 0;
 protected:
  std::shared_ptr<mysqlshdk::db::mysqlx::Result> safe_exec(
      std::function<std::shared_ptr<mysqlshdk::db::IResult>()> func);

  std::shared_ptr<Session> session();
  std::shared_ptr<DatabaseObject> _owner;

  void parse_string_list(const shcore::Argument_list &args,
                         std::vector<std::string> &data);

 protected:
  std::vector<std::string> _placeholders;
  std::vector<std::unique_ptr<Mysqlx::Datatypes::Scalar>> _bound_values;

  virtual void parse_string_expression(::Mysqlx::Expr::Expr *expr,
                                       const std::string &expr_str) = 0;

  std::unique_ptr<Mysqlx::Datatypes::Scalar> convert_value(
      const shcore::Value &value);
  void encode_expression_value(Mysqlx::Expr::Expr *expr,
                               const shcore::Value &value);
  void encode_expression_object(Mysqlx::Expr::Expr *expr,
                                const shcore::Value &value);

  void bind_value(const std::string &name, shcore::Value value);

  void insert_bound_values(
      ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Scalar>
          *target);
  void init_bound_values();
  void validate_bind_placeholder(const std::string &name);
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_CRUD_DEFINITION_H_
