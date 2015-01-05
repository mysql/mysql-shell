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


#include "shellcore/object_registry.h"

using namespace shcore;

Object_registry::Object_registry()
{
  _registry = boost::shared_ptr<Value::Map_type>(new Value::Map_type);
}


void Object_registry::set_reg(const std::string &name, const Value &value)
{
  (*_registry)[name] = value;
}


Value &Object_registry::get_reg(const std::string &name)
{
  Value::Map_type::iterator iter = _registry->find(name);
  if (iter != _registry->end())
    return iter->second;
  throw std::invalid_argument("Registry value "+name+" does not exist");
}


void Object_registry::add_to_reg_list(const std::string &list_name, const boost::shared_ptr<Object_bridge> &object)
{
  Value::Map_type::iterator liter = _registry->find(list_name);

  if (liter == _registry->end())
    (*_registry)[list_name] = Value(boost::shared_ptr<Value::Array_type>(new Value::Array_type()));
  else if (liter->second.type != Array)
    throw std::invalid_argument("Registry "+list_name+" is not a list");

  (*liter->second.value.array)->push_back(Value(object));
}


void Object_registry::add_to_reg_list(const std::string &list_name, const Value &value)
{
  Value::Map_type::iterator liter = _registry->find(list_name);

  if (liter == _registry->end())
    (*_registry)[list_name] = Value(boost::shared_ptr<Value::Array_type>(new Value::Array_type()));
  else if (liter->second.type != Array)
    throw std::invalid_argument("Registry "+list_name+" is not a list");

  (*liter->second.value.array)->push_back(value);
}


void Object_registry::remove_from_reg_list(const std::string &list_name, const boost::shared_ptr<Object_bridge> &object)
{
  Value::Map_type::iterator liter = _registry->find(list_name);
  if (liter != _registry->end() || liter->second.type != Array)
    throw std::invalid_argument("Registry "+list_name+" is not a list");

  Value &list(liter->second);
  Value::Array_type::iterator iter = std::find((*list.value.array)->begin(), (*list.value.array)->end(), Value(object));
  if (iter != (*list.value.array)->end())
    (*list.value.array)->erase(iter);
}


void Object_registry::remove_from_reg_list(const std::string &list_name, Value::Array_type::iterator iterator)
{
  Value::Map_type::iterator liter = _registry->find(list_name);
  if (liter != _registry->end() || liter->second.type != Array)
    throw std::invalid_argument("Registry "+list_name+" is not a list");

  (*liter->second.value.array)->erase(iterator);
}


boost::shared_ptr<Value::Array_type> &Object_registry::get_reg_list(const std::string &list_name)
{
  Value::Map_type::iterator liter = _registry->find(list_name);
  if (liter != _registry->end() || liter->second.type != Array)
    throw std::invalid_argument("Registry "+list_name+" is not a list");

  return *liter->second.value.array;
}
