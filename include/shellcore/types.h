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

#ifndef _CORE_TYPES_H_
#define _CORE_TYPES_H_

#include <vector>
#include <map>
#include <string>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace shcore {

/** Basic types that can be passed around code in different languages.
 
 With the exception of Native and Function, all types can be serialized to JSON.
 */
enum Value_type
{
  Undefined, //! Undefined
  Null, //! Null/None value
  Bool, //! true or false
  String, //! String values, UTF-8 encoding
  Integer, //! 64bit integer numbers
  Float, //! double numbers

  Object, //! Native/bridged C++ object references, may or may not be serializable

  Array, //! Array/List container
  Map, //! Dictionary/Map/Object container
  MapRef, //! A weak reference to a Map

  Function //! A function reference, not serializable.
};


/** A generic value that can be used from any language we support.

 Anything that can be represented using this can be passed as a parameter to 
 scripting functions or stored in the internal registry or anywhere. If serializable
 types are used, then they may also be stored as a JSON document.
 */
struct Value
{
  typedef std::vector<Value> Array_type;
  typedef std::map<std::string, Value> Map_type;

  Value_type type;
  union
  {
    bool b;
    std::string *s;
    int64_t i;
    double d;
    boost::shared_ptr<class Object_bridge> *o;
    boost::shared_ptr<Array_type> *array;
    boost::shared_ptr<Map_type> *map;
    boost::weak_ptr<Map_type> *mapref;
    boost::shared_ptr<class Function_base> *func;
  } value;

  Value() : type(Undefined) {}
  Value(const Value &copy);

  explicit Value(Value_type type);
  explicit Value(bool b);
  explicit Value(const std::string &s);
  explicit Value(int64_t i);
  explicit Value(double d);
  explicit Value(boost::shared_ptr<Function_base> f);
  explicit Value(boost::shared_ptr<Object_bridge> o);
  explicit Value(boost::shared_ptr<Map_type> n);
  explicit Value(boost::weak_ptr<Map_type> n);
  explicit Value(boost::shared_ptr<Array_type> n);

  //! parse a string returned by repr() back into a Value
  static Value parse(const std::string &s);

  ~Value();

  Value &operator= (const Value &other);

  bool operator == (const Value &other) const;

  operator bool() const
  {
    return type != Undefined;
  }

  //! returns a human-readable description text for the value.
  // if pprint is true, it will try to pretty-print it (like adding newlines)
  std::string descr(bool pprint) const;
  //! returns a string representation of the serialized object, suitable to be passed to parse()
  std::string repr() const;

  std::string &append_descr(std::string &s_out, bool pprint) const;
  std::string &append_repr(std::string &s_out) const;
};


class Argument_list : public std::vector<Value>
{
public:
  const std::string &string_at(int i) const;
  bool bool_at(int i) const;
  int64_t int_at(int i) const;
  double double_at(int i) const;
  boost::shared_ptr<Object_bridge> object_at(int i) const;
  boost::shared_ptr<Value::Map_type> map_at(int i) const;
  boost::shared_ptr<Value::Array_type> array_at(int i) const;

  void ensure_count(int c, const char *context) const;
};


template<class T> struct value_type_for_native { };

template<> struct value_type_for_native<bool> { static const Value_type type = Bool; };
template<> struct value_type_for_native<int64_t> { static const Value_type type = Integer; };
template<> struct value_type_for_native<double> { static const Value_type type = Float; };
template<> struct value_type_for_native<std::string> { static const Value_type type = String; };
template<> struct value_type_for_native<Object_bridge*> { static const Value_type type = Object; };
template<> struct value_type_for_native<Value::Map_type> { static const Value_type type = Map; };
template<> struct value_type_for_native<Value::Array_type> { static const Value_type type = Array; };


/** An instance of an object, that's implemented in some language.
 *
 * Objects of this type can be interacted with through it's accessor methods
 * can be used as generic object wrappers/references.
 *
 * They can be instantiated through their Metaclass factory
 */
class Object_bridge
{
public:
  virtual ~Object_bridge() {}

  virtual std::string class_name() const = 0;

  //! Appends descriptive text to the string, suitable for showing to the user
  virtual std::string &append_descr(std::string &s_out, bool pprint) const = 0;

  //! Returns a representation of the object suitable to be passed to a Factory
  // constructor of this object, which would create an equivalent instance of the object
  // Format for repr() of Native_objects must follow this pattern:
  // <Typename:data> where Typename is the same name used to register the type
  virtual std::string &append_repr(std::string &s_out) const = 0;

  //! Returns the list of members that this object has
  virtual std::vector<std::string> get_members() const = 0;

  //! Implements equality operator
  virtual bool operator == (const Object_bridge &other) const = 0;

  //! Returns the value of a member
  virtual Value get_member(const std::string &prop) const = 0;

  //! Sets the value of a member
  virtual void set_member(const std::string &prop, Value value) = 0;

  //! Calls the named method with the given args
  virtual Value call(const std::string &name, const Argument_list &args) = 0;
};


class Object_factory
{
public:
  virtual ~Object_factory() {}

  virtual std::string name() const = 0;

  //! Factory method... creates an instance of the class
  virtual boost::shared_ptr<Object_bridge> construct(const Argument_list &args) = 0;

  //! Factory method... recreates an instance of the class through the repr value of it
//  virtual boost::shared_ptr<Object_bridge> construct_from_repr(const std::string &repr) = 0;

public:
  //! Registers a metaclass
  static void register_factory(const std::string &package, Object_factory *fac);

  static boost::shared_ptr<Object_bridge> call_constructor(const std::string &package, const std::string &name,
                                                           const Argument_list &args);

  static std::vector<std::string> package_names();
  static std::vector<std::string> package_contents(const std::string &package);

  static bool has_package(const std::string &package);

  // call_constructor_with_repr(const std::string &repr)
};


//! checks that the arglist in args matches the types provided in the args to the function
// in case of a mismatch, an exception is thrown
void validate_args(const std::string &context, const Argument_list &args, Value_type vtype, ...);


/** Pointer to a function that may be implemented in any language.
 */
class Function_base
{
public:
  //! The name of the function
  virtual std::string name() = 0;

  //! Returns the signature of the function, as a list of
  // argname - description, Type pairs
  virtual std::vector<std::pair<std::string, Value_type> > signature() = 0;

  //! Return type of the function, as a description, Type pair
  virtual std::pair<std::string, Value_type> return_type() = 0;

  //! Implements equality operator
  virtual bool operator == (const Function_base &other) const = 0;

  //! Invokes the function and passes back the return value.
  // arglist must match the signature
  virtual Value invoke(const Argument_list &args) = 0;

  //! 0 arg convenience wrapper for invoke
  virtual Value invoke();

  //! 1 arg convenience wrapper for invoke
  virtual Value invoke(const Value &arg1);

  //! 2 arg convenience wrapper for invoke
  virtual Value invoke(const Value &arg1, const Value &arg2);
};


class Invalid_value_error : public std::logic_error
{
public:
  Invalid_value_error(const std::string &e) : std::logic_error(e) {}
};


/** Exception thrown when a Value object is attempted to be accessed as the wrong type
 */
class Type_error : public std::logic_error
{
public:
  Type_error(const std::string &e) : std::logic_error(e) {}
};

};

#endif // _CORE_TYPES_H_

