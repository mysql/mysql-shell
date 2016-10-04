/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "../modules/base_session.h"
#include "../modules/mod_mysql_session.h"
#include "../modules/mod_mysqlx_session.h"
#include "../utils/utils_mysql_parsing.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>

using namespace shcore;
using namespace boost::system;

Shell_sql::Shell_sql(IShell_core *owner)
  : Shell_language(owner) {
  _delimiter = ";";
}

void Shell_sql::handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor) {
  Value ret_val;
  state = Input_state::Ok;
  auto session = _owner->get_dev_session();

  _last_handled.clear();

  if (session) {
    std::vector<std::pair<size_t, size_t> > ranges;
    size_t statement_count;

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
    statement_count = shcore::mysql::splitter::determineStatementRanges(code.c_str(), code.length(), _delimiter, ranges, "\n", _parsing_context_stack);
    //}

    // statement_count is > 0 if the splitter determined a statement was completed
    // ranges: contains the char ranges having statements.
    // special case: statement_count is 1 but there are no ranges: the delimiter was sent on the last call to the splitter
    //               and it determined the continued statement is now complete, but there's no additional data for it
    size_t index = 0;

    // Gets the total number of ranges
    size_t range_count = ranges.size();
    std::vector<std::string> statements;
    if (statement_count) {
      // If cache has data it is part of the found statement so it has to
      // be flushed at this point into the statements list for execution
      if (!_sql_cache.empty()) {
        if (statement_count > range_count)
          statements.push_back(_sql_cache);
        else {
          statements.push_back(_sql_cache.append("\n").append(code.substr(ranges[0].first, ranges[0].second)));
          index++;
        }

        _sql_cache.clear();
      }

      if (range_count) {
        // Now also adds the rest of the statements for execution
        for (; index < statement_count; index++)
          statements.push_back(code.substr(ranges[index].first, ranges[index].second));

        // If there's still data, itis a partial statement: gets cached
        if (index < range_count)
          _sql_cache = code.substr(ranges[index].first, ranges[index].second);
      }

      code = _sql_cache;

      // Executes every found statement
      for (index = 0; index < statements.size(); index++) {
        shcore::Argument_list query;
        query.push_back(Value(statements[index]));

        try {
          // ClassicSession has runSql and returns a ClassicResult object
          if (session->has_member("runSql"))
            ret_val = session->call("runSql", query);

          // NodeSession uses SqlExecute object in which we need to call
          // .execute() to get the Resultset object
          else if (session->has_member("sql"))
            ret_val = session->call("sql", query).as_object()->call("execute", shcore::Argument_list());
          else
            throw shcore::Exception::logic_error("The current session type (" + session->class_name() + ") can't be used for SQL execution.");

          // If reached this point, processes the returned result object
          if (!_killed)
            result_processor(ret_val);
          _killed = false;
        } catch (shcore::Exception &exc) {
          print_exception(exc);
        }

        if (_last_handled.empty())
          _last_handled = statements[index].append(_delimiter);
        else
          _last_handled.append("\n").append(statements[index]).append(_delimiter);
      }
    } else if (range_count) {
      std::string line = code.substr(ranges[0].first, ranges[0].second);
      boost::trim_right_if(line, boost::is_any_of("\n"));
      if (_sql_cache.empty())
        _sql_cache = line;
      else
        _sql_cache.append("\n").append(line);
    } else // Multiline code, all is "processed"
      code = "";

    // Nothing was processed so it is not an error
    if (!statement_count)
      ret_val = Value::Null();

    if (_parsing_context_stack.empty())
      state = Input_state::Ok;
    else
      state = Input_state::ContinuedSingle;
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
    return (boost::format("%9s> ") % _parsing_context_stack.top().c_str()).str();
  else {
    std::string node_type = "mysql";
    Value session_wrapper = _owner->active_session();
    if (session_wrapper) {
      std::shared_ptr<mysh::ShellBaseSession> session = session_wrapper.as_object<mysh::ShellBaseSession>();

      if (session) {
        shcore::Value st = session->get_capability("node_type");

        if (st)
          node_type = st.as_string();
      }
    }

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
  Value session_wrapper = _owner->active_session();
  std::shared_ptr<mysh::ShellBaseSession> session = session_wrapper.as_object<mysh::ShellBaseSession>();
  // duplicate the connection
  std::shared_ptr<mysh::mysql::ClassicSession> kill_session;
  std::shared_ptr<mysh::mysqlx::NodeSession> kill_session2;
  mysh::mysql::ClassicSession* classic = NULL;
  mysh::mysqlx::NodeSession* node = NULL;
  classic = dynamic_cast<mysh::mysql::ClassicSession*>(session.get());
  node = dynamic_cast<mysh::mysqlx::NodeSession*>(session.get());
  if (classic) {
    kill_session.reset(new mysh::mysql::ClassicSession(*dynamic_cast<mysh::mysql::ClassicSession*>(classic)));
  } else {
    kill_session2.reset(new mysh::mysqlx::NodeSession(*dynamic_cast<mysh::mysqlx::NodeSession*>(node)));
  }
  uint64_t connection_id = session->get_connection_id();
  if (connection_id != 0) {
    shcore::Argument_list a;
    a.push_back(shcore::Value(""));
    if (classic) {
      kill_session->connect(a);
      if (!kill_session) {
        throw std::runtime_error(boost::format("Error duplicating classic connection").str());
      }
      std::string cmd = (boost::format("kill query %u") % connection_id).str();
      a.clear();
      a.push_back(shcore::Value(cmd));
      kill_session->run_sql(a);
    } else if (node) {
      kill_session2->connect(a);
      if (!kill_session2) {
        throw std::runtime_error(boost::format("Error duplicating xplugin connection").str());
      }

      std::string cmd = (boost::format("kill query %u") % connection_id).str();
      a.clear();
      shcore::Value v = kill_session2->execute_sql(cmd, a);
    }
    _killed = true;
  }
}
