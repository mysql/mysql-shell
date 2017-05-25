/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_DROP_INDEX_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_DROP_INDEX_H_

#include <memory>
#include <string>
#include "modules/devapi/dynamic_object.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;

/**
* \ingroup XDevAPI
* $(COLLECTIONDROPINDEX_BRIEF)
*
* $(COLLECTIONDROPINDEX_DETAIL)
*
* $(COLLECTIONDROPINDEX_DETAIL1)
*
* \sa Collection
*/
class CollectionDropIndex
    : public Dynamic_object,
      public std::enable_shared_from_this<CollectionDropIndex> {
 public:
  explicit CollectionDropIndex(std::shared_ptr<Collection> owner);

  virtual std::string class_name() const { return "CollectionDropIndex"; }

  shcore::Value drop_index(const shcore::Argument_list &args);
  virtual shcore::Value execute(const shcore::Argument_list &args);

#if DOXYGEN_JS
  CollectionDropIndex dropIndex(String indexName);
  Result execute();
#elif DOXYGEN_PY
  CollectionDropIndex drop_index(str indexName);
  Result execute();
#endif

 private:
  std::weak_ptr<Collection> _owner;
  shcore::Argument_list _drop_index_args;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_DROP_INDEX_H_
