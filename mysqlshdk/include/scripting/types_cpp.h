/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_common.h"

namespace shcore {

class SHCORE_PUBLIC Cpp_property_name {
 public:
  explicit Cpp_property_name(const std::string &name, bool constant = false);
  std::string name(const NamingStyle &style) const;
  std::string base_name() const;

 private:
  // Each instance holds it's names on the different styles
  std::string _name[2];
};

/**
 * The parameter classes below, can be used to define both
 * parameters and options.
 *
 * This helper class will be used on the proper generation of
 * error messages both on parameter definition and validation.
 */
struct Parameter_context {
  std::string title;

  struct Context_definition {
    std::string name;
    mysqlshdk::utils::nullable<int> position;
  };

  std::vector<Context_definition> levels;

  std::string str() const;
};

struct Parameter;
struct Parameter_validator {
 public:
  virtual ~Parameter_validator() = default;
  virtual bool valid_type(const Parameter &param, Value_type type) const;
  virtual void validate(const Parameter &param, const Value &data,
                        Parameter_context *context) const;
  void set_enabled(bool enabled) { m_enabled = enabled; }

 protected:
  // Parameter validators were initially created to validate data provided on
  // functions in EXTENSION OBJECTS: when a function is an API extension object
  // is called the parameter validators are executed, this way we ensure the
  // same validation scheme is used in all extension objects.
  //
  // The parameter validators contain metadata that is useful in CLI integration
  // so we are overloading its use, not only for extension objects but also for
  // any API exposed from C++. This is: we require the metadata for proper CLI
  // integration but we do not need the validators to be executed, for this
  // reason this flag is required.
  bool m_enabled = true;
};

/**
 * Holds the element types to be allowed, Undefined for ANY
 */
struct List_validator : public Parameter_validator {
 public:
  void set_element_type(shcore::Value_type type) { m_element_type = type; }
  shcore::Value_type element_type() const { return m_element_type; }

 protected:
  shcore::Value_type m_element_type = shcore::Undefined;
};

/**
 * This is used as a base class for validators that enable defining specific set
 * of allowed data:
 *
 * - String validator could define set of allowed values.
 * - Option validator could define set of allowed options.
 * - List validator could define set of allowed items.
 *
 * Also enables referring to the allowed list either internally or externally.
 */
template <typename T>
struct Parameter_validator_with_allowed : public Parameter_validator {
 public:
  Parameter_validator_with_allowed() : m_allowed_ref(&m_allowed) {}
  void set_allowed(std::vector<T> &&allowed) {
    m_allowed = std::move(allowed);
    set_allowed(&m_allowed);
  }

  void set_allowed(const std::vector<T> *allowed) { m_allowed_ref = allowed; }

  const std::vector<T> &allowed() const { return *m_allowed_ref; }

 protected:
  std::vector<T> m_allowed;
  const std::vector<T> *m_allowed_ref;
};

struct Object_validator : public Parameter_validator_with_allowed<std::string> {
  void validate(const Parameter &param, const Value &data,
                Parameter_context *context) const override;
};

struct String_validator : public Parameter_validator_with_allowed<std::string> {
  void validate(const Parameter &param, const Value &data,
                Parameter_context *context) const override;
};

struct Option_validator
    : public Parameter_validator_with_allowed<std::shared_ptr<Parameter>> {
  void validate(const Parameter &param, const Value &data,
                Parameter_context *context) const override;
};

enum class Param_flag { Mandatory, Optional };

struct Parameter final {
  Parameter() = default;

  Parameter(const std::string &n, Value_type t, Param_flag f,
            const std::string &sname = "", bool cmd_line = true)
      : name(n), flag(f), short_name(sname), cmd_line_enabled(cmd_line) {
    set_type(t);
  }

  std::string name;
  Param_flag flag;
  Value def_value;

  // Supports defining a shortname, i.e. for options in CMD line args
  std::string short_name;

  // Supports disabling a specific option for the CLI integration.
  // Scenario: same option being valid in 2 different parameters even both of
  // them are used for the same thing, it makes no sense to allow both in CLI so
  // we can disable one to avoid unnecessary inconsistency.
  // i.e. Sandbox Operations support password option both in instance and
  // options param.
  bool cmd_line_enabled;

