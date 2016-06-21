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

Type::Type()
{
  add_constant("BIT");
  add_constant("TINYINT");
  add_constant("SMALLINT");
  add_constant("MEDIUMINT");
  add_constant("INT");
  add_constant("BIGINT");
  add_constant("FLOAT");
  add_constant("DECIMAL");
  add_constant("DOUBLE");
  add_constant("JSON");
  add_constant("STRING");
  add_constant("BYTES");
  add_constant("TIME");
  add_constant("DATE");
  add_constant("DATETIME");
  add_constant("TIMESTAMP");
  add_constant("SET");
  add_constant("ENUM");
  add_constant("GEOMETRY");
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

IndexType::IndexType()
{
  add_constant("UNIQUE");
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