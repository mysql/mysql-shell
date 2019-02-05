/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#include "types_common.h"

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk_export.h"

namespace shcore {
/** Basic types that can be passed around code in different languages.

 With the exception of Native and Function, all types can be serialized to JSON.
 */
enum Value_type {
  Undefined,  //! Undefined
  Null,       //! Null/None value
  Bool,       //! true or false
  String,     //! String values, UTF-8 encoding
  Integer,    //! 64bit integer numbers
  UInteger,   //! unsigned 64bit integer numbers
  Float,      //! double numbers

  Object,  //! Native/bridged C++ object refs, may or may not be serializable

  Array,   //! Array/List container
  Map,     //! Dictionary/Map/Object container
  MapRef,  //! A weak reference to a Map

  Function  //! A function reference, not serializable.
};

// kTypeConvertible[from_type][to_type] = is_convertible
// from_type = row, to_type = column
extern const bool kTypeConvertible[12][12];

std::string type_description(Value_type type);

class Object_bridge;
typedef std::shared_ptr<Object_bridge> Object_bridge_ref;

/** A generic value that can be used from any language we support.

 Anything that can be represented using this can be passed as a parameter to
 scripting functions or stored in the internal registry or anywhere. If
 serializable types are used, then they may also be stored as a JSON document.

 Values are exposed to scripting languages according to the following rules:

 - Simple types (Null, Bool, String, Integer, Float, UInteger) are converted
 directly to the target type, both ways

 - Arrays and Maps are converted directly to the target type, both ways

 - Functions are wrapped into callable objects from C++ to scripting language
 - Scripting language functions are wrapped into an instance of a language
 specific subclass of Function_base

 - C++ Objects are generically wrapped into a scripting language object, except
 when there's a specific native counterpart

 - Scripting language objects are either generically wrapped to a language
 specific generic object wrapper or converted to a specific C++ Object subclass

 Example: JS Date object is converted to a C++ Date object and vice-versa, but
 Mysql_connection is wrapped generically

 @section Implicit type conversions

           Null Bool  String  Integer UInteger  Float Object  Array Map
 Null      OK   -     -       -       -         -     OK      OK    OK
 Bool      -    OK    -       OK      OK        OK    -       -     -
 String    -    OK    OK      OK      OK        OK    -       -     -
 Integer   -    OK    -       OK      OK        OK    -       -     -
 UInteger  -    OK    -       OK      OK        OK    -       -     -
 Float     -    OK    -       OK      OK        OK    -       -     -
 Object    -    -     -       -       -         -     OK      -     -
 Array     -    -     -       -       -         -     -       OK    -
 Map       -    -     -       -       -         -     -       -     OK

 * Integer <-> UInteger conversions are only possible if the range allows it
 * Null can be cast to Object/Array/Map, but a valid Object/Array/Map pointer
 is not NULL, so it can't be cast to it.
  */
struct SHCORE_PUBLIC Value {
  typedef std::vector<Value> Array_type;
  typedef std::shared_ptr<Array_type> Array_type_ref;

  class SHCORE_PUBLIC Map_type {
   public:
    typedef std::map<std::string, Value> container_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;

    inline bool has_key(const std::string &k) const { return find(k) != end(); }

    Value_type get_type(const std::string &k) const;

    bool is_null(const std::string &k) const {
      return get_type(k) == shcore::Null;
    }

    std::string get_string(const std::string &k,
                           const std::string &def = "") const;
    bool get_bool(const std::string &k, bool def = false) const;
    int64_t get_int(const std::string &k, int64_t def = 0) const;
    uint64_t get_uint(const std::string &k, uint64_t def = 0) const;
    double get_double(const std::string &k, double def = 0.0) const;
    std::shared_ptr<Value::Map_type> get_map(
        const std::string &k,
        std::shared_ptr<Map_type> def = std::shared_ptr<Map_type>()) const;
    std::shared_ptr<Value::Array_type> get_array(
        const std::string &k,
        std::shared_ptr<Array_type> def = std::shared_ptr<Array_type>()) const;
    void merge_contents(std::shared_ptr<Map_type> source, bool overwrite);