  void validate(const Value &data, Parameter_context *context) const;

  bool valid_type(Value_type type) const;

  void set_type(Value_type type) {
    m_type = type;

    switch (m_type) {
      case Value_type::Object:
        set_validator(std::make_unique<Object_validator>());
        break;

      case Value_type::String:
        set_validator(std::make_unique<String_validator>());
        break;

      case Value_type::Map:
        set_validator(std::make_unique<Option_validator>());
        break;

      case Value_type::Array:
        set_validator(std::make_unique<List_validator>());
        break;
      default:
        // no validator in the default case
        break;
    }
  }

  Value_type type() const { return m_type; }

  void set_validator(std::unique_ptr<Parameter_validator> v) {
    m_validator = std::move(v);
  }

  template <typename T,
            typename std::enable_if<
                std::is_base_of<Parameter_validator, T>::value, int>::type = 0>
  T *validator() const {
    return dynamic_cast<T *>(m_validator.get());
  }

 private:
  Value_type m_type;
  std::unique_ptr<Parameter_validator> m_validator;
};

/**
 * Holds the option metadata for an option pack.
 */
class IOption_pack_def {
 public:
  const std::vector<std::shared_ptr<shcore::Parameter>> &options() const {
    return m_option_md;
  }

  bool empty() const { return m_option_md.empty(); }

