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

#include "interactive_object_wrapper.h"
using namespace shcore;

Interactive_object_wrapper::Interactive_object_wrapper(const std::string& alias, shcore::Shell_core& core) : _alias(alias), _shell_core(core), _delegate(core.lang_delegate()){};

shcore::Value Interactive_object_wrapper::get_member(const std::string &prop) const
{
  shcore::Value ret_val;

  // If the target object is not set, the resolve function enables resolving the proper target object
  // Child classes shouls implement the interactivenes required to resolve the target object
  if (!_target)
    resolve();

  // Member resolution only makes sense if the target is set and it also
  // has the member
  if (_target && _target->has_member(prop))
  {
    // Interactive functions are defined on the wrapper so
    // we verify if the member is an interactive function
    if (Cpp_object_bridge::has_member(prop))
      ret_val = Cpp_object_bridge::get_member(prop);

    // If not an interactive function then it will be resolved as interactive property
    else
      ret_val = interactive_get_member(prop);
  }
  else
    throw Exception::attrib_error("Invalid object member " + prop);

  return ret_val;
}

Value Interactive_object_wrapper::call(const std::string &name, const Argument_list &args)
{
  shcore::Value ret_val;

  // If the target object is not set, the resolve function enables resolving the proper target object
  // Child classes shouls implement the interactivenes required to resolve the target object
  if (!_target)
    resolve();

  // Method execution only makes sense if the target object is set and it
  // also has the method
  if (_target && _target->has_member(name))
  {
    // Interactive functions are defined on the wrapper so
    // we verify if the member is an interactive function
    if (Cpp_object_bridge::has_member(name))
      ret_val = Cpp_object_bridge::call(name, args);

    // If not an interactive method then it will be called directly on the target object
    else
      ret_val = _target->call(name, args);
  }

  return ret_val;
}

shcore::Value Interactive_object_wrapper::interactive_get_member(const std::string &prop) const
{
  // The default implementation does not do add any interaction, it will just pass the call to
  // the target object.
  return _target->get_member(prop);
}

bool Interactive_object_wrapper::has_member(const std::string &prop) const
{
  return _target ? _target->has_member(prop) : false;
}

/*-----------------------------------------------------------------*/
/* Helper functions to ease adding interaction on derived classes */
/*-----------------------------------------------------------------*/
void Interactive_object_wrapper::print(const std::string& text) const
{
  _delegate->print(_delegate->user_data, text.c_str());
}

void Interactive_object_wrapper::print_error(const std::string& error) const
{
  _delegate->print_error(_delegate->user_data, error.c_str());
}

bool Interactive_object_wrapper::prompt(const std::string& prompt, std::string &ret_val) const
{
  return _delegate->prompt(_delegate->user_data, prompt.c_str(), ret_val);
}

bool Interactive_object_wrapper::password(const std::string& prompt, std::string &ret_val) const
{
  return _delegate->password(_delegate->user_data, prompt.c_str(), ret_val);
}