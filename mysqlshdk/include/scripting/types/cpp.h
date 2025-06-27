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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/type_info/validators.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types/function_metadata.h"
#include "mysqlshdk/include/scripting/types/parameter.h"
#include "mysqlshdk/include/scripting/types/validator.h"

namespace shcore {

class SHCORE_PUBLIC Cpp_property_name {
 public:
  explicit Cpp_property_name(std::string_view name, bool constant = false);
  const std::string &name(NamingStyle style) const;
  const std::string &base_name() const;

 private:
  // Each instance holds it's names on the different styles
  std::array<std::string, 2> _name;
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

  static std::shared_ptr<Function_base> create(
      const std::string &name, const Function &func,
      const std::vector<std::pair<std::string, Value_type>> &signature);

 protected:
  friend class Cpp_object_bridge;

  static std::tuple<bool, int, shcore::Exception> match_signatures(
      const Raw_signature &cand, const std::vector<Value_type> &wanted,
      const shcore::Dictionary_t &kwds);

  Cpp_function(const std::string &name, const Function &func,
               const std::vector<std::pair<std::string, Value_type>>
                   &signature);  // delme
  Cpp_function(const Function_metadata *meta, const Function &func);

  const Raw_signature &function_signature() const { return _meta->signature; }

  const Function_metadata *get_metadata() const { return _meta; }

 private:
  // Each instance holds it's names on the different styles
  Function _func;

  const Function_metadata *_meta;
  Function_metadata
      _meta_tmp;  // temporary memory for legacy versions of Cpp_function
};

class SHCORE_PUBLIC Cpp_object_bridge : public Object_bridge {
 protected:
  Cpp_object_bridge();
  Cpp_object_bridge(const Cpp_object_bridge &) = delete;

 public:
  ~Cpp_object_bridge() override = default;

  bool operator==(const Object_bridge &other) const override {
    return this == &other;
  }

  // required to expose() help method
  std::string class_name() const override { return "Cpp_object_bridge"; }

  std::vector<std::string> get_members() const override;
  Value get_member(const std::string &prop) const override;
  std::vector<std::string> get_cli_members() const;

  bool has_member(const std::string &prop) const override;
  void set_member(const std::string &prop, Value value) override;

  bool is_indexed() const override;
  Value get_member(size_t index) const override;
  void set_member(size_t index, Value value) override;
  size_t length() const override;

  bool has_method(const std::string &name) const override;

  /**
   * Returns the metadata associated to a function with the given name
   * considering whether it is enabled for CLI or not.
   *
   * @param name: the name of the function for which the metadata will be
   * returned.
   * @param cli_enabled: flag to indicate whether only functions with CLI
   * enabled should be considered.
   *
   * If cli_enabled is FALSE, then the metadata of the first function matching
   * the name will be returned.
   *
   * If cli_enabled is TRUE, only functions enabled for CLI will be considered
   * in the search.
   */
  const Function_metadata *get_function_metadata(const std::string &name,
                                                 bool cli_enabled = false);

  Value call(const std::string &name, const Argument_list &args) override;

  // Helper method to retrieve properties using a method
  shcore::Value get_member_method(const shcore::Argument_list &args,
                                  const std::string &method,
                                  const std::string &prop);

  // These advanced functions verify the requested property/function to see if
  // it is valid on the active naming style first, if so, then the normal
  // functions (above) are called with the base property/function name
  virtual Value get_member_advanced(const std::string &prop) const;
  virtual bool has_member_advanced(const std::string &prop) const;
  virtual void set_member_advanced(const std::string &prop, Value value);
  virtual bool has_method_advanced(const std::string &name) const;
  virtual Value call_advanced(const std::string &name,
                              const Argument_list &args,
                              const shcore::Dictionary_t &kwargs = {});

  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  std::string &append_repr(std::string &s_out) const override;

  virtual std::string help(const std::string &item = {});
  virtual std::string get_help_id() const { return class_name(); }

 protected:
  void detect_overload_conflicts(const std::string &name,
                                 const Function_metadata &md);

  /**
   * exposes a function defined either in JavaScript or Python
   * @returns a reference to the function metadata so the caller can set
   * the help details and register them in the help system.
   */
  Function_metadata *expose(const std::string &name,
                            const shcore::Function_base_ref &func,
                            const Raw_signature &parameters);