 protected:
  std::vector<std::shared_ptr<shcore::Parameter>> m_option_md;
};

/**
 * Creates an option pack handler for the C options class.
 *
 * Any option class to be handled as an Option_pack_def must implement:
 * - static const Option_pack_def<C> &options();
 *
 * Option clases can be primary or not.
 *
 * A primary option pack is that to be used as a parameter in an API function to
 * be exposed to the user with expose() [see Option_pack_ref below].
 *
 * Non primary option packs can be included as part of a primary option pack.
 *
 * Primary option packs must also implement:
 *
 * - void unpack(const Dictionary_t &options)
 *
 * Typical implementation of the options function is as follows:
 *
 * const Option_pack_def<C> &C::options() {
 *   static const auto opts = Option_pack_def<C>()
 *          .on_start(...)
 *          .optional(...)
 *          .optional(...)
 *          .optional(...)
 *          .include(...)
 *          .on_done(...);
 *
 *   return opts;
 * }
 */
template <typename C>
class Option_pack_def : public IOption_pack_def {
 public:
  /**
   * Allows defining an optional option for the pack, using a member attribute
   * as the target location for the unpacked value.
   *
   * @param name: name of the option.
   * @param var: pointer to a member attribute.
   * @param sname: short name for the option (if applicable).
   * @param cmd_line: whether the option is valid for CLI integration
   */
  template <typename T>
  Option_pack_def<C> &optional(const std::string &name, T C::*var,
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, var](shcore::Option_unpacker *unpacker, C *instance) {
          // this is where the magic happens
          unpacker->optional(name.c_str(), &((*instance).*var));
        });
    return *this;
  }

  /**
   * Allows defining an optional option for the pack, using a member attribute
   * that is read as strings but mapped to enum values.
   *
   * @param name: name of the option.
   * @param var: pointer to a member attribute.
   * @param mapping: map that relates string values to enum values.
   * @param sname: short name for the option (if applicable).
   * @param cmd_line: whether the option is valid for CLI integration
   */
  template <typename T>
  Option_pack_def<C> &optional(const std::string &name, T C::*var,
                               const std::map<std::string, T> &mapping,
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<std::string>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, var, mapping, this](shcore::Option_unpacker *unpacker,
                                   C *instance) {
          // this is where the magic happens
          mysqlshdk::utils::nullable<std::string> value;
          unpacker->optional(name.c_str(), &value);

          if (!value.is_null()) {
            (*instance).*var = get_enum_value(name, *value, mapping);
          }
        });
    return *this;
  }

  /**
   * Allows defining an optional option for the pack, using a member function as
   * a callback.
   *
   * @param name: name of the option.
   * @param callback: pointer to a member function to be called if the option is
   * provided on the options being unpacked.
   * @param sname: short name for the option (if applicable).
   * @param cmd_line: whether the option is valid for CLI integration
   *
   * The callback must be a member function with the following sugnature:
   *
   * callback(const string &name, const T &value)
   *
   * If the callback can be used as a proxy function for options having the same
   * data type.
   */
  template <typename T>
  Option_pack_def<C> &optional(const std::string &name,
                               void (C::*callback)(const std::string &,
                                                   const T &),
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, callback](shcore::Option_unpacker *unpacker, C *instance) {
          mysqlshdk::utils::nullable<T> value;
          unpacker->optional(name.c_str(), &value);
          if (!value.is_null()) {
            ((*instance).*callback)(name, *value);
          }
        });

    return *this;
  }

  template <typename T>
  Option_pack_def<C> &optional(const std::string &name,
                               void (C::*callback)(const std::string &option,
                                                   T value),
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, callback](shcore::Option_unpacker *unpacker, C *instance) {
          mysqlshdk::utils::nullable<T> value;
          unpacker->optional(name.c_str(), &value);
          if (!value.is_null()) {
            ((*instance).*callback)(name, *value);
          }
        });

    return *this;
  }

  /**
   * Allows defining an optional option for the pack, using a member function as
   * a callback.
   *
   * @param name: name of the option.
   * @param callback: pointer to a member function to be called if the option is
   * provided on the options being unpacked.
   * @param sname: short name for the option (if applicable).
   * @param cmd_line: whether the option is valid for CLI integration
   *
   * The callback must be a member function with the following sugnature:
   *
   * callback(const string &name, const T &value)
   *
   * If the callback can be used as a proxy function for options having the same
   * data type.
   */
  template <typename T>
  Option_pack_def<C> &optional(const std::string &name,
                               void (C::*callback)(const T &),
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, callback](shcore::Option_unpacker *unpacker, C *instance) {
          mysqlshdk::utils::nullable<T> value;
          unpacker->optional(name.c_str(), &value);
          if (!value.is_null()) {
            ((*instance).*callback)(*value);
          }
        });

    return *this;
  }

  template <typename T>
  Option_pack_def<C> &optional(const std::string &name,
                               void (C::*callback)(T value),
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, callback](shcore::Option_unpacker *unpacker, C *instance) {
          mysqlshdk::utils::nullable<T> value;
          unpacker->optional(name.c_str(), &value);
          if (!value.is_null()) {
            ((*instance).*callback)(*value);
          }
        });

    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T>
  Option_pack_def<C> &required(const std::string &name, T C::*var,
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Mandatory);

    m_unpack_callbacks.emplace_back(
        [name, var](shcore::Option_unpacker *unpacker, C *instance) {
          // this is where the magic happens
          unpacker->required(name.c_str(), &((*instance).*var));
        });
    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T>
  Option_pack_def<C> &required(const std::string &name, T C::*var,
                               const std::map<std::string, T> &mapping,
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<std::string>(name, sname, cmd_line, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, var, mapping, this](shcore::Option_unpacker *unpacker,
                                   C *instance) {
          // this is where the magic happens
          mysqlshdk::utils::nullable<std::string> value;
          unpacker->required(name.c_str(), &value);

          if (!value.is_null()) {
            (*instance).*var = get_enum_value(name, *value, mapping);
          }
        });
    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T>
  Option_pack_def<C> &required(const std::string &name,
                               void (C::*callback)(const std::string &option,
                                                   const T &),
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Mandatory);

    m_unpack_callbacks.emplace_back(
        [name, callback](shcore::Option_unpacker *unpacker, C *instance) {
          mysqlshdk::utils::nullable<T> value;
          unpacker->required(name.c_str(), &value);
          if (!value.is_null()) {
            ((*instance).*callback)(name, *value);
          }
        });

    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T>
  Option_pack_def<C> &required(const std::string &name,
                               void (C::*callback)(const T &),
                               const std::string &sname = "",
                               bool cmd_line = true) {
    add_option<T>(name, sname, cmd_line, Param_flag::Mandatory);

    m_unpack_callbacks.emplace_back(
        [name, callback](shcore::Option_unpacker *unpacker, C *instance) {
          mysqlshdk::utils::nullable<T> value;
          unpacker->required(name.c_str(), &value);
          if (!value.is_null()) {
            ((*instance).*callback)(*value);
          }
        });

    return *this;
  }

  /**
   * Allows aggregating a non primary option pack into this option pack.
   *
   * It is expected that the option pack being included is a member of the C
   * class.
   */
  template <typename T>
  Option_pack_def<C> &include(T C::*var) {
    add_options<T>();

    m_unpack_callbacks.emplace_back(
        [var](shcore::Option_unpacker *unpacker, C *instance) {
          T::options().unpack(unpacker, &((*instance).*var));
        });

    return *this;
  }

  /**
   * Allows aggregating an option pack defined into class T.
   *
   * The common use for this function is when T is the parent class of C.
   */
  template <typename T>
  Option_pack_def<C> &include() {
    add_options<T>();

    m_unpack_callbacks.emplace_back(
        [](shcore::Option_unpacker *unpacker, C *instance) {
          T::options().unpack(unpacker, instance);
        });

    return *this;
  }

  /**
   * Allows defining a callback for when the unpacking starts.
   *
   * The callback should receive the dictionary of options to be used on the
   * unpacking.
   *
   * The provided callback should be a member function of C.
   */
  Option_pack_def<C> &on_start(
      const std::function<void(C *, const shcore::Dictionary_t &)> &callback) {
    m_start_callback = callback;

    return *this;
  }

  /**
   * Allows defining a callback for when the option unpacking has been
   * completed.
   *
   * The provided callback should be a member function of C.
   */
  Option_pack_def<C> &on_done(const std::function<void(C *)> &callback) {
    m_done_callback = callback;

    return *this;
  }

  /**
   * Unpacks the options on this pack.
   *
   * This is the entry point for the unpacking process so this function is
   * called for primary option packs.
   */
  void unpack(const Dictionary_t &options, C *instance) const {
    Option_unpacker unpacker;
    unpacker.set_options(options);

    unpack(&unpacker, instance);

    unpacker.end();
  }

  /**
   * Unpacks the options on this pack.
   *
   * This function is called for non primary option packs.
   */
  void unpack(Option_unpacker *unpacker, C *instance) const {
    if (m_start_callback) m_start_callback(instance, unpacker->options());

    for (const auto &unpack_cb : m_unpack_callbacks) {
      unpack_cb(unpacker, instance);
    }

    if (m_done_callback) m_done_callback(instance);
  }

 private:
  template <typename T>
  void add_options() {
    auto options = T::options().options();
    m_option_md.insert(m_option_md.end(), options.begin(), options.end());
  }

  template <typename T>
  void add_option(const std::string &name, const std::string &sname,
                  bool cmd_line, shcore::Param_flag flag) {
    m_option_md.push_back(std::make_shared<shcore::Parameter>(
        name, shcore::Type_info<T>::vtype(), flag, sname, cmd_line));

    auto validator = Type_info<T>::validator();
    if (validator) {
      validator->set_enabled(false);
      m_option_md.back()->set_validator(std::move(validator));
    }
  }

  template <typename T>
  T get_enum_value(const std::string &option, const std::string &value,
                   const std::map<std::string, T> &mapping) {
    auto lc_value = value;
    std::transform(lc_value.begin(), lc_value.end(), lc_value.begin(),
                   ::tolower);

    auto pos = mapping.find(lc_value);
    if (pos == mapping.end()) {
      std::string error = "Invalid value '" + value + "' for " + option +
                          " option, allowed values: ";

      std::vector<std::string> allowed;
      size_t index = 0;
      size_t count = mapping.size();
      for (const auto &it : mapping) {
        if (index > 0) {
          if (index < count - 1) {
            error.append(", ");
          } else {
            error.append(" and ");
          }
        }

        error.append("'" + it.first + "'");
        index++;
      }

      error.append(".");

      throw std::invalid_argument(error);
    }

    return pos->second;
  }

  std::vector<std::function<void(shcore::Option_unpacker *, C *)>>
      m_unpack_callbacks;

  std::function<void(C *, const shcore::Dictionary_t &)> m_start_callback;
  std::function<void(C *)> m_done_callback;
};

