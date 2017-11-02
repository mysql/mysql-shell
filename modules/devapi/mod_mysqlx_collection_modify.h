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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_MODIFY_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_MODIFY_H_

#include <algorithm>
#include <memory>
#include <string>
#include "modules/devapi/collection_crud_definition.h"

namespace mysqlsh {
namespace mysqlx {
class Collection;
class Result;
/**
* \ingroup XDevAPI
* $(COLLECTIONMODIFY_BRIEF)
*
* $(COLLECTIONMODIFY_DETAIL)
*
* $(COLLECTIONMODIFY_DETAIL1)
*
* \sa Collection
*/
class CollectionModify : public Collection_crud_definition,
                         public std::enable_shared_from_this<CollectionModify> {
 public:
  explicit CollectionModify(std::shared_ptr<Collection> owner);

 public:
#if DOXYGEN_JS
  CollectionModify modify(String searchCondition);
  CollectionModify set(String attribute, Value value);
  CollectionModify unset(String attribute);
  CollectionModify unset(List attributes);
  CollectionModify merge(Document document);
  CollectionModify patch(Document document);
  CollectionModify arrayAppend(String path, Value value);
  CollectionModify arrayInsert(String path, Value value);
  CollectionModify arrayDelete(String path);
  CollectionModify sort(List sortExprStr);
  CollectionModify limit(Integer numberOfRows);
  CollectionModify skip(Integer limitOffset);
  CollectionFind bind(String name, Value value);
  Result execute();
#elif DOXYGEN_PY
  CollectionModify modify(str searchCondition);
  CollectionModify set(str attribute, Value value);
  CollectionModify unset(str attribute);
  CollectionModify unset(list attributes);
  CollectionModify merge(Document document);
  CollectionModify patch(Document document);
  CollectionModify array_append(str path, Value value);
  CollectionModify array_insert(str path, Value value);
  CollectionModify array_delete(str path);
  CollectionModify sort(list sortExprStr);
  CollectionModify limit(int numberOfRows);
  CollectionModify skip(int limitOffset);
  CollectionFind bind(str name, Value value);
  Result execute();
#endif
  virtual std::string class_name() const { return "CollectionModify"; }
  static std::shared_ptr<shcore::Object_bridge> create(
      const shcore::Argument_list &args);
private:
  shcore::Value modify(const shcore::Argument_list &args);
  shcore::Value set(const shcore::Argument_list &args);
  shcore::Value unset(const shcore::Argument_list &args);
  shcore::Value merge(const shcore::Argument_list &args);
  shcore::Value patch(const shcore::Argument_list &args);
  shcore::Value array_insert(const shcore::Argument_list &args);
  shcore::Value array_append(const shcore::Argument_list &args);
  shcore::Value array_delete(const shcore::Argument_list &args);
  shcore::Value sort(const shcore::Argument_list &args);
  shcore::Value limit(const shcore::Argument_list &args);
  shcore::Value bind_(const shcore::Argument_list &args);

  virtual shcore::Value execute(const shcore::Argument_list &args);
  shcore::Value execute();
  friend class Collection;
  Mysqlx::Crud::Update message_;
  CollectionModify &set_filter(const std::string& filter);
  CollectionModify &bind(const std::string &name, shcore::Value value);
  void set_operation(int type, const std::string &path,
                     const shcore::Value &value, bool validate_array = false);
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_MODIFY_H_