    template <class C>
    std::shared_ptr<C> get_object(
        const std::string &k,
        std::shared_ptr<C> def = std::shared_ptr<C>()) const {
      const_iterator iter = find(k);
      if (iter == end()) return def;
      iter->second.check_type(Object);
      return iter->second.as_object<C>();
    }

    const_iterator find(const std::string &k) const { return _map.find(k); }
    iterator find(const std::string &k) { return _map.find(k); }

    void erase(const std::string &k) { _map.erase(k); }
    void clear() { _map.clear(); }

    const_iterator begin() const { return _map.begin(); }
    iterator begin() { return _map.begin(); }

    const_iterator end() const { return _map.end(); }
    iterator end() { return _map.end(); }

    void set(const std::string &k, const shcore::Value &v) { _map[k] = v; }

    const container_type::mapped_type &at(const std::string &k) const {
      return _map.at(k);
    }
    container_type::mapped_type &operator[](const std::string &k) {
      return _map[k];
    }
    bool operator==(const Map_type &other) const { return _map == other._map; }

    bool empty() const { return _map.empty(); }
    size_t size() const { return _map.size(); }
    size_t count(const std::string &k) const { return _map.count(k); }

    template <class T>
    std::pair<iterator, bool> emplace(const std::string &key, const T &value) {
      return _map.emplace(key, Value(value));
    }

   private:
    container_type _map;
  };
  typedef std::shared_ptr<Map_type> Map_type_ref;

  Value_type type;
  union {
    bool b;
    std::string *s;
    int64_t i;
    uint64_t ui;
    double d;
    std::shared_ptr<class Object_bridge> *o;
    std::shared_ptr<Array_type> *array;
    std::shared_ptr<Map_type> *map;
    std::weak_ptr<Map_type> *mapref;
    std::shared_ptr<class Function_base> *func;
  } value;

  Value() : type(Undefined) {}
  Value(const Value &copy);

  explicit Value(const std::string &s);
  explicit Value(const char *);
  explicit Value(const char *, size_t n);
  explicit Value(int i);
  explicit Value(unsigned int ui);
  explicit Value(int64_t i);
  explicit Value(uint64_t ui);
  explicit Value(float f);
  explicit Value(double d);
  explicit Value(bool b);
  explicit Value(std::shared_ptr<Function_base> f);
  explicit Value(std::shared_ptr<Object_bridge> o);
  explicit Value(std::weak_ptr<Map_type> n);
  explicit Value(Map_type_ref n);
  explicit Value(Array_type_ref n);

  static Value wrap(Object_bridge *o) {
    return Value(std::shared_ptr<Object_bridge>(o));
  }

  template <class T>
  static Value wrap(T *o) {
    return Value(
        std::static_pointer_cast<Object_bridge>(std::shared_ptr<T>(o)));
  }

  static Value new_array() {
    return Value(std::shared_ptr<Array_type>(new Array_type()));
  }
  static Value new_map() {
    return Value(std::shared_ptr<Map_type>(new Map_type()));
  }

  static Value Null() {
    Value v;
    v.type = shcore::Null;
    return v;
  }
  static Value True() {
    Value v;
    v.type = shcore::Bool;
    v.value.b = true;
    return v;
  }
  static Value False() {
    Value v;
    v.type = shcore::Bool;
    v.value.b = false;
    return v;
  }

  //! parse a string returned by repr() back into a Value
  static Value parse(const std::string &s);

  ~Value();

  Value &operator=(const Value &other);

  bool operator==(const Value &other) const;

  bool operator!=(const Value &other) const { return !(*this == other); }

  operator bool() const { return type != Undefined && type != shcore::Null; }

  //! returns a human-readable description text for the value.
  // if pprint is true, it will try to pretty-print it (like adding newlines)
  std::string descr(bool pprint = false) const;

