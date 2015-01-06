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


#ifndef _TYPES_CPP_H_
#define _TYPES_CPP_H_

#include "shellcore/types.h"

namespace shcore {

class Cpp_function : public Function_base
{
public:
  typedef boost::function<Value (const shcore::Argument_list &)> Function;

  virtual ~Cpp_function() {}

  virtual std::string name();

  virtual std::vector<std::pair<std::string, Value_type> > signature();

  virtual std::pair<std::string, Value_type> return_type();

  virtual bool operator == (const Function_base &other) const;

  virtual Value invoke(const Argument_list &args);


  static boost::shared_ptr<Function_base> create(const std::string &name, const Function &func, const char *arg1_name, Value_type arg1_type = Undefined, ...);

  static boost::shared_ptr<Function_base> create(const std::string &name, const Function &func, const std::vector<std::pair<std::string, Value_type> > &signature);

protected:
  friend class Cpp_object_bridge;
  
  Cpp_function(const std::string &name, const Function &func, const char *arg1_name, Value_type arg1_type = Undefined, ...);
  Cpp_function(const std::string &name, const Function &func, const std::vector<std::pair<std::string, Value_type> > &signature);

  std::string _name;
  Function _func;

  std::vector<std::pair<std::string, Value_type> > _signature;
  Value_type _return_type;
};


class Cpp_object_bridge : public Object_bridge
{
public:
  virtual ~Cpp_object_bridge();

  virtual std::vector<std::string> get_members() const;
  virtual Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, Value value);

  virtual Value call(const std::string &name, const Argument_list &args);
protected:
  void add_method(const char *name, Cpp_function::Function func,
                  const char *arg1_name, Value_type arg1_type = Undefined, ...);

  std::map<std::string, boost::shared_ptr<Cpp_function> > _funcs;
};


};

#endif
