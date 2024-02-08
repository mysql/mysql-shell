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

#include "shellcore/shell_sql.h"
#include <array>
#include <fstream>
#include <functional>
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/utils_error.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/shell_options.h"
#include "utils/utils_general.h"
#include "utils/utils_lexing.h"
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

namespace {
const std::initializer_list<std::string_view> k_keyword_commands = {"source",
                                                                    "use"};

bool ansi_quotes_enabled(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (!session) return false;

  FI_SUPPRESS(mysql);
  FI_SUPPRESS(mysqlx);
  return session->ansi_quotes_enabled();
}

bool no_backslash_escapes_enabled(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (!session) return false;

  FI_SUPPRESS(mysql);
  FI_SUPPRESS(mysqlx);
  return session->no_backslash_escapes_enabled();
}

bool dollar_quoted_strings(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (!session) return false;

  FI_SUPPRESS(mysql);
  FI_SUPPRESS(mysqlx);
  return session->dollar_quoted_strings();
}

}  // namespace

// How many bytes at a time to process when executing large SQL scripts
static constexpr auto k_sql_chunk_size = 64 * 1024;

Shell_sql::Context::Context(Shell_sql *parent_)
    : parent(parent_),
      splitter(
          [parent_](std::string_view s, bool bol, size_t /*lnum*/) {
            return parent_->handle_command(s.data(), s.size(), bol);
          },
          [](std::string_view err) {
            mysqlsh::current_console()->print_error(std::string{err});
          },
          k_keyword_commands),
      old_buffer(parent->m_buffer),
      old_splitter(parent->m_splitter) {
  parent->m_buffer = &buffer;
  parent->m_splitter = &splitter;
  last_handled.swap(parent->_last_handled);
}

Shell_sql::Context::~Context() {
  parent->m_buffer = old_buffer;
  parent->m_splitter = old_splitter;
  last_handled.swap(parent->_last_handled);
}

