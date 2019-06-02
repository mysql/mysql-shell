/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_
#define MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <memory>
#include <string>
#include <vector>
#include "modules/util/import_table/dialect.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace import_table {

using Connection_options = mysqlshdk::db::Connection_options;

class Import_table_options {
 public:
  Import_table_options() = default;

  explicit Import_table_options(const std::string &filename,
                                const shcore::Dictionary_t &options);

  Import_table_options(const Import_table_options &other) = default;
  Import_table_options(Import_table_options &&other) = default;

  Import_table_options &operator=(const Import_table_options &other) = default;
  Import_table_options &operator=(Import_table_options &&other) = default;

  ~Import_table_options() = default;

  Dialect dialect() const { return m_dialect; }

  void validate();

  void base_session(const std::shared_ptr<mysqlsh::ShellBaseSession> &session) {
    m_base_session = session;
  }

  Connection_options connection_options() const;

  const std::string &full_path() const { return m_full_path; }

  size_t max_rate() const;

  bool replace_duplicates() const { return m_replace_duplicates; }

  const std::vector<std::string> &columns() const { return m_columns; }

  bool show_progress() const { return m_show_progress; }

  uint64_t skip_rows_count() const { return m_skip_rows_count; }

  int64_t threads_size() const { return m_threads_size; }

  const std::string &table() const { return m_table; }

  const std::string &schema() const { return m_schema; }

  size_t file_size() const { return m_file_size; }

  size_t bytes_per_chunk() const;

  std::string target_import_info() const;

 private:
  void unpack(const shcore::Dictionary_t &options);

  size_t calc_thread_size();

  std::string m_filename;
  std::string m_full_path;
  size_t m_file_size;
  std::string m_table;
  std::string m_schema;
  int64_t m_threads_size = 8;
  std::string m_bytes_per_chunk{"50M"};
  std::vector<std::string> m_columns;
  bool m_replace_duplicates = false;
  std::string m_max_rate;
  bool m_show_progress = isatty(fileno(stdout)) ? true : false;
  uint64_t m_skip_rows_count = 0;
  std::string m_base_dialect_name;
  Dialect m_dialect;

  struct Oci_shell_options final {
   public:
    Oci_shell_options();
    Oci_shell_options(const Oci_shell_options &other) = default;
    Oci_shell_options(Oci_shell_options &&other) = default;

    Oci_shell_options &operator=(const Oci_shell_options &other) = default;
    Oci_shell_options &operator=(Oci_shell_options &&other) = default;

    ~Oci_shell_options();

    std::string profile;
    std::string config_file;
    std::string profile_original;
    std::string config_file_original;
  };
  Oci_shell_options m_oci;
  std::shared_ptr<mysqlsh::ShellBaseSession> m_base_session;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_IMPORT_TABLE_OPTIONS_H_