  //! returns a string representation of the serialized object, suitable to be
  //! passed to parse()
  std::string repr() const;

  //! returns a JSON representation of the object
  std::string json(bool pprint = false) const;

  //! returns a YAML representation of the Value
  std::string yaml() const;

  std::string &append_descr(std::string &s_out, int indent = -1,
                            char quote_strings = '\0') const;
  std::string &append_repr(std::string &s_out) const;

  void check_type(Value_type t) const;

  bool as_bool() const;
  int64_t as_int() const;
  uint64_t as_uint() const;
  double as_double() const;
  std::string as_string() const;
  const std::string &get_string() const {
    check_type(String);
    return *value.s;
  }
  template <class C>
  std::shared_ptr<C> as_object() const {
    check_type(Object);
    return std::dynamic_pointer_cast<C>(type == shcore::Null ? nullptr
                                                             : *value.o);
  }

  std::shared_ptr<Object_bridge> as_object() const {
    check_type(Object);
    return std::dynamic_pointer_cast<Object_bridge>(
        type == shcore::Null ? nullptr : *value.o);
  }

  std::shared_ptr<Map_type> as_map() const {
    check_type(Map);
    return type == shcore::Null ? nullptr : *value.map;
  }

  std::shared_ptr<Array_type> as_array() const {
    check_type(Array);
    return type == shcore::Null ? nullptr : *value.array;
  }

  std::vector<std::string> to_string_vector() const {
    std::vector<std::string> vec;
    check_type(Array);
    for (const Value &v : *as_array()) {
      vec.push_back(v.get_string());
    }
    return vec;
  }

  std::shared_ptr<Function_base> as_function() const {
    check_type(Function);
    return std::dynamic_pointer_cast<Function_base>(
        type == shcore::Null ? nullptr : *value.func);
  }

 private:
  static Value parse(const char **pc);
  static Value parse_map(const char **pc);
  static Value parse_array(const char **pc);
  static Value parse_string(const char **pc, char quote);
  static Value parse_single_quoted_string(const char **pc);
  static Value parse_double_quoted_string(const char **pc);
  static Value parse_number(const char **pc);

  std::string yaml(int indent) const;
};
typedef Value::Map_type_ref Dictionary_t;
typedef Value::Array_type_ref Array_t;

inline Dictionary_t make_dict() {
  return Value::Map_type_ref(new Value::Map_type());
}
inline Array_t make_array() {
  return Value::Array_type_ref(new Value::Array_type());
}

class SHCORE_PUBLIC Exception : public std::exception {
  std::shared_ptr<Value::Map_type> _error;

 public:
  explicit Exception(const std::shared_ptr<Value::Map_type> e);

  virtual ~Exception() noexcept {}

  static Exception runtime_error(const std::string &message);
  static Exception argument_error(const std::string &message);
  static Exception attrib_error(const std::string &message);
  static Exception value_error(const std::string &message);
  static Exception type_error(const std::string &message);
  static Exception logic_error(const std::string &message);
  static Exception metadata_error(const std::string &message);
  static Exception external_action_required(const std::string &message);

  static Exception mysql_error_with_code(const std::string &message, int code) {
    return error_with_code("MySQL Error", message, code);
  }

  static Exception mysql_error_with_code_and_state(const std::string &message,
                                                   int code,
                                                   const char *sqlstate) {
    return error_with_code_and_state("MySQL Error", message, code, sqlstate);
  }

  static Exception error_with_code(const std::string &type,
                                   const std::string &message, int code);
  static Exception error_with_code_and_state(const std::string &type,
                                             const std::string &message,
                                             int code, const char *sqlstate);
  static Exception parser_error(const std::string &message);
  static Exception scripting_error(const std::string &message);

  void set_file_context(const std::string &file, size_t line);

  bool is_argument() const;
  bool is_attribute() const;
  bool is_value() const;
  bool is_type() const;
  bool is_server() const;
  bool is_mysql() const;
  bool is_parser() const;

