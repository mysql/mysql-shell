/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef MODULES_DEVAPI_MOD_MYSQLX_SCHEMA_H_
#define MODULES_DEVAPI_MOD_MYSQLX_SCHEMA_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/devapi/base_database_object.h"
#include "modules/mod_common.h"
#include "scripting/types.h"

namespace shcore {
class Proxy_object;
}  // namespace shcore

namespace mysqlsh {
namespace mysqlx {
class Session;
class Table;
class Collection;

/**
 * \ingroup XDevAPI
 * $(SCHEMA_BRIEF)
 *
 * $(SCHEMA_DETAIL)
 *
 * $(SCHEMA_DETAIL1)
 *
 * $(SCHEMA_DETAIL2)
 *
 * $(SCHEMA_DETAIL3)
 *
 * $(SCHEMA_DETAIL4)
 *
 * $(DYNAMIC_PROPERTIES)
 */
class SHCORE_PUBLIC Schema : public DatabaseObject,
                             public std::enable_shared_from_this<Schema> {
 public:
  Schema(std::shared_ptr<Session> owner, const std::string &name);
  ~Schema() noexcept = default;

  std::string class_name() const override { return "Schema"; }

  std::vector<std::string> get_members() const override;
  shcore::Value get_member(const std::string &prop) const override;

  void update_cache() override;
  void _remove_object(const std::string &name, const std::string &type);

  friend class Table;
  friend class Collection;
#if DOXYGEN_JS
  List getTables();
  List getCollections();

  Table getTable(String name);
  Collection getCollection(String name);
  Table getCollectionAsTable(String name);
  Collection createCollection(String name, Dictionary options);
  Undefined modifyCollection(String name, Dictionary options);
  Undefined dropCollection(String name);
#elif DOXYGEN_PY
  list get_tables();
  list get_collections();

  Table get_table(str name);
  Collection get_collection(str name);
  Table get_collection_as_table(str name);
  Collection create_collection(str name, dict options);
  None modify_collection(str name, dict options);
  None drop_collection(str name);
#endif
 public:
  shcore::Value get_tables(const shcore::Argument_list &args);
  shcore::Value get_collections(const shcore::Argument_list &args);
  shcore::Value get_table(const shcore::Argument_list &args);
  shcore::Value get_collection(const shcore::Argument_list &args);
  shcore::Value get_collection_as_table(const shcore::Argument_list &args);
  shcore::Value create_collection(const std::string &name,
                                  const shcore::Dictionary_t &options);
  void modify_collection(const std::string &name,
                         const shcore::Dictionary_t &options);
  shcore::Value drop_schema_object(const shcore::Argument_list &args,
                                   const std::string &type);

 private:
  void init();
  bool use_object_handles() const;
  mutable bool _using_object_handles = false;

  // Object cache
  std::shared_ptr<shcore::Value::Map_type> _tables;
  std::shared_ptr<shcore::Value::Map_type> _collections;
  std::shared_ptr<shcore::Value::Map_type> _views;

  std::function<void(const std::string &, bool exists)> update_table_cache,
      update_view_cache, update_collection_cache;
  std::function<void(const std::vector<std::string> &)> update_full_table_cache,
      update_full_view_cache, update_full_collection_cache;
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_SCHEMA_H_
