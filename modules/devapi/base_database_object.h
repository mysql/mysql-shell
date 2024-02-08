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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef MODULES_DEVAPI_BASE_DATABASE_OBJECT_H_
#define MODULES_DEVAPI_BASE_DATABASE_OBJECT_H_

#include <memory>
#include <string>
#include <vector>
#include "modules/mod_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace shcore {
class Proxy_object;
}  // namespace shcore

namespace mysqlsh {
class ShellBaseSession;
class CoreSchema;
/**
 * \ingroup ShellAPI
 * $(DATABASEOBJECT_BRIEF)
 */
class SHCORE_PUBLIC DatabaseObject : public shcore::Cpp_object_bridge {
 public:
  DatabaseObject(std::shared_ptr<ShellBaseSession> session,
                 std::shared_ptr<DatabaseObject> schema,
                 const std::string &name);
  ~DatabaseObject();

  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  virtual bool operator==(const Object_bridge &other) const;

  shcore::Value existsInDatabase(const shcore::Argument_list &args);

  virtual std::string get_object_type() const { return class_name(); }

  const std::string &name() const { return _name; }

  std::shared_ptr<ShellBaseSession> session() const { return _session.lock(); }

  std::shared_ptr<DatabaseObject> schema() const { return _schema.lock(); }

#if DOXYGEN_JS
  String name;     //!< $(DATABASEOBJECT_NAME_BRIEF)
  Object session;  //!< $(DATABASEOBJECT_SESSION_BRIEF)
  Object schema;   //!< $(DATABASEOBJECT_SCHEMA_BRIEF)

  String getName();
  Object getSession();
  Object getSchema();
  Bool existsInDatabase();
#elif DOXYGEN_PY
  str name;        //!< $(DATABASEOBJECT_NAME_BRIEF)
  object session;  //!< $(DATABASEOBJECT_SESSION_BRIEF)
  object schema;   //!< $(DATABASEOBJECT_SCHEMA_BRIEF)

  str get_name();
  object get_session();
  object get_schema();
  bool exists_in_database();
#endif

 protected:
  std::weak_ptr<ShellBaseSession> _session;
  std::weak_ptr<DatabaseObject> _schema;
  std::string _name;

  size_t _base_property_count;
  bool is_base_member(const std::string &prop) const;
  virtual bool has_count() const { return false; }
  uint64_t count();

 private:
  void init();
  // Handling of database object caches
 public:
  typedef std::shared_ptr<shcore::Value::Map_type> Cache;
  static void update_cache(
      const std::vector<std::string> &names,
      const std::function<shcore::Value(const std::string &name)> &generator,
      Cache target_cache, DatabaseObject *target = NULL);
  static void update_cache(
      const std::string &name,
      const std::function<shcore::Value(const std::string &name)> &generator,
      bool exists, Cache target_cache, DatabaseObject *target = NULL);
  static void flush_cache(Cache target_cache, DatabaseObject *target = NULL);
  static void get_object_list(Cache target_cache,
                              shcore::Value::Array_type_ref list);
  static shcore::Value find_in_cache(const std::string &name,
                                     Cache target_cache);
  virtual void update_cache() {}
};
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_BASE_DATABASE_OBJECT_H_