  virtual const char *what() const noexcept;

  const char *type() const noexcept;

  int64_t code() const noexcept;

  std::shared_ptr<Value::Map_type> error() const { return _error; }

  std::string format();
};

class SHCORE_PUBLIC Argument_list {
 public:
  // TODO(alfredo) remove most of this when possible
  const std::string &string_at(unsigned int i) const;
  bool bool_at(unsigned int i) const;
  int64_t int_at(unsigned int i) const;
  uint64_t uint_at(unsigned int i) const;
  double double_at(unsigned int i) const;

  template <class C>
  std::shared_ptr<C> object_at(unsigned int i) const {
    std::shared_ptr<C> ret_val;
    try {
      auto object = object_at(i);
      ret_val = std::dynamic_pointer_cast<C>(object);
    } catch (...) {
      // We don't care, return value will be empty
    }
    return ret_val;
  }

  std::shared_ptr<Object_bridge> object_at(unsigned int i) const;
  std::shared_ptr<Value::Map_type> map_at(unsigned int i) const;
  std::shared_ptr<Value::Array_type> array_at(unsigned int i) const;

  void ensure_count(unsigned int c, const char *context) const;
  void ensure_count(unsigned int minc, unsigned int maxc,
                    const char *context) const;
  void ensure_at_least(unsigned int minc, const char *context) const;

  void push_back(const Value &value) { _args.push_back(value); }
  void pop_back() { _args.pop_back(); }
  size_t size() const { return _args.size(); }
  const Value &at(size_t i) const { return _args.at(i); }
  Value &operator[](size_t i) { return _args[i]; }
  const Value &operator[](size_t i) const { return _args[i]; }
  void clear() { _args.clear(); }

  std::vector<Value>::const_iterator begin() const { return _args.begin(); }
  std::vector<Value>::const_iterator end() const { return _args.end(); }

  bool operator==(const Argument_list &other) const;

  template <class T>
  void emplace_back(const T &value) {
    return _args.emplace_back(Value(value));
  }

 private:
  std::vector<Value> _args;
};

class SHCORE_PUBLIC Argument_map {
 public:
  Argument_map();
  Argument_map(const Value::Map_type &map);

  const std::string &string_at(const std::string &key) const;
  bool bool_at(const std::string &key) const;
  int64_t int_at(const std::string &key) const;
  uint64_t uint_at(const std::string &key) const;
  double double_at(const std::string &key) const;
  std::shared_ptr<Object_bridge> object_at(const std::string &key) const;
  std::shared_ptr<Value::Map_type> map_at(const std::string &key) const;
  std::shared_ptr<Value::Array_type> array_at(const std::string &key) const;
  template <class C>
  std::shared_ptr<C> object_at(const std::string &key) const {
    std::shared_ptr<C> ret_val;
    try {
      auto object = object_at(key);
      ret_val = std::dynamic_pointer_cast<C>(object);
    } catch (...) {
      // We don't care, return value will be empty
    }
    return ret_val;
  }

  void ensure_keys(const std::set<std::string> &mandatory_keys,
                   const std::set<std::string> &optional_keys,
                   const char *context, bool case_sensitive = true) const;

  bool validate_keys(const std::set<std::string> &mandatory_keys,
                     const std::set<std::string> &optional_keys,
                     std::vector<std::string> &missing_keys,
                     std::vector<std::string> &invalid_keys,
                     bool case_sensitive = true) const;

  size_t size() const { return _map.size(); }
  const Value &at(const std::string &key) const { return _map.at(key); }
  Value &operator[](const std::string &key) { return _map[key]; }
  void clear() { _map.clear(); }
  bool has_key(const std::string &key) const { return _map.has_key(key); }

  const Value::Map_type &as_map() const { return _map; }

 private:
  Value::Map_type _map;
  static bool comp(const std::string &lhs, const std::string &rhs);
  static bool icomp(const std::string &lhs, const std::string &rhs);
};

template <class T>
struct value_type_for_native {};

template <>
struct value_type_for_native<bool> {
  static const Value_type type = Bool;

