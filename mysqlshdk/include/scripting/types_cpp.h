/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <functional>

#include "scripting/types.h"
#include "scripting/types_common.h"

namespace shcore {
enum NamingStyle {
  LowerCamelCase = 0,
  LowerCaseUnderscores = 1,
  Constants = 2
};

// Helper type traits for automatic method wrapping
template <typename T>
struct Type_info {
  static T to_native(const shcore::Value &in);
};

template <>
struct Type_info<void> {
  static Value_type vtype() { return shcore::Null; }
};

template <>
struct Type_info<int64_t> {
  static int64_t to_native(const shcore::Value &in) { return in.as_int(); }
  static Value_type vtype() { return shcore::Integer; }
  static const char *code() { return "i"; }
  static int64_t default_value() { return 0; }
};

template <>
struct Type_info<uint64_t> {
  static uint64_t to_native(const shcore::Value &in) { return in.as_uint(); }
  static Value_type vtype() { return shcore::UInteger; }
  static const char *code() { return "u"; }
  static uint64_t default_value() { return 0; }
};

template <>
struct Type_info<int> {
  static int to_native(const shcore::Value &in) {
    return static_cast<int>(in.as_int());
  }
  static Value_type vtype() { return shcore::Integer; }
  static const char *code() { return "i"; }
  static int default_value() { return 0; }
};

template <>
struct Type_info<unsigned int> {
  static unsigned int to_native(const shcore::Value &in) {
    return static_cast<unsigned int>(in.as_uint());
  }
  static Value_type vtype() { return shcore::UInteger; }
  static const char *code() { return "u"; }
  static unsigned int default_value() { return 0; }
};

template <>
struct Type_info<double> {
  static double to_native(const shcore::Value &in) { return in.as_double(); }
  static Value_type vtype() { return shcore::Float; }
  static const char *code() { return "f"; }
  static double default_value() { return 0.0; }
};

template <>
struct Type_info<float> {
  static float to_native(const shcore::Value &in) { return in.as_double(); }
  static Value_type vtype() { return shcore::Float; }
  static const char *code() { return "f"; }
  static float default_value() { return 0.0f; }
};

template <>
struct Type_info<bool> {
  static bool to_native(const shcore::Value &in) { return in.as_bool(); }
  static Value_type vtype() { return shcore::Bool; }
  static const char *code() { return "b"; }
  static bool default_value() { return false; }
};

template <>
struct Type_info<std::string> {
  static const std::string &to_native(const shcore::Value &in) {
    return in.as_string();
  }
  static Value_type vtype() { return shcore::String; }
  static const char *code() { return "s"; }
  static std::string default_value() { return std::string(); }
};

template <>
struct Type_info<const std::string &> {
  static const std::string &to_native(const shcore::Value &in) {
    return in.as_string();
  }
  static Value_type vtype() { return shcore::String; }
  static const char *code() { return "s"; }
  static std::string default_value() { return std::string(); }
};

template <>
struct Type_info<const std::vector<std::string> &> {
  static std::vector<std::string> to_native(const shcore::Value &in) {
    std::vector<std::string> strs;
    shcore::Array_t array(in.as_array());
    for (size_t i = 0; i < array->size(); ++i) {
      strs.push_back(array->at(i).as_string());
    }
    return strs;
  }
  static Value_type vtype() { return shcore::Array; }
  static const char *code() { return "A"; }
  static std::vector<std::string> default_value() { return {}; }
};

template <>
struct Type_info<const shcore::Dictionary_t&> {
  static shcore::Dictionary_t to_native(const shcore::Value &in) {
    return in.as_map();
  }
  static Value_type vtype() { return shcore::Map; }
  static const char *code() { return "D"; }
  static shcore::Dictionary_t default_value() { return shcore::Dictionary_t(); }
};

template <>
struct Type_info<shcore::Dictionary_t> {
  static shcore::Dictionary_t to_native(const shcore::Value &in) {
    return in.as_map();
  }
  static Value_type vtype() { return shcore::Map; }
  static const char *code() { return "D"; }
  static shcore::Dictionary_t default_value() { return shcore::Dictionary_t(); }
};

template <>
struct Type_info<const shcore::Array_t&> {
  static shcore::Array_t to_native(const shcore::Value &in) {
    return in.as_array();
  }
  static Value_type vtype() { return shcore::Array; }
  static const char *code() { return "A"; }
  static shcore::Array_t default_value() { return shcore::Array_t(); }
};

template <>
struct Type_info<shcore::Array_t> {
  static shcore::Array_t to_native(const shcore::Value &in) {
    return in.as_array();
  }
  static Value_type vtype() { return shcore::Array; }
  static const char *code() { return "A"; }
  static shcore::Array_t default_value() { return shcore::Array_t(); }
};

template <typename Bridge_class>
struct Type_info<std::shared_ptr<Bridge_class>> {
  static std::shared_ptr<Bridge_class> to_native(const shcore::Value &in) {
    return in.as_object<Bridge_class>();
  }
  static Value_type vtype() { return shcore::Object; }
  static const char *code() { return "O"; }
  static std::shared_ptr<Bridge_class> default_value() {
    return std::shared_ptr<Bridge_class>();
  }
};

class SHCORE_PUBLIC Cpp_property_name {
 public:
  explicit Cpp_property_name(const std::string &name, bool constant = false);
  Cpp_property_name(const Cpp_property_name &other) {
    _name[0] = other._name[0];
    _name[1] = other._name[1];
  }
  std::string name(const NamingStyle &style) const;
  std::string base_name() const;