  /**
   * Expose method with no arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)()) {
    return expose_(name, func, {});
  }

  template <typename R, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)() const) {
    return expose_(name, func, {});
  }

  template <typename R>
  Function_metadata *expose(const std::string &name, R (*func)()) {
    return expose_(name, func, {});
  }

  template <typename R>
  Function_metadata *expose(const std::string &name,
                            std::function<R()> &&func) {
    return expose_(name, std::move(func), {});
  }

  /**
   * Expose method with 1 argument, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)(A1),
                            const std::string &a1doc,
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc}, std::move(a1def));
  }

  template <typename R, typename A1, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)(A1) const,
                            const std::string &a1doc,
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc}, std::move(a1def));
  }

  template <typename R, typename A1>
  Function_metadata *expose(const std::string &name, R (*func)(A1),
                            const std::string &a1doc,
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc}, std::move(a1def));
  }

  template <typename R, typename A1>
  Function_metadata *expose(const std::string &name,
                            std::function<R(A1)> &&func,
                            const std::string &a1doc,
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func), {a1doc}, std::move(a1def));
  }

  /**
   * Expose method with 2 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)(A1, A2),
                            const std::string &a1doc, const std::string &a2doc,
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  template <typename R, typename A1, typename A2, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)(A1, A2) const,
                            const std::string &a1doc, const std::string &a2doc,
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  template <typename R, typename A1, typename A2>
  Function_metadata *expose(const std::string &name, R (*func)(A1, A2),
                            const std::string &a1doc, const std::string &a2doc,
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  template <typename R, typename A1, typename A2>
  Function_metadata *expose(const std::string &name,
                            std::function<R(A1, A2)> &&func,
                            const std::string &a1doc, const std::string &a2doc,
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func), {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  /**
   * Expose method with 3 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename C>
  Function_metadata *expose(const std::string &name, R (C::*func)(A1, A2, A3),
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc,
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename C>
  Function_metadata *expose(const std::string &name,
                            R (C::*func)(A1, A2, A3) const,
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc,
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def));
  }

  template <typename R, typename A1, typename A2, typename A3>
  Function_metadata *expose(const std::string &name, R (*func)(A1, A2, A3),
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc,
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def));
  }

  template <typename R, typename A1, typename A2, typename A3>
  Function_metadata *expose(const std::string &name,
                            std::function<R(A1, A2, A3)> &&func,
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc,
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func), {a1doc, a2doc, a3doc},
                   std::move(a1def), std::move(a2def), std::move(a3def));
  }

  /**
   * Expose method with 4 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename C>
  Function_metadata *expose(const std::string &name,
                            R (C::*func)(A1, A2, A3, A4),
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc, const std::string &a4doc,
                            Type_info_t<A4> &&a4def = {},
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename C>
  Function_metadata *expose(const std::string &name,
                            R (C::*func)(A1, A2, A3, A4) const,
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc, const std::string &a4doc,
                            Type_info_t<A4> &&a4def = {},
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4>
  Function_metadata *expose(const std::string &name, R (*func)(A1, A2, A3, A4),
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc, const std::string &a4doc,
                            Type_info_t<A4> &&a4def = {},
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4>
  Function_metadata *expose(const std::string &name,
                            std::function<R(A1, A2, A3, A4)> &&func,
                            const std::string &a1doc, const std::string &a2doc,
                            const std::string &a3doc, const std::string &a4doc,
                            Type_info_t<A4> &&a4def = {},
                            Type_info_t<A3> &&a3def = {},
                            Type_info_t<A2> &&a2def = {},
                            Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func), {a1doc, a2doc, a3doc, a4doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def));
  }

  /**
   * Expose method with 5 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename C>
  Function_metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename C>
  Function_metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5>
  Function_metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4, A5),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5>
  Function_metadata *expose(
      const std::string &name, std::function<R(A1, A2, A3, A4, A5)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func), {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  /**
   * Expose method with 6 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename C>
  Function_metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename C>
  Function_metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6>
  Function_metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4, A5, A6),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6>
  Function_metadata *expose(
      const std::string &name, std::function<R(A1, A2, A3, A4, A5, A6)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func),
                   {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def),
                   std::move(a5def), std::move(a6def));
  }

  /**
   * Expose method with 7 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7, typename C>
  Function_metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6, A7),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc, Type_info_t<A7> &&a7def = {},
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(
        name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
        std::move(a1def), std::move(a2def), std::move(a3def), std::move(a4def),
        std::move(a5def), std::move(a6def), std::move(a7def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7, typename C>
  Function_metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6, A7) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc, Type_info_t<A7> &&a7def = {},
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(
        name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
        std::move(a1def), std::move(a2def), std::move(a3def), std::move(a4def),
        std::move(a5def), std::move(a6def), std::move(a7def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7>
  Function_metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4, A5, A6, A7),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc, Type_info_t<A7> &&a7def = {},
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(
        name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
        std::move(a1def), std::move(a2def), std::move(a3def), std::move(a4def),
        std::move(a5def), std::move(a6def), std::move(a7def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7>
  Function_metadata *expose(
      const std::string &name,
      std::function<R(A1, A2, A3, A4, A5, A6, A7)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc, Type_info_t<A7> &&a7def = {},
      Type_info_t<A6> &&a6def = {}, Type_info_t<A5> &&a5def = {},
      Type_info_t<A4> &&a4def = {}, Type_info_t<A3> &&a3def = {},
      Type_info_t<A2> &&a2def = {}, Type_info_t<A1> &&a1def = {}) {
    return expose_(name, std::move(func),
                   {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def),
                   std::move(a7def));
  }

  // adding overloads for more arguments requires changes in
  // Cpp_function::match_signatures()

  // Lambdas cannot be used directly in the expose() methods, as compiler
  // is unable to deduce the correct type (no conversions are done during
  // deduction). This method allows to automatically wrap any lambda into
  // corresponding std::function.
  template <typename L>
  static auto wrap(L &&l) {
    return to_function_t<decltype(&L::operator())>(std::forward<L>(l));
  }

  // delme
  void add_method_(const std::string &name, Cpp_function::Function func,
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
                         const char *arg1_name, Value_type arg1_type,
                         const char *arg2_name,
                         Value_type arg2_type = shcore::Undefined) {
    assert(arg1_name);
    assert(arg2_name);
    std::vector<std::pair<std::string, Value_type>> signature;
    signature.push_back({arg1_name, arg1_type});
    signature.push_back({arg2_name, arg2_type});
    add_method_(name, func, &signature);
  }

  // Constants and properties are not handled through the Cpp_property_name
  // class which supports different naming styles
  virtual void add_constant(const std::string &name);
  virtual void add_property(const std::string &name,
                            const std::string &getter = "");
  virtual void delete_property(const std::string &name,
                               const std::string &getter = "");

  // Helper function that retrieves a qualified function name using the active
  // naming style Used mostly for errors in function validations
  std::string get_function_name(std::string_view member,
                                bool fully_specified = true) const;

  std::vector<Cpp_property_name> _properties;

  // Returns named function which signature that matches the given argument list
  std::shared_ptr<Cpp_function> lookup_function_overload(
      std::string_view method, const shcore::Argument_list &args,
      const shcore::Dictionary_t &kwds = {}) const;

  std::shared_ptr<Cpp_function> lookup_function(std::string_view method) const;

 private:
  template <typename T>
  struct to_function {};

  template <typename R, typename C, typename... A>
  struct to_function<R (C::*)(A...) const> {
    using type = std::function<R(A...)>;
  };

  template <typename T>
  using to_function_t = typename to_function<T>::type;

  std::multimap<std::string, std::shared_ptr<Cpp_function>> _funcs;

  // Returns the base name of the given member
  std::string get_base_name(const std::string &member) const;

  static std::mutex s_mtx;
  static std::map<std::string, Function_metadata> mdtable;
  static Function_metadata &get_metadata(const std::string &method);
  static void set_metadata(
      Function_metadata &meta, const std::string &name, Value_type rtype,
      const std::vector<std::pair<std::string, Value_type>> &ptypes,
      const std::string &pcodes);

  Value call_function(const std::string &scope,
                      const std::shared_ptr<Cpp_function> &func,
                      const Argument_list &args);

  // Helper methods which handle pointers to static, const and non-const
  // methods, functions, and lambdas.
  // These allow to call expose__() method without specifying the
  // template arguments.
  template <typename R, typename C, typename... A>
  Function_metadata *expose_(const std::string &name, R (C::*func)(A...),
                             const std::vector<std::string> &docs,
                             Type_info_t<A> &&...defs) {
    return expose__<R, std::function<R(A...)>, A...>(
        name,
        [this, func](A &&...args) -> R {
          return (static_cast<C *>(this)->*func)(std::forward<A>(args)...);
        },
        docs, std::move(defs)...);
  }

  template <typename R, typename C, typename... A>
  Function_metadata *expose_(const std::string &name, R (C::*func)(A...) const,
                             const std::vector<std::string> &docs,
                             Type_info_t<A> &&...defs) {
    return expose__<R, std::function<R(A...)>, A...>(
        name,
        [this, func](A &&...args) -> R {
          return (static_cast<C *>(this)->*func)(std::forward<A>(args)...);
        },
        docs, std::move(defs)...);
  }

  template <typename R, typename... A>
  Function_metadata *expose_(const std::string &name, R (*func)(A...),
                             const std::vector<std::string> &docs,
                             Type_info_t<A> &&...defs) {
    return expose__<R, std::function<R(A...)>, A...>(name, func, docs,
                                                     std::move(defs)...);
  }

  template <typename R, typename... A>
  Function_metadata *expose_(const std::string &name,
                             std::function<R(A...)> &&func,
                             const std::vector<std::string> &docs,
                             Type_info_t<A> &&...defs) {
    return expose__<R, std::function<R(A...)>, A...>(name, std::move(func),
                                                     docs, std::move(defs)...);
  }

  /**
   * Expose a method with variable number of arguments with automatic
   * bridging.
   *
   * To mark a parameter as optional, add ? at the beginning of the name
   * string. Optional parameters must be marked from right to left, with
   * no skips.
   *
   * Run-time type checking and conversions are done automatically.
   *
   * @param name The name of the method, with description separated by
   * space. e.g. "expr filter expression for query"
   * @param func The function to call.
   * @param docs Names of the arguments (a name must not be empty!).
   * @param defs The default values to be used if a parameter is
   * optional and not given. The number of docs and defs must be the
   * same. The order of docs and defs must be the same.
   *
   * @return A pointer to the function metadata so the default parameter
   *         validator can be replaced with a more complex one.
   */
  template <typename R, typename F, typename... A>
  Function_metadata *expose__(const std::string &name, F &&func,
                              const std::vector<std::string> &docs,
                              Type_info_t<A> &&...defs) {
    assert(func != nullptr);
    assert(!name.empty());
    assert(docs.size() == sizeof...(A));
#ifndef NDEBUG
    for (const auto &d : docs) {
      assert(!d.empty());
    }
#endif  // NDEBUG

    const auto size = docs.size();

    std::string pcodes;
    (..., pcodes.append(1, Type_info<A>::code));
    auto mangled_name = class_name() + "::" + name + ":";
    mangled_name.append(pcodes);
    auto &md = get_metadata(mangled_name);
    std::lock_guard<std::mutex> lock(s_mtx);
    if (md.name[0].empty()) {
      std::vector<std::pair<std::string, Value_type>> ptypes;
      std::vector<Value_type> vtypes = {Type_info<A>::vtype...};

      for (size_t i = 0; i < size; ++i) {
        ptypes.emplace_back(docs[i], vtypes[i]);
      }

      set_metadata(md, name, Type_info<R>::vtype, ptypes, pcodes);
    }

    {
      std::vector<std::unique_ptr<Parameter_validator>> validators;
      (void)std::initializer_list<int>{
          (validators.emplace_back(Type_info<A>::validator()), 0)...};

      for (size_t i = 0; i < size; ++i) {
        if (validators[i]) {
          md.signature[i]->set_validator(std::move(validators[i]));
        }
      }
    }

    const auto registered_name = name.substr(0, name.find("|"));
    detect_overload_conflicts(registered_name, md);

    _funcs.emplace(
        registered_name,
        std::shared_ptr<Cpp_function>(
            new Cpp_function(&md, [&md, func = std::forward<F>(func),
                                   defs = std::make_tuple(std::move(defs)...)](
                                      const shcore::Argument_list &args) {
              // Executes parameter validators
              for (size_t index = 0, count = args.size(); index < count;
                   ++index) {
                Parameter_context context{
                    "", {{"Argument", static_cast<int>(index + 1)}}};
                md.signature[index]->validate(args[index], &context);
              }

              return detail::Result_wrapper<R>::call([&func, &args, &defs]() {
                return call<R, F, A...>(func, args, defs,
                                        std::index_sequence_for<A...>{});
              });
            })));

    return &md;
  }

  // helper method which allows to bind position in template parameter
  // pack with parameter pack expansion
  template <typename R, typename F, typename... A, size_t... I>
  static R call(const F &func, const shcore::Argument_list &args,
                const std::tuple<Type_info_t<A>...> &defs,
                std::index_sequence<I...>) {
    const auto size = args.size();
    return func((size <= I || args.at(I).get_type() == Value_type::Undefined
                     ? get_copy<I>(defs)
                     : Arg_handler<A>::get(I, args))...);
  }

  template <size_t I>
  static auto get_copy(const auto &defs) {
    return std::get<I>(defs);
  }

#ifdef FRIEND_TEST
  friend class Types_cpp;
#endif
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_
