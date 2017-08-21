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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_REMOVE_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_REMOVE_H_

#include <algorithm>
#include <memory>
#include <string>
#include "modules/devapi/collection_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;

/**
* \ingroup XDevAPI
* $(COLLECTIONREMOVE_BRIEF)
*
* $(COLLECTIONREMOVE_DETAIL)
*
* $(COLLECTIONREMOVE_DETAIL1)
*
* \sa Collection
*/
class CollectionRemove : public Collection_crud_definition,
                         public std::enable_shared_from_this<CollectionRemove> {
 public:
  explicit CollectionRemove(std::shared_ptr<Collection> owner);

 public:
#if DOXYGEN_JS
  CollectionRemove remove(String searchCondition);
  CollectionRemove sort(List sortExprStr);
  CollectionRemove limit(Integer numberOfRows);
  CollectionFind bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  CollectionRemove remove(str searchCondition);
  CollectionRemove sort(list sortExprStr);
  CollectionRemove limit(int numberOfRows);
  CollectionFind bind(str name, Value value);
  Result execute();
#endif
  virtual std::string class_name() const { return "CollectionRemove"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
  shcore::Value remove(const shcore::Argument_list &args);
  shcore::Value sort(const shcore::Argument_list &args);
  shcore::Value limit(const shcore::Argument_list &args);
  shcore::Value bind(const shcore::Argument_list &args);

  virtual shcore::Value execute(const shcore::Argument_list &args);

 private:
   Mysqlx::Crud::Delete message_;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_REMOVE_H_