 private:
  // Each instance holds it's names on the different styles
  std::string _name[2];
};

class SHCORE_PUBLIC Cpp_function : public Function_base {
 public:
  typedef std::function<Value(const shcore::Argument_list &)> Function;

  const std::string &name() const override;
  virtual const std::string &name(const NamingStyle &style) const;

  const std::vector<std::pair<std::string, Value_type>> &signature()
      const override;

  Value_type return_type() const override;

  bool operator==(const Function_base &other) const override;

  Value invoke(const Argument_list &args) override;

  bool is_legacy = false;
  // TODO(alfredo) delme
  bool has_var_args() override { return _meta->var_args; }

  static std::shared_ptr<Function_base> create(
      const std::string &name, const Function &func,
      const std::vector<std::pair<std::string, Value_type>> &signature);

 protected:
  friend class Cpp_object_bridge;

  enum Param_flag { Mandatory, Optional };
  typedef std::vector<std::pair<Value_type, Param_flag>> Raw_signature;
  static Raw_signature gen_signature(
      const std::vector<std::pair<std::string, Value_type>> &param_types);
  static std::pair<bool, int> match_signatures(
      const Raw_signature &cand, const std::vector<Value_type> &wanted);

  struct Metadata {
    std::string name[2];
    Raw_signature signature;

    std::vector<std::pair<std::string, Value_type>> param_types;
    Value_type return_type;
    bool var_args;  // delme

    void set(const std::string &name, Value_type rtype,
             const std::vector<std::pair<std::string, Value_type>> &ptypes);
  };

  Cpp_function(const std::string &name, const Function &func,
               bool var_args);  // delme
  Cpp_function(const std::string &name, const Function &func,
               const std::vector<std::pair<std::string, Value_type>>
                   &signature);  // delme
  Cpp_function(const Metadata *meta, const Function &func);

  const Raw_signature &function_signature() const { return _meta->signature; }

 private:
  // Each instance holds it's names on the different styles
  Function _func;

  const Metadata *_meta;
  Metadata _meta_tmp;  // temporary memory for legacy versions of Cpp_function
};


namespace internal {
template<typename R>
struct Result_wrapper {
  template<typename F>
  static inline shcore::Value call(F f) {
    return shcore::Value(f());
  }
};

template<>
struct Result_wrapper<void> {
  template<typename F>
  static inline shcore::Value call(F f) {
    f();
    return shcore::Value();
  }
};
}  // namespace internal

class SHCORE_PUBLIC Cpp_object_bridge : public Object_bridge {
 public:
  struct ScopedStyle {
   public:
    ScopedStyle(const Cpp_object_bridge *target, NamingStyle style)
        : _target(target) {
      _old_style = _target->naming_style;
      _target->naming_style = style;
    }
    ~ScopedStyle() { _target->naming_style = _old_style; }

   private:
     NamingStyle _old_style;
     const Cpp_object_bridge *_target;
  };

 protected:
  Cpp_object_bridge();
  Cpp_object_bridge(const Cpp_object_bridge &) = delete;

 public:
  virtual ~Cpp_object_bridge();

