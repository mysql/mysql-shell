/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_H_

#include "types_common.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/libs/utils/error.h"
#include "mysqlshdk/libs/utils/utils_string.h"

// For error codes used by the shell
#define SHERR_FIRST 50000

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

  Array,  //! Array/List container
  Map,    //! Dictionary/Map/Object container

  Function,  //! A function reference, not serializable.
  Binary     //! Binary data
};

bool is_compatible_type(Value_type source_type, Value_type target_type);

std::string type_description(Value_type type);
std::string SHCORE_PUBLIC type_name(Value_type type);

class Object_bridge;
typedef std::shared_ptr<Object_bridge> Object_bridge_ref;

/** Pointer to a function that may be implemented in any language.
 */
class Function_base;
using Function_base_ref = std::shared_ptr<Function_base>;

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
struct SHCORE_PUBLIC Value final {
  typedef std::vector<Value> Array_type;
  typedef std::shared_ptr<Array_type> Array_type_ref;

  class SHCORE_PUBLIC Map_type final {
   public:
    typedef std::map<std::string, Value> container_type;
    typedef container_type::const_iterator const_iterator;
    typedef container_type::iterator iterator;
    using value_type = container_type::value_type;
    using reverse_iterator = container_type::reverse_iterator;
    using const_reverse_iterator = container_type::const_reverse_iterator;

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

    size_t erase(const std::string &k) { return _map.erase(k); }
    iterator erase(const_iterator it) { return _map.erase(it); }
    iterator erase(iterator it) { return _map.erase(it); }
    void clear() { _map.clear(); }

    const_iterator begin() const { return _map.begin(); }
    iterator begin() { return _map.begin(); }

    const_reverse_iterator rbegin() const { return _map.rbegin(); }
    reverse_iterator rbegin() { return _map.rbegin(); }

    const_iterator end() const { return _map.end(); }
    iterator end() { return _map.end(); }

    const_reverse_iterator rend() const { return _map.rend(); }
    reverse_iterator rend() { return _map.rend(); }

    void set(const std::string &k, shcore::Value &&v) {
      _map[k] = std::move(v);
    }

    void set(const std::string &k, const shcore::Value &v) { _map[k] = v; }

    const container_type::mapped_type &at(const std::string &k) const {
      return _map.at(k);
    }
    container_type::mapped_type &operator[](const std::string &k) {
      return _map[k];
    }

    bool operator==(const Map_type &other) const { return _map == other._map; }
    bool operator!=(const Map_type &other) const { return !(*this == other); }

    // prevent default usage of these
    bool operator<(const Map_type &) const = delete;
    bool operator>(const Map_type &) const = delete;
    bool operator<=(const Map_type &) const = delete;
    bool operator>=(const Map_type &) const = delete;

    bool empty() const { return _map.empty(); }
    size_t size() const { return _map.size(); }
    size_t count(const std::string &k) const { return _map.count(k); }

    template <class T>
    std::pair<iterator, bool> emplace(const std::string &key, T &&v) {
      return _map.emplace(key, Value(std::forward<T>(v)));
    }

   private:
    container_type _map;
  };
  typedef std::shared_ptr<Map_type> Map_type_ref;

 public:
  Value() = default;
  Value(const Value &) = default;
  Value(Value &&) noexcept = default;
  Value &operator=(const Value &) = default;
  Value &operator=(Value &&) noexcept = default;

  ~Value() noexcept = default;

  explicit Value(const std::string &s, bool binary = false);
  explicit Value(std::string &&s, bool binary = false);
  explicit Value(const char *);
  explicit Value(const char *, size_t n, bool binary = false);
  explicit Value(std::string_view s, bool binary = false);
  explicit Value(std::wstring_view s);
  explicit Value(std::nullptr_t);
  explicit Value(int i);
  explicit Value(unsigned int ui);
  explicit Value(int64_t i);
  explicit Value(uint64_t ui);
  explicit Value(float f);
  explicit Value(double d);
  explicit Value(bool b);
  explicit Value(const std::shared_ptr<Function_base> &f);
  explicit Value(std::shared_ptr<Function_base> &&f);
  explicit Value(const std::shared_ptr<Object_bridge> &o);
  explicit Value(std::shared_ptr<Object_bridge> &&o);
  explicit Value(const Map_type_ref &n);
  explicit Value(Map_type_ref &&n);
  explicit Value(const Array_type_ref &n);
  explicit Value(Array_type_ref &&n);

