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

#include "shellcore/types.h"
#include <stdexcept>
#include <boost/weak_ptr.hpp>

using namespace shcore;

static std::map<std::string, Native_object *(*)(const std::string&)> Native_object_factory;

// --

Value Native_object::construct(const std::string &type_name)
{

  std::map<std::string, Native_object *(*)(const std::string&)>::iterator iter = Native_object_factory.find(type_name);
  if (iter != Native_object_factory.end())
  {
    boost::shared_ptr<Native_object> n(iter->second(""));
    if (n)
      return Value(n);
  }
  throw std::invalid_argument("Invalid native type name "+type_name);
}


Value Native_object::reconstruct(const std::string &repr)
{
  if (repr.length() > 3 && repr[0] == '<' && repr[repr.size()-1] == '>')
  {
    std::string::size_type p = repr.find(':');
    if (p != std::string::npos)
    {
      std::map<std::string, Native_object *(*)(const std::string&)>::iterator iter = Native_object_factory.find(repr.substr(1, p-1));
      if (iter != Native_object_factory.end())
      {
        boost::shared_ptr<Native_object> n(iter->second(repr));
        if (n)
          return Value(n);
      }
    }
  }
  throw std::invalid_argument("Invalid repr value "+repr);
}

void Native_object::register_native_type(const std::string &type_name,
                                         Native_object *(*factory)(const std::string&))
{
  if (Native_object_factory.find(type_name) != Native_object_factory.end())
    throw std::logic_error("Registering duplicate Native type "+type_name);
  Native_object_factory[type_name] = factory;
}


// --

//! 0 arg convenience wrapper for invoke
bool Function_base::invoke(Value &return_value)
{
  std::vector<Value> args;
  return invoke(args, return_value);
}

//! 1 arg convenience wrapper for invoke
bool Function_base::invoke(const Value &arg1, Value &return_value)
{
  std::vector<Value> args;
  args.push_back(arg1);
  return invoke(args, return_value);
}

//! 2 arg convenience wrapper for invoke
bool Function_base::invoke(const Value &arg1, const Value &arg2, Value &return_value)
{
  std::vector<Value> args;
  args.push_back(arg1);
  args.push_back(arg2);
  return invoke(args, return_value);
}


// --


Value::Value(const Value &copy)
: type(Null)
{
  operator=(copy);
}


Value::Value(Value_type type)
: type(type)
{
  switch (type)
  {
    case Null:
      break;
    case Bool:
      value.b = false;
      break;
    case Integer:
      value.i = 0;
      break;
    case Float:
      value.d = 0.0;
      break;
    case String:
      value.s = new std::string();
      break;
    case Native:
      value.n = new boost::shared_ptr<Native_object>();
      break;
    case Array:
      value.array = new boost::shared_ptr<Array_type>();
      break;
    case Map:
      value.map = new boost::shared_ptr<Map_type>();
      break;
    case MapRef:
      value.mapref = new boost::weak_ptr<Map_type>();
      break;
    case Function:
      value.func = new boost::shared_ptr<Function_base>();
      break;
  }
}


Value::Value(bool b)
: type(Bool)
{
  value.b = b;
}


Value::Value(const std::string &s)
: type(String)
{
  value.s = new std::string(s);
}


Value::Value(int64_t i)
: type(Integer)
{
  value.i = i;
}


Value::Value(double d)
: type(Float)
{
  value.d = d;
}


Value::Value(boost::shared_ptr<Function_base> f)
: type(Function)
{
  value.func = new boost::shared_ptr<Function_base>(f);
}


Value::Value(boost::shared_ptr<Native_object> n)
: type(Native)
{
  value.n = new boost::shared_ptr<Native_object>(n);
}

Value::Value(boost::shared_ptr<Map_type> n)
: type(Map)
{
  value.map = new boost::shared_ptr<Map_type>(n);
}

Value::Value(boost::weak_ptr<Map_type> n)
: type(MapRef)
{
  value.mapref = new boost::weak_ptr<Map_type>(n);
}

Value::Value(boost::shared_ptr<Array_type> n)
: type(Array)
{
  value.array = new boost::shared_ptr<Array_type>(n);
}


