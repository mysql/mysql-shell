
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

#include "shellcore/shell_sql.h"
#include <fstream>
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "utils/utils_string.h"

REGISTER_HELP(CMD_G_UC_BRIEF,
              "Send command to mysql server, display result vertically.");
REGISTER_HELP(CMD_G_UC_SYNTAX, "<statement>\\G");
REGISTER_HELP(CMD_G_UC_DETAIL,
              "Execute the statement in the MySQL server and display results "
              "in a vertical format, one field and value per line.");
REGISTER_HELP(
    CMD_G_UC_DETAIL1,
    "Useful for results that are too wide to fit the screen horizontally.");

REGISTER_HELP(CMD_G_LC_BRIEF, "Send command to mysql server.");
REGISTER_HELP(CMD_G_LC_SYNTAX, "<statement>\\g");
REGISTER_HELP(CMD_G_LC_DETAIL,
              "Execute the statement in the MySQL server and display results.");
REGISTER_HELP(CMD_G_LC_DETAIL1,
              "Same as executing with the current delimiter (default ;).");

namespace shcore {

Shell_sql::Shell_sql(IShell_core *owner)
    : Shell_language(owner), _delimiters({";", "\\G", "\\g"}) {
  // Inject help for statement commands. Actual handling of these
  // commands is done in a way different from other commands
  SET_CUSTOM_SHELL_COMMAND("\\G", "CMD_G_UC", Shell_command_function(), true,
                           IShell_core::Mode_mask(IShell_core::Mode::SQL));
  SET_CUSTOM_SHELL_COMMAND("\\g", "CMD_G_LC", Shell_command_function(), true,
                           IShell_core::Mode_mask(IShell_core::Mode::SQL));
}

void Shell_sql::kill_query(uint64_t conn_id,
                           const mysqlshdk::db::Connection_options &conn_opts) {
  try {
    std::shared_ptr<mysqlshdk::db::ISession> kill_session;

    if (conn_opts.get_scheme() == "mysqlx")
      kill_session = mysqlshdk::db::mysqlx::Session::create();
    else
      kill_session = mysqlshdk::db::mysql::Session::create();
    kill_session->connect(conn_opts);
    kill_session->executef("kill query ?", conn_id);
    _owner->print("-- query aborted\n");

    kill_session->close();
  } catch (std::exception &e) {
    _owner->print(std::string("-- error aborting query: ") + e.what() + "\n");
  }
}

bool Shell_sql::process_sql(const std::string &query_str,
                            mysql::splitter::Delimiters::delim_type_t delimiter,
                            std::shared_ptr<mysqlshdk::db::ISession> session) {
  bool ret_val = false;
  if (session) {
    try {
      std::shared_ptr<mysqlshdk::db::IResult> result;
      Sql_result_info info;
      if (delimiter == "\\G") info.show_vertical = true;
      try {
        mysqlshdk::utils::Profile_timer timer;
        timer.stage_begin("query");
        // Install kill query as ^C handler
        uint64_t conn_id = session->get_connection_id();
        const auto &conn_opts = session->get_connection_options();
        Interrupts::push_handler([this, conn_id, conn_opts]() {
          kill_query(conn_id, conn_opts);
          return true;
        });

        result = session->query(query_str);
        Interrupts::pop_handler();

        timer.stage_end();
        info.ellapsed_seconds = timer.total_seconds_ellapsed();
      } catch (mysqlshdk::db::Error &e) {
        Interrupts::pop_handler();
        throw shcore::Exception::mysql_error_with_code_and_state(
            e.what(), e.code(), e.sqlstate());
      } catch (...) {
        Interrupts::pop_handler();
        throw;
      }

      _result_processor(result, info);

      ret_val = true;
    } catch (shcore::Exception &exc) {
      print_exception(exc);
      ret_val = false;
    }
  }
  _last_handled += query_str + delimiter;

  return ret_val;
}

void Shell_sql::handle_input(std::string &code, Input_state &state) {
  state = Input_state::Ok;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  bool got_error = false;

  {
    const auto s = _owner->get_dev_session();

    if (s) {
      session = s->get_core_session();
    }

    if (!s || !session || !session->is_open()) {
      print_exception(shcore::Exception::logic_error("Not connected."));
    }
  }

  _last_handled.clear();

  {
    // NOTE: We need to find a nice way to decide whether parsing or not
    // multiline blocks is enabled or not, for now will let this commented out
    // and do parsing all the time
    //-----------------------------------------------------------------------------------
    // If no cached code and new code is a multiline statement
    // allows multiline code to bypass the splitter
    // This way no delimiter change is needed for i.e.
    // stored procedures and functions
    // if (_sql_cache.empty() && code.find("\n") != std::string::npos)
    //{
    //  ranges.push_back(std::make_pair<size_t, size_t>(0, code.length()));
    //  statement_count = 1;
    //}
    // else
    //{
    // Parses the input string to identify individual statements in it.
    // Will return a range for every statement that ends with the delimiter, if
    // there is additional code after the last delimiter, a range for it will be
    // included too.
    auto ranges = shcore::mysql::splitter::determineStatementRanges(
        code.data(), code.length(), _delimiters, "\n", _parsing_context_stack);
    //}

    size_t range_index = 0;

    for (; range_index < ranges.size(); range_index++) {
      if (ranges[range_index].get_delimiter().empty()) {
        // There is no delimiter, partial command added to cache
        std::string line = code.substr(ranges[range_index].offset(),
                                       ranges[range_index].length());

        line = str_rstrip(line, "\n");
        if (_sql_cache.empty())
          _sql_cache = line;
        else
          _sql_cache.append("\n").append(line);
      } else {
        if (!_sql_cache.empty()) {
          std::string cached_query = _sql_cache + "\n" +
                                     code.substr(ranges[range_index].offset(),
                                                 ranges[range_index].length());

          _sql_cache.clear();

          if (!process_sql(cached_query, ranges[range_index].get_delimiter(),
                           session))
            got_error = true;
        } else {
          if (!process_sql(code.substr(ranges[range_index].offset(),
                                       ranges[range_index].length()),
                           ranges[range_index].get_delimiter(), session))
            got_error = true;
        }
      }
    }
    code = _sql_cache;

    if (_parsing_context_stack.empty())
      state = Input_state::Ok;
    else
      state = Input_state::ContinuedSingle;
  }

  // TODO: previous to file processing the caller was caching unprocessed code
  // and sending it again on next
  //       call. On file processing an internal handling of this cache was
  //       required. Clearing the code here prevents it being sent again. We
  //       need to decide if the caching logic we introduced on the caller is
  //       still required or not.
  code = "";

  // signal error during input processing
  if (got_error) _result_processor(nullptr, {});
}

void Shell_sql::clear_input() {
  std::stack<std::string> empty;
  _parsing_context_stack.swap(empty);
  _sql_cache.clear();
}

std::string Shell_sql::get_continued_input_context() {
  if (_parsing_context_stack.empty()) return "";
  return _parsing_context_stack.top();
}

void Shell_sql::print_exception(const shcore::Exception &e) {
  // Sends a description of the exception data to the error handler wich will
  // define the final format.
  shcore::Value exception(e.error());
  _owner->get_delegate()->print_value(_owner->get_delegate()->user_data,
                                      exception, "error");
}

}  // namespace shcore
