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

Interactive_object_wrapper::Interactive_object_wrapper(const std::string& alias, shcore::Shell_core& core) : _alias(alias), _shell_core(core), _delegate(core.get_delegate()){};

Value Interactive_object_wrapper::get_member_advanced(const std::string &prop, const NamingStyle &style)
{
  shcore::Value ret_val;
  auto func = std::find_if(_wrapper_functions.begin(), _wrapper_functions.end(), [style, prop](const FunctionEntry &f){ return f.second->name(style) == prop; });

  if (func != _wrapper_functions.end())
    ret_val = get_member(func->first);
  else
  {
    // If the target object is not set, the resolve function enables resolving the proper target object
    // Child classes shouls implement the interactivenes required to resolve the target object
    if (!_target)
    resolve();

    // Member resolution only makes sense if the target is set and it also
    // has the member
    if (_target && _target->has_member_advanced(prop, style))
    {
      // Interactive functions are defined on the wrapper so
      // we verify if the member is an interactive function
      if (Cpp_object_bridge::has_member_advanced(prop, style))
        ret_val = Cpp_object_bridge::get_member_advanced(prop, style);

      // If not an interactive function then it will be resolved as interactive property
      else
        ret_val = interactive_get_member_advanced(prop, style);
    }
    else
    throw Exception::attrib_error("Invalid object member " + prop);
  }

  return ret_val;
}

shcore::Value Interactive_object_wrapper::get_member(const std::string &prop) const
{
  shcore::Value ret_val;

  auto func = std::find_if(_wrapper_functions.begin(), _wrapper_functions.end(), [prop](const FunctionEntry &f){ return f.first == prop; });
  if (func != _wrapper_functions.end())
    ret_val = Cpp_object_bridge::get_member(prop);
  else
  {
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
  }

  return ret_val;
}

Value Interactive_object_wrapper::call(const std::string &name, const Argument_list &args)
{
  shcore::Value ret_val;

  auto func = std::find_if(_wrapper_functions.begin(), _wrapper_functions.end(), [name](const FunctionEntry &f){ return f.first == name; });
  if (func != _wrapper_functions.end())
    ret_val = Cpp_object_bridge::call(name, args);
  else
  {
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
  }

  return ret_val;
}

bool Interactive_object_wrapper::has_method_advanced(const std::string &name, const NamingStyle &style)
{
  bool ret_val = Cpp_object_bridge::has_method_advanced(name, style);

  if (!ret_val && _target)
    ret_val = _target->has_method_advanced(name, style);

  return ret_val;
}

Value Interactive_object_wrapper::call_advanced(const std::string &name, const Argument_list &args, const NamingStyle &style)
{
  shcore::Value ret_val;

  auto func = std::find_if(_wrapper_functions.begin(), _wrapper_functions.end(), [style, name](const FunctionEntry &f){ return f.second->name(style) == name; });
  if (func != _wrapper_functions.end())
    ret_val = Cpp_object_bridge::call_advanced(name, args, style);
  else
  {
    // If the target object is not set, the resolve function enables resolving the proper target object
    // Child classes shouls implement the interactivenes required to resolve the target object
    if (!_target)
      resolve();

    // Method execution only makes sense if the target object is set and it
    // also has the method
    if (_target && _target->has_member_advanced(name, style))
    {
      // Interactive functions are defined on the wrapper so
      // we verify if the member is an interactive function
      if (Cpp_object_bridge::has_member_advanced(name, style))
        ret_val = Cpp_object_bridge::call_advanced(name, args, style);

      // If not an interactive method then it will be called directly on the target object
      else
        ret_val = _target->call_advanced(name, args, style);
    }
  }

  return ret_val;
}

shcore::Value Interactive_object_wrapper::interactive_get_member(const std::string &prop) const
{
  // The default implementation does not do add any interaction, it will just pass the call to
  // the target object.
  return _target->get_member(prop);
}

shcore::Value Interactive_object_wrapper::interactive_get_member_advanced(const std::string &prop, const NamingStyle &style)
{
  // The default implementation does not do add any interaction, it will just pass the call to
  // the target object.
  return _target->get_member_advanced(prop, style);
}

bool Interactive_object_wrapper::has_member(const std::string &prop) const
{
  return _target ? _target->has_member(prop) : false;
}

bool Interactive_object_wrapper::has_member_advanced(const std::string &prop, const NamingStyle &style)
{
  return _target ? _target->has_member_advanced(prop, style) : false;
}
/*-----------------------------------------------------------------*/
/* Helper functions to ease adding interaction on derived classes */
/*-----------------------------------------------------------------*/
void Interactive_object_wrapper::print(const std::string& text) const
{
  _shell_core.print(text);
}

void Interactive_object_wrapper::println(const std::string& text, const std::string& tag) const
{
  _shell_core.println(text, tag);
}

void Interactive_object_wrapper::print_value(const shcore::Value& value, const std::string& tag) const
{
  _shell_core.print_value(value, tag);
}

bool Interactive_object_wrapper::prompt(const std::string& prompt, std::string &ret_val) const
{
  return _shell_core.prompt(prompt, ret_val);
}

bool Interactive_object_wrapper::password(const std::string& prompt, std::string &ret_val) const
{
  return _shell_core.password(prompt, ret_val);
}

std::string Interactive_object_wrapper::get_help_text(const std::string& topic, bool full)
{
  std::string ret_val;

  if (_target)
    ret_val = _target->get_help_text(topic, full);

  return ret_val;
}