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

#ifndef _MOD_CRUD_TABLE_SELECT_H_
#define _MOD_CRUD_TABLE_SELECT_H_

#include "table_crud_definition.h"

namespace mysh
{
  namespace mysqlx
  {
    class Table;

    /**
    * Handler for Select operation on Tables.
    * \todo Document select([field, field, ...])
    * \todo Document where(searchCriteria)
    * \todo Document groupBy([expr, expr, ...])
    * \todo Document having(condition)
    * \todo Document orderBy([expr, expr, ...])
    * \todo Document limit(lim)
    * \todo Document offset(off)
    * \todo Implement and document bind({var:val, var:val, ...})
    * \todo Update execute to support options and document it
    */
    class TableSelect : public Table_crud_definition, public boost::enable_shared_from_this<TableSelect>
    {
    public:
      TableSelect(boost::shared_ptr<Table> owner);
    public:
      virtual std::string class_name() const { return "TableSelect"; }
      shcore::Value select(const shcore::Argument_list &args);
      shcore::Value where(const shcore::Argument_list &args);
      shcore::Value group_by(const shcore::Argument_list &args);
      shcore::Value having(const shcore::Argument_list &args);
      shcore::Value order_by(const shcore::Argument_list &args);
      shcore::Value limit(const shcore::Argument_list &args);
      shcore::Value offset(const shcore::Argument_list &args);
      shcore::Value bind(const shcore::Argument_list &args);

      virtual shcore::Value execute(const shcore::Argument_list &args);
#ifdef DOXYGEN
      TableSelect select([field, field, ...]);
      TableSelect where(searchCriteria);
      TableSelect groupBy([expr, expr, ...]);
      TableSelect having(String searchCondition);
      TableSelect orderBy([expr, expr, ...]);
      TableSelect limit(Integer numberOfRows);
      TableSelect offset(Integer limitOffset);
      TableSelect bind({ var:val, var : val, ... });
      Resultset execute(ExecuteOptions opt)
#endif
    private:
      std::auto_ptr< ::mysqlx::SelectStatement> _select_statement;
    };
  };
};

#endif
