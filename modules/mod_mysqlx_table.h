/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

// Interactive Table access module
// (the one exposed as the table members of the db object in the shell)

#ifndef _MOD_MYSQLX_TABLE_H_
#define _MOD_MYSQLX_TABLE_H_

#include "base_database_object.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "mysqlx_crud.h"

#include "mod_mysqlx_table_insert.h"
#include "mod_mysqlx_table_select.h"
#include "mod_mysqlx_table_delete.h"
#include "mod_mysqlx_table_update.h"

namespace mysh
{
  namespace mysqlx
  {
    class Schema;

    /**
    * Represents a Table on an Schema, retrieved with a session created using mysqlx module.
    */
    class Table : public DatabaseObject, public boost::enable_shared_from_this<Table>
    {
    public:
      Table(boost::shared_ptr<Schema> owner, const std::string &name);
      Table(boost::shared_ptr<const Schema> owner, const std::string &name);
      virtual ~Table();

      virtual std::string class_name() const { return "Table"; }
#ifdef DOXYGEN
      TableInsert insert();
      TableInsert insert(List columns);
      TableInsert insert(String col1, String col2, ...);
      TableSelect select();
      TableSelect select(List columns);
      TableUpdate update();
      TableDelete delete();
#endif
    private:
      shcore::Value insert_(const shcore::Argument_list &args);
      shcore::Value select_(const shcore::Argument_list &args);
      shcore::Value update_(const shcore::Argument_list &args);
      shcore::Value delete_(const shcore::Argument_list &args);

      void init();

    private:
      boost::shared_ptr< ::mysqlx::Table> _table_impl;

      // Allows initial functions on the CRUD operations
      friend shcore::Value TableInsert::insert(const shcore::Argument_list &args);
      friend shcore::Value TableSelect::select(const shcore::Argument_list &args);
      friend shcore::Value TableDelete::remove(const shcore::Argument_list &args);
      friend shcore::Value TableUpdate::update(const shcore::Argument_list &args);
    };
  }
}

#endif