Value &Value::operator= (const Value &other)
{
  if (type == other.type)
  {
    switch (type)
    {
      case Null:
        break;
      case Bool:
        value.b = other.value.b;
        break;
      case Integer:
        value.i = other.value.i;
        break;
      case Float:
        value.d = other.value.d;
        break;
      case String:
        *value.s = *other.value.s;
        break;
      case Native:
        *value.n = *other.value.n;
        break;
      case Array:
        *value.array = *other.value.array;
        break;
      case Map:
        *value.map = *other.value.map;
        break;
      case MapRef:
        *value.mapref = *other.value.mapref;
        break;
      case Function:
        *value.func = *other.value.func;
        break;
    }
  }
  else
  {
    switch (type)
    {
      case Null:
      case Bool:
      case Integer:
      case Float:
        break;
      case String:
        delete value.s;
        break;
      case Native:
        delete value.n;
        break;
      case Array:
        delete value.array;
        break;
      case Map:
        delete value.map;
        break;
      case MapRef:
        delete value.mapref;
        break;
      case Function:
        delete value.func;
        break;
    }
    type = other.type;
    switch (type)
    {
      case Null:
        break;
      case Bool:
        value.b = other.value.b;
        break;
      case Integer:
        value.i = other.value.i;
        break;
      case Float:
        value.d = other.value.d;
        break;
      case String:
        value.s = new std::string(*other.value.s);
        break;
      case Native:
        value.n = new boost::shared_ptr<Native_object>(*other.value.n);
        break;
      case Array:
        value.array = new boost::shared_ptr<Array_type>(*other.value.array);
        break;
      case Map:
        value.map = new boost::shared_ptr<Map_type>(*other.value.map);
        break;
      case MapRef:
        value.mapref = new boost::weak_ptr<Map_type>(*other.value.mapref);
        break;
      case Function:
        value.func = new boost::shared_ptr<Function_base>(*other.value.func);
        break;
    }
  }
  return *this;
}


Value Value::parse(const std::string &s)
{
  //XXX
  return Value();
}


bool Value::operator == (const Value &other) const
{
  if (type == other.type)
  {
    switch (type)
    {
      case Null:
        return true;
      case Bool:
        return value.b == other.value.b;
      case Integer:
        return value.i == other.value.i;
      case Float:
        return value.d == other.value.d;
      case String:
        return *value.s == *other.value.s;
      case Native:
        return *value.n == *other.value.n;
      case Array:
        return *value.array == *other.value.array;
      case Map:
        return *value.map == *other.value.map;
      case MapRef:
        return *value.mapref->lock() == *other.value.mapref->lock();
      case Function:
        return *value.func == *other.value.func;
    }
  }
  return false;
}


bool Value::operator != (const Value &other) const
{
  if (type == other.type)
  {
    switch (type)
    {
      case Null:
        return true;
      case Bool:
        return value.b != other.value.b;
      case Integer:
        return value.i != other.value.i;
      case Float:
        return value.d != other.value.d;
      case String:
        return *value.s != *other.value.s;
      case Native:
        return *value.n != *other.value.n;
      case Array:
        return *value.array != *other.value.array;
      case Map:
        return *value.map != *other.value.map;
      case MapRef:
        return *value.mapref->lock() != *other.value.mapref->lock();
      case Function:
        return *value.func != *other.value.func;
    }
  }
  return true;
}


std::string Value::descr(bool pprint) const
{
  std::string s;
  append_descr(s, pprint);
  return s;
}

std::string Value::repr() const
{
  std::string s;
  append_repr(s);
  return s;
}


std::string &Value::append_descr(std::string &s_out, bool pprint) const
{//XXX
  switch (type)
  {
    case Null:
    case Bool:
    case Integer:
    case Float:
      break;
    case String:
      break;
    case Native:
      break;
    case Array:
      break;
    case Map:
      break;
    case MapRef:
      break;
    case Function:
      break;
  }
  return s_out;
}


std::string &Value::append_repr(std::string &s_out) const
{//XXX
  switch (type)
  {
    case Null:
    case Bool:
    case Integer:
    case Float:
      break;
    case String:
      break;
    case Native:
      break;
    case Array:
      break;
    case Map:
      break;
    case MapRef:
      break;
    case Function:
      break;
  }
  return s_out;
}


Value::~Value()
{
  switch (type)
  {
    case Null:
    case Bool:
    case Integer:
    case Float:
      break;
    case String:
      delete value.s;
      break;
    case Native:
      delete value.n;
      break;
    case Array:
      delete value.array;
      break;
    case Map:
      delete value.map;
      break;
    case MapRef:
      delete value.mapref;
      break;
    case Function:
      delete value.func;
      break;
  }
}


