/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "shellcore/shell_sql.h"
#include "shellcore/base_session.h"
#include "../modules/mod_mysql_session.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include <fstream>
#include "utils/utils_string.h"

using namespace shcore;

Shell_sql::Shell_sql(IShell_core *owner)
  : Shell_language(owner), _delimiters({";", "\\G", "\\g"})
{
  static const std::string cmd_help_G =
      "SYNTAX:\n"
      "   <statement>\\G\n\n"
      "Execute the statement in the MySQL server and display results in a vertical\n"
      "format, one field and value per line.\n"
      "Useful for results that are too wide to fit the screen horizontally.\n";

  static const std::string cmd_help_g =
      "SYNTAX:\n"
      "   <statement>\\g\n\n"
      "Execute the statement in the MySQL server and display results.\n"
      "Same as executing with the current delimiter (default ;)\n";

  // Inject help for statement commands. Actual handling of these
  // commands is done in a way different from other commands
  SET_CUSTOM_SHELL_COMMAND("\\G", "Send command to mysql server, display result vertically.", cmd_help_G, Shell_command_function());
  SET_CUSTOM_SHELL_COMMAND("\\g", "Send command to mysql server.", cmd_help_g, Shell_command_function());
}

Value Shell_sql::process_sql(const std::string &query_str,
    mysql::splitter::Delimiters::delim_type_t delimiter,
    std::shared_ptr<mysqlsh::ShellBaseSession> session,
    std::function<void(shcore::Value)> result_processor) {
  Value ret_val;
  try {
    shcore::Argument_list query;
    query.push_back(Value(query_str));

    // ClassicSession has runSql and returns a ClassicResult object
    if (session->has_member("runSql"))
      ret_val = session->call("runSql", query);

    // NodeSession uses SqlExecute object in which we need to call
    // .execute() to get the Resultset object
    else if (session->has_member("sql"))
      ret_val = session->call("sql", query).as_object()->call("execute",
                              shcore::Argument_list());
    else
      throw shcore::Exception::logic_error("The current session type (" +
          session->class_name() + ") can't be used for SQL execution.");

    // If reached this point, processes the returned result object
    if (!_killed) {
      auto shcore_options = Shell_core_options::get();
      auto old_format = (*shcore_options)[SHCORE_OUTPUT_FORMAT];
      if (delimiter == "\\G")
        (*shcore_options)[SHCORE_OUTPUT_FORMAT] = Value("vertical");
      result_processor(ret_val);
      (*shcore_options)[SHCORE_OUTPUT_FORMAT] = old_format;
    }
    _killed = false;
  } catch (shcore::Exception &exc) {
    print_exception(exc);
  }

  _last_handled += query_str + delimiter;

  return ret_val;
}

void Shell_sql::handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor) {
  Value ret_val;
  state = Input_state::Ok;
  auto session = _owner->get_dev_session();
  bool no_query_executed = true;

  _last_handled.clear();

  if (session) {

    // NOTE: We need to find a nice way to decide whether parsing or not multiline blocks
    // is enabled or not, for now will let this commented out and do parsing all the time
    //-----------------------------------------------------------------------------------
    // If no cached code and new code is a multiline statement
    // allows multiline code to bypass the splitter
    // This way no delimiter change is needed for i.e.
    // stored procedures and functions
    //if (_sql_cache.empty() && code.find("\n") != std::string::npos)
    //{
    //  ranges.push_back(std::make_pair<size_t, size_t>(0, code.length()));
    //  statement_count = 1;
    //}
    //else
    //{
    // Parses the input string to identify individual statements in it.
    // Will return a range for every statement that ends with the delimiter, if there
    // is additional code after the last delimiter, a range for it will be included too.
    auto ranges = shcore::mysql::splitter::determineStatementRanges(code.data(),
        code.length(), _delimiters, "\n", _parsing_context_stack);
    //}

    size_t range_index = 0;

    for (; range_index < ranges.size(); range_index++)
    {
      if (ranges[range_index].get_delimiter().empty()) {
        // There is no delimiter, partial command added to cache
        std::string line = code.substr(ranges[range_index].offset(),
            ranges[range_index].length());

        str_rstrip(line, "\n");
        if (_sql_cache.empty())
          _sql_cache = line;
        else
          _sql_cache.append("\n").append(line);
      } else {
        no_query_executed = false;
        if (!_sql_cache.empty()){
          no_query_executed = false;
          std::string cached_query = _sql_cache + "\n" +
              code.substr(ranges[range_index].offset(),
              ranges[range_index].length());

          _sql_cache.clear();

          ret_val = process_sql(cached_query,
              ranges[range_index].get_delimiter(),
              session, result_processor);
        }
        else {
          ret_val = process_sql(code.substr(ranges[range_index].offset(),
              ranges[range_index].length()),
              ranges[range_index].get_delimiter(),
              session, result_processor);
        }
      }
    }
    code = _sql_cache;

    if (_parsing_context_stack.empty())
      state = Input_state::Ok;
    else
      state = Input_state::ContinuedSingle;

    // Nothing was processed so it is not an error
    if (no_query_executed)
      ret_val = Value::Null();

  } else
    // handle_input implementations are not throwing exceptions
    // They handle the printing internally
    print_exception(shcore::Exception::logic_error("Not connected."));

  // TODO: previous to file processing the caller was caching unprocessed code and sending it again on next
  //       call. On file processing an internal handling of this cache was required.
  //       Clearing the code here prevents it being sent again.
  //       We need to decide if the caching logic we introduced on the caller is still required or not.
  code = "";

  // If ret_val still Undefined, it means there was an error on the processing
  if (ret_val.type == Undefined)
    result_processor(ret_val);
}

