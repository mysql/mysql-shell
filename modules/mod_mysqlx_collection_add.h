/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_CRUD_COLLECTION_ADD_H_
#define _MOD_CRUD_COLLECTION_ADD_H_

#include "collection_crud_definition.h"

namespace mysh {
namespace mysqlx {
class Collection;

/**
* Handler for document addition on a Collection.
*
* This object provides the necessary functions to allow adding documents into a collection.
*
* This object should only be created by calling any of the add functions on the collection object where the documents will be added.
*
* \sa Collection
*/
class CollectionAdd : public Collection_crud_definition, public std::enable_shared_from_this<CollectionAdd> {
public:
  CollectionAdd(std::shared_ptr<Collection> owner);

  virtual std::string class_name() const { return "CollectionAdd"; }

  shcore::Value add(const shcore::Argument_list &args);
  virtual shcore::Value execute(const shcore::Argument_list &args);

#if DOXYGEN_JS
  CollectionAdd add(DocDefinition document[, DocDefinition document, ...]);
  Result execute();
#elif DOXYGEN_PY
  CollectionAdd add(DocDefinition document[, DocDefinition document, ...]);
  Result execute();
#endif

private:
  std::string get_new_uuid();

  std::unique_ptr< ::mysqlx::AddStatement> _add_statement;
};
}
}

#endif