  static Value wrap(Object_bridge *o) {
    return Value(std::shared_ptr<Object_bridge>(o));
  }

  template <class T>
  static Value wrap(std::shared_ptr<T> o) {
    return Value(std::static_pointer_cast<Object_bridge>(std::move(o)));
  }

  static Value new_array() {
    return Value(std::shared_ptr<Array_type>(new Array_type()));
  }
  static Value new_map() {
    return Value(std::shared_ptr<Map_type>(new Map_type()));
  }

  static Value Null() {
    Value v;
    v.m_value = null_value{};
    return v;
  }

  static Value True() { return Value{true}; }
  static Value False() { return Value{false}; }

  //! parse a string returned by repr() back into a Value
  static Value parse(std::string_view s);

  bool operator==(const Value &other) const;
  bool operator!=(const Value &other) const { return !(*this == other); }

  // prevent default usage of these
  bool operator<(const Value &) const = delete;
  bool operator>(const Value &) const = delete;
  bool operator<=(const Value &) const = delete;
  bool operator>=(const Value &) const = delete;

  explicit operator bool() const noexcept {
    auto type = get_type();
    return (type != shcore::Undefined) && (type != shcore::Null);
  }

  bool is_null() const noexcept { return (get_type() == shcore::Null); }

  // helper used by gtest
  friend std::ostream &operator<<(std::ostream &os, const Value &v);

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
  Value_type get_type() const noexcept;

  bool as_bool() const;
  int64_t as_int() const;
  uint64_t as_uint() const;
  double as_double() const;
  std::string as_string() const;
  std::wstring as_wstring() const;

  const std::string &get_string() const {
    check_type(String);

    if (std::holds_alternative<binary_string>(m_value))
      return std::get<binary_string>(m_value);
    return std::get<std::string>(m_value);
  }

  template <class C>
  std::shared_ptr<C> as_object() const {
    check_type(Object);

    if (is_null()) return nullptr;
    return std::dynamic_pointer_cast<C>(
        std::get<std::shared_ptr<Object_bridge>>(m_value));
  }

  std::shared_ptr<Object_bridge> as_object() const {
    check_type(Object);

    if (is_null()) return nullptr;
    return std::get<std::shared_ptr<Object_bridge>>(m_value);
  }

  std::shared_ptr<Map_type> as_map() const {
    check_type(Map);

    if (is_null()) return nullptr;
    return std::get<std::shared_ptr<Map_type>>(m_value);
  }

  std::shared_ptr<Array_type> as_array() const {
    check_type(Array);

    if (is_null()) return nullptr;
    return std::get<std::shared_ptr<Array_type>>(m_value);
  }

  std::shared_ptr<Function_base> as_function() const {
    check_type(Function);

    if (is_null()) return nullptr;
    return std::get<std::shared_ptr<Function_base>>(m_value);
  }

  template <class C>
  C to_string_container() const {
    C vec;
    auto arr = as_array();

    if (arr) {
      std::transform(arr->begin(), arr->end(), std::inserter<C>(vec, vec.end()),
                     [](const shcore::Value &v) { return v.get_string(); });
    }

    return vec;
  }

  std::map<std::string, std::string> to_string_map() const {
    check_type(Map);

    std::map<std::string, std::string> map;
    for (const auto &v : *as_map()) {
      map.emplace(v.first, v.second.get_string());
    }
    return map;
  }

  template <class C>
  std::map<std::string, C> to_container_map() const {
    check_type(Map);

    std::map<std::string, C> map;
    for (const auto &v : *as_map()) {
      map.emplace(v.first, v.second.to_string_container<C>());
    }
    return map;
  }