Shell_sql::Shell_sql(IShell_core *owner)
    : Shell_language(owner), m_base_context(this) {
  assert(m_splitter != nullptr);
  assert(m_buffer != nullptr);
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

bool Shell_sql::process_sql(std::string_view query, std::string_view delimiter,
                            size_t line_num,
                            std::shared_ptr<mysqlshdk::db::ISession> session,
                            mysqlshdk::utils::Sql_splitter *splitter) {
  bool ret_val = false;
  std::optional<bool> server_state_changed;
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
        {
          shcore::Interrupt_handler interrupt([this, conn_id, conn_opts]() {
            kill_query(conn_id, conn_opts);
            return true;
          });

          result =
              session->querys(query.data(), query.size(), false,
                              _owner->get_dev_session()->query_attributes());

          _owner->get_dev_session()->query_attribute_store().clear();

          if ((mysqlshdk::db::replay::g_replay_mode ==
               mysqlshdk::db::replay::Mode::Direct) &&
              _owner->get_dev_session()->is_sql_mode_tracking_enabled()) {
            server_state_changed =
                session->get_server_status() & SERVER_SESSION_STATE_CHANGED;
          }
        }

        timer.stage_end();
        info.elapsed_seconds = timer.total_seconds_elapsed();
      } catch (const mysqlshdk::db::Error &e) {
        // Clean the query attributes unless it is a connection lost error so
        // they are available on next execution
        if (!mysqlshdk::db::is_server_connection_error(e.code())) {
          _owner->get_dev_session()->query_attribute_store().clear();
        }

        auto exc = shcore::Exception::mysql_error_with_code_and_state(
            e.what(), e.code(), e.sqlstate());
        if (line_num > 0) exc.set_file_context("", line_num);
        throw exc;
      } catch (...) {
        throw;
      }

      _result_processor(result, info);

      ret_val = true;
    } catch (const mysqlshdk::db::Error &exc) {
      print_exception(shcore::Exception::mysql_error_with_code_and_state(
          exc.what(), exc.code(), exc.sqlstate()));
      ret_val = false;
    } catch (const shcore::Exception &exc) {
      print_exception(exc);
      ret_val = false;
    }
  }

  _last_handled.append(query).append(delimiter);

  // check if the value of sql_mode have changed - statement can either begin
  // with 'SET' or, because our splitter does not strip them, with a C style
  // comment e.g.: /*...*/ set sql_mode...
  if (ret_val) {
    if (server_state_changed) {
      if (*server_state_changed) {
        session->refresh_sql_mode();
      }

      splitter->set_ansi_quotes(session->ansi_quotes_enabled());
      splitter->set_no_backslash_escapes(
          session->no_backslash_escapes_enabled());

    } else if (query.size() > 12 &&
               (query[2] == 't' || query[2] == 'T' || query[1] == '*')) {
      mysqlshdk::utils::SQL_iterator it(_last_handled);
      auto next = it.next_token();
      if (shcore::str_caseeq(next, "SET")) {
        constexpr std::array<std::string_view, 4> mods = {"GLOBAL", "PERSIST",
                                                          "SESSION", "LOCAL"};
        next = it.next_token();
        for (auto mod : mods) {
          if (shcore::str_caseeq(next, mod)) {
            next = it.next_token();
            break;
          }
        }

        while (next == "@") next = it.next_token();

        if (shcore::str_upper(next).find("SQL_MODE") != std::string::npos) {
          session->refresh_sql_mode();
          splitter->set_ansi_quotes(session->ansi_quotes_enabled());
          splitter->set_no_backslash_escapes(
              session->no_backslash_escapes_enabled());
        }
      }
    }
  }

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

  mysqlshdk::utils::Sql_splitter *splitter = nullptr;
  if (!mysqlshdk::utils::iterate_sql_stream(
          istream, k_sql_chunk_size,
          [&](std::string_view s, std::string_view delim, size_t lnum, size_t) {
            std::string_view file;

            if (shcore::str_beginswith(s, "source"))
              file = s.substr(6);
            else if (shcore::str_beginswith(s, "\\."))
              file = s.substr(2);

            bool ret = false;
            if (!file.empty())
              ret =
                  _owner->handle_shell_command("\\source " + std::string{file});
            else if (!s.empty())
              ret = process_sql(s, delim, lnum, session, splitter);
            return ret ? ret : mysqlsh::current_shell_options()->get().force;
          },
          [](std::string_view err) {
            mysqlsh::current_console()->print_error(std::string{err});
          },
          ansi_quotes_enabled(session), no_backslash_escapes_enabled(session),
          dollar_quoted_strings(session), nullptr, &splitter)) {
    // signal error during input processing
    _result_processor(nullptr, {});
    return false;
  }
  return true;
}

std::shared_ptr<mysqlshdk::db::ISession> Shell_sql::get_session() {
  const auto s = _owner->get_dev_session();
  std::shared_ptr<mysqlshdk::db::ISession> session;

  if (s) {
    session = s->get_core_session();
  }

  if (!s || !session || !session->is_open()) {
    print_exception(shcore::Exception::logic_error("Not connected."));
  }

  return session;
}

void Shell_sql::handle_input(std::string &code, Input_state &state) {
  // TODO(kolesink) this is a temporary solution and should be removed when
  // splitter is adjusted to be able to restart parsing from previous state
  if (state == Input_state::ContinuedSingle) {
    bool statement_complete = false;
    for (const auto s : {m_splitter->delimiter().c_str(), "\\G", "\\g"})
      if (code.find(s) != std::string::npos) {
        statement_complete = true;
        break;
      }
    // no need to re-parse code until statement is complete
    if (!statement_complete) {
      return;
    }
  }
  auto session = get_session();

  m_splitter->set_ansi_quotes(ansi_quotes_enabled(session));
  m_splitter->set_no_backslash_escapes(no_backslash_escapes_enabled(session));
  _last_handled.clear();

  // the whole code to be executed is in the `code` variable
  *m_buffer = std::move(code);
  code.clear();

  // Reinitializes the input state, will be changed back to the Continued state
  // if no complete statement is found
  m_input_state = Input_state::Ok;
  handle_input(session, false);

  state = m_input_state;

  // code could have been executed partially ('statement; incomplete statement')
  // store the incomplete statement back in the input buffer
  code = *m_buffer;
}

void Shell_sql::flush_input(const std::string &code) {
  auto session = get_session();
  *m_buffer = code;
  handle_input(session, true);
}

