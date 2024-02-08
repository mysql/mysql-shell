/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#ifndef _SHELL_SQL_H_
#define _SHELL_SQL_H_

#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"
#include "scripting/common.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_core.h"

namespace shcore {

struct Sql_result_info {
  double elapsed_seconds = 0.0;
  bool show_vertical = false;
};

class SHCORE_PUBLIC Shell_sql : public Shell_language {
 public:
  explicit Shell_sql(IShell_core *owner);
  virtual ~Shell_sql() {}

  void set_result_processor(
      std::function<void(std::shared_ptr<mysqlshdk::db::IResult>,
                         const Sql_result_info &)>
          result_processor) {
    _result_processor = result_processor;
  }

  std::function<void(std::shared_ptr<mysqlshdk::db::IResult>,
                     const Sql_result_info &)>
  result_processor() const {
    return _result_processor;
  }

  void set_global(const std::string &, const Value &) override {}

  void handle_input(std::string &code, Input_state &state) override;

  void flush_input(const std::string &code) override;

  bool handle_input_stream(std::istream *istream) override;

  void clear_input() override;
  std::string get_continued_input_context() override;

  void print_exception(const shcore::Exception &e);

  const std::string &get_main_delimiter() const {
    return m_splitter->delimiter();
  }

  void kill_query(uint64_t conn_id,
                  const mysqlshdk::db::Connection_options &conn_opts);

  void execute(std::string_view sql);

 private:
  struct Context {
    explicit Context(Shell_sql *parent_);
    ~Context();

    Shell_sql *parent;
    std::string buffer;
    std::string last_handled;
    mysqlshdk::utils::Sql_splitter splitter;
    std::string *old_buffer;
    mysqlshdk::utils::Sql_splitter *old_splitter;
  };

  std::string *m_buffer = nullptr;
  mysqlshdk::utils::Sql_splitter *m_splitter = nullptr;
  Context m_base_context;
  std::function<void(std::shared_ptr<mysqlshdk::db::IResult>,
                     const Sql_result_info &)>
      _result_processor;

  bool process_sql(std::string_view query_str, std::string_view delimiter,
                   size_t line_num,
                   std::shared_ptr<mysqlshdk::db::ISession> session,
                   mysqlshdk::utils::Sql_splitter *splitter);

  std::pair<size_t, bool> handle_command(const char *p, size_t len, bool bol);

  void cmd_process_file(const std::vector<std::string> &params);

  std::shared_ptr<mysqlshdk::db::ISession> get_session();
  void handle_input(std::shared_ptr<mysqlshdk::db::ISession> session,
                    bool flush);
};
}  // namespace shcore

#endif
