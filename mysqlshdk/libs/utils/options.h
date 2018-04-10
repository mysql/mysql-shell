/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_OPTIONS_H_
#define MYSQLSHDK_LIBS_UTILS_OPTIONS_H_

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/include/scripting/common.h"

namespace shcore {
class Options;

namespace opts {

enum class Source {
  Compiled_default,
  Environment_variable,
  Configuration_file,
  Command_line,
  User
};

std::string to_string(Source s);

/// Convenience function for passing command line options.
template <class... Ts>
std::vector<std::string> cmdline(Ts... params) {
  return std::vector<std::string>{params...};
}

/// Base class for all the options handled by Options class.
class Generic_option {
 public:
  Generic_option(const std::string &name, const char *environment_variable,
                 std::vector<std::string> &&command_line_names,
                 const char *help);

  virtual ~Generic_option() {}

  void set(const std::string &new_value) { set(new_value, Source::User); }

  virtual std::string get_value_as_string() const {
    assert(false);  // unimplemented
    return "";
  }

  virtual void reset_to_default_value() {
    assert(false);  // unimplemented
  }

  void handle_environment_variable();

  virtual void handle_command_line_input(const std::string &option,
                                         const char *value) = 0;

  void handle_persisted_value(const char *value);

  virtual std::vector<std::string> get_cmdline_names();

  bool accepts_null_argument() const { return accept_null; }
  bool accepts_no_cmdline_value() const { return no_cmdline_value; }

  std::vector<std::string> get_cmdline_help(std::size_t options_width,
                                            std::size_t help_width) const;

  const std::string &get_name() const { return name; }

  Source get_source() const { return source; }

  const char *get_help() const { return help; }

 protected:
  virtual void set(const std::string &new_value, Source source) = 0;

  const std::string name;
  Source source = Source::Compiled_default;
  const char *environment_variable;
  std::vector<std::string> command_line_names;
  bool accept_null = true;
  bool no_cmdline_value = true;
  const char *help;
};

template <class T>
class Concrete_option : public Generic_option {
 public:
  using Validator =
      typename std::function<T(const std::string &, Source source)>;

  using Serializer = typename std::function<std::string(const T &)>;

  Concrete_option(T *landing_spot, T default_value, const std::string &name,
                  const char *environment_variable,
                  std::vector<std::string> &&command_line_names,
                  const char *help, Validator validator,
                  Serializer serializer = nullptr)
      : Generic_option(
            name, environment_variable,
            std::forward<std::vector<std::string> &&>(command_line_names),
            help),
        landing_spot(landing_spot),
        default_value(default_value),
        validator(validator),
        serializer(serializer) {
    assert(validator != nullptr);
    *landing_spot = default_value;
  }

  void set(const std::string &new_value, Source source) override {
    *landing_spot = validator(new_value, source);
    this->source = source;
  }

  std::string get_value_as_string() const override {
    if (!serializer)
      throw std::runtime_error("Serializer has not been defined for option: " +
                               name);
    return serializer(*landing_spot);
  }

  void reset_to_default_value() override {
    if (source != Source::Compiled_default) {
      *landing_spot = default_value;
      source = Source::Compiled_default;
    }
  }

  void handle_command_line_input(const std::string &option,
                                 const char *value) override {
    if (value == nullptr) {
      if (accept_null)
        value = "";
      else
        throw std::invalid_argument("Option " + option + " needs value");
    }
    set(value, Source::Command_line);
  }

  const T &get() const { return *landing_spot; }

 protected:
  T *landing_spot;
  const T default_value;
  Validator validator;
  Serializer serializer;
};

/* This class is supposed to handle special cases.
 *
 * 1) Option that has no concrete storage but rather method/function (handler)
 *    that is supposed to be called when new value is available.
 * 2) Option that is there only to appear in help message, but is handled
 *    outside of normal processing e.g. in Options::Custom_cmdline_handler.
 * 3) Deprecated options (use deprecated function to define them).
 */
class Proxy_option : public Generic_option {
 public:
  using Handler =
      std::function<void(const std::string &optname, const char *new_value)>;

