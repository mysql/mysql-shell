/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysql.h"
#include "base_session.h"

using namespace std::placeholders;
using namespace mysh::mysql;

REGISTER_MODULE(Mysql, mysql)
{
  REGISTER_VARARGS_FUNCTION(Mysql, get_classic_session, getClassicSession);
}

DEFINE_FUNCTION(Mysql, get_classic_session)
{
  auto session = connect_session(args, mysh::Classic);
  return shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(session));
}