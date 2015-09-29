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

#ifndef _MOD_CRUD_COLLECTION_REMOVE_H_
#define _MOD_CRUD_COLLECTION_REMOVE_H_

#include "collection_crud_definition.h"

namespace mysh
{
  namespace mysqlx
  {
    class Collection;

    /**
    * Handler for document removal from a Collection.
    *
    * This object provides the necessary functions to allow removing documents from a collection.
    *
    * This object should only be created by calling the remove function on the collection object from which the documents will be removed.
    *
    * \sa Collection
    */
    class CollectionRemove : public Collection_crud_definition, public boost::enable_shared_from_this<CollectionRemove>
    {
    public:
      CollectionRemove(boost::shared_ptr<Collection> owner);
    public:
      virtual std::string class_name() const { return "CollectionRemove"; }
      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      shcore::Value remove(const shcore::Argument_list &args);
      shcore::Value sort(const shcore::Argument_list &args);
      shcore::Value limit(const shcore::Argument_list &args);
      shcore::Value bind(const shcore::Argument_list &args);

      virtual shcore::Value execute(const shcore::Argument_list &args);
#ifdef DOXYGEN
      CollectionRemove remove(String searchCondition);
      CollectionRemove sort(List sortExprStr);
      CollectionRemove limit(Integer numberOfRows);
      CollectionFind bind(String name, Value value);
      CollectionResultset execute(ExecuteOptions opt);
#endif
    private:
      std::auto_ptr< ::mysqlx::RemoveStatement> _remove_statement;
    };
  };
};

#endif
