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

#include "crud_definition.h"

namespace mysh
{
  namespace mysqlx
  {
    class TableInsert : public Crud_definition
    {
    public:
      TableInsert(const shcore::Argument_list &args);
    public:
      virtual std::string class_name() const { return "TableInsert"; }
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      shcore::Value insert(const shcore::Argument_list &args);
      shcore::Value values(const shcore::Argument_list &args);
      shcore::Value bind(const shcore::Argument_list &args);
    };
  };
};

#endif
