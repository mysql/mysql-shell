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

// Interactive ClassicTable access module
// (the one exposed as the table members of the db object in the shell)

#ifndef _MOD_MYSQL_TABLE_H_
#define _MOD_MYSQL_TABLE_H_

#include "base_database_object.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

namespace mysh
{
  namespace mysql
  {
    class ClassicSchema;

    /**
    * Represents a ClassicTable on an ClassicSchema, retrieved with a session created using the MySQL Protocol.
    */
    class ClassicTable : public DatabaseObject
    {
    public:
      ClassicTable(boost::shared_ptr<ClassicSchema> owner, const std::string &name);
      ClassicTable(boost::shared_ptr<const ClassicSchema> owner, const std::string &name);
      virtual ~ClassicTable();

      virtual std::string class_name() const { return "ClassicTable"; }
    };
  }
}

#endif
