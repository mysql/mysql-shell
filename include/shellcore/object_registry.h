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


#ifndef _OBJECT_REGISTRY_H_
#define _OBJECT_REGISTRY_H_

#include "shellcore/types.h"
#include <list>

namespace shcore
{

class Object_registry
{
public:
  typedef std::list<boost::shared_ptr<Object_bridge> > Object_list;

  Object_registry();

  void set_reg(const std::string &name, const Value &value);
  Value &get_reg(const std::string &name);

  void add_to_reg_list(const std::string &list_name, const boost::shared_ptr<Object_bridge> &object);
  void add_to_reg_list(const std::string &list_name, const Value &object);
  void remove_from_reg_list(const std::string &list_name, const boost::shared_ptr<Object_bridge> &object);
  void remove_from_reg_list(const std::string &list_name, Value::Array_type::iterator iterator);
  boost::shared_ptr<Value::Array_type> &get_reg_list(const std::string &list_name);

private:
  friend class JScript_context;
  friend class Python_context;

  boost::shared_ptr<Value::Map_type> _registry; // map of values
};

};

#endif
