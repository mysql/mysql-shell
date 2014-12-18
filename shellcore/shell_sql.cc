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

using namespace shcore;


Shell_sql::Shell_sql(Shell_core *owner)
: Shell_language(owner)
{
  _delimiter = ";";
}


Interactive_input_state Shell_sql::handle_interactive_input_line(std::string &line)
{
  // TODO check if line contains a full statement (terminated by the delimiter)
  // and if so, consume it, otherwise return Input_continue

  return Input_ok;
}
