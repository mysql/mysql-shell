/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "scripting/types_cpp.h"

namespace shcore {
class Proxy_object;
};

namespace mysqlsh {
namespace mysqlx {
class NodeSession;
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
*/
class SHCORE_PUBLIC Schema : public DatabaseObject,
                             public std::enable_shared_from_this<Schema> {
 public:
  Schema(std::shared_ptr<NodeSession> owner, const std::string &name);
  ~Schema();

  virtual std::string class_name() const { return "Schema"; }

  virtual shcore::Value get_member(const std::string &prop) const;

  virtual void update_cache();
  void _remove_object(const std::string &name, const std::string &type);

  friend class Table;
  friend class Collection;
#if DOXYGEN_JS
  List getTables();
  List getCollections();

  Table getTable(String name);
  Collection getCollection(String name);
  Table getCollectionAsTable(String name);
  Collection createCollection(String name);
#elif DOXYGEN_PY
  list get_tables();
  list get_collections();

  Table get_table(str name);
  Collection get_collection(str name);
  Table get_collection_as_table(str name);
  Collection create_collection(str name);
#endif
 public:
  shcore::Value get_tables(const shcore::Argument_list &args);
  shcore::Value get_collections(const shcore::Argument_list &args);
  shcore::Value get_table(const shcore::Argument_list &args);
  shcore::Value get_collection(const shcore::Argument_list &args);
  shcore::Value get_collection_as_table(const shcore::Argument_list &args);
  shcore::Value create_collection(const shcore::Argument_list &args);

private:
  void init();

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