/**
 * Wrapper for option packs to be used as parameters on exposed APIs.
 *
 * Option packs used as parameters on APIs should be defined as:
 *
 * Option_pack_ref<T>
 *
 * This enables automatic metadata definition for the options on the pack.
 */
template <typename T>
class Option_pack_ref {
 public:
  T *operator->() { return &m_options; }
  const T *operator->() const { return &m_options; }

  T &operator*() { return m_options; }
  const T &operator*() const { return m_options; }

  void unpack(const shcore::Dictionary_t &options) {
    T::options().unpack(options, &m_options);
  }

 private:
  T m_options;
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

  using Raw_signature = std::vector<std::shared_ptr<Parameter>>;
  struct Metadata {
    Metadata() = default;
    Metadata(const Metadata &) = delete;
    std::string name[2];
    Raw_signature signature;
    mysqlshdk::utils::nullable<bool> cli_enabled;

    std::vector<std::pair<std::string, Value_type>> param_types;
    std::string param_codes;
    Value_type return_type;

    void set_name(const std::string &name);
    void set(const std::string &name, Value_type rtype,
             const std::vector<std::pair<std::string, Value_type>> &ptypes,
             const std::string &pcodes);
    void set(const std::string &name, Value_type rtype,
             const Raw_signature &params,
             const std::vector<std::pair<std::string, Value_type>> &ptypes,
             const std::string &pcodes);
    void cli(bool enable = true);
  };

