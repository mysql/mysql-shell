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
#include "../modules/mod_db.h"
#include "../utils/utils_mysql_parsing.h"

using namespace shcore;


Shell_sql::Shell_sql(Shell_core *owner)
: Shell_language(owner)
{
  _continuing_line = 0;
  _delimiter = ";";
}

Interactive_input_state Shell_sql::handle_interactive_input(std::string &code)
{
  Interactive_input_state ret_val = Input_ok;
  Value db = _owner->get_global("db");
  MySQL_splitter splitter;

  _continuing_line = 0;
  _last_handled.clear();
  
  if (db)
  {
    boost::shared_ptr<mysh::Db> _db = db.as_object<mysh::Db>();
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
        _db->sql(query);
      }
    }
    
    if (ranges.size() > statement_count)
    {
      _continuing_line = '-';
      ret_val = Input_continued;
      
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

  return ret_val;
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

