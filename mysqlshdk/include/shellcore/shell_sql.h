/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <stack>

#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "scripting/common.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_core.h"

namespace shcore {

struct Sql_result_info {
  double ellapsed_seconds = 0.0;
  bool show_vertical = false;
};

class SHCORE_PUBLIC Shell_sql : public Shell_language {
 public:
  Shell_sql(IShell_core *owner);
  virtual ~Shell_sql() {}

  void set_result_processor(
      std::function<void(std::shared_ptr<mysqlshdk::db::IResult>,
                         const Sql_result_info &)>
          result_processor) {
    _result_processor = result_processor;
  }

  virtual void set_global(const std::string &, const Value &) {}

  virtual void handle_input(std::string &code, Input_state &state);

  virtual void clear_input();
  virtual std::string get_continued_input_context();

  void print_exception(const shcore::Exception &e);

  const std::string &get_main_delimiter() const {
    return _delimiters.get_main_delimiter();
  }

  void kill_query(uint64_t conn_id,
                  const mysqlshdk::db::Connection_options &conn_opts);

 private:
  std::string _sql_cache;
  mysql::splitter::Delimiters _delimiters;
  std::stack<std::string> _parsing_context_stack;
  std::function<void(std::shared_ptr<mysqlshdk::db::IResult>,
                     const Sql_result_info &)>
      _result_processor;

  bool process_sql(const std::string &query_str,
                   mysql::splitter::Delimiters::delim_type_t delimiter,
                   std::shared_ptr<mysqlshdk::db::ISession> session);

  void cmd_process_file(const std::vector<std::string> &params);
};
};  // namespace shcore

#endif
