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

#ifndef MODULES_DEVAPI_CRUD_DEFINITION_H_
#define MODULES_DEVAPI_CRUD_DEFINITION_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "db/mysqlx/mysqlx_parser.h"
#include "db/mysqlx/mysqlxclient_clean.h"
#include "modules/devapi/dynamic_object.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "scripting/common.h"

namespace mysqlsh {
class DatabaseObject;
namespace mysqlx {

static constexpr char K_LIMIT_PLACEHOLDER[] = ":__LIMIT_PH__";
static constexpr char K_OFFSET_PLACEHOLDER[] = ":__OFFSET_PH__";
static constexpr char K_LIMIT_BIND_TAG[] = "__LIMIT_PH__";
static constexpr char K_OFFSET_BIND_TAG[] = "__OFFSET_PH__";

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

  ~Crud_definition() override;

  // The last step on CRUD operations
  virtual shcore::Value execute(const shcore::Argument_list &args) = 0;
  Crud_definition &bind(const std::string &name, shcore::Value value);

 protected:
  std::shared_ptr<mysqlshdk::db::mysqlx::Result> safe_exec(
      std::function<std::shared_ptr<mysqlshdk::db::IResult>()> func);

  std::shared_ptr<Session> session();
  std::shared_ptr<DatabaseObject> _owner;

  void parse_string_list(const shcore::Argument_list &args,
                         std::vector<std::string> &data);

 protected:
  Mysqlx::Prepare::Prepare m_prep_stmt;
  uint64_t m_execution_count;
  mysqlshdk::utils::nullable<uint64_t> m_limit;
  mysqlshdk::utils::nullable<uint64_t> m_offset;
  std::vector<std::string> _placeholders;
  std::unordered_map<std::string, std::unique_ptr<Mysqlx::Datatypes::Scalar>>
      _bound_values;

  virtual void parse_string_expression(::Mysqlx::Expr::Expr *expr,
                                       const std::string &expr_str) = 0;

  void encode_expression_value(Mysqlx::Expr::Expr *expr,
                               const shcore::Value &value);
  void encode_expression_object(Mysqlx::Expr::Expr *expr,
                                const shcore::Value &value);

  void bind_value(const std::string &name, shcore::Value value);

  void insert_bound_values(
      ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Scalar>
          *target);
  void insert_bound_values(
      ::google::protobuf::RepeatedPtrField<::Mysqlx::Datatypes::Any> *target);
  void validate_bind_placeholder(const std::string &name);
  virtual void set_prepared_stmt() {}
  virtual bool allow_prepared_statements();
  virtual void update_limits(){};
  void reset_prepared_statement();
  bool use_prepared() {
    return allow_prepared_statements() && m_execution_count;
  }

  virtual shcore::Value this_object() { return shcore::Value(); }
  shcore::Value limit(const shcore::Argument_list &args,
                      Dynamic_object::Allowed_function_mask limit_func_id,
                      bool reset_offset);
  shcore::Value offset(const shcore::Argument_list &args,
                       Dynamic_object::Allowed_function_mask limit_func_id,
                       const std::string fname);
  shcore::Value bind_(const shcore::Argument_list &args,
                      Dynamic_object::Allowed_function_mask limit_func_id);

  template <class T>
  void set_limits_on_message(T *message) {
    if (!m_limit.is_null()) {
      if (use_prepared()) {
        message->clear_limit();

        if (!message->has_limit_expr()) {
          message->mutable_limit_expr()->set_allocated_row_count(
              ::mysqlx::parser::parse_collection_filter(K_LIMIT_PLACEHOLDER,
                                                        &_placeholders));
          if (!m_offset.is_null()) {
            message->mutable_limit_expr()->set_allocated_offset(
                ::mysqlx::parser::parse_collection_filter(K_OFFSET_PLACEHOLDER,
                                                          &_placeholders));
          }
        }

        bind_value(K_LIMIT_BIND_TAG, shcore::Value(*m_limit));
        if (!m_offset.is_null())
          bind_value(K_OFFSET_BIND_TAG, shcore::Value(*m_offset));
      } else {
        message->clear_limit_expr();

        message->mutable_limit()->set_row_count(*m_limit);

        if (!m_offset.is_null())
          message->mutable_limit()->set_offset(*m_offset);

        _placeholders.erase(std::remove(_placeholders.begin(),
                                        _placeholders.end(), K_LIMIT_BIND_TAG),
                            _placeholders.end());
        _placeholders.erase(std::remove(_placeholders.begin(),
                                        _placeholders.end(), K_OFFSET_BIND_TAG),
                            _placeholders.end());
      }
    }
  }

 private:
  void validate_placeholders();
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_CRUD_DEFINITION_H_
