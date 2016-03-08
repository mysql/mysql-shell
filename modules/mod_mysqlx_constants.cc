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

#include "base_constants.h"
#include "mod_mysqlx_constants.h"
#include "shellcore/object_factory.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

using namespace shcore;
using namespace mysh::mysqlx;

std::vector<std::string> Type::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());

  members.push_back("Bit");
  members.push_back("TinyInt");
  members.push_back("SmallInt");
  members.push_back("MediumInt");
  members.push_back("Int");
  members.push_back("BigInt");
  members.push_back("Float");
  members.push_back("Decimal");
  members.push_back("Double");
  members.push_back("Json");
  members.push_back("String");
  members.push_back("Bytes");
  members.push_back("Time");
  members.push_back("Date");
  members.push_back("DateTime");
  members.push_back("Timestamp");
  members.push_back("Set");
  members.push_back("Enum");
  members.push_back("Geometry");

  return members;
}

shcore::Value Type::get_member(const std::string &prop) const
{
  shcore::Value ret_val = mysh::Constant::get_constant("mysqlx", "Type", prop, shcore::Argument_list());

  if (!ret_val)
     ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

boost::shared_ptr<shcore::Object_bridge> Type::create(const shcore::Argument_list &args)
{
  args.ensure_count(0, "mysqlx.Type");

  boost::shared_ptr<Type> ret_val(new Type());

  return ret_val;
}

std::vector<std::string> IndexType::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());

  members.push_back("Unique");

  return members;
}

shcore::Value IndexType::get_member(const std::string &prop) const
{
  shcore::Value ret_val = mysh::Constant::get_constant("mysqlx", "IndexType", prop, shcore::Argument_list());

  if (!ret_val)
     ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

boost::shared_ptr<shcore::Object_bridge> IndexType::create(const shcore::Argument_list &args)
{
  args.ensure_count(0, "mysqlx.IndexType");

  boost::shared_ptr<IndexType> ret_val(new IndexType());

  return ret_val;
}