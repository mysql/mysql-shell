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

#include "shellcore/types_common.h"
#include "shellcore/types.h"

namespace shcore
{
  class SHCORE_PUBLIC Cpp_function : public Function_base
  {
  public:
    //TODO make this work with direct function pointers and skip boost::function
    typedef boost::function<Value(const shcore::Argument_list &)> Function;

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

  class SHCORE_PUBLIC Cpp_object_bridge : public Object_bridge
  {
  public:
    virtual ~Cpp_object_bridge();

    virtual std::vector<std::string> get_members() const;
    virtual Value get_member(const std::string &prop) const;

    virtual bool has_member(const std::string &prop) const;
    virtual void set_member(const std::string &prop, Value value);

    virtual bool is_indexed() const;
    virtual Value get_member(size_t index) const;
    virtual void set_member(size_t index, Value value);

    virtual bool has_method(const std::string &name) const;

    virtual Value call(const std::string &name, const Argument_list &args);
    
    // Helper method to retrieve properties using a method
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);
    

    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;

  protected:
    virtual void add_method(const std::string &name, Cpp_function::Function func,
                    const char *arg1_name, Value_type arg1_type = Undefined, ...);

    virtual void add_property(const std::string &name, const std::string &getter = "");

    std::map<std::string, boost::shared_ptr<Cpp_function> > _funcs;
    std::vector<std::string> _properties;
  };
};

#endif
