/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

// Interactive Table access module
// (the one exposed as the table members of the db object in the shell)

#ifndef MODULES_DEVAPI_MOD_MYSQLX_TABLE_H_
#define MODULES_DEVAPI_MOD_MYSQLX_TABLE_H_

#include <memory>
#include <string>
#include "modules/devapi/base_database_object.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

#include "modules/devapi/mod_mysqlx_table_delete.h"
#include "modules/devapi/mod_mysqlx_table_insert.h"
#include "modules/devapi/mod_mysqlx_table_select.h"
#include "modules/devapi/mod_mysqlx_table_update.h"

namespace mysqlsh {
namespace mysqlx {
class Schema;

/**
 * \ingroup XDevAPI
 * $(TABLE_BRIEF)
 */
class Table : public DatabaseObject,
              public std::enable_shared_from_this<Table> {
 public:
#if DOXYGEN_JS
  TableInsert insert(...);
  TableSelect select(...);
  TableUpdate update();
  TableDelete delete ();
  Bool isView();
  Integer count();
#elif DOXYGEN_PY
  TableInsert insert(...);
  TableSelect select(...);
  TableUpdate update();
  TableDelete delete ();
  bool is_view();
  int count();
#endif
  Table(std::shared_ptr<Schema> owner, const std::string &name,
        bool is_view = false);
  Table(std::shared_ptr<const Schema> owner, const std::string &name,
        bool is_view = false);
  virtual ~Table();

  std::string class_name() const override { return "Table"; }

  std::string get_object_type() const override {
    return _is_view ? "View" : "Table";
  }

  bool is_view() const { return _is_view; }
  shcore::Value insert_(const shcore::Argument_list &args);
  shcore::Value select_(const shcore::Argument_list &args);
  shcore::Value update_(const shcore::Argument_list &args);
  shcore::Value delete_(const shcore::Argument_list &args);
  shcore::Value is_view_(const shcore::Argument_list &args);

 private:
  bool _is_view;

  void init();
  bool has_count() const override { return true; }

  // Allows initial functions on the CRUD operations
  friend shcore::Value TableInsert::insert(const shcore::Argument_list &args);
  friend shcore::Value TableSelect::select(const shcore::Argument_list &args);
  friend shcore::Value TableDelete::remove(const shcore::Argument_list &args);
  friend shcore::Value TableUpdate::update(const shcore::Argument_list &args);
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_TABLE_H_
