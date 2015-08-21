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

#ifndef _MOD_CRUD_TABLE_UPDATE_H_
#define _MOD_CRUD_TABLE_UPDATE_H_

#include "table_crud_definition.h"

namespace mysh
{
  namespace mysqlx
  {
    class Table;

    /**
    * Handler for Update operations on Tables.
    * \todo Implement and document bind({var:val, var:val, ...})
    * \todo Update execute to support options and document it
    */
    class TableUpdate : public Table_crud_definition, public boost::enable_shared_from_this<TableUpdate>
    {
    public:
      TableUpdate(boost::shared_ptr<Table> owner);
    public:
      virtual std::string class_name() const { return "TableUpdate"; }
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      shcore::Value update(const shcore::Argument_list &args);
      shcore::Value set(const shcore::Argument_list &args);
      shcore::Value where(const shcore::Argument_list &args);
      shcore::Value order_by(const shcore::Argument_list &args);
      shcore::Value limit(const shcore::Argument_list &args);
      shcore::Value bind(const shcore::Argument_list &args);

      virtual shcore::Value execute(const shcore::Argument_list &args);
#ifdef DOXYGEN
      TableUpdate update();
      TableUpdate set({ field : value, field : value, ... });
      TableUpdate where(searchCriteria);
      TableUpdate orderBy([expr, expr, ...]);
      TableUpdate limit(lim);
      TableUpdate bind({ var:val, var : val, ... });
      ResultSet execute(ExecuteOptions opt);
#endif
    private:
      std::auto_ptr< ::mysqlx::UpdateStatement> _update_statement;
    };
  };
};

#endif