 private:
  std::string yaml(int indent) const;

 private:
  struct null_value {};
  struct binary_string : std::string {};

  std::variant<std::monostate, null_value, bool, std::string, binary_string,
               int64_t, uint64_t, double, std::shared_ptr<Object_bridge>,
               std::shared_ptr<Array_type>, std::shared_ptr<Map_type>,
               std::shared_ptr<Function_base>>
      m_value;
};

typedef Value::Map_type_ref Dictionary_t;
typedef Value::Array_type_ref Array_t;

inline Dictionary_t make_dict() { return std::make_shared<Value::Map_type>(); }

template <typename K, typename V, typename... Arg>
inline Dictionary_t make_dict(K &&key, V &&value, Arg &&...args) {
  Dictionary_t dict = make_dict(std::forward<Arg>(args)...);
  dict->emplace(std::forward<K>(key), std::forward<V>(value));
  return dict;
}

inline Array_t make_array() { return std::make_shared<Value::Array_type>(); }

template <typename... Arg>
inline Array_t make_array(Arg &&...args) {
  auto array = make_array();
  (void)std::initializer_list<int>{
      (array->emplace_back(std::forward<Arg>(args)), 0)...};
  return array;
}

template <template <typename...> class C, typename T>
inline Array_t make_array(const C<T> &container) {
  auto array = make_array();
  for (const auto &item : container) {
    array->emplace_back(item);
  }
  return array;
}

template <template <typename...> class C, typename T>
inline Array_t make_array(C<T> &&container) {
  auto array = make_array();
  for (auto &item : container) {
    array->emplace_back(std::move(item));
  }
  return array;
}

class SHCORE_PUBLIC Exception : public shcore::Error {
  std::shared_ptr<Value::Map_type> _error;

 public:
  Exception(const std::string &message, int code,
            const std::shared_ptr<Value::Map_type> &e);
  Exception(const std::string &message, int code);
  Exception(const std::string &type, const std::string &message, int code);

  virtual ~Exception() noexcept = default;

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
  bool is_mysql() const;
  bool is_mysqlsh() const;
  bool is_parser() const;
  bool is_runtime() const;
  bool is_metadata() const;

  const char *type() const noexcept;

  std::shared_ptr<Value::Map_type> error() const { return _error; }

  std::string format() const override;
};

class SHCORE_PUBLIC Argument_list final {
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
  bool empty() const { return _args.empty(); }

  std::vector<Value>::const_iterator begin() const { return _args.begin(); }
  std::vector<Value>::const_iterator end() const { return _args.end(); }

  bool operator==(const Argument_list &other) const;

  template <class T>
  void emplace_back(const T &value) {
    _args.emplace_back(Value(value));
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
struct value_type_for_native<int> {
  static const Value_type type = Integer;

  static int extract(const Value &value) {
    return static_cast<int>(value.as_int());
  }
};
template <>
struct value_type_for_native<unsigned int> {
  static const Value_type type = UInteger;

  static unsigned int extract(const Value &value) {
    return static_cast<unsigned int>(value.as_uint());
  }
};
template <>
struct value_type_for_native<long int> {
  static const Value_type type = Integer;

  static long int extract(const Value &value) {
    return static_cast<long int>(value.as_int());
  }
};
template <>
struct value_type_for_native<unsigned long int> {
  static const Value_type type = UInteger;

  static unsigned long int extract(const Value &value) {
    return static_cast<unsigned long int>(value.as_uint());
  }
};
template <>
struct value_type_for_native<long long int> {
  static const Value_type type = Integer;

  static long long int extract(const Value &value) {
    return static_cast<long long int>(value.as_int());
  }
};
template <>
struct value_type_for_native<unsigned long long int> {
  static const Value_type type = UInteger;

  static unsigned long long int extract(const Value &value) {
    return static_cast<unsigned long long int>(value.as_uint());
  }
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
    return value.to_string_container<std::vector<std::string>>();
  }
};

template <>
struct value_type_for_native<const std::vector<std::string> &> {
  static const Value_type type = Array;

