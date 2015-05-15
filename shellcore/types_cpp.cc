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

#include "shellcore/types_cpp.h"
#include <cstdarg>

using namespace shcore;

Cpp_object_bridge::~Cpp_object_bridge()
{
  _funcs.clear();
}

std::string &Cpp_object_bridge::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  s_out.append("<" + class_name() + ">");
  return s_out;
}

std::string &Cpp_object_bridge::append_repr(std::string &s_out) const
{
  return append_descr(s_out, 0, '"');
}

std::vector<std::string> Cpp_object_bridge::get_members() const
{
  std::vector<std::string> _members;
  for (std::map<std::string, boost::shared_ptr<Cpp_function> >::const_iterator i = _funcs.begin(); i != _funcs.end(); ++i)
  {
    _members.push_back(i->first);
  }
  return _members;
}

Value Cpp_object_bridge::get_member(const std::string &prop) const
{
  std::map<std::string, boost::shared_ptr<Cpp_function> >::const_iterator i;
  if ((i = _funcs.find(prop)) != _funcs.end())
    return Value(boost::shared_ptr<Function_base>(i->second));
  throw Exception::attrib_error("Invalid object member " + prop);
}

bool Cpp_object_bridge::has_member(const std::string &prop) const
{
  std::map<std::string, boost::shared_ptr<Cpp_function> >::const_iterator i;
  return ((i = _funcs.find(prop)) != _funcs.end());
}

void Cpp_object_bridge::set_member(const std::string &prop, Value value)
{
  throw Exception::attrib_error("Can't set object member " + prop);
}

void Cpp_object_bridge::add_method(const char *name, Cpp_function::Function func,
                                   const char *arg1_name, Value_type arg1_type, ...)
{
  std::vector<std::pair<std::string, Value_type> > signature;
  va_list l;
  if (arg1_name && arg1_type != Undefined)
  {
    const char *n;
    Value_type t;

    va_start(l, arg1_type);
    signature.push_back(std::make_pair(arg1_name, arg1_type));
    do
    {
      n = va_arg(l, const char*);
      if (n)
      {
        t = (Value_type)va_arg(l, int);
        if (t != Undefined)
          signature.push_back(std::make_pair(n, t));
      }
    } while (n && t != Undefined);
    va_end(l);
  }

  _funcs[name] = boost::shared_ptr<Cpp_function>(new Cpp_function(name, func, NULL));
}

Value Cpp_object_bridge::call(const std::string &name, const Argument_list &args)
{
  std::map<std::string, boost::shared_ptr<Cpp_function> >::const_iterator i;
  if ((i = _funcs.find(name)) == _funcs.end())
    throw Exception::attrib_error("Invalid object function " + name);
  return i->second->invoke(args);
}

//-------

Cpp_function::Cpp_function(const std::string &name, const Function &func, const std::vector<std::pair<std::string, Value_type> > &signature)
: _name(name), _func(func), _signature(signature)
{
}

Cpp_function::Cpp_function(const std::string &name, const Function &func, const char *arg1_name, Value_type arg1_type, ...)
: _name(name), _func(func)
{
  va_list l;
  if (arg1_name && arg1_type != Undefined)
  {
    const char *n;
    Value_type t;

    va_start(l, arg1_type);
    _signature.push_back(std::make_pair(arg1_name, arg1_type));
    do
    {
      n = va_arg(l, const char*);
      if (n)
      {
        t = (Value_type)va_arg(l, int);
        if (t != Undefined)
          _signature.push_back(std::make_pair(n, t));
      }
    } while (n && t != Undefined);
    va_end(l);
  }
}

std::string Cpp_function::name()
{
  return _name;
}

std::vector<std::pair<std::string, Value_type> > Cpp_function::signature()
{
  return _signature;
}

std::pair<std::string, Value_type> Cpp_function::return_type()
{
  return std::make_pair("", _return_type);
}

bool Cpp_function::operator == (const Function_base &other) const
{
  throw Exception::logic_error("Cannot compare function objects");
  return false;
}

Value Cpp_function::invoke(const Argument_list &args)
{
  return _func(args);
}

boost::shared_ptr<Function_base> Cpp_function::create(const std::string &name, const Function &func, const char *arg1_name, Value_type arg1_type, ...)
{
  va_list l;
  std::vector<std::pair<std::string, Value_type> > signature;

  if (arg1_name && arg1_type != Undefined)
  {
    const char *n;
    Value_type t;

    va_start(l, arg1_type);
    signature.push_back(std::make_pair(arg1_name, arg1_type));
    do
    {
      n = va_arg(l, const char*);
      if (n)
      {
        t = (Value_type)va_arg(l, int);
        if (t != Undefined)
          signature.push_back(std::make_pair(n, t));
      }
    } while (n && t != Undefined);
    va_end(l);
  }
  return boost::shared_ptr<Function_base>(new Cpp_function(name, func, signature));
}

boost::shared_ptr<Function_base> Cpp_function::create(const std::string &name, const Function &func,
                                                      const std::vector<std::pair<std::string, Value_type> > &signature)
{
  return boost::shared_ptr<Function_base>(new Cpp_function(name, func, signature));
}