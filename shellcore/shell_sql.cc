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
#include "../modules/mod_session.h"
#include "../modules/mod_mysql.h"
#include "../utils/utils_mysql_parsing.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>

using namespace shcore;


Shell_sql::Shell_sql(Shell_core *owner)
: Shell_language(owner)
{
  _continuing_line = 0;
  _delimiter = ";";
}

Value Shell_sql::handle_interactive_input(std::string &code, Interactive_input_state &state)
{
  state = Input_ok;
  Value session_wrapper = _owner->get_global("_S");
  MySQL_splitter splitter;

  _continuing_line = 0;
  _last_handled.clear();
  
  if (session_wrapper)
  {
    boost::shared_ptr<mysh::Session> session = session_wrapper.as_object<mysh::Session>();
    // Parses the input string to identify individual statements in it.
    // Will return a range for every statement that ends with the delimiter, if there
    // is additional code after the last delimiter, a range for it will be included too.
    std::vector<std::pair<size_t, size_t> > ranges;
    size_t statement_count = splitter.determineStatementRanges(code.c_str(), code.length(), _delimiter, ranges, "\n");
    
    size_t index;
    if (statement_count)
    {
      for(index = 0; index < statement_count; index++)
      {
        std::string statement = code.substr(ranges[index].first, ranges[index].second);
        shcore::Argument_list query;
        query.push_back(Value(statement));
        Value result_wrapper = session->sql(query);

        if (result_wrapper)
        {
          boost::shared_ptr<mysh::Mysql_resultset> result = result_wrapper.as_object<mysh::Mysql_resultset>();

          if (result)
          {
            result->print(Argument_list());

            // Prints the warnings if required
            if (result->warning_count())
              print_warnings(session);
          }
        }
      }
    }
    
    if (ranges.size() > statement_count)
    {
      _continuing_line = '-';
      state = Input_continued;
      
      // Sets the executed code if any
      // and updates the remaining code too
      if (statement_count)
      {
        _last_handled = code.substr(0, ranges[index].first - 1);
      
        // Updates the code to let only the non yet executed
        code = code.substr(ranges[index].first, ranges[index].second);
      }
    }
    else
      _last_handled = code;
  }

  return Value();
}

void Shell_sql::print_warnings(boost::shared_ptr<mysh::Session> session)
{
  Argument_list warnings_query;
  warnings_query.push_back(Value("show warnings"));

  Value::Map_type_ref options(new shcore::Value::Map_type);
  (*options)["key_by_index"] = Value::True();
  warnings_query.push_back(Value(options));

  Value result_wrapper = session->sql(warnings_query);

  if (result_wrapper)
  {
    boost::shared_ptr<mysh::Mysql_resultset> result = result_wrapper.as_object<mysh::Mysql_resultset>();

    if (result)
    {
      Value record;

      while ((record = result->next(Argument_list())))
      {
        boost::shared_ptr<Value::Map_type> row = record.as_map();

        
        unsigned long error = ((*row)["1"].as_int());

        std::string type = (*row)["0"].as_string();
        std::string msg = (*row)["2"].as_string();
        _owner->print((boost::format("%s (Code %ld): %s\n") % type % error % msg).str());
      }
    }
  }
}

int Shell_sql::run_script(const std::string &path, boost::system::error_code &err)
{
  return 0;
}


std::string Shell_sql::prompt()
{
  if (_continuing_line)
    return "    -> ";
  else
    return "mysql> ";
}