std::string Shell_sql::prompt() {
  if (!_parsing_context_stack.empty())
    return str_format("%9s> ", _parsing_context_stack.top().c_str());
  else {
    std::string node_type = "mysql";
    std::shared_ptr<mysqlsh::ShellBaseSession> session = _owner->get_dev_session();

    if (session)
      node_type = session->get_node_type();

    return node_type + "-sql> ";
  }
}

bool Shell_sql::print_help(const std::string& topic) {
  bool ret_val = true;
  if (topic.empty())
    _owner->print(_shell_command_handler.get_commands("===== SQL Mode Commands ====="));
  else {
    std::string help;
    ret_val = _shell_command_handler.get_command_help(topic, help);
    if (ret_val) {
      help += "\n";
      _owner->print(help);
    }
  }

  return ret_val;
}

void Shell_sql::print_exception(const shcore::Exception &e) {
  // Sends a description of the exception data to the error handler wich will define the final format.
  shcore::Value exception(e.error());
  _owner->get_delegate()->print_value(_owner->get_delegate()->user_data, exception, "error");
}

void Shell_sql::abort() {
  std::shared_ptr<mysqlsh::ShellBaseSession> session = _owner->get_dev_session();
  if (!session) {
    return;
  }

  // duplicate the connection
  std::shared_ptr<mysqlsh::mysql::ClassicSession> kill_session;
  std::shared_ptr<mysqlsh::mysqlx::NodeSession> kill_session2;
  mysqlsh::mysql::ClassicSession* classic = NULL;
  mysqlsh::mysqlx::NodeSession* node = NULL;
  classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
  node = dynamic_cast<mysqlsh::mysqlx::NodeSession*>(session.get());
  if (classic) {
    kill_session.reset(new mysqlsh::mysql::ClassicSession(*dynamic_cast<mysqlsh::mysql::ClassicSession*>(classic)));
  } else if (node) {
    kill_session2.reset(new mysqlsh::mysqlx::NodeSession(*dynamic_cast<mysqlsh::mysqlx::NodeSession*>(node)));
  }
  uint64_t connection_id = session->get_connection_id();
  if (connection_id != 0) {
    shcore::Argument_list a;
    a.push_back(shcore::Value(""));
    if (classic) {
      kill_session->connect(a);
      if (!kill_session) {
        throw std::runtime_error(str_format("Error duplicating classic connection"));
      }
      std::ostringstream cmd;
      cmd << "kill query " << connection_id;
      a.clear();
      a.push_back(shcore::Value(cmd.str()));
      kill_session->run_sql(a);
    } else if (node) {
      kill_session2->connect(a);
      if (!kill_session2) {
        throw std::runtime_error(str_format("Error duplicating xplugin connection"));
      }
      std::ostringstream cmd;
      cmd << "kill query " << connection_id;
      a.clear();
      shcore::Value v = kill_session2->execute_sql(cmd.str(), a);
    }
    _killed = true;
  }
}
