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

#include "base_resultset.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"

#include <boost/bind.hpp>
#include <boost/pointer_cast.hpp>

using namespace mysh;
using namespace shcore;

BaseResultset::BaseResultset()
{
  add_method("nextDataSet", boost::bind(&BaseResultset::next_result, this, _1), NULL);
  add_method("next", boost::bind(&BaseResultset::next, this, _1), NULL);
  add_method("all", boost::bind(&BaseResultset::all, this, _1), NULL);

  add_method("getColumnMetadata", boost::bind(&BaseResultset::get_member_method, this, _1, "getColumnMetadata", "columnMetadata"), NULL);
  add_method("getAffectedRows", boost::bind(&BaseResultset::get_member_method, this, _1, "getAffectedRows", "affectedRows"), NULL);
  add_method("getFetchedRowCount", boost::bind(&BaseResultset::get_member_method, this, _1, "getFetchedRowCount", "fetchedRowCount"), NULL);
  add_method("getWarningCount", boost::bind(&BaseResultset::get_member_method, this, _1, "getWarningCount", "warningCount"), NULL);
  add_method("getWarnings", boost::bind(&BaseResultset::get_member_method, this, _1, "getWarnings", "warnings"), NULL);
  add_method("getExecutionTime", boost::bind(&BaseResultset::get_member_method, this, _1, "getExecutionTime", "executionTime"), NULL);
  add_method("getLastInsertId", boost::bind(&BaseResultset::get_member_method, this, _1, "getLastInsertId", "lastInsertId"), NULL);
  add_method("getInfo", boost::bind(&BaseResultset::get_member_method, this, _1, "getInfo", "info"), NULL);
  add_method("getHasData", boost::bind(&BaseResultset::get_member_method, this, _1, "getHasData", "hasData"), NULL);
}

std::vector<std::string> BaseResultset::get_members() const
{
  std::vector<std::string> members(shcore::Cpp_object_bridge::get_members());
  members.push_back("columnMetadata");
  members.push_back("fetchedRowCount");
  members.push_back("affectedRows");
  members.push_back("warningCount");
  members.push_back("executionTime");
  members.push_back("lastInsertId");
  members.push_back("info");
  members.push_back("hasData");
  return members;
}

bool BaseResultset::operator == (const Object_bridge &other) const
{
  return this == &other;
}

shcore::Value BaseResultset::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "." + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

Row::Row()
{
  add_method("getLength", boost::bind(&Row::get_member_method, this, _1, "getLength", "__length__"), NULL);
}

std::string &Row::append_descr(std::string &s_out, int indent, int UNUSED(quote_strings)) const
{
  std::string nl = (indent >= 0) ? "\n" : "";
  s_out += "[";
  for (size_t index = 0; index < value_iterators.size(); index++)
  {
    if (index > 0)
      s_out += ",";

    s_out += nl;

    if (indent >= 0)
      s_out.append((indent + 1) * 4, ' ');

    value_iterators[index]->second.append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
  }

  s_out += nl;
  if (indent > 0)
    s_out.append(indent * 4, ' ');

  s_out += "]";

  return s_out;
}

void Row::append_json(const shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  for (size_t index = 0; index < value_iterators.size(); index++)
    dumper.append_value(value_iterators[index]->first, value_iterators[index]->second);

  dumper.end_object();
}

std::string &Row::append_repr(std::string &s_out) const
{
  return append_descr(s_out);
}

shcore::Value Row::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "." + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

//! Returns the list of members that this object has
std::vector<std::string> Row::get_members() const
{
  std::vector<std::string> l = shcore::Cpp_object_bridge::get_members();
  l.push_back("__length__");

  for (size_t index = 0; index < value_iterators.size(); index++)
    l.push_back(value_iterators[index]->first);

  return l;
}

//! Implements equality operator
bool Row::operator == (const Object_bridge &UNUSED(other)) const
{
  return false;
}

//! Returns the value of a member
shcore::Value Row::get_member(const std::string &prop) const
{
  if (prop == "__length__")
    return shcore::Value((int)values.size());
  else
  {
    std::map<std::string, shcore::Value>::const_iterator it;
    if ((it = values.find(prop)) != values.end())
      return it->second;
  }

  return shcore::Cpp_object_bridge::get_member(prop);
}

shcore::Value Row::get_member(size_t index) const
{
  if (index < value_iterators.size())
    return value_iterators[index]->second;
  else
    return shcore::Value();
}

void Row::add_item(const std::string &key, shcore::Value value)
{
  value_iterators.push_back(values.insert(values.end(), std::pair<std::string, shcore::Value>(key, value)));
}