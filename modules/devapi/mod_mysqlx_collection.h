/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// Interactive Table access module
// (the one exposed as the table members of the db object in the shell)

#ifndef MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_H_
#define MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_H_

#include <memory>
#include <string>
#include "modules/devapi/base_database_object.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

#include "modules/devapi/mod_mysqlx_collection_add.h"
#include "modules/devapi/mod_mysqlx_collection_find.h"
#include "modules/devapi/mod_mysqlx_collection_modify.h"
#include "modules/devapi/mod_mysqlx_collection_remove.h"

namespace mysqlsh {
namespace mysqlx {
class Schema;
/**
 * \ingroup XDevAPI
 * $(COLLECTION_BRIEF)
 *
 * $(COLLECTION_DETAIL)
 */
class Collection : public DatabaseObject,
                   public std::enable_shared_from_this<Collection> {
 public:
  Collection(std::shared_ptr<Schema> owner, const std::string &name);
  ~Collection();

  std::string class_name() const override { return "Collection"; }

#if DOXYGEN_JS
  CollectionAdd add(...);
  CollectionFind find(...);
  CollectionModify modify(String searchCondition);
  CollectionRemove remove(String searchCondition);
  Result createIndex(String name, JSON indexDefinition);
  Undefined dropIndex(String name);
  Result replaceOne(String id, Document doc);
  Result addOrReplaceOne(String id, Document doc);
  Document getOne(String id);
  Result removeOne(String id);
  Integer count();
#elif DOXYGEN_PY
  CollectionAdd add(...);
  CollectionFind find(...);
  CollectionModify modify(str search_condition);
  CollectionRemove remove(str search_condition);
  Result create_index(str name, JSON indexDefinition);
  None drop_index(str name);
  Result replace_one(str id, document doc);
  Result add_or_replace_one(str id, document doc);
  Document get_one(str id);
  Result remove_one(str id);
  int count();
#endif
  shcore::Value add_(const shcore::Argument_list &args);
  shcore::Value find_(const shcore::Argument_list &args);
  std::shared_ptr<CollectionModify> modify_(
      const std::string &search_condition);
  shcore::Value remove_(const shcore::Argument_list &args);
  shcore::Value create_index_(const shcore::Argument_list &args);
  shcore::Value drop_index_(const shcore::Argument_list &args);
  shcore::Value replace_one_(const shcore::Argument_list &args);
  shcore::Value add_or_replace_one(const shcore::Argument_list &args);
  shcore::Dictionary_t get_one(const std::string &id);
  shcore::Value remove_one(const shcore::Argument_list &args);

 private:
  void init();
  bool has_count() const override { return true; }

  // Allows initial functions on the CRUD operations
  friend shcore::Value CollectionAdd::add(const shcore::Argument_list &args);
  friend shcore::Value CollectionFind::find(const shcore::Argument_list &args);
  friend shcore::Value CollectionRemove::remove(
      const shcore::Argument_list &args);
  friend std::shared_ptr<CollectionModify> CollectionModify::modify(
      const std::string &search_condition);
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_COLLECTION_H_
