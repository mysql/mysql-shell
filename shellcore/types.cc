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
#include <cstdarg>
#include <boost/format.hpp>
#include <boost/weak_ptr.hpp>

using namespace shcore;

// --

Exception::Exception(const boost::shared_ptr<Value::Map_type> e)
: _error(e)
{
}


Exception Exception::argument_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ArgumentError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::attrib_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("AttributeError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::type_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("TypeError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::error_with_code(const std::string &type, const std::string &message, int code)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  return Exception(error);
}


const char *Exception::what() const BOOST_NOEXCEPT_OR_NOTHROW
{
  if ((*_error)["message"].type == String)
    return (*_error)["message"].value.s->c_str();
  return "?";
}

// --

//! 0 arg convenience wrapper for invoke
Value Function_base::invoke()
{
  Argument_list args;
  return invoke(args);
}

//! 1 arg convenience wrapper for invoke
Value Function_base::invoke(const Value &arg1)
{
  Argument_list args;
  args.push_back(arg1);
  return invoke(args);
}

//! 2 arg convenience wrapper for invoke
Value Function_base::invoke(const Value &arg1, const Value &arg2)
{
  Argument_list args;
  args.push_back(arg1);
  args.push_back(arg2);
  return invoke(args);
}

// --

std::string Value::Map_type::get_string(const std::string &k, const std::string &def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(String);
  return iter->second.as_string();
}

bool Value::Map_type::get_bool(const std::string &k, bool def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Bool);
  return iter->second.as_bool();
}

int64_t Value::Map_type::get_int(const std::string &k, int64_t def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Integer);
  return iter->second.as_int();
}

double Value::Map_type::get_double(const std::string &k, double def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Float);
  return iter->second.as_double();
}

boost::shared_ptr<Object_bridge> Value::Map_type::get_object(const std::string &k,
                                                             boost::shared_ptr<Object_bridge> def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Object);
  return iter->second.as_object();
}

boost::shared_ptr<Value::Map_type> Value::Map_type::get_map(const std::string &k,
                                                            boost::shared_ptr<Map_type> def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Map);
  return iter->second.as_map();
}

boost::shared_ptr<Value::Array_type> Value::Map_type::get_array(const std::string &k,
                                                                boost::shared_ptr<Array_type> def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Array);
  return iter->second.as_array();
}



Value::Value(const Value &copy)
: type(shcore::Null)
{
  operator=(copy);
}


Value::Value(Value_type type)
: type(type)
{
  switch (type)
  {
    case Undefined:
      break;
    case shcore::Null:
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
    case Object:
      value.o = new boost::shared_ptr<Object_bridge>();
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


Value::Value(const std::string &s)
: type(String)
{
  value.s = new std::string(s);
}


Value::Value(int i)
: type(Integer)
{
  value.i = i;
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


Value::Value(boost::shared_ptr<Object_bridge> n)
: type(Object)
{
  value.o = new boost::shared_ptr<Object_bridge>(n);
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
      case Undefined:
        break;
      case shcore::Null:
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
      case Object:
        *value.o = *other.value.o;
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
      case Undefined:
      case shcore::Null:
      case Bool:
      case Integer:
      case Float:
        break;
      case String:
        delete value.s;
        break;
      case Object:
        delete value.o;
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
      case Undefined:
      case shcore::Null:
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
      case Object:
        value.o = new boost::shared_ptr<Object_bridge>(*other.value.o);
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
      case Undefined:
        return false;
      case shcore::Null:
        return true;
      case Bool:
        return value.b == other.value.b;
      case Integer:
        return value.i == other.value.i;
      case Float:
        return value.d == other.value.d;
      case String:
        return *value.s == *other.value.s;
      case Object:
        return *value.o == *other.value.o;
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
    case Undefined:
      s_out.append("undefined");
      break;
    case shcore::Null:
      s_out.append("null");
      break;
    case Bool:
      s_out.append("bool");
      break;
    case Integer:
      s_out.append("int");
      break;
    case Float:
      s_out.append("fl");
      break;
    case String:
      s_out.append(*value.s);
      break;
    case Object:
      s_out.append("obj");
      break;
    case Array:
      s_out.append("arr");
      break;
    case Map:
      s_out.append("{");
      for (Map_type::const_iterator iter = (*value.map)->begin(); iter != (*value.map)->end(); ++iter)
      {
        if (iter != (*value.map)->begin())
          s_out.append(", ");
        s_out.append(iter->first).append(": ");
        iter->second.append_descr(s_out, pprint);
      }
      s_out.append("}");
      break;
    case MapRef:
      s_out.append("mapref");
      break;
    case Function:
      s_out.append("func");
      break;
  }
  return s_out;
}


std::string &Value::append_repr(std::string &s_out) const
{//XXX
  switch (type)
  {
    case Undefined:
    case shcore::Null:
    case Bool:
    case Integer:
    case Float:
      break;
    case String:
      break;
    case Object:
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
    case Undefined:
    case shcore::Null:
    case Bool:
    case Integer:
    case Float:
      break;
    case String:
      delete value.s;
      break;
    case Object:
      delete value.o;
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

void Value::check_type(Value_type t) const
{
  if (type != t)
    throw Exception::type_error("Invalid typecast");
}

//---

const std::string &Argument_list::string_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be a string") % i).str());
  return *at(i).value.s;
}


bool Argument_list::bool_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Bool)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be a bool") % i).str());
  return at(i).value.b;
}


int64_t Argument_list::int_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Integer)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be an int") % i).str());
  return at(i).value.i;
}


double Argument_list::double_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Float)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be a double") % i).str());
  return at(i).value.d;
}


boost::shared_ptr<Object_bridge> Argument_list::object_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Object)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be an object") % i).str());
  return *at(i).value.o;
}


boost::shared_ptr<Value::Map_type> Argument_list::map_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Map)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be a map") % i).str());
  return *at(i).value.map;
}


boost::shared_ptr<Value::Array_type> Argument_list::array_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Array)
    throw Exception::type_error((boost::format("Element at index %1% is expected to be an array") % i).str());
  return *at(i).value.array;
}


void Argument_list::ensure_count(int c, const char *context) const
{
  if (c != size())
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected %2% but got %3%") % context % c % size()).str());
}


void Argument_list::ensure_count(int minc, int maxc, const char *context) const
{
  if (size() < minc || size() > maxc)
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected %2% to %3% but got %4%") % context % minc % maxc % size()).str());
}
