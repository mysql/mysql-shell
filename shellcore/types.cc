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

typedef std::map<std::string, Object_factory*> Package;

static std::map<std::string, Package> *Object_Factories = NULL;

// --

void Object_factory::register_factory(const std::string &package, Object_factory *meta)
{
  if (Object_Factories == NULL)
    Object_Factories = new std::map<std::string, Package>();

  Package &pkg = (*Object_Factories)[package];
  if (pkg.find(meta->name()) != pkg.end())
    throw std::logic_error("Registering duplicate Object Factory "+package+"::"+meta->name());
  pkg[meta->name()] = meta;
}


boost::shared_ptr<Object_bridge> Object_factory::call_constructor(const std::string &package, const std::string &name,
                                                                  const Argument_list &args)
{
  std::map<std::string, Package>::iterator iter;
  Package::iterator piter;
  if ((iter = Object_Factories->find(package)) != Object_Factories->end()
      && (piter = iter->second.find(name)) != iter->second.end())
  {
    return boost::shared_ptr<Object_bridge>(piter->second->construct(args));
  }
  throw std::invalid_argument("Invalid factory constructor "+package+"."+name+" invoked");
}


std::vector<std::string> Object_factory::package_names()
{
  std::vector<std::string> names;

  for (std::map<std::string, Package>::const_iterator iter = Object_Factories->begin();
       iter != Object_Factories->end(); ++iter)
    names.push_back(iter->first);
  return names;
}


bool Object_factory::has_package(const std::string &package)
{
  return Object_Factories->find(package) != Object_Factories->end();
}


std::vector<std::string> Object_factory::package_contents(const std::string &package)
{
  std::vector<std::string> names;

  std::map<std::string, Package>::iterator iter;
  if ((iter = Object_Factories->find(package)) != Object_Factories->end())
  {
    for (Package::iterator i = iter->second.begin(); i != iter->second.end(); ++i)
      names.push_back(i->first);
  }
  return names;
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

void shcore::validate_args(const std::string &context, const Argument_list &args, Value_type vtype, ...)
{
  va_list vl;
  va_start(vl, vtype);
  Argument_list::const_iterator a = args.begin();
  Value_type atype = vtype;
  int i = 0;

  do
  {
    if (atype == Undefined)
    {
      throw std::invalid_argument((boost::format("Too many arguments for %1%") % context).str());
    }
    if (atype != a->type)
    {
      throw std::invalid_argument((boost::format("Type mismatch in argument #%1% in %2%") % (i+1) % context).str());
    }
    ++a;
    atype = (Value_type)va_arg(vl, int);
  }
  while (a != args.end());

  if (atype != Undefined)
  {
    throw std::invalid_argument((boost::format("Too few arguments for %1%") % context).str());
  }
  va_end(vl);
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
    case Undefined:
      break;
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
      case Null:
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
    case Null:
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
      s_out.append("str");
      break;
    case Object:
      s_out.append("obj");
      break;
    case Array:
      s_out.append("arr");
      break;
    case Map:
      s_out.append("map");
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
    case Null:
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
    case Null:
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




const std::string &Argument_list::string_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be a string") % i).str());
  return *at(i).value.s;
}


bool Argument_list::bool_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be a bool") % i).str());
  return at(i).value.b;
}


int64_t Argument_list::int_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be an int") % i).str());
  return at(i).value.i;
}


double Argument_list::double_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be a double") % i).str());
  return at(i).value.d;
}


boost::shared_ptr<Object_bridge> Argument_list::object_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be an object") % i).str());
  return *at(i).value.o;
}


boost::shared_ptr<Value::Map_type> Argument_list::map_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be a map") % i).str());
  return *at(i).value.map;
}


boost::shared_ptr<Value::Array_type> Argument_list::array_at(int i) const
{
  if (i >= size())
    throw std::range_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Type_error((boost::format("Element at index %1% is expected to be an array") % i).str());
  return *at(i).value.array;
}


void Argument_list::ensure_count(int c, const char *context) const
{
  if (c != size())
    throw std::invalid_argument((boost::format("Invalid number of arguments in %1%, expected %2% but got %3%") % context % c % size()).str());
}

