/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_CREATE_INDEX_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_CREATE_INDEX_H_

#include <memory>
#include <string>
#include "modules/devapi/dynamic_object.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;

/**
* \ingroup XDevAPI
* $(COLLECTIONCREATEINDEX_BRIEF)
*
* $(COLLECTIONCREATEINDEX_DETAIL)
*
* $(COLLECTIONCREATEINDEX_DETAIL1)
*
* \sa Collection
*/
class CollectionCreateIndex
    : public Dynamic_object,
      public std::enable_shared_from_this<CollectionCreateIndex> {
 public:
  explicit CollectionCreateIndex(std::shared_ptr<Collection> owner);

  virtual std::string class_name() const { return "CollectionCreateIndex"; }

  shcore::Value create_index(const shcore::Argument_list &args);
  shcore::Value field(const shcore::Argument_list &args);
  virtual shcore::Value execute(const shcore::Argument_list &args);

#if DOXYGEN_JS
  CollectionCreateIndex createIndex(String name);
  CollectionCreateIndex createIndex(String name, IndexType type);
  CollectionCreateIndex field(DocPath documentPath, IndexColumnType type,
                              Bool isRequired);
  Result execute();
#elif DOXYGEN_PY
  CollectionCreateIndex create_index(str name);
  CollectionCreateIndex create_index(str name, IndexType type);
  CollectionCreateIndex field(DocPath documentPath, IndexColumnType type,
                              bool isRequired);
  Result execute();
#endif

 private:
  std::weak_ptr<Collection> _owner;
  shcore::Argument_list _create_index_args;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_CREATE_INDEX_H_