  virtual bool operator==(const Object_bridge &other) const {
    return class_name() == other.class_name() && this == &other;
  }

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
  shcore::Value get_member_method(const shcore::Argument_list &args,
                                  const std::string &method,
                                  const std::string &prop);

  // These advanced functions verify the requested property/function to see if
  // it is valid on the active naming style first, if so, then the normal
  // functions (above) are called with the base property/function name
  virtual std::vector<std::string> get_members_advanced(
      const NamingStyle &style);
  virtual Value get_member_advanced(const std::string &prop,
                                    const NamingStyle &style) const;
  virtual bool has_member_advanced(const std::string &prop,
                                   const NamingStyle &style) const;
  virtual void set_member_advanced(const std::string &prop, Value value,
                                   const NamingStyle &style);
  virtual bool has_method_advanced(const std::string &name,
                                   const NamingStyle &style) const;
  virtual Value call_advanced(const std::string &name,
                              const Argument_list &args,
                              const NamingStyle &style);

  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  void set_naming_style(const NamingStyle &style);
  std::shared_ptr<ScopedStyle> set_scoped_naming_style(
      const NamingStyle &style);

  virtual shcore::Value help(const shcore::Argument_list &args);

 protected:
  /** Expose a method with 1 argument with automatic bridging.

  For use with methods with 1 argument, using shcore::Value compatible
  arguments and return type. String arguments must be passed by const ref.

  To mark a parameter as optional, add ? at the beginning of the name string.
  Optional parameters must be marked from right to left, with no skips.

  Runtime type checking and conversions done automatically.

  @param name - the name of the method, with description separated by space.
    e.g. "expr filter expression for query"
  @param func - function pointer to the method
  @param a1doc - name of the 1st argument (must not be empty!)
  @param a1def - default value to be used if parameter is optional and not given
  */
  template <typename R, typename A1, typename C>
  void expose(const std::string &name, R (C::*func)(A1),
              const std::string &a1doc,
              const typename std::remove_const<A1>::type &a1def =
                  Type_info<A1>::default_value()) {
    assert(func != nullptr);
    assert(!name.empty());
    assert(!a1doc.empty());

    Cpp_function::Metadata &md =
        get_metadata(class_name() + "::" + name + ":" + Type_info<A1>::code());
    if (md.name[0].empty()) {
      set_metadata(md, name, Type_info<R>::vtype(),
                   {{a1doc, Type_info<A1>::vtype()}});
    }

    _funcs.emplace(std::make_pair(
        name.substr(0, name.find("|")),
        std::shared_ptr<Cpp_function>(new Cpp_function(
            &md,
            [this, func,
             a1def](const shcore::Argument_list &args) -> shcore::Value {
              const A1 &&a1 = args.size() == 0
                                  ? a1def
                                  : Type_info<A1>::to_native(args.at(0));
              return internal::Result_wrapper<R>::call([this, func, a1]() {
                return (static_cast<C *>(this)->*func)(a1);
              });
            }))));
  }

  /** Expose method with no arguments, with automatic bridging.
      See above for details.
  */
  template <typename R, typename C>
  void expose(const std::string &name, R (C::*func)()) {
    assert(func != nullptr);
    assert(!name.empty());

    Cpp_function::Metadata &md = get_metadata(class_name() + "::" + name + ":");
    if (md.name[0].empty()) {
      set_metadata(md, name, Type_info<R>::vtype(), {});
    }

    _funcs.emplace(std::make_pair(
        name.substr(0, name.find("|")),
        std::shared_ptr<Cpp_function>(new Cpp_function(
            &md, [this, func](const shcore::Argument_list &) -> shcore::Value {
              return internal::Result_wrapper<R>::call([this, func]() {
                return (static_cast<C *>(this)->*func)();
              });
            }))));
  }

