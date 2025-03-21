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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_OPTION_PACK_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_OPTION_PACK_H_

#include <algorithm>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/type_info/validators.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types/parameter.h"
#include "mysqlshdk/include/scripting/types/unpacker.h"

namespace shcore {

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
 * Option classes can be primary or not.
 *
 * A primary option pack is that to be used as a parameter in an API function to
 * be exposed to the user with expose().
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

enum class Option_extract_mode {
  CASE_SENSITIVE = 1 << 0,    // Allows data type conversion
  CASE_INSENSITIVE = 1 << 1,  // Allows data type conversion
  EXACT = 1 << 2              // Restricts data type conversion
};

enum class Option_scope { GLOBAL, CLI_DISABLED, DEPRECATED };

template <typename C>
class Option_pack_def : public IOption_pack_def {
 public:
  template <typename T>
  Option_pack_def<C> &optional(
      const std::string &name,
      std::function<void(C &instance, std::string_view name, T value)> callback,
      const std::string &sname = "",
      Option_extract_mode extract_mode = Option_extract_mode::CASE_INSENSITIVE,
      Option_scope option_scope = Option_scope::GLOBAL) {
    add_option<T>(name, sname, option_scope, Param_flag::Optional);

    // cannot capture this here, static instances are initialized via copy/move,
    // original object will no longer exist when unpack callback is called
    m_unpack_callbacks.emplace_back(
        [name, callback = std::move(callback), extract_mode](
            const Option_pack_def<C> *self, shcore::Option_unpacker *unpacker,
            C *instance) {
          std::optional<T> value;
          self->get_optional(unpacker, extract_mode, name, &value);

          if (value.has_value()) callback(*instance, name, std::move(*value));
        });
    return *this;
  }

