/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELL_SQL_H_
#define _SHELL_SQL_H_

#include <stack>
#include "scripting/common.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_core.h"
#include "utils/utils_mysql_parsing.h"

namespace shcore {
class SHCORE_PUBLIC Shell_sql : public Shell_language {
 public:
  Shell_sql(IShell_core *owner);
  virtual ~Shell_sql(){};

  virtual void set_global(const std::string &, const Value &) {}

  virtual void handle_input(
      std::string &code, Input_state &state,
      std::function<void(shcore::Value)> result_processor);

  virtual void clear_input();
  virtual std::string get_continued_input_context();

  virtual bool print_help(const std::string &topic);
  void print_exception(const shcore::Exception &e);
  std::shared_ptr<mysqlsh::ShellBaseSession> get_session();
  const std::string &get_main_delimiter() const {
    return _delimiters.get_main_delimiter();
  }

 private:
  std::string _sql_cache;
  mysql::splitter::Delimiters _delimiters;
  std::stack<std::string> _parsing_context_stack;

  Value process_sql(const std::string &query_str,
                    mysql::splitter::Delimiters::delim_type_t delimiter,
                    std::shared_ptr<mysqlsh::ShellBaseSession> session,
                    std::function<void(shcore::Value)> result_processor);

  void cmd_process_file(const std::vector<std::string> &params);
};
};  // namespace shcore

#endif