  Proxy_option(const char *environment_variable,
               std::vector<std::string> &&command_line_names, const char *help,
               Handler handler = nullptr);

  void set(const std::string &new_value, Source source) override;

  void handle_command_line_input(const std::string &option,
                                 const char *value) override;

  std::vector<std::string> get_cmdline_names() override;

 protected:
  Handler handler;
};

Proxy_option::Handler deprecated(
    const char *replacement = nullptr, Proxy_option::Handler handler = {},
    const char *def_value = nullptr,
    const std::map<std::string, std::string> &map = {});

template <class T, class S>
Proxy_option::Handler assign_value(T *landing_spot, S value) {
  return [landing_spot, value](const std::string &, const char *) {
    *landing_spot = value;
  };
}

template <class T>
T convert(const std::string &data) {
  // assuming that option specification turns it on
  if (data.empty()) return static_cast<T>(1);
  T t;
  std::istringstream iss(data);
  iss >> t;
  if (iss.fail()) {
    iss.clear();
    iss >> std::boolalpha >> t;
    if (iss.fail()) throw std::invalid_argument("Incorrect option value.");
  } else if (!iss.eof()) {
    throw std::invalid_argument("Malformed option value.");
  }
  return t;
}

template <>
std::string convert(const std::string &data);

/// Standard serialization mechanism for options
template <class T>
std::string serialize(const T &val) {
  std::ostringstream os;
  os << std::boolalpha << val;
  return os.str();
}

template <>
std::string serialize(const std::string &val);

/// Validator that does some standard type conversion
template <class T>
struct Basic_type {
  T operator()(const std::string &data, Source) { return convert<T>(data); }
};

/// Wrapper for validator that prohibits user from setting option value
template <class T>
struct Read_only : public Basic_type<T> {
  T operator()(const std::string &data, Source source) {
    if (source == Source::User)
      throw std::logic_error("This option is read only.");
    return Basic_type<T>::operator()(data, source);
  }
};

/// Extension of Basic_type validator to enable range checks
template <class T>
struct Range : public Basic_type<T> {
  Range(T min, T max) : min(min), max(max) { assert(max >= min); }

  T operator()(const std::string &data, Source source) {
    T t = Basic_type<T>::operator()(data, source);
    if (t < min || t > max) throw std::out_of_range("value out of range");
    return t;
  }

  T min;
  T max;
};
}  // namespace opts

class Options {
  using NamedOptions = std::map<std::string, opts::Generic_option *>;

  struct Add_named_option_helper {
    explicit Add_named_option_helper(Options &options) : parent(options) {}

    template <class T, class S>
    Add_named_option_helper &operator()(
        T *landing_spot, S &&default_value, const std::string &optname,
        const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>(),
        typename opts::Concrete_option<T>::Serializer serializer =
            opts::serialize<T>) {
      return (*this)(landing_spot, std::forward<S>(default_value), optname,
                     nullptr, opts::cmdline(), help, validator, serializer);
    }

    template <class T, class S>
    Add_named_option_helper &operator()(
        T *landing_spot, S &&default_value, const std::string &optname,
        std::vector<std::string> &&command_line_names, const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>(),
        typename opts::Concrete_option<T>::Serializer serializer =
            opts::serialize<T>) {
      return (*this)(landing_spot, std::forward<S>(default_value), optname,
                     nullptr, std::move(command_line_names), help, validator,
                     serializer);
    }

    template <class T, class S>
    Add_named_option_helper &operator()(
        T *landing_spot, S &&default_value, const std::string &optname,
        const char *envname, std::vector<std::string> &&command_line_names,
        const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>(),
        typename opts::Concrete_option<T>::Serializer serializer =
            opts::serialize<T>) {
      parent.add_option(new opts::Concrete_option<T>(
          landing_spot, T(std::forward<S>(default_value)), optname, envname,
          std::move(command_line_names), help, validator, serializer));
      return *this;
    }

    Options &parent;
  };

  struct Add_startup_option_helper {
    explicit Add_startup_option_helper(Options &options) : parent(options) {}

