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

#ifndef _MOD_CRUD_DEFINITION_H_
#define _MOD_CRUD_DEFINITION_H_

#include "shellcore/types_cpp.h"
#include "shellcore/common.h"

#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "dynamic_object.h"

#include <set>

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

namespace mysh
{
  class DatabaseObject;
  namespace mysqlx
  {
    class Crud_definition : public Dynamic_object
    {
    public:
      Crud_definition(boost::shared_ptr<DatabaseObject> owner);

      // The last step on CRUD operations
      virtual shcore::Value execute(const shcore::Argument_list &args) = 0;
    protected:
      boost::weak_ptr<DatabaseObject> _owner;
      // The CRUD operations will use "dynamic" functions to control the method chaining.
      // A dynamic function is one that will be enabled/disabled based on the method
      // chain sequence.

      void parse_string_list(const shcore::Argument_list &args, std::vector<std::string> &data);
    };
  }
}

#endif
