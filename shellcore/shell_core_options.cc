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

#include <boost/format.hpp>
#include "shellcore/shell_core_options.h"

using namespace shcore;

boost::shared_ptr<Shell_core_options> Shell_core_options::_instance;

std::string Shell_core_options::class_name() const
{
  return "Shell Object";
}

std::string &Shell_core_options::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  Value(_options).append_descr(s_out, indent, quote_strings);
  return s_out;
}

bool Shell_core_options::operator == (const Object_bridge &other) const
{
  throw Exception::logic_error("There's only one shell object!");
  return false;
};

std::vector<std::string> Shell_core_options::get_members() const
{
  std::vector<std::string> members;
  Value::Map_type::const_iterator index, end = _options->end();

  for (index = _options->begin(); index != end; index++)
    members.push_back(index->first);

  return members;
}

Value Shell_core_options::get_member(const std::string &prop) const
{
  Value ret_val;
  if (_options->has_key(prop))
    ret_val = (*_options)[prop];

  return ret_val;
}

bool Shell_core_options::has_member(const std::string &prop) const
{
  return _options->has_key(prop);
}

void Shell_core_options::set_member(const std::string &prop, Value value)
{
  if (_options->has_key(prop))
  {
    if (prop == SHCORE_OUTPUT_FORMAT)
    {
      std::string format = value.as_string();
      if (format != "table" && format != "json" && format != "json/raw")
        throw shcore::Exception::value_error((boost::format("The option %s must be one of: table, json or json/raw.") % prop).str());
    }
    else if (prop == SHCORE_INTERACTIVE || prop == SHCORE_BATCH_CONTINUE_ON_ERROR)
      throw shcore::Exception::value_error((boost::format("The option %s is read only.") % prop).str());

    else if (prop == SHCORE_SHOW_WARNINGS && value.type != shcore::Bool)
        throw shcore::Exception::value_error((boost::format("The option %s requires a boolean value.") % prop).str());

    (*_options)[prop] = value;
  }
  else
    throw shcore::Exception::attrib_error("Unable to set the property " + prop + " on the shell object.");
}

Shell_core_options::Shell_core_options() :
_options(new shcore::Value::Map_type)
{
  (*_options)[SHCORE_OUTPUT_FORMAT] = Value("table");
  (*_options)[SHCORE_INTERACTIVE] = Value::True();
  (*_options)[SHCORE_SHOW_WARNINGS] = Value::True();
  (*_options)[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value::False();
}

Shell_core_options::~Shell_core_options()
{
  if (_instance)
    _instance.reset();
}

Value::Map_type_ref Shell_core_options::get()
{
  if (!_instance)
    _instance.reset(new Shell_core_options());

  return _instance->_options;
}

boost::shared_ptr<Shell_core_options> Shell_core_options::get_instance()
{
  if (!_instance)
    _instance.reset(new Shell_core_options());

  return _instance;
}