    template <class T, class S>
    Add_startup_option_helper &operator()(
        T *landing_spot, S &&default_value,
        std::vector<std::string> &&command_line_names, const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>()) {
      return (*this)(landing_spot, std::forward<S>(default_value), nullptr,
                     std::move(command_line_names), help, validator);
    }

    template <class T, class S>
    Add_startup_option_helper &operator()(
        T *landing_spot, S &&default_value, const char *envname,
        std::vector<std::string> &&command_line_names, const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>()) {
      parent.add_option(new opts::Concrete_option<T>(
          landing_spot, T(std::forward<S>(default_value)), "", envname,
          std::move(command_line_names), help, validator));
      return *this;
    }

    // Proxy options - custom cmdline handle/just help definition.
    Add_startup_option_helper &operator()(
        std::vector<std::string> &&command_line_names,
        const char *help,  // full help text
        opts::Proxy_option::Handler handler = nullptr) {
      parent.add_option(new opts::Proxy_option(
          nullptr, std::move(command_line_names), help, handler));
      return *this;
    }

    // Proxy options - deprecated/secret option not appearing in help.
    Add_startup_option_helper &operator()(
        std::vector<std::string> &&command_line_names,
        opts::Proxy_option::Handler handler = nullptr) {
      return (*this)(std::move(command_line_names), nullptr, handler);
    }

    Options &parent;
  };

 public:
  using Custom_cmdline_handler = std::function<bool(char **argv, int *argi)>;

  static int cmdline_arg_with_value(char **argv, int *argi, const char *arg,
                                    const char *larg, char **value,
                                    bool accept_null = false) noexcept;

  explicit Options(const std::string &config_file = "");
  virtual ~Options() {}

  Options(const Options &) = delete;
  Options &operator=(const Options &) = delete;

  /// Interface for adding options visible through dict like interface.
  Add_named_option_helper add_named_options() {
    return Add_named_option_helper(*this);
  }

  /// Interface for adding options that only appear on command line.
  Add_startup_option_helper add_startup_options() {
    return Add_startup_option_helper(*this);
  }

  void set(const std::string &option_name, const std::string &new_value);

  template <class T>
  const T &get(const std::string &option_name) const {
    auto it = find_option(option_name);
    typename opts::Concrete_option<T> *opt =
        dynamic_cast<typename opts::Concrete_option<T> *>(it->second);
    if (opt == nullptr)
      throw std::invalid_argument(
          option_name +
          " has different type then specified in template argument");
    return opt->get();
  }

  std::string get_value_as_string(const std::string &option_name);

  /// Gets values from environment variables to options.
  void handle_environment_options();

  void save(const std::string &option_name);
  void unsave(const std::string &option_name);

  /// Reads values persisted in configuration file.
  void handle_persisted_options();

  /// Parses command line and stores values in options.
  void handle_cmdline_options(
      int argc, char **argv, bool allow_unregistered_options = true,
      Custom_cmdline_handler custom_cmdline_handler = nullptr);

  /** Returns formatted help for all defined command line options.
   *
   * @param options_width - max space allowed for option names on single line.
   * @param help_width - max space allowed for help message on single line.
   *
   * Option's help will be broken into multiple lines if it exceeds those
   * limits.
   */
  std::vector<std::string> get_cmdline_help(std::size_t options_width = 28,
                                            std::size_t help_width = 50) const;

  std::vector<std::string> get_options_description(
      bool show_origin = false) const;

  std::string get_named_help(const std::string &filter = "",
                             std::size_t output_width = 80,
                             std::size_t front_padding = 0);

 protected:
  struct Command_line_comparator {
    bool operator()(const std::string &lhs, const std::string &rhs) const;
  };

  void add_option(opts::Generic_option *opt);
  NamedOptions::const_iterator find_option(
      const std::string &option_name) const;

  std::vector<std::unique_ptr<opts::Generic_option>> options;
  NamedOptions named_options;
  std::map<std::string, opts::Generic_option *, Command_line_comparator>
      cmdline_options;
  std::string config_file;
};

} /* namespace shcore */

#endif  // MYSQLSHDK_LIBS_UTILS_OPTIONS_H_"