  static std::vector<std::string> extract(const Value &value) {
    return value.to_string_container<std::vector<std::string>>();
  }
};

template <>
struct value_type_for_native<std::list<std::string>> {
  static const Value_type type = Array;

  static std::list<std::string> extract(const Value &value) {
    return value.to_string_container<std::list<std::string>>();
  }
};

template <>
struct value_type_for_native<std::set<std::string>> {
  static const Value_type type = Array;

  static std::set<std::string> extract(const Value &value) {
    return value.to_string_container<std::set<std::string>>();
  }
};

template <>
struct value_type_for_native<std::unordered_set<std::string>> {
  static const Value_type type = Array;

  static std::unordered_set<std::string> extract(const Value &value) {
    return value.to_string_container<std::unordered_set<std::string>>();
  }
};

template <>
struct value_type_for_native<std::map<std::string, std::string>> {
  static const Value_type type = Map;

  static std::map<std::string, std::string> extract(const Value &value) {
    return value.to_string_map();
  }
};

template <class C>
struct value_type_for_native<std::map<std::string, C>> {
  static const Value_type type = Map;

  static std::map<std::string, C> extract(const Value &value) {
    return value.to_container_map<C>();
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

template <typename T>
struct value_type_for_native<std::optional<T>> {
  static const Value_type type = value_type_for_native<T>::type;

  static T extract(const Value &value) {
    return value_type_for_native<T>::extract(value);
  }
};

template <>
struct value_type_for_native<Function_base_ref> {
  static const Value_type type = Function;
  static shcore::Function_base_ref extract(const Value &value) {
    return value.as_function();
  }
};

template <typename... Types>
struct value_type_for_native<std::variant<Types...>> {
  static const Value_type type = Undefined;

  static std::variant<Types...> extract(const Value &value) {
    std::variant<Types...> result;

    if ((extract<Types>(value, &result) || ...)) {
      return result;
    } else {
      throw std::invalid_argument(
          "is expected to be of type " +
          shcore::str_join(std::vector<std::string>{type_name(
                               value_type_for_native<Types>::type)...},
                           " or ") +
          ", but is " + type_name(value.get_type()));
    }
  }

 private:
  template <typename T>
  static bool extract(const Value &value, std::variant<Types...> *out) {
    try {
      *out = value_type_for_native<T>::extract(value);
      return true;
    } catch (...) {
      return false;
    }
  }
};

// Extract option values from an options dictionary, with validations
// Replaces Argument_map
class Option_unpacker {
 public:
  Option_unpacker() = default;
  explicit Option_unpacker(const Dictionary_t &options);

  virtual ~Option_unpacker() = default;

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

  // Extract optional option with exact type (no conversions)
  template <typename T>
  Option_unpacker &optional_exact(const char *name, T *out_value) {
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

  void end(std::string_view context = "");

  void set_options(const shcore::Dictionary_t &options);

  void set_ignored(std::unordered_set<std::string_view> ignored) {
    m_ignored = std::move(ignored);
  }

  bool is_ignored(std::string_view option) const {
    return m_ignored.count(option) > 0;
  }

  const shcore::Dictionary_t &options() const { return m_options; }

 protected:
  Dictionary_t m_options;
  std::set<std::string> m_unknown;
  std::set<std::string> m_missing;
  std::unordered_set<std::string_view> m_ignored;

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

  void validate(std::string_view context = "");
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
  virtual ~Object_bridge() = default;

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

  virtual bool operator<(const Object_bridge &) const { return false; }

  virtual bool operator<=(const Object_bridge &other) const {
    return (*this < other) || (*this == other);
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

  //! Returns the number of indexable members
  virtual size_t length() const = 0;

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

  virtual std::string &append_descr(std::string *s_out, int indent = -1,
                                    int quote_strings = 0) const;

  virtual std::string &append_repr(std::string *s_out) const;

  virtual void append_json(JSON_dumper *dumper) const;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_H_
