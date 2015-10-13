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

#ifndef _MOD_CRUD_COLLECTION_DROP_INDEX_H_
#define _MOD_CRUD_COLLECTION_DROP_INDEX_H_

#include "dynamic_object.h"

namespace mysh
{
  namespace mysqlx
  {
    class Collection;

    /**
    * Handler for index dropping handler on a Collection.
    *
    * This object allows dropping an index from a collection.
    *
    * This object should only be created by calling the dropIndex function on the collection object where the index will be removed.
    *
    * \sa Collection
    */
    class CollectionDropIndex : public Dynamic_object, public boost::enable_shared_from_this<CollectionDropIndex>
    {
    public:
      CollectionDropIndex(boost::shared_ptr<Collection> owner);

      virtual std::string class_name() const { return "CollectionDropIndex"; }

      shcore::Value drop_index(const shcore::Argument_list &args);
      virtual shcore::Value execute(const shcore::Argument_list &args);

#ifdef DOXYGEN
      CollectionDropIndex dropIndex(Document document);
      CollectionDropIndex execute(List documents);
      Result execute();
#endif

    private:
      boost::weak_ptr<Collection> _owner;
      shcore::Argument_list _drop_index_args;
    };
  }
}

#endif
