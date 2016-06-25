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

#ifndef _MOD_COLLECTION_CRUD_DEFINITION_H_
#define _MOD_COLLECTION_CRUD_DEFINITION_H_

#include "shellcore/types_cpp.h"
#include "shellcore/common.h"
#include "crud_definition.h"
#include "mysqlx_crud.h"
#include "mysqlxtest_utils.h"

#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <set>

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

namespace mysh
{
  namespace mysqlx
  {
#if DOXYGEN_CPP
    //! Base class for collection CRUD operations.
#endif
    class Collection_crud_definition : public Crud_definition
    {
    public:
      Collection_crud_definition(boost::shared_ptr<DatabaseObject> owner) :Crud_definition(owner){}

    protected:
      ::mysqlx::DocumentValue map_document_value(shcore::Value source);
    };
  }
}

#endif
