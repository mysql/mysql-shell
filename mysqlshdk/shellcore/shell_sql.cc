
/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <functional>
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/shell_options.h"
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

using mysqlshdk::utils::Sql_splitter;

namespace shcore {

// How many bytes at a time to process when executing large SQL scripts
static constexpr auto k_sql_chunk_size = 64 * 1024;

Shell_sql::Shell_sql(IShell_core *owner)
    : Shell_language(owner),
      m_splitter(
          [this](const char *s, size_t len, bool bol, size_t /*lnum*/) {
            return handle_command(s, len, bol);
          },
          [](const std::string &err) {
            mysqlsh::current_console()->print_error(err);
          }) {
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
    mysqlsh::current_console()->print("-- query aborted\n");

    kill_session->close();
  } catch (const std::exception &e) {
    mysqlsh::current_console()->print(std::string("-- error aborting query: ") +
                                      e.what() + "\n");
  }
}

bool Shell_sql::process_sql(const char *query_str, size_t query_len,
                            const std::string &delimiter, size_t line_num,
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

        result = session->querys(query_str, query_len);
        Interrupts::pop_handler();

        timer.stage_end();
        info.ellapsed_seconds = timer.total_seconds_ellapsed();
      } catch (const mysqlshdk::db::Error &e) {
        Interrupts::pop_handler();
        auto exc = shcore::Exception::mysql_error_with_code_and_state(
            e.what(), e.code(), e.sqlstate());
        if (line_num > 0) exc.set_file_context("", line_num);
        throw exc;
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
  _last_handled.append(query_str, query_len).append(delimiter);

  return ret_val;
}

bool Shell_sql::handle_input_stream(std::istream *istream) {
  std::shared_ptr<mysqlshdk::db::ISession> session;
  {
    auto s = _owner->get_dev_session();
    if (!s)
      print_exception(shcore::Exception::logic_error("Not connected."));
    else
      session = s->get_core_session();
  }

  if (!mysqlshdk::utils::iterate_sql_stream(
          istream, k_sql_chunk_size,
          [this, session](const char *s, size_t len, const std::string &delim,
                          size_t lnum) {
            if (len > 0 && !process_sql(s, len, delim, lnum, session)) {
              if (!mysqlsh::current_shell_options()->get().force) return false;
            }
            return true;
          },
          [](const std::string &err) {
            mysqlsh::current_console()->print_error(err);
          },
          false)) {
    // signal error during input processing
    _result_processor(nullptr, {});
    return false;
  }
  return true;
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

  m_buffer.append(code);
  code.clear();

  {
    // Parses the input string to identify individual statements in it.
    // Will return a range for every statement that ends with the delimiter, if
    // there is additional code after the last delimiter, a range for it will be
    // included too.
    m_splitter.feed_chunk(&m_buffer[0], m_buffer.size());

    mysqlshdk::utils::Sql_splitter::Range range;
    std::string delim;

    for (;;) {
      if (m_splitter.next_range(&range, &delim)) {
        if (!process_sql(&m_buffer[range.offset], range.length, delim, 0,
                         session)) {
          got_error = true;
        }
      } else {
        m_splitter.pack_buffer(&m_buffer, range);
        if (m_buffer.size() > 0 && range.length > 0) {
          state = m_splitter.context() == Sql_splitter::NONE
                      ? Input_state::Ok
                      : Input_state::ContinuedSingle;
        } else {
          m_buffer.clear();
        }
        break;
      }
    }
  }

  // signal error during input processing
  if (got_error) _result_processor(nullptr, {});
}

void Shell_sql::clear_input() {
  m_buffer.clear();
  m_splitter.reset();
}

std::string Shell_sql::get_continued_input_context() {
  return to_string(m_splitter.context());
}

std::pair<size_t, bool> Shell_sql::handle_command(const char *p, size_t len,
                                                  bool bol) {
  // whitelist of inline commands that can appear in SQL
  static const char k_allowed_inline_commands[] = "wW";
  // handle single-letter commands
  if (len >= 2) {
    if (p[1] == 'g' || p[1] == 'G') return {2, true};
    if (strchr(k_allowed_inline_commands, p[1])) {
      size_t skip = _owner->handle_inline_shell_command(std::string(p, len));
      if (skip > 0) return {skip, false};
    }
  }

  if (bol && memchr(p, '\n', len)) {
    // handle as full-line command
    if (_owner->handle_shell_command(std::string(p, len)))
      return std::make_pair(len, false);
  }

  mysqlsh::current_console()->print_error("Unknown command '" +
                                          std::string(p, 2) + "'");
  // consume the command
  return std::make_pair(2, false);
}

void Shell_sql::execute(const std::string &sql) {
  std::shared_ptr<mysqlshdk::db::ISession> session;
  const auto s = _owner->get_dev_session();

  if (s) {
    session = s->get_core_session();
  }

  if (!s || !session || !session->is_open())
    print_exception(shcore::Exception::logic_error("Not connected."));
  else
    process_sql(sql.c_str(), sql.length(), get_main_delimiter(), 0, session);
}

void Shell_sql::print_exception(const shcore::Exception &e) {
  // Sends a description of the exception data to the error handler which will
  // define the final format.
  shcore::Value exception(e.error());
  mysqlsh::current_console()->print_value(exception, "error");
}

}  // namespace shcore
