/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
  enum NamingStyle
  {
    LowerCamelCase = 0,
    LowerCaseUnderscores = 1,
    Constants = 2
  };

  class SHCORE_PUBLIC Cpp_property_name
  {
  public:
    Cpp_property_name(const std::string &name, bool constant = false);
    std::string name(const NamingStyle& style);
    std::string base_name();

  private:

    // Each instance holds it's names on the different styles
    std::string _name[2];
  };

  class SHCORE_PUBLIC Cpp_function : public Function_base
  {
  public:
    //TODO make this work with direct function pointers and skip std::function
    typedef std::function<Value(const shcore::Argument_list &)> Function;

    virtual ~Cpp_function() {}

    virtual std::string name();
    virtual std::string name(const NamingStyle& style);

    virtual std::vector<std::pair<std::string, Value_type> > signature();

    virtual std::pair<std::string, Value_type> return_type();

    virtual bool operator == (const Function_base &other) const;

    virtual Value invoke(const Argument_list &args);

    virtual bool has_var_args() { return _var_args; }

    static std::shared_ptr<Function_base> create(const std::string &name, const Function &func, const char *arg1_name, Value_type arg1_type = Undefined, ...);

    static std::shared_ptr<Function_base> create(const std::string &name, const Function &func, const std::vector<std::pair<std::string, Value_type> > &signature);

  protected:
    friend class Cpp_object_bridge;

    Cpp_function(const std::string &name, const Function &func, bool var_args);
    Cpp_function(const std::string &name, const Function &func, const char *arg1_name, Value_type arg1_type = Undefined, ...);
    Cpp_function(const std::string &name, const Function &func, const std::vector<std::pair<std::string, Value_type> > &signature);

    // Each instance holds it's names on the different styles
    std::string _name[2];
    Function _func;

    std::vector<std::pair<std::string, Value_type> > _signature;
    Value_type _return_type;
    bool _var_args;
  };

  class SHCORE_PUBLIC Cpp_object_bridge : public Object_bridge
  {
  public:
    struct ScopedStyle
    {
    public:
      ScopedStyle(Cpp_object_bridge *target, NamingStyle style) :_target(target)
      {
        _target->naming_style = style;
      }
      ~ScopedStyle()
      {
        _target->naming_style = LowerCamelCase;
      }
    private:
      Cpp_object_bridge *_target;
    };

    Cpp_object_bridge();
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

    // These advanced functions verify the requested property/function to see if it is
    // valid on the active naming style first, if so, then the normal functions (above)
    // are called with the base property/function name
    virtual std::vector<std::string> get_members_advanced(const NamingStyle &style);
    virtual Value get_member_advanced(const std::string &prop, const NamingStyle &style);
    virtual bool has_member_advanced(const std::string &prop, const NamingStyle &style);
    virtual void set_member_advanced(const std::string &prop, Value value, const NamingStyle &style);
    virtual bool has_method_advanced(const std::string &name, const NamingStyle &style);
    virtual Value call_advanced(const std::string &name, const Argument_list &args, const NamingStyle &style);

    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;
    std::shared_ptr<ScopedStyle> set_scoped_naming_style(const NamingStyle& style);

    virtual shcore::Value help(const shcore::Argument_list &args);
    virtual std::string get_help_text(const std::string& topic){ return ""; }

  protected:
    virtual void add_method(const std::string &name, Cpp_function::Function func,
                    const char *arg1_name, Value_type arg1_type = Undefined, ...);
    virtual void add_varargs_method(const std::string &name, Cpp_function::Function func);

    // Constants and properties are not handled through the Cpp_property_name class
    // which supports different naming styles
    virtual void add_constant(const std::string &name);
    virtual void add_property(const std::string &name, const std::string &getter = "");
    virtual void delete_property(const std::string &name, const std::string &getter = "");

    // Helper function that retrieves a qualified functio name using the active naming style
    // Used mostly for errors in function validations
    std::string get_function_name(const std::string& member, bool fully_specified = true) const;

    typedef std::pair< std::string, std::shared_ptr<Cpp_function> > FunctionEntry;
    std::map<std::string, std::shared_ptr<Cpp_function> > _funcs;
    std::vector<std::shared_ptr<Cpp_property_name> > _properties;

    // The global active naming style
    NamingStyle naming_style;
  };

  // Helper function to get a name on the active naming style, input is expected to be lowerCamelCase (the used all the time)
  //std::string get_member_name(const std::string& name, NamingStyle style = Cpp_object_bridge::get_naming_style());
  std::string get_member_name(const std::string& name, NamingStyle style);
};

#endif