  static bool extract(const Value &value) { return value.as_bool(); }
};
template <>
struct value_type_for_native<int64_t> {
  static const Value_type type = Integer;

  static int64_t extract(const Value &value) { return value.as_int(); }
};
template <>
struct value_type_for_native<uint64_t> {
  static const Value_type type = UInteger;

  static uint64_t extract(const Value &value) { return value.as_uint(); }
};
template <>
struct value_type_for_native<double> {
  static const Value_type type = Float;

  static double extract(const Value &value) { return value.as_double(); }
};
template <>
struct value_type_for_native<std::string> {
  static const Value_type type = String;

  static std::string extract(const Value &value) { return value.as_string(); }
};
template <>
struct value_type_for_native<const std::string> {
  static const Value_type type = String;

  static std::string extract(const Value &value) { return value.as_string(); }
};

template <>
struct value_type_for_native<std::vector<std::string>> {
  static const Value_type type = Array;

  static std::vector<std::string> extract(const Value &value) {
    return value.to_string_vector();
  }
};

template <>
struct value_type_for_native<Object_bridge *> {
  static const Value_type type = Object;
};
template <>
struct value_type_for_native<Object_bridge_ref> {
  static const Value_type type = Object;

  static Object_bridge_ref extract(const Value &value) {
    return value.as_object();
  }
};
template <>
struct value_type_for_native<Value::Map_type> {
  static const Value_type type = Map;
};
template <>
struct value_type_for_native<Dictionary_t> {
  static const Value_type type = Map;

  static Dictionary_t extract(const Value &value) { return value.as_map(); }
};
template <>
struct value_type_for_native<Value::Array_type> {
  static const Value_type type = Array;
};
template <>
struct value_type_for_native<Array_t> {
  static const Value_type type = Array;

  static Array_t extract(const Value &value) { return value.as_array(); }
};

template <>
struct value_type_for_native<Value> {
  static const Value_type type = Undefined;

  static Value extract(const Value &value) { return value; }
};

std::string SHCORE_PUBLIC type_name(Value_type type);

// Extract option values from an options dictionary, with validations
// Replaces Argument_map
class Option_unpacker {
 public:
  explicit Option_unpacker(const Dictionary_t &options);

  virtual ~Option_unpacker() {}

  // For external unpacker objects
  template <typename T>
  Option_unpacker &unpack(T *extra) {
    extra->unpack(this);
    return *this;
  }

  // Extract required option
  template <typename T>
  Option_unpacker &required(const char *name, T *out_value) {
    extract_value<T>(name, out_value,
                     get_required(name, value_type_for_native<T>::type));
    return *this;
  }

  // Extract optional option
  template <typename T>
  Option_unpacker &optional(const char *name, T *out_value) {
    extract_value<T>(name, out_value,
                     get_optional(name, value_type_for_native<T>::type));
    return *this;
  }

  template <typename T>
  Option_unpacker &optional(const char *name,
                            mysqlshdk::utils::nullable<T> *out_value) {
    extract_value<T>(name, out_value,
                     get_optional(name, value_type_for_native<T>::type));
    return *this;
  }

  // Extract optional option with exact type (no conversions)
  template <typename T>
  Option_unpacker &optional_exact(const char *name, T *out_value) {
    extract_value<T>(name, out_value,
                     get_optional_exact(name, value_type_for_native<T>::type));
    return *this;
  }

  template <typename T>
  Option_unpacker &optional_exact(const char *name,
                                  mysqlshdk::utils::nullable<T> *out_value) {
    extract_value<T>(name, out_value,
                     get_optional_exact(name, value_type_for_native<T>::type));
    return *this;
  }

  // Case insensitive
  template <typename T>
  Option_unpacker &optional_ci(const char *name, T *out_value) {
    extract_value<T>(name, out_value,
                     get_optional(name, value_type_for_native<T>::type, true));
    return *this;
  }