  /** Expose method with 2 arguments, with automatic bridging.
      See above for details.
  */
  template <typename R, typename A1, typename A2, typename C>
  void expose(const std::string &name, R (C::*func)(A1, A2),
              const std::string &a1doc, const std::string &a2doc,
              const typename std::remove_const<A2>::type &a2def =
                  Type_info<A2>::default_value(),
              const typename std::remove_const<A1>::type &a1def =
                  Type_info<A1>::default_value()) {
    assert(func != nullptr);
    assert(!name.empty());
    assert(!a1doc.empty());
    assert(!a2doc.empty());

    Cpp_function::Metadata &md =
        get_metadata(class_name() + "::" + name + ":" + Type_info<A1>::code() +
                     Type_info<A2>::code());
    if (md.name[0].empty()) {
      set_metadata(
          md, name, Type_info<R>::vtype(),
          {{a1doc, Type_info<A1>::vtype()}, {a2doc, Type_info<A2>::vtype()}});
    }

    _funcs.emplace(std::make_pair(
        name.substr(0, name.find("|")),
        std::shared_ptr<Cpp_function>(new Cpp_function(
            &md,
            [this, func, a1def,
             a2def](const shcore::Argument_list &args) -> shcore::Value {
              const A1 &&a1 = args.size() == 0
                                  ? a1def
                                  : Type_info<A1>::to_native(args.at(0));
              const A2 &&a2 = args.size() <= 1
                                  ? a2def
                                  : Type_info<A2>::to_native(args.at(1));
              return internal::Result_wrapper<R>::call([this, func, a1, a2]() {
                return (static_cast<C *>(this)->*func)(a1, a2);
              });
            }))));
  }

  /** Expose method with 3 arguments, with automatic bridging.
      See above for details.
  */
  template <typename R, typename A1, typename A2, typename A3, typename C>
  void expose(const std::string &name, R (C::*func)(A1, A2, A3),
              const std::string &a1doc, const std::string &a2doc,
              const std::string &a3doc,
              const typename std::remove_const<A3>::type &a3def =
                  Type_info<A3>::default_value(),
              const typename std::remove_const<A2>::type &a2def =
                  Type_info<A2>::default_value(),
              const typename std::remove_const<A1>::type &a1def =
                  Type_info<A1>::default_value()) {
    assert(func != nullptr);
    assert(!name.empty());
    assert(!a1doc.empty());
    assert(!a2doc.empty());
    assert(!a3doc.empty());

    Cpp_function::Metadata &md =
        get_metadata(class_name() + "::" + name + ":" + Type_info<A2>::code() +
                     Type_info<A2>::code() + Type_info<A3>::code());
    if (md.name[0].empty()) {
      set_metadata(md, name, Type_info<R>::vtype(),
                   {{a1doc, Type_info<A1>::vtype()},
                    {a2doc, Type_info<A2>::vtype()},
                    {a3doc, Type_info<A3>::vtype()}});
    }

    _funcs.emplace(std::make_pair(
        name.substr(0, name.find("|")),
        std::shared_ptr<Cpp_function>(new Cpp_function(
            &md,
            [this, func, a1def, a2def,
             a3def](const shcore::Argument_list &args) -> shcore::Value {
               const A1 &&a1 = args.size() == 0
                                   ? a1def
                                   : Type_info<A1>::to_native(args.at(0));
               const A2 &&a2 = args.size() <= 1
                                   ? a2def
                                   : Type_info<A2>::to_native(args.at(1));
               const A3 &&a3 = args.size() <= 2
                                   ? a3def
                                   : Type_info<A3>::to_native(args.at(2));
               return internal::Result_wrapper<R>::call(
                   [this, func, a1, a2, a3]() {
                     return (static_cast<C *>(this)->*func)(a1, a2, a3);
                   });
            }))));
  }

