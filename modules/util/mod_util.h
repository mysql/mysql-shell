/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_MOD_UTIL_H_
#define MODULES_UTIL_MOD_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/connection_options.h"

#include "modules/util/dump/dump_instance_options.h"
#include "modules/util/dump/dump_schemas_options.h"
#include "modules/util/dump/dump_tables_options.h"
#include "modules/util/dump/export_table_options.h"
#include "modules/util/import_table/import_table_options.h"
#include "modules/util/load/load_dump_options.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace shcore {
class IShell_core;
}

namespace mysqlsh {

struct Import_json_options {
  std::string schema;
  std::string table;
  std::string collection;
  std::string table_column;
  shcore::Document_reader_options doc_reader;

  static const shcore::Option_pack_def<Import_json_options> &options();
};

/**
 * \defgroup util util
 * \ingroup ShellAPI
 * $(UTIL_BRIEF)
 */
class SHCORE_PUBLIC Util : public shcore::Cpp_object_bridge,
                           public std::enable_shared_from_this<Util> {
 public:
  explicit Util(shcore::IShell_core *owner);

  std::string class_name() const override { return "Util"; };

#if DOXYGEN_JS
  Undefined checkForServerUpgrade(ConnectionData connectionData,
                                  Dictionary options);
  Undefined checkForServerUpgrade(Dictionary options);
#elif DOXYGEN_PY
  None check_for_server_upgrade(ConnectionData connectionData, dict options);
  None check_for_server_upgrade(dict options);
#endif
  void check_for_server_upgrade(
      const mysqlshdk::db::Connection_options &connection_options =
          mysqlshdk::db::Connection_options(),
      const shcore::Option_pack_ref<Upgrade_check_options> &options = {});

#if DOXYGEN_JS
  Undefined importJson(String file, Dictionary options);
#elif DOXYGEN_PY
  None import_json(str file, dict options);
#endif
  void import_json(
      const std::string &file,
      const shcore::Option_pack_ref<Import_json_options> &options = {});

#if DOXYGEN_JS
  Undefined importTable(List files, Dictionary options);
#elif DOXYGEN_PY
  None import_table(list files, dict options);
#endif
  void import_table_file(
      const std::string &filename,
      const shcore::Option_pack_ref<import_table::Import_table_option_pack>
          &options);
  void import_table_files(
      const std::vector<std::string> &filenames,
      const shcore::Option_pack_ref<import_table::Import_table_option_pack>
          &options);

#if DOXYGEN_JS
  Undefined loadDump(String url, Dictionary options);
#elif DOXYGEN_PY
  None load_dump(str url, dict options);
#endif
  void load_dump(const std::string &url,
                 const shcore::Option_pack_ref<Load_dump_options> &options);

#if DOXYGEN_JS
  Undefined exportTable(String table, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
  None export_table(str table, str outputUrl, dict options);
#endif
  void export_table(
      const std::string &table, const std::string &file,
      const shcore::Option_pack_ref<dump::Export_table_options> &options);

#if DOXYGEN_JS
  Undefined dumpTables(String schema, List tables, String outputUrl,
                       Dictionary options);
#elif DOXYGEN_PY
  None dump_tables(str schema, list tables, str outputUrl, dict options);
#endif
  void dump_tables(
      const std::string &schema, const std::vector<std::string> &tables,
      const std::string &directory,
      const shcore::Option_pack_ref<dump::Dump_tables_options> &options);

#if DOXYGEN_JS
  Undefined dumpSchemas(List schemas, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
  None dump_schemas(list schemas, str outputUrl, dict options);
#endif
  void dump_schemas(
      const std::vector<std::string> &schemas, const std::string &directory,
      const shcore::Option_pack_ref<dump::Dump_schemas_options> &options);

#if DOXYGEN_JS
  Undefined dumpInstance(String outputUrl, Dictionary options);
#elif DOXYGEN_PY
  None dump_instance(str outputUrl, dict options);
#endif
  void dump_instance(
      const std::string &directory,
      const shcore::Option_pack_ref<dump::Dump_instance_options> &options);

 private:
  shcore::IShell_core &_shell_core;
};

} /* namespace mysqlsh */

#endif  // MODULES_UTIL_MOD_UTIL_H_
