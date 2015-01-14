/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_db.h"
#include "mod_db_table.h"

using namespace mysh;
using namespace shcore;


#include <iostream>


Db_table::Db_table(boost::shared_ptr<Db> owner, const std::string &name)
: _owner(owner), _table_name(name)
{
}


Db_table::~Db_table()
{
}


std::string Db_table::class_name() const
{
  return "Db_table";
}

std::string &Db_table::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  s_out.append("<"+class_name()+":"+_table_name+">");
  return s_out;
}


std::vector<std::string> Db_table::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("insert");
  members.push_back("update");
  members.push_back("find");
  return members;
}



Value Db_table::get_member(const std::string &prop) const
{
  if (prop == "__name__")
    return Value(_table_name);

  if (prop == "insert")
  {
    std::cout << "not implemented\n";
  }
  else if (prop == "update")
  {
    std::cout << "not implemented\n";
  }
  else if (prop == "find")
  {
    std::cout << "not implemented\n";
  }

  return Cpp_object_bridge::get_member(prop);
}



bool Db_table::operator == (const Object_bridge &other) const
{
  //XXX there's gotta be a better/safer way to do this
  if (other.class_name() == class_name())
    return _owner.lock() == ((const Db_table*)&other)->_owner.lock() && _table_name == ((const Db_table*)&other)->_table_name;
  return false;
}

