/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

using namespace shcore;


Shell_sql::Shell_sql(Shell_core *owner)
: Shell_language(owner)
{
  _delimiter = ";";
}


Interactive_input_state Shell_sql::handle_interactive_input(const std::string &code)
{
  // TODO check if line contains a full statement (terminated by the delimiter)
  // and if so, consume it, otherwise return Input_continue
  //_owner->handle_interactive_input(code);
  Value db = _owner->get_global("db");
  
  if (db)
  {
    boost::shared_ptr<mysh::Db> _db = db.as_object<mysh::Db>();
    
    shcore::Argument_list query;
    query.push_back(Value(code));
    _db->sql(query);
  }
  

  return Input_ok;
}


int Shell_sql::run_script(const std::string &path, boost::system::error_code &err)
{
  return 0;
}
