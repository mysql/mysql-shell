/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_API_H_
#define MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_API_H_

#include <optional>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk {

class Auto_complete_sql_options final {
 public:
  Auto_complete_sql_options() = default;

  Auto_complete_sql_options(const Auto_complete_sql_options &) = default;
  Auto_complete_sql_options(Auto_complete_sql_options &&) = default;

  Auto_complete_sql_options &operator=(const Auto_complete_sql_options &) =
      default;
  Auto_complete_sql_options &operator=(Auto_complete_sql_options &&) = default;

  ~Auto_complete_sql_options() = default;

  static constexpr const char *server_version_option() {
    return "serverVersion";
  }

  static constexpr const char *sql_mode_option() { return "sqlMode"; }

  static constexpr const char *statement_offset_option() {
    return "statementOffset";
  }

  static constexpr const char *uppercase_keywords_option() {
    return "uppercaseKeywords";
  }

  static constexpr const char *filtered_option() { return "filtered"; }

  static const shcore::Option_pack_def<Auto_complete_sql_options> &options();

  const utils::Version &server_version() const { return m_server_version; }

  const std::string &sql_mode() const { return m_sql_mode; }

  const std::optional<std::size_t> &statement_offset() const {
    return m_statement_offset;
  }

  bool uppercase_keywords() const { return m_uppercase_keywords; }

  bool filtered() const { return m_filtered; }

 private:
  void set_server_version(const std::string &version) {
    m_server_version = utils::Version(version);
  }

  void on_unpacked_options();

  utils::Version m_server_version;
  std::string m_sql_mode;
  std::optional<std::size_t> m_statement_offset;
  bool m_uppercase_keywords = true;
  bool m_filtered = true;
};

shcore::Dictionary_t auto_complete_sql(
    const std::string &sql, const Auto_complete_sql_options &options);

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_API_H_
