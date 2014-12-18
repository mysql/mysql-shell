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

#include <boost/shared_ptr.hpp>

namespace shcore {

/** Basic types that can be passed around code in different languages.
 
 With the exception of Native and Function, all types can be serialized to JSON.
 */
enum Value_type
{
  Null, //! Null/None value
  Bool, //! true or false
  String, //! String values, UTF-8 encoding
  Integer, //! 64bit integer numbers
  Float, //! double numbers

  Native, //! Native C++ object references, may or may not be serializable

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
    boost::shared_ptr<class Native_object> *n;
    boost::shared_ptr<Array_type> *array;
    boost::shared_ptr<Map_type> *map;
    boost::weak_ptr<Map_type> *mapref;
    boost::shared_ptr<class Function_base> *func;
  } value;

  Value() : type(Null) {}
  Value(const Value &copy);

  explicit Value(Value_type type);
  explicit Value(bool b);
  explicit Value(const std::string &s);
  explicit Value(int64_t i);
  explicit Value(double d);
  explicit Value(boost::shared_ptr<Function_base> f);
  explicit Value(boost::shared_ptr<Native_object> n);
  explicit Value(boost::shared_ptr<Map_type> n);
  explicit Value(boost::weak_ptr<Map_type> n);
  explicit Value(boost::shared_ptr<Array_type> n);

  //! parse a string returned by repr() back into a Value
  static Value parse(const std::string &s);

  ~Value();

  Value &operator= (const Value &other);

  bool operator == (const Value &other) const;
  bool operator != (const Value &other) const;

  //! returns a human-readable description text for the value.
  // if pprint is true, it will try to pretty-print it (like adding newlines)
  std::string descr(bool pprint) const;
  //! returns a string representation of the serialized object, suitable to be passed to parse()
  std::string repr() const;

  std::string &append_descr(std::string &s_out, bool pprint) const;
  std::string &append_repr(std::string &s_out) const;
};


/** An opaque, C++ pointer object.
 *
 * Objects of this type can be interacted with in a limited number of ways and
 * can be used as generic object wrappers/references. 
 *
 * To implement one, subclass it and pass the factory function to register_native_type()
 */
class Native_object
{
public:
  virtual ~Native_object() {}

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
  virtual bool operator == (const Native_object &other) const = 0;

  //! Implements inequality operator
  virtual bool operator != (const Native_object &other) const = 0;

  //! Returns the value of a .property
  virtual Value get_property(const std::string &prop) const = 0;

  //! Sets the value of a .property
  virtual void set_property(const std::string &prop, Value value) = 0;

  //! Calls the named method with the given args
  virtual bool call(const std::string &name, const std::vector<Value> &args, Value &return_value) = 0;

  //! Factory method... creates an instance of Native_object of the given type name
  static Value construct(const std::string &type_name);

  //! Factory method... recreates an instance of Native_object from a value returned by repr()
  static Value reconstruct(const std::string &repr);

public:
  //! Registers a function that creates an instance of a native object
  static void register_native_type(const std::string &type_name,
                                   Native_object *(*factory)(const std::string&));
};


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

  //! Implements inequality operator
  virtual bool operator != (const Function_base &other) const = 0;

  //! Invokes the function and passes back the return value.
  // arglist must match the signature
  virtual bool invoke(const std::vector<Value> &args, Value &return_value) = 0;

  //! 0 arg convenience wrapper for invoke
  virtual bool invoke(Value &return_value);

  //! 1 arg convenience wrapper for invoke
  virtual bool invoke(const Value &arg1, Value &return_value);

  //! 2 arg convenience wrapper for invoke
  virtual bool invoke(const Value &arg1, const Value &arg2, Value &return_value);
};

};

#endif // _CORE_TYPES_H_

