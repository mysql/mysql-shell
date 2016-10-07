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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _MOD_INTERACTIVE_PROXY_H_
#define _MOD_INTERACTIVE_PROXY_H_

#include "shellcore/types_cpp.h"
#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

namespace shcore {
class Shell_core;

/**
* Base object to provide interaction capabilities on calls to the wrapped object
* which on this context is called the Target Object.
*
* Adding Object Interaction
*
* It can be done at three different points:
*
* - When trying to access a property or execute a method of the Target Object
*   and the Target Object is NOT set.
* - When trying to access any of the Target Object properties.
* - When trying to execute any of the Target Object methods.
*
* For the first type, derived classes should implement the resolve() function.
* For the second type, derived classes should implement interactive_get_member.
* For the third type, derived classes should implement a function for each function
* where interactivity will be added.
*/
class SHCORE_PUBLIC Interactive_object_wrapper : public Cpp_object_bridge {
public:
  Interactive_object_wrapper(const std::string& alias, Shell_core& core);

  // Returns Undefined if the _target is NOT set
  virtual std::string class_name() const { return _target ? _target->class_name() : "Undefined"; }

  // Bridged functions, they will work on the target object if set
  virtual bool operator == (const Object_bridge &other) const { return _target ? *_target == other : *this == other; };
  virtual bool operator != (const Object_bridge &other) const { return _target ? *_target != other : *this != other; };
  virtual std::vector<std::string> get_members() const { return _target ? _target->get_members() : Cpp_object_bridge::get_members(); }
  virtual std::vector<std::string> get_members_advanced(const NamingStyle &style) { return _target ? _target->get_members_advanced(style) : Cpp_object_bridge::get_members(); }
  virtual void set_member(const std::string &prop, Value value) { _target ? _target->set_member(prop, value) : Cpp_object_bridge::set_member(prop, value); }
  virtual bool is_indexed() const { return _target ? _target->is_indexed() : Cpp_object_bridge::is_indexed(); }
  virtual Value get_member(size_t index) const { return _target ? _target->get_member(index) : Cpp_object_bridge::get_member(index); }
  virtual void set_member(size_t index, Value value) { _target ? _target->set_member(index, value) : Cpp_object_bridge::set_member(index, value); }
  virtual bool has_method(const std::string &name) const { return _target ? _target->has_method(name) : Cpp_object_bridge::has_method(name); }
  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const { return _target ? _target->append_descr(s_out, indent, quote_strings) : Cpp_object_bridge::append_descr(s_out, indent, quote_strings); }
  virtual std::string &append_repr(std::string &s_out) const { return _target ? _target->append_repr(s_out) : Cpp_object_bridge::append_repr(s_out); }
  virtual void append_json(JSON_dumper& dumper) const { _target ? _target->append_json(dumper) : Cpp_object_bridge::append_json(dumper); }

  // These functions are bridged too, but they include a resolution step: if the target object os not set, resolve() will be called on an attempt to
  // set _target based on input from the user (interaction)
  // These functions are called on attempts to access an object's property or method
  virtual bool has_method_advanced(const std::string &name, const NamingStyle &style);
  virtual bool has_member(const std::string &prop) const;
  virtual bool has_member_advanced(const std::string &prop, const NamingStyle &style);
  virtual Value get_member(const std::string &prop) const;
  virtual Value get_member_advanced(const std::string &prop, const NamingStyle &style);
  virtual Value call(const std::string &name, const Argument_list &args);
  virtual Value call_advanced(const std::string &name, const Argument_list &args, const NamingStyle &style);

  virtual shcore::Value help(const shcore::Argument_list &args);

  /**
  * resolve() is called when the target object is not defined and an attempt to use it
  * has been done either by trying to:
  * - Access a property
  * - Execute a method
  *
  * Derived classes should implement this function to resolve the target object either by
  * user interaction or other means.
  */
  virtual void resolve() const {};

  /**
  * interactive_get_member must be implemented by derived classes to add
  * interactivity when accessing specific properties of the target object.
  *
  * Default implementation retrieves the property from the target object
  * making the existence of this wrapper transparent.
  */
  virtual Value interactive_get_member(const std::string &prop) const;
  virtual Value interactive_get_member_advanced(const std::string &prop, const NamingStyle &style);

public:
  // Accessors for the target object.
  void set_target(std::shared_ptr<Cpp_object_bridge> target) { _target = target; }
  std::shared_ptr<Cpp_object_bridge> get_target() { return _target; }

protected:
  std::string _alias;
  std::shared_ptr<Cpp_object_bridge> _target;
  Shell_core& _shell_core;
  Interpreter_delegate *_delegate;

  // This array would contain function names that work even when _target is not set
  std::map<std::string, std::shared_ptr<Cpp_function> > _wrapper_functions;

  void set_wrapper_function(const std::string& name) { _wrapper_functions[name] = _funcs[name]; }

  // Helper functions to enable implementing interaction
  void print(const std::string& text) const;
  void println(const std::string& text = "", const std::string& tag = "") const;
  void print_value(const shcore::Value& value, const std::string& tag) const;
  bool prompt(const std::string& prompt, std::string &ret_val) const;
  bool password(const std::string& prompt, std::string &ret_val) const;
  
  shcore::Value call_target(const std::string &name, const Argument_list &args);
};
}

#endif