  /** Expose method with 4 arguments, with automatic bridging.
      See above for details.
  */
  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename C>
  void expose(const std::string &name, R (C::*func)(A1, A2, A3, A4),
              const std::string &a1doc, const std::string &a2doc,
              const std::string &a3doc, const std::string &a4doc,
              const typename std::remove_const<A4>::type &a4def =
                  Type_info<A4>::default_value(),
              const typename std::remove_const<A3>::type &a3def =
                  Type_info<A3>::default_value(),
              const typename std::remove_const<A2>::type &a2def =
                  Type_info<A2>::default_value(),
              const typename std::remove_const<A1>::type &a1def =
                  Type_info<A1>::default_value()) {
    assert(func != nullptr);
    assert(!name.empty());
    assert(!a1doc.empty());
    assert(!a2doc.empty());
    assert(!a3doc.empty());
    assert(!a4doc.empty());

    Cpp_function::Metadata &md = get_metadata(
        class_name() + "::" + name + ":" + Type_info<A2>::code() +
        Type_info<A2>::code() + Type_info<A3>::code() + Type_info<A4>::code());
    if (md.name[0].empty()) {
      set_metadata(md, name, Type_info<R>::vtype(),
                   {{a1doc, Type_info<A1>::vtype()},
                    {a2doc, Type_info<A2>::vtype()},
                    {a3doc, Type_info<A3>::vtype()},
                    {a4doc, Type_info<A4>::vtype()}});
    }

    _funcs.emplace(std::make_pair(
        name.substr(0, name.find("|")),
        std::shared_ptr<Cpp_function>(new Cpp_function(
            &md,
            [this, func, a1def, a2def, a3def,
             a4def](const shcore::Argument_list &args) -> shcore::Value {
               const A1 &&a1 = args.size() == 0
                                   ? a1def
                                   : Type_info<A1>::to_native(args.at(0));
               const A2 &&a2 = args.size() <= 1
                                   ? a2def
                                   : Type_info<A2>::to_native(args.at(1));
               const A3 &&a3 = args.size() <= 2
                                   ? a3def
                                   : Type_info<A3>::to_native(args.at(2));
               const A4 &&a4 = args.size() <= 3
                                   ? a4def
                                   : Type_info<A4>::to_native(args.at(3));
               return internal::Result_wrapper<R>::call(
                   [this, func, a1, a2, a3, a4]() {
                     return (static_cast<C *>(this)->*func)(a1, a2, a3, a4);
                   });
            }))));
  }

 protected:
  // delme
  void add_method_(
      const std::string &name, Cpp_function::Function func,
      std::vector<std::pair<std::string, Value_type>> *signature);

  inline void add_method(const std::string &name, Cpp_function::Function func) {
    std::vector<std::pair<std::string, Value_type>> signature;
    add_method_(name, func, &signature);
  }

  inline void add_method(const std::string &name, Cpp_function::Function func,
                         const char *arg1_name,
                         Value_type arg1_type = shcore::Undefined) {
    assert(arg1_name);
    std::vector<std::pair<std::string, Value_type>> signature;
    signature.push_back({arg1_name, arg1_type});
    add_method_(name, func, &signature);
  }

  inline void add_method(const std::string &name, Cpp_function::Function func,
                         const char *arg1_name,
                         Value_type arg1_type,
                         const char *arg2_name,
                         Value_type arg2_type = shcore::Undefined) {
    assert(arg1_name);
    assert(arg2_name);
    std::vector<std::pair<std::string, Value_type>> signature;
    signature.push_back({arg1_name, arg1_type});
    signature.push_back({arg2_name, arg2_type});
    add_method_(name, func, &signature);
  }

  // delme - replace varargs with type overloading
  virtual void add_varargs_method(const std::string &name,
                                  Cpp_function::Function func);

  // Constants and properties are not handled through the Cpp_property_name
  // class which supports different naming styles
  virtual void add_constant(const std::string &name);
  virtual void add_property(const std::string &name,
                            const std::string &getter = "");
  virtual void delete_property(const std::string &name,
                               const std::string &getter = "");

  // Helper function that retrieves a qualified function name using the active
  // naming style Used mostly for errors in function validations
  std::string get_function_name(const std::string &member,
                                bool fully_specified = true) const;

  std::vector<Cpp_property_name> _properties;

  // The global active naming style
  mutable NamingStyle naming_style;

 protected:
  // Returns named function which signature that matches the given argument list
  std::shared_ptr<Cpp_function> lookup_function_overload(
      const std::string &method, const NamingStyle &style,
      const shcore::Argument_list &args) const;
  std::shared_ptr<Cpp_function> lookup_function(const std::string &method,
                                                const NamingStyle &style) const;
  std::shared_ptr<Cpp_function> lookup_function(
      const std::string &method) const;

 private:
  std::multimap<std::string, std::shared_ptr<Cpp_function>> _funcs;

  // Returns the base name of the given member
  std::string get_base_name(const std::string &member) const;

  static std::map<std::string, Cpp_function::Metadata> mdtable;
  static void clear_metadata();
  static Cpp_function::Metadata &get_metadata(const std::string &method);
  static void set_metadata(
      Cpp_function::Metadata &meta, const std::string &name, Value_type rtype,
      const std::vector<std::pair<std::string, Value_type>> &ptypes);

#ifdef FRIEND_TEST
  friend class Types_cpp;
#endif
};
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_
