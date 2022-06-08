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

#ifndef MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_CONTEXT_H_
#define MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_CONTEXT_H_

#include <memory>
#include <string>
#include <string_view>

#include "mysqlshdk/libs/utils/version.h"

#include "mysqlshdk/libs/parser/code-completion/mysql_code_completion_result.h"

namespace mysqlshdk {

using parsers::Sql_completion_result;

class Sql_completion_context final {
 public:
  Sql_completion_context() = delete;

  explicit Sql_completion_context(const utils::Version &version);

  Sql_completion_context(const Sql_completion_context &) = delete;
  Sql_completion_context(Sql_completion_context &&) = default;

  Sql_completion_context &operator=(const Sql_completion_context &) = delete;
  Sql_completion_context &operator=(Sql_completion_context &&) = default;

  ~Sql_completion_context();

  Sql_completion_result complete(const std::string &sql, std::size_t offset);

  void set_server_version(const utils::Version &version);

  void set_sql_mode(const std::string &sql_mode);

  void set_uppercase_keywords(bool uppercase);

  void set_filtered(bool filtered);

  void set_active_schema(const std::string &active_schema);

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_PARSER_CODE_COMPLETION_MYSQL_CODE_COMPLETION_CONTEXT_H_
