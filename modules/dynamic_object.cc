/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "proj_parser.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "crud_definition.h"
#include "base_database_object.h"
#include "mod_mysqlx_expression.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

std::vector<std::string> Dynamic_object::get_members() const
{
  std::vector<std::string> _members;
  for (auto i : _funcs)
  {
    // Only returns the enabled functions
    if (_enabled_functions.at(i.first))
      _members.push_back(i.second->name(naming_style));
  }
  return _members;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * This object represents objects that can dynamically enable or disable functions. The functions will only be returned if they are found and enabled, otherwise a shcore::Exception will be thrown:
 *
 * \li Invalid object member: when the function is not found.
 * \li Forbidden usage of: when the function found but is disabled.
 *
 */
#endif
Value Dynamic_object::get_member(const std::string &prop) const
{
  std::map<std::string, boost::shared_ptr<shcore::Cpp_function> >::const_iterator i;
  if ((i = _funcs.find(prop)) == _funcs.end())
    throw shcore::Exception::attrib_error("Invalid object member " + prop);
  else if (!_enabled_functions.at(prop))
    throw shcore::Exception::logic_error("Forbidden usage of " + prop);
  else
    return Value(boost::shared_ptr<shcore::Function_base>(i->second));
}

bool Dynamic_object::has_member(const std::string &prop) const
{
  bool ret_val = false;

  // A function is considered only if it is enanbled
  auto i = std::find_if(_funcs.begin(), _funcs.end(), [this, prop](const FunctionEntry &f){ return f.second->name(naming_style) == prop; });
  if (i != _funcs.end())
    ret_val = _enabled_functions.find(i->first) != _enabled_functions.end() && _enabled_functions.at(i->first);
  else
    ret_val = Cpp_object_bridge::has_member(prop);

  return ret_val;
}

Value Dynamic_object::call(const std::string &name, const shcore::Argument_list &args)
{
  std::map<std::string, boost::shared_ptr<shcore::Cpp_function> >::const_iterator i;
  if ((i = _funcs.find(name)) == _funcs.end())
    throw shcore::Exception::attrib_error("Invalid object function " + name);
  else if (!_enabled_functions.at(name))
    throw shcore::Exception::logic_error("Forbidden usage of " + name);
  return i->second->invoke(args);
}

/*
* This method registers the "dynamic" behavior of the functions exposed by the object.
* Parameters:
*   - name: indicates the exposed function to be enabled/disabled.
*   - enable_after: indicate the "states" under which the function should be enabled.
*/
void Dynamic_object::register_dynamic_function(const std::string& name, const std::string& enable_after)
{
  // Adds the function to the enabled/disabled state registry
  _enabled_functions[name] = true;

  // Splits the 'enable' states and associates them to the function
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, enable_after, boost::is_any_of(", "), boost::token_compress_on);
  std::set<std::string> after(tokens.begin(), tokens.end());
  _enable_paths[name] = after;
}

void Dynamic_object::update_functions(const std::string& source)
{
  std::map<std::string, bool>::iterator it, end = _enabled_functions.end();

  for (it = _enabled_functions.begin(); it != end; it++)
  {
    size_t count = _enable_paths[it->first].count(source);
    enable_function(it->first.c_str(), count > 0);
  }
}

void Dynamic_object::enable_function(const char *name, bool enable)
{
  if (_enabled_functions.find(name) != _enabled_functions.end())
    _enabled_functions[name] = enable;
}