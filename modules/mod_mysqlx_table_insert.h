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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_CRUD_TABLE_INSERT_H_
#define _MOD_CRUD_TABLE_INSERT_H_

#include "table_crud_definition.h"

namespace mysh
{
  namespace mysqlx
  {
    class Table;

    /**
    * Handler for Insert operations on Tables.
    */
    class TableInsert : public Table_crud_definition, public boost::enable_shared_from_this<TableInsert>
    {
    public:
      TableInsert(boost::shared_ptr<Table> owner);
    public:
      virtual std::string class_name() const { return "TableInsert"; }
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      shcore::Value insert(const shcore::Argument_list &args);
      shcore::Value values(const shcore::Argument_list &args);

      virtual shcore::Value execute(const shcore::Argument_list &args);
#ifdef DOXYGEN
      TableInsert insert();
      TableInsert insert(List columns);
      TableInsert insert(String col1, String col2, ...);
      TableInsert values(Value value, Value value, ...);
      Resultset execute(ExecuteOptions options);
#endif
    private:
      std::auto_ptr< ::mysqlx::InsertStatement> _insert_statement;
    };
  };
};

#endif