  /**
   * Allows defining an optional option for the pack, using a member attribute
   * as the target location for the unpacked value.
   *
   * @param name: name of the option.
   * @param var: pointer to a member attribute.
   * @param sname: short name for the option (if applicable).
   * @param cmd_line: whether the option is valid for CLI integration
   */
  template <typename T, typename SC>
  Option_pack_def<C> &optional(
      const std::string &name, T SC::*var, const std::string &sname = "",
      Option_extract_mode extract_mode = Option_extract_mode::CASE_INSENSITIVE,
      Option_scope option_scope = Option_scope::GLOBAL) {
    add_option<T>(name, sname, option_scope, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, var, extract_mode](const Option_pack_def<C> *self,
                                  shcore::Option_unpacker *unpacker,
                                  C *instance) {
          self->get_optional(unpacker, extract_mode, name, &((*instance).*var));
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
  template <typename T, typename SC>
  Option_pack_def<C> &optional(
      const std::string &name, T SC::*var,
      const std::map<std::string, T> &mapping, const std::string &sname = "",
      Option_extract_mode extract_mode = Option_extract_mode::CASE_INSENSITIVE,
      Option_scope option_scope = Option_scope::GLOBAL) {
    add_option<std::string>(name, sname, option_scope, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, var, mapping, extract_mode](const Option_pack_def<C> *self,
                                           shcore::Option_unpacker *unpacker,
                                           C *instance) {
          // this is where the magic happens
          std::optional<std::string> value;
          self->get_optional(unpacker, extract_mode, name, &value);

          if (value.has_value()) {
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
   * The callback must be a member function with the following signature:
   *
   * callback(const string &name, const T &value)
   *
   * If the callback can be used as a proxy function for options having the same
   * data type.
   */
  template <typename T, typename SC>
  Option_pack_def<C> &optional(
      const std::string &name,
      void (SC::*callback)(const std::string &option, T value),
      const std::string &sname = "",
      Option_extract_mode extract_mode = Option_extract_mode::CASE_INSENSITIVE,
      Option_scope option_scope = Option_scope::GLOBAL) {
    using TT = Type_info_t<T>;
    add_option<TT>(name, sname, option_scope, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, callback, extract_mode](const Option_pack_def<C> *self,
                                       shcore::Option_unpacker *unpacker,
                                       C *instance) {
          std::optional<TT> value;
          self->get_optional(unpacker, extract_mode, name, &value);

          unpacker->optional(name.c_str(), &value);
          if (value.has_value()) {
            ((*instance).*callback)(name, std::forward<T>(*value));
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
  template <typename T, typename SC>
  Option_pack_def<C> &optional(
      const std::string &name, void (SC::*callback)(T value),
      const std::string &sname = "",
      Option_extract_mode extract_mode = Option_extract_mode::CASE_INSENSITIVE,
      Option_scope option_scope = Option_scope::GLOBAL) {
    using TT = Type_info_t<T>;
    add_option<TT>(name, sname, option_scope, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, callback, extract_mode](const Option_pack_def<C> *self,
                                       shcore::Option_unpacker *unpacker,
                                       C *instance) {
          std::optional<TT> value;
          self->get_optional(unpacker, extract_mode, name, &value);

          if (value.has_value()) {
            ((*instance).*callback)(std::forward<T>(*value));
          }
        });

    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T, typename SC>
  Option_pack_def<C> &required(
      const std::string &name, T SC::*var, const std::string &sname = "",
      Option_scope option_scope = Option_scope::GLOBAL) {
    add_option<T>(name, sname, option_scope, Param_flag::Mandatory);

    m_unpack_callbacks.emplace_back(
        [name, var](const Option_pack_def<C> *,
                    shcore::Option_unpacker *unpacker, C *instance) {
          // this is where the magic happens
          unpacker->required(name.c_str(), &((*instance).*var));
        });
    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T, typename SC>
  Option_pack_def<C> &required(
      const std::string &name, T SC::*var,
      const std::map<std::string, T> &mapping, const std::string &sname = "",
      Option_scope option_scope = Option_scope::GLOBAL) {
    add_option<std::string>(name, sname, option_scope, Param_flag::Optional);

    m_unpack_callbacks.emplace_back(
        [name, var, mapping](const Option_pack_def<C> *,
                             shcore::Option_unpacker *unpacker, C *instance) {
          // this is where the magic happens
          std::optional<std::string> value;
          unpacker->required(name.c_str(), &value);

          if (value.has_value()) {
            (*instance).*var = get_enum_value(name, *value, mapping);
          }
        });
    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T, typename SC>
  Option_pack_def<C> &required(
      const std::string &name,
      void (SC::*callback)(const std::string &option, T),
      const std::string &sname = "",
      Option_scope option_scope = Option_scope::GLOBAL) {
    using TT = Type_info_t<T>;
    add_option<TT>(name, sname, option_scope, Param_flag::Mandatory);

    m_unpack_callbacks.emplace_back(
        [name, callback](const Option_pack_def<C> *,
                         shcore::Option_unpacker *unpacker, C *instance) {
          std::optional<TT> value;
          unpacker->required(name.c_str(), &value);

          if (value.has_value()) {
            ((*instance).*callback)(name, std::forward<T>(*value));
          }
        });

    return *this;
  }

  /**
   * Same as the optional() equivalent, except that the option is mandatory
   */
  template <typename T, typename SC>
  Option_pack_def<C> &required(
      const std::string &name, void (SC::*callback)(T),
      const std::string &sname = "",
      Option_scope option_scope = Option_scope::GLOBAL) {
    using TT = Type_info_t<T>;
    add_option<TT>(name, sname, option_scope, Param_flag::Mandatory);

    m_unpack_callbacks.emplace_back(
        [name, callback](const Option_pack_def<C> *,
                         shcore::Option_unpacker *unpacker, C *instance) {
          std::optional<TT> value;
          unpacker->required(name.c_str(), &value);

          if (value.has_value()) {
            ((*instance).*callback)(std::forward<T>(*value));
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
  template <typename T, typename SC>
  Option_pack_def<C> &include(T SC::*var) {
    add_options<T>();

    m_unpack_callbacks.emplace_back([var](const Option_pack_def<C> *,
                                          shcore::Option_unpacker *unpacker,
                                          C *instance) {
      T::options().unpack(unpacker, &((*instance).*var));
    });

    ignore(T::options().ignored());

    return *this;
  }

  /**
   * Allows aggregating a non primary option pack into this option pack.
   *
   * It is expected that the callback returns a class which is an option pack.
   */
  template <typename T, typename SC, typename V>
  Option_pack_def<C> &include(V SC::*var, T &(V::*callback)()) {
    add_options<T>();

    m_unpack_callbacks.emplace_back(
        [var, callback](const Option_pack_def<C> *,
                        shcore::Option_unpacker *unpacker, C *instance) {
          T::options().unpack(unpacker, &std::invoke(callback, instance->*var));
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
        [](const Option_pack_def<C> *, shcore::Option_unpacker *unpacker,
           C *instance) { T::options().unpack(unpacker, instance); });

    ignore(T::options().ignored());

    return *this;
  }

  /**
   * Ignores given options. Options in this list are treated as unknown.
   *
   * @params options Options to be ignored
   */
  Option_pack_def<C> &ignore(
      const std::unordered_set<std::string_view> &ignored_options) {
    if (!ignored_options.empty()) {
      m_ignored.insert(ignored_options.begin(), ignored_options.end());

      m_option_md.erase(std::remove_if(m_option_md.begin(), m_option_md.end(),
                                       [this](const auto &param) {
                                         return is_ignored(param->name);
                                       }),
                        m_option_md.end());
    }

    return *this;
  }

  /**
   * Ignores all options from the given option pack type.
   */
  template <typename T>
  Option_pack_def<C> &ignore() {
    const auto &options = T::options().options();
    std::unordered_set<std::string_view> ignored;

    for (const auto &o : options) {
      ignored.emplace(o->name);
    }

    ignore(ignored);

    return *this;
  }

  const std::unordered_set<std::string_view> &ignored() const {
    return m_ignored;
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

  Option_pack_def<C> &on_log(
      const std::function<void(C *, const std::string &)> &callback) {
    m_log_callback = callback;

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
    unpacker.set_ignored(m_ignored);

    unpack(&unpacker, instance);

    unpacker.end();
  }

  /**
   * Unpacks the options on this pack.
   *
   * This function is called for non primary option packs.
   */
  void unpack(Option_unpacker *unpacker, C *instance) const {
    log(instance, Value(unpacker->options()).json());

    if (m_start_callback) m_start_callback(instance, unpacker->options());

    for (const auto &unpack_cb : m_unpack_callbacks) {
      unpack_cb(this, unpacker, instance);
    }

    if (m_done_callback) m_done_callback(instance);
  }

 private:
  template <typename T>
  void add_options() {
    for (const auto &o : T::options().options()) {
      add_option(o);
    }
  }

  template <typename T>
  void get_optional(Option_unpacker *unpacker, Option_extract_mode extract_mode,
                    const std::string &name, T *value) const {
    switch (extract_mode) {
      case Option_extract_mode::CASE_INSENSITIVE:
        unpacker->optional_ci(name.c_str(), value);
        break;
      case Option_extract_mode::CASE_SENSITIVE:
        unpacker->optional(name.c_str(), value);
        break;
      case Option_extract_mode::EXACT:
        unpacker->optional_exact(name.c_str(), value);
        break;
    }
  }

  template <typename T>
  void add_option(const std::string &name, const std::string &sname,
                  Option_scope scope, shcore::Param_flag flag) {
    if (is_ignored(name)) {
      return;
    }

    const auto option = std::make_shared<shcore::Parameter>(
        name, shcore::Type_info<T>::vtype, flag, sname,
        scope != Option_scope::CLI_DISABLED, scope == Option_scope::DEPRECATED);
    if (auto validator = Type_info<T>::validator()) {
      validator->set_enabled(false);
      option->set_validator(std::move(validator));
    }

    add_option(option);
  }

  void add_option(const std::shared_ptr<shcore::Parameter> &option) {
    if (is_ignored(option->name)) {
      return;
    }

    if (const auto o = m_all_options.find(option->name);
        o != m_all_options.end()) {
      if (*option != *m_option_md[o->second]) {
        throw std::logic_error(
            "Trying to overwrite an option which already exists: " +
            option->name);
      }
    } else {
      m_all_options.emplace(option->name, m_option_md.size());
      m_option_md.emplace_back(option);
    }
  }

  template <typename T>
  static T get_enum_value(const std::string &option, const std::string &value,
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

  bool should_log() const { return !!m_log_callback; }

  void log(C *instance, const std::string &s) const {
    if (should_log()) {
      m_log_callback(instance, s);
    }
  }

  bool is_ignored(std::string_view option) const {
    return m_ignored.count(option) > 0;
  }

  std::vector<std::function<void(const Option_pack_def<C> *,
                                 shcore::Option_unpacker *, C *)>>
      m_unpack_callbacks;

  std::function<void(C *, const shcore::Dictionary_t &)> m_start_callback;
  std::function<void(C *)> m_done_callback;
  std::function<void(C *, const std::string &)> m_log_callback;
  std::unordered_set<std::string_view> m_ignored;
  std::unordered_map<std::string_view, std::size_t> m_all_options;
};

namespace detail {

/**
 * A type which has a `static const Option_pack_def<T> &options();` method.
 */
// clang-format off
template <typename T>
concept Option_pack_t = requires() {
  { T::options() } -> std::same_as<const Option_pack_def<T> &>;
};
// clang-format on

template <Option_pack_t OP>
struct Type_info<OP> {
  static constexpr auto vtype = shcore::Map;
  static constexpr auto code = 'D';

  static OP to_native(const shcore::Value &in) {
    OP ret_val;

    // Null or Undefined get interpreted as a default option pack
    if (const auto type = in.get_type();
        type != shcore::Null && type != shcore::Undefined) {
      OP::options().unpack(in.as_map(), &ret_val);
    }

    return ret_val;
  }

  static std::string desc() { return "an options dictionary"; }
};

template <Option_pack_t OP>
struct Validator_for<OP> {
  static std::unique_ptr<Parameter_validator> get() {
    auto validator = std::make_unique<Option_validator>();
    validator->set_allowed(&OP::options().options());
    // Option validators should be disabled as validation is done on unpacking
    validator->set_enabled(false);
    return validator;
  }
};

}  // namespace detail

template <class TType>
struct Option_data {
  using Type = TType;

  template <class TFilterType = TType>
  using TFilter = void (*)(std::string_view, TFilterType &);

  constexpr explicit Option_data(std::string_view option_name) noexcept
      : name{option_name} {}

  constexpr explicit Option_data(std::string_view option_name,
                                 TFilter<> option_filter) noexcept
      : name{option_name}, filter{option_filter} {}

  constexpr explicit Option_data(std::string_view option_name,
                                 shcore::Option_scope option_scope,
                                 TFilter<> option_filter = nullptr) noexcept
      : name{option_name}, scope{option_scope}, filter{option_filter} {}

  constexpr explicit Option_data(
      std::string_view option_name,
      shcore::Option_extract_mode option_extract_mode,
      TFilter<> option_filter = nullptr) noexcept
      : name{option_name},
        extract_mode{option_extract_mode},
        filter{option_filter} {}

  constexpr explicit Option_data(
      std::string_view option_name,
      shcore::Option_extract_mode option_extract_mode,
      shcore::Option_scope option_scope,
      TFilter<> option_filter = nullptr) noexcept
      : name{option_name},
        extract_mode{option_extract_mode},
        scope{option_scope},
        filter{option_filter} {}

  constexpr Option_data(std::string_view option_name,
                        std::string_view option_sname) noexcept
      : name{option_name}, sname{option_sname} {}

  constexpr explicit Option_data(
      std::string_view option_name, std::string_view option_sname,
      shcore::Option_extract_mode option_extract_mode,
      shcore::Option_scope option_scope,
      TFilter<> option_filter = nullptr) noexcept
      : name{option_name},
        sname{option_sname},
        extract_mode{option_extract_mode},
        scope{option_scope},
        filter{option_filter} {}

  template <class TNewType>
  constexpr auto as(TFilter<TNewType> new_filter = nullptr) const noexcept {
    return Option_data<TNewType>{name, sname, extract_mode, scope, new_filter};
  }

  auto deprecate() const noexcept {
    auto new_option = *this;
    new_option.scope = shcore::Option_scope::DEPRECATED;
    return new_option;
  }

  std::string_view name;
  std::string_view sname;
  shcore::Option_extract_mode extract_mode{
      shcore::Option_extract_mode::CASE_INSENSITIVE};
  shcore::Option_scope scope{shcore::Option_scope::GLOBAL};
  TFilter<> filter{nullptr};
};

template <class TOption>
class Option_pack_builder {
  template <class>
  static constexpr bool dependent_false = false;

 public:
  template <class TOptionType, class TTarget>
    requires(std::is_member_object_pointer_v<TTarget>)
  Option_pack_builder &optional(const Option_data<TOptionType> &option,
                                TTarget &&target) {
    if (option.filter) {
      m_pack.template optional<TOptionType>(
          std::string{option.name},
          [target = std::forward<TTarget>(target), filter = option.filter](
              TOption &options, std::string_view option_name,
              TOptionType option_value) {
            filter(option_name, option_value);
            options.*target = std::move(option_value);
          },
          std::string{option.sname}, option.extract_mode, option.scope);

    } else {
      m_pack.optional(std::string{option.name}, std::move(target),
                      std::string{option.sname}, option.extract_mode,
                      option.scope);
    }

    return *this;
  }

  template <class TOptionType, class TTarget>
    requires(std::is_member_object_pointer_v<TTarget>)
  Option_pack_builder &required(const Option_data<TOptionType> &option,
                                TTarget &&target) {
    m_pack.required(std::string{option.name}, std::move(target),
                    std::string{option.sname}, option.extract_mode,
                    option.scope);

    return *this;
  }

  template <class TFilterType, class TOptionType, class TTarget, class TFilter>
    requires(std::is_member_object_pointer_v<TTarget> &&
             !std::is_same_v<std::decay<TFilterType>, std::decay<TOptionType>>)
  Option_pack_builder &optional_as(const Option_data<TOptionType> &option,
                                   TTarget &&target, TFilter &&filter) {
    if constexpr (std::is_invocable_r_v<TOptionType, TFilter,
                                        const TFilterType &>) {
      m_pack.template optional<TFilterType>(
          std::string{option.name},
          [target = std::forward<TTarget>(target),
           filter = std::forward<TFilter>(filter)](
              TOption &options, std::string_view,
              const TFilterType &option_value) {
            options.*target = filter(option_value);
          },
          std::string{option.sname}, option.extract_mode, option.scope);

    } else {
      static_assert(
          dependent_false<TFilter>,
          "Convertion callback signature is not supported");  // until
                                                              // CWG2518/P2593R1
    }

    return *this;
  }

  template <class TFilterType, class TOptionType, class TFilter>
  Option_pack_builder &optional_as(const Option_data<TOptionType> &option,
                                   TFilter &&callback) {
    if constexpr (std::is_invocable_r_v<void, TFilter, TOption &,
                                        std::string_view,
                                        const TFilterType &>) {
      m_pack.template optional<TFilterType>(
          std::string{option.name},
          [callback = std::forward<TFilter>(callback)](
              TOption &options, std::string_view option_name,
              const TFilterType &option_value) {
            callback(options, option_name, option_value);
          },
          std::string{option.sname}, option.extract_mode, option.scope);
    } else {
      static_assert(
          dependent_false<TFilter>,
          "Convertion callback signature is not supported");  // until
                                                              // CWG2518/P2593R1
    }

    return *this;
  }

  template <class TOptionType, class TTarget, class TFilter,
            class TFilterType = TOptionType>
    requires(std::is_member_object_pointer_v<TTarget>)
  Option_pack_builder &optional(const Option_data<TOptionType> &option,
                                TTarget &&target, TFilter &&filter) {
    if constexpr (std::is_invocable_r_v<void, TFilter, TOptionType &>) {
      m_pack.template optional<TOptionType>(
          std::string{option.name},
          [target = std::forward<TTarget>(target),
           filter = std::forward<TFilter>(filter)](
              TOption &options, std::string_view, TOptionType option_value) {
            filter(option_value);
            options.*target = std::move(option_value);
          },
          std::string{option.sname}, option.extract_mode, option.scope);
    } else if constexpr (std::is_invocable_r_v<void, TFilter, std::string_view,
                                               TOptionType &>) {
      m_pack.template optional<TOptionType>(
          std::string{option.name},
          [target = std::forward<TTarget>(target),
           filter = std::forward<TFilter>(filter)](TOption &options,
                                                   std::string_view option_name,
                                                   TOptionType option_value) {
            filter(option_name, option_value);
            options.*target = std::move(option_value);
          },
          std::string{option.sname}, option.extract_mode, option.scope);
    } else if constexpr (std::is_invocable_r_v<void, TFilter, const TOption &,
                                               std::string_view,
                                               TOptionType &>) {
      m_pack.template optional<TOptionType>(
          std::string{option.name},
          [target = std::forward<TTarget>(target),
           filter = std::forward<TFilter>(filter)](TOption &options,
                                                   std::string_view option_name,
                                                   TOptionType option_value) {
            filter(options, option_name, option_value);
            options.*target = std::move(option_value);
          },
          std::string{option.sname}, option.extract_mode, option.scope);
    } else {
      static_assert(
          dependent_false<TFilter>,
          "Filter callback signature is not supported");  // until
                                                          // CWG2518/P2593R1
    }

    return *this;
  }

  template <class T>
  Option_pack_builder &include() {
    m_pack.template include<T>();
    return *this;
  }

  template <class T>
    requires(std::is_member_object_pointer_v<T>)
  Option_pack_builder &include(T &&member) {
    m_pack.include(std::forward<T>(member));
    return *this;
  }

  shcore::Option_pack_def<TOption> build() noexcept {
    return std::exchange(m_pack, {});
  }

 private:
  shcore::Option_pack_def<TOption> m_pack;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_OPTION_PACK_H_