 protected:
  friend class Cpp_object_bridge;

  static Raw_signature gen_signature(
      const std::vector<std::pair<std::string, Value_type>> &param_types);
  static std::tuple<bool, int, shcore::Exception> match_signatures(
      const Raw_signature &cand, const std::vector<Value_type> &wanted,
      const shcore::Dictionary_t &kwds);

  Cpp_function(const std::string &name, const Function &func,
               const std::vector<std::pair<std::string, Value_type>>
                   &signature);  // delme
  Cpp_function(const Metadata *meta, const Function &func);

  const Raw_signature &function_signature() const { return _meta->signature; }

  const Metadata *get_metadata() const { return _meta; }

 private:
  // Each instance holds it's names on the different styles
  Function _func;

  const Metadata *_meta;
  Metadata _meta_tmp;  // temporary memory for legacy versions of Cpp_function
};

class SHCORE_PUBLIC Cpp_object_bridge : public Object_bridge {
 protected:
  Cpp_object_bridge();
  Cpp_object_bridge(const Cpp_object_bridge &) = delete;

 public:
  ~Cpp_object_bridge() override;

  bool operator==(const Object_bridge &other) const override {
    return class_name() == other.class_name() && this == &other;
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
  const Cpp_function::Metadata *get_function_metadata(const std::string &name,
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
                                 const Cpp_function::Metadata &md);

  /**
   * exposes a function defined either in JavaScript or Python
   * @returns a reference to the function metadata so the caller can set
   * the help details and register them in the help system.
   */
  Cpp_function::Metadata *expose(const std::string &name,
                                 const shcore::Function_base_ref &func,
                                 const Cpp_function::Raw_signature &parameters);

  /**
   * Expose method with no arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename C>
  Cpp_function::Metadata *expose(const std::string &name, R (C::*func)()) {
    return expose_(name, func, {});
  }

  template <typename R, typename C>
  Cpp_function::Metadata *expose(const std::string &name,
                                 R (C::*func)() const) {
    return expose_(name, func, {});
  }

  template <typename R>
  Cpp_function::Metadata *expose(const std::string &name, R (*func)()) {
    return expose_(name, func, {});
  }

  template <typename R>
  Cpp_function::Metadata *expose(const std::string &name,
                                 std::function<R()> &&func) {
    return expose_(name, std::move(func), {});
  }

  /**
   * Expose method with 1 argument, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1), const std::string &a1doc,
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc}, std::move(a1def));
  }

  template <typename R, typename A1, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1) const, const std::string &a1doc,
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc}, std::move(a1def));
  }

  template <typename R, typename A1>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1), const std::string &a1doc,
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc}, std::move(a1def));
  }

  template <typename R, typename A1>
  Cpp_function::Metadata *expose(
      const std::string &name, std::function<R(A1)> &&func,
      const std::string &a1doc,
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, std::move(func), {a1doc}, std::move(a1def));
  }

  /**
   * Expose method with 2 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2), const std::string &a1doc,
      const std::string &a2doc,
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  template <typename R, typename A1, typename A2, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2) const,
      const std::string &a1doc, const std::string &a2doc,
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  template <typename R, typename A1, typename A2>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1, A2), const std::string &a1doc,
      const std::string &a2doc,
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  template <typename R, typename A1, typename A2>
  Cpp_function::Metadata *expose(
      const std::string &name, std::function<R(A1, A2)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, std::move(func), {a1doc, a2doc}, std::move(a1def),
                   std::move(a2def));
  }

  /**
   * Expose method with 3 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc,
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc,
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def));
  }

  template <typename R, typename A1, typename A2, typename A3>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3), const std::string &a1doc,
      const std::string &a2doc, const std::string &a3doc,
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def));
  }

  template <typename R, typename A1, typename A2, typename A3>
  Cpp_function::Metadata *expose(
      const std::string &name, std::function<R(A1, A2, A3)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc,
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, std::move(func), {a1doc, a2doc, a3doc},
                   std::move(a1def), std::move(a2def), std::move(a3def));
  }

  /**
   * Expose method with 4 arguments, with automatic bridging.
   * See expose__() for details.
   */
  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc}, std::move(a1def),
                   std::move(a2def), std::move(a3def), std::move(a4def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4>
  Cpp_function::Metadata *expose(
      const std::string &name, std::function<R(A1, A2, A3, A4)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
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
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc,
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc,
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4, A5),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc,
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5>
  Cpp_function::Metadata *expose(
      const std::string &name, std::function<R(A1, A2, A3, A4, A5)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc,
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
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
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4, A5, A6),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc},
                   std::move(a1def), std::move(a2def), std::move(a3def),
                   std::move(a4def), std::move(a5def), std::move(a6def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6>
  Cpp_function::Metadata *expose(
      const std::string &name, std::function<R(A1, A2, A3, A4, A5, A6)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
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
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6, A7),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc,
      Type_info_t<A7> &&a7def = Type_info<A7>::default_value(),
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(
        name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
        std::move(a1def), std::move(a2def), std::move(a3def), std::move(a4def),
        std::move(a5def), std::move(a6def), std::move(a7def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7, typename C>
  Cpp_function::Metadata *expose(
      const std::string &name, R (C::*func)(A1, A2, A3, A4, A5, A6, A7) const,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc,
      Type_info_t<A7> &&a7def = Type_info<A7>::default_value(),
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(
        name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
        std::move(a1def), std::move(a2def), std::move(a3def), std::move(a4def),
        std::move(a5def), std::move(a6def), std::move(a7def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7>
  Cpp_function::Metadata *expose(
      const std::string &name, R (*func)(A1, A2, A3, A4, A5, A6, A7),
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc,
      Type_info_t<A7> &&a7def = Type_info<A7>::default_value(),
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
    return expose_(
        name, func, {a1doc, a2doc, a3doc, a4doc, a5doc, a6doc, a7doc},
        std::move(a1def), std::move(a2def), std::move(a3def), std::move(a4def),
        std::move(a5def), std::move(a6def), std::move(a7def));
  }

  template <typename R, typename A1, typename A2, typename A3, typename A4,
            typename A5, typename A6, typename A7>
  Cpp_function::Metadata *expose(
      const std::string &name,
      std::function<R(A1, A2, A3, A4, A5, A6, A7)> &&func,
      const std::string &a1doc, const std::string &a2doc,
      const std::string &a3doc, const std::string &a4doc,
      const std::string &a5doc, const std::string &a6doc,
      const std::string &a7doc,
      Type_info_t<A7> &&a7def = Type_info<A7>::default_value(),
      Type_info_t<A6> &&a6def = Type_info<A6>::default_value(),
      Type_info_t<A5> &&a5def = Type_info<A5>::default_value(),
      Type_info_t<A4> &&a4def = Type_info<A4>::default_value(),
      Type_info_t<A3> &&a3def = Type_info<A3>::default_value(),
      Type_info_t<A2> &&a2def = Type_info<A2>::default_value(),
      Type_info_t<A1> &&a1def = Type_info<A1>::default_value()) {
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
  std::string get_function_name(const std::string &member,
                                bool fully_specified = true) const;

  std::vector<Cpp_property_name> _properties;

  // Returns named function which signature that matches the given argument list
  std::shared_ptr<Cpp_function> lookup_function_overload(
      const std::string &method, const shcore::Argument_list &args,
      const shcore::Dictionary_t &kwds = {}) const;

  std::shared_ptr<Cpp_function> lookup_function(
      const std::string &method) const;

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

  static std::map<std::string, Cpp_function::Metadata> mdtable;
  static void clear_metadata();
  static Cpp_function::Metadata &get_metadata(const std::string &method);
  static void set_metadata(
      Cpp_function::Metadata &meta, const std::string &name, Value_type rtype,
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
  Cpp_function::Metadata *expose_(const std::string &name, R (C::*func)(A...),
                                  const std::vector<std::string> &docs,
                                  Type_info_t<A> &&... defs) {
    return expose__<R, std::function<R(A...)>, A...>(
        name,
        [this, func](A... args) -> R {
          return (static_cast<C *>(this)->*func)(args...);
        },
        docs, std::move(defs)...);
  }

  template <typename R, typename C, typename... A>
  Cpp_function::Metadata *expose_(const std::string &name,
                                  R (C::*func)(A...) const,
                                  const std::vector<std::string> &docs,
                                  Type_info_t<A> &&... defs) {
    return expose__<R, std::function<R(A...)>, A...>(
        name,
        [this, func](A... args) -> R {
          return (static_cast<C *>(this)->*func)(args...);
        },
        docs, std::move(defs)...);
  }

  template <typename R, typename... A>
  Cpp_function::Metadata *expose_(const std::string &name, R (*func)(A...),
                                  const std::vector<std::string> &docs,
                                  Type_info_t<A> &&... defs) {
    return expose__<R, std::function<R(A...)>, A...>(name, func, docs,
                                                     std::move(defs)...);
  }

  template <typename R, typename... A>
  Cpp_function::Metadata *expose_(const std::string &name,
                                  std::function<R(A...)> &&func,
                                  const std::vector<std::string> &docs,
                                  Type_info_t<A> &&... defs) {
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
  Cpp_function::Metadata *expose__(const std::string &name, F &&func,
                                   const std::vector<std::string> &docs,
                                   Type_info_t<A> &&... defs) {
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
    auto mangled_name = class_name() + "::" + name + ":";
    // fold expressions are available in C++17, use
    // std::initializer_list + comma operator trick instead
    (void)std::initializer_list<int>{
        (pcodes.append(Type_info<A>::code()), 0)...};
    mangled_name.append(pcodes);
    auto &md = get_metadata(mangled_name);

    if (md.name[0].empty()) {
      std::vector<std::pair<std::string, Value_type>> ptypes;
      std::vector<Value_type> vtypes = {Type_info<A>::vtype()...};

      for (size_t i = 0; i < size; ++i) {
        ptypes.emplace_back(docs[i], vtypes[i]);
      }

      set_metadata(md, name, Type_info<R>::vtype(), ptypes, pcodes);
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
    return func((size <= I || args.at(I).type == Value_type::Undefined
                     ? std::get<I>(defs)
                     : Arg_handler<A>::get(I, args))...);
  }

#ifdef FRIEND_TEST
  friend class Types_cpp;
#endif
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_CPP_H_