  void end(const std::string &context = "");

 protected:
  Dictionary_t m_options;
  std::set<std::string> m_unknown;
  std::set<std::string> m_missing;

  template <typename T, typename S>
  void extract_value(const char *name, S *out_value, const Value &value) {
    if (value) {
      try {
        *out_value = value_type_for_native<T>::extract(value);
      } catch (const std::exception &e) {
        std::string msg = e.what();
        if (msg.compare(0, 18, "Invalid typecast: ") == 0) msg = msg.substr(18);
        throw Exception::type_error(std::string("Option '") + name + "' " +
                                    msg);
      }
    }
  }

  Value get_required(const char *name, Value_type type);
  Value get_optional(const char *name, Value_type type,
                     bool case_insensitive = false);
  Value get_optional_exact(const char *name, Value_type type,
                           bool case_insensitive = false);

  void validate(const std::string &context = "");
};

class JSON_dumper;

/** An instance of an object, that's implemented in some language.
 *
 * Objects of this type can be interacted with through it's accessor methods
 * can be used as generic object wrappers/references.
 *
 * They can be instantiated through their Metaclass factory
 */
class SHCORE_PUBLIC Object_bridge {
 public:
  virtual ~Object_bridge() {}

  virtual std::string class_name() const = 0;

  //! Appends JSON representation of the object if supported
  virtual void append_json(JSON_dumper &dumper) const;

  //! Appends descriptive text to the string, suitable for showing to the user
  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const = 0;

  //! Returns a representation of the object suitable to be passed to a Factory
  // constructor of this object, which would create an equivalent instance of
  // the object Format for repr() of Native_objects must follow this pattern:
  // <Typename:data> where Typename is the same name used to register the type
  virtual std::string &append_repr(std::string &s_out) const = 0;

  //! Returns the list of members that this object has
  virtual std::vector<std::string> get_members() const = 0;

  //! Implements equality operator
  virtual bool operator==(const Object_bridge &other) const = 0;

  virtual bool operator!=(const Object_bridge &other) const {
    return !(*this == other);
  }

  //! Returns the value of a member
  virtual Value get_member(const std::string &prop) const = 0;

  //! Verifies if the object has a member
  virtual bool has_member(const std::string &prop) const = 0;

  //! Sets the value of a member
  virtual void set_member(const std::string &prop, Value value) = 0;

  //! Returns the value of a member
  virtual bool is_indexed() const = 0;

  //! Returns the value of a member
  virtual Value get_member(size_t index) const = 0;

  //! Sets the value of a member
  virtual void set_member(size_t index, Value value) = 0;

  //! Returns true if a method with the given name exists.
  virtual bool has_method(const std::string &name) const = 0;

  //! Calls the named method with the given args
  virtual Value call(const std::string &name, const Argument_list &args) = 0;
};

class SHCORE_PUBLIC Function_base {
 public:
  Function_base() = default;
  Function_base(const Function_base &other) = default;
  Function_base(Function_base &&other) = default;
  Function_base &operator=(const Function_base &other) = default;
  Function_base &operator=(Function_base &&other) = default;
  virtual ~Function_base() = default;

  //! The name of the function
  virtual const std::string &name() const = 0;

  //! Returns the signature of the function, as a list of
  // argname - description, Type pairs
  virtual const std::vector<std::pair<std::string, Value_type>> &signature()
      const = 0;

  //! Return type of the function, as a description, Type pair
  virtual Value_type return_type() const = 0;

  //! Implements equality operator
  virtual bool operator==(const Function_base &other) const = 0;

  //! Invokes the function and passes back the return value.
  // arglist must match the signature
  virtual Value invoke(const Argument_list &args) = 0;

  virtual bool has_var_args() = 0;
};

/** Pointer to a function that may be implemented in any language.
 */
typedef std::shared_ptr<Function_base> Function_base_ref;

bool my_strnicmp(const char *c1, const char *c2, size_t n);
}  // namespace shcore

#endif  // _TYPES_H_
