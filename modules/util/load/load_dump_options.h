/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_LOAD_LOAD_DUMP_OPTIONS_H_
#define MODULES_UTIL_LOAD_LOAD_DUMP_OPTIONS_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "modules/mod_utils.h"
#include "modules/util/import_table/helpers.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlsh {

inline std::string schema_table_key(const std::string &schema,
                                    const std::string &table) {
  return shcore::quote_identifier(schema) + "." +
         shcore::quote_identifier(table);
}

class Load_dump_options final {
 public:
  using Connection_options = mysqlshdk::db::Connection_options;

  enum class Analyze_table_mode { OFF, HISTOGRAM, ON };

  Load_dump_options() = default;

  explicit Load_dump_options(const std::string &url);

  Load_dump_options(const Load_dump_options &other) = default;
  Load_dump_options(Load_dump_options &&other) = default;

  Load_dump_options &operator=(const Load_dump_options &other) = default;
  Load_dump_options &operator=(Load_dump_options &&other) = default;

  ~Load_dump_options() = default;

  void validate();

  void set_options(const shcore::Dictionary_t &options);

  void set_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  mysqlshdk::db::ISession *base_session() const { return m_base_session.get(); }

  const Connection_options &connection_options() const { return m_target; }

  std::string target_import_info() const;

  std::unique_ptr<mysqlshdk::storage::IDirectory> create_dump_handle() const;

  std::unique_ptr<mysqlshdk::storage::IFile> create_progress_file_handle()
      const;

  const mysqlshdk::null_string &progress_file() const {
    return m_progress_file;
  }

  const mysqlshdk::oci::Oci_options &oci_options() const {
    return m_oci_options;
  }

  bool show_progress() const { return m_show_progress; }

  int64_t threads_count() const { return m_threads_count; }

  uint64_t dump_wait_timeout() const { return m_wait_dump_timeout; }

  const std::string &default_character_set() const {
    return m_default_character_set;
  }

  bool load_data() const { return m_load_data; }
  bool load_ddl() const { return m_load_ddl; }
  bool load_users() const { return m_load_users; }

  bool dry_run() const { return m_dry_run; }

  bool reset_progress() const { return m_reset_progress; }

  bool skip_binlog() const { return m_skip_binlog; }

  bool force() const { return m_force; }

  bool include_schema(const std::string &schema) const;
  bool include_table(const std::string &schema, const std::string &table) const;

  bool ignore_existing_objects() const { return m_ignore_existing_objects; }

  bool ignore_version() const { return m_ignore_version; }

  Analyze_table_mode analyze_tables() const { return m_analyze_tables; }

  static std::vector<std::string> get_excluded_users(bool is_mds);

 private:
  std::string m_url;
  int64_t m_threads_count = 4;
  bool m_show_progress = isatty(fileno(stdout)) ? true : false;

  mysqlshdk::oci::Oci_options m_oci_options;
  Connection_options m_target;
  std::shared_ptr<mysqlshdk::db::ISession> m_base_session;

  std::unordered_set<std::string> m_include_schemas;  // only load these schemas
  std::unordered_set<std::string> m_include_tables;   // only load these tables
  std::unordered_set<std::string> m_exclude_schemas;  // skip these schemas
  std::unordered_set<std::string> m_exclude_tables;   // skip these tables

  uint64_t m_wait_dump_timeout = 0;
  bool m_reset_progress = false;
  mysqlshdk::null_string m_progress_file;
  std::string m_default_progress_file;
  std::string m_default_character_set;
  bool m_load_data = true;
  bool m_load_ddl = true;
  bool m_load_users = false;
  Analyze_table_mode m_analyze_tables = Analyze_table_mode::OFF;
  bool m_dry_run = false;
  bool m_force = false;
  bool m_skip_binlog = false;
  bool m_ignore_existing_objects = false;
  bool m_ignore_version = false;
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_LOAD_DUMP_OPTIONS_H_
