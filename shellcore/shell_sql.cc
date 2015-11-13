/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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
#include "../utils/utils_mysql_parsing.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>

using namespace shcore;
using namespace boost::system;

Shell_sql::Shell_sql(IShell_core *owner)
  : Shell_language(owner)
{
  _delimiter = ";";
}

void Shell_sql::handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor)
{
  Value ret_val;
  state = Input_ok;
  Value session_wrapper = _owner->get_global("session");

  _last_handled.clear();

  if (session_wrapper)
  {
    boost::shared_ptr<mysh::ShellBaseSession> session = session_wrapper.as_object<mysh::ShellBaseSession>();

    if (session)
    {
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
      if (statement_count)
      {
        // If cache has data it is part of the found statement so it has to
        // be flushed at this point into the statements list for execution
        if (!_sql_cache.empty())
        {
          if (statement_count > range_count)
            statements.push_back(_sql_cache);
          else
          {
            statements.push_back(_sql_cache.append("\n").append(code.substr(ranges[0].first, ranges[0].second)));
            index++;
          }

          _sql_cache.clear();
        }

        if (range_count)
        {
          // Now also adds the rest of the statements for execution
          for (; index < statement_count; index++)
            statements.push_back(code.substr(ranges[index].first, ranges[index].second));

          // If there's still data, itis a partial statement: gets cached
          if (index < range_count)
            _sql_cache = code.substr(ranges[index].first, ranges[index].second);
        }

        code = _sql_cache;

        // Executes every found statement
        for (index = 0; index < statements.size(); index++)
        {
          shcore::Argument_list query;
          query.push_back(Value(statements[index]));

          try
          {
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
            result_processor(ret_val);
          }
          catch (shcore::Exception &exc)
          {
            print_exception(exc);
          }

          if (_last_handled.empty())
            _last_handled = statements[index];
          else
            _last_handled.append("\n").append(statements[index]);
        }
      }
      else if (range_count)
      {
        if (_sql_cache.empty())
          _sql_cache = code.substr(ranges[0].first, ranges[0].second);
        else
          _sql_cache.append("\n").append(code.substr(ranges[0].first, ranges[0].second));
      }
      else // Multiline code, all is "processed"
        code = "";

      // Nothing was processed so it is not an error
      if (!statement_count)
        ret_val = Value::Null();

      if (_parsing_context_stack.empty())
        state = Input_ok;
      else
        state = Input_continued_single;
    }
    else
      // handle_input implementations are not throwing exceptions
      // They handle the printing internally
      print_exception(shcore::Exception::logic_error("Not connected."));
  }
  else
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

std::string Shell_sql::prompt()
{
  if (!_parsing_context_stack.empty())
    return (boost::format("%9s> ") % _parsing_context_stack.top().c_str()).str();
  else
    return "mysql-sql> ";
}

bool Shell_sql::print_help(const std::string& topic)
{
  bool ret_val = true;
  if (topic.empty())
    _shell_command_handler.print_commands("===== SQL Mode Commands =====");
  else
    ret_val = _shell_command_handler.print_command_help(topic);

  return ret_val;
}

void Shell_sql::print_exception(const shcore::Exception &e)
{
  // Sends a description of the exception data to the error handler wich will define the final format.
  shcore::Value exception(e.error());
  _owner->print_error(exception.descr());
}