void Shell_sql::handle_input(std::shared_ptr<mysqlshdk::db::ISession> session,
                             bool flush) {
  bool got_error = false;
  if (!m_buffer->empty()) {
    // Parses the input string to identify individual statements in it.
    // Will return a range for every statement that ends with the delimiter, if
    // there is additional code after the last delimiter, a range for it will be
    // included too.
    m_splitter->feed_chunk(&m_buffer->at(0), m_buffer->size());

    mysqlshdk::utils::Sql_splitter::Range range;
    std::string delim;

    for (;;) {
      // If there's no more input, the code in the buffer should be executed no
      // matter if it is a complete statement or not, any generated error will
      // bubble up
      if (m_splitter->next_range(&range, &delim) || flush) {
        if (!process_sql({&m_buffer->at(range.offset), range.length}, delim, 0,
                         session, m_splitter)) {
          got_error = true;
        }
        if (flush) {
          break;
        }
      } else {
        m_splitter->pack_buffer(m_buffer, range);
        if (m_buffer->size() > 0 && range.length > 0) {
          m_input_state = m_splitter->context() == Sql_splitter::Context::kNone
                              ? Input_state::Ok
                              : Input_state::ContinuedSingle;
        } else {
          m_buffer->clear();
        }
        break;
      }
    }
  }

  // signal error during input processing
  if (got_error) {
    _result_processor(nullptr, {});
  }
  if (flush) {
    clear_input();
  }
}

void Shell_sql::clear_input() {
  m_buffer->clear();
  m_splitter->reset();
}

std::string Shell_sql::get_continued_input_context() {
  return to_string(m_splitter->context());
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

  std::string cmd(p, len);
  const auto handle_sql_style_command =
      [&](std::string_view kwd) -> std::pair<size_t, bool> {
    {
      std::unique_ptr<Context> ctx;
      if (kwd == "source") ctx = std::make_unique<Context>(this);
      if (!_owner->handle_shell_command("\\" + std::string{kwd} +
                                        cmd.substr(kwd.size())))
        return std::make_pair(0, false);
    }
    _last_handled.append(cmd);
    if (strncmp(p + len, m_splitter->delimiter().c_str(),
                m_splitter->delimiter().length()) == 0)
      _last_handled.append(m_splitter->delimiter());
    return std::make_pair(len, false);
  };

  for (const auto &c : k_keyword_commands)
    if (shcore::str_ibeginswith(cmd, c)) return handle_sql_style_command(c);

  if (bol && memchr(p, '\n', len)) {
    // handle as full-line command
    std::unique_ptr<Context> ctx;
    if (shcore::str_ibeginswith(cmd, "\\source") ||
        shcore::str_beginswith(cmd, "\\."))
      ctx = std::make_unique<Context>(this);
    if (_owner->handle_shell_command(cmd)) return std::make_pair(len, false);
  }

  mysqlsh::current_console()->print_error("Unknown command '" +
                                          std::string(p, 2) + "'");
  // consume the command
  return std::make_pair(2, false);
}

void Shell_sql::execute(std::string_view sql) {
  std::shared_ptr<mysqlshdk::db::ISession> session;
  const auto s = _owner->get_dev_session();

  if (s) {
    session = s->get_core_session();
  }

  if (!s || !session || !session->is_open()) {
    print_exception(shcore::Exception::logic_error("Not connected."));
  } else {
    auto check_delim = [this, &sql, &session](std::string_view delim) {
      if (!shcore::str_endswith(sql, delim)) return false;

      process_sql(sql.substr(0, sql.length() - delim.length()), delim, 0,
                  session, m_splitter);
      return true;
    };

    if (check_delim(";")) return;
    if (check_delim("\\G")) return;
    if (check_delim("\\g")) return;
    if (check_delim(get_main_delimiter())) return;

    process_sql(sql, get_main_delimiter(), 0, session, m_splitter);
  }
}

void Shell_sql::print_exception(const shcore::Exception &e) {
  // Sends a description of the exception data to the error handler which will
  // define the final format.
  shcore::Value exception(e.error());
  mysqlsh::current_console()->print_value(exception, "error");
}

}  // namespace shcore
