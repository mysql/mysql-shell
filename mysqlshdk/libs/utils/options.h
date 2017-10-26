/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_OPTIONS_H_
#define MYSQLSHDK_LIBS_UTILS_OPTIONS_H_

#include <algorithm>
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

namespace opts {

enum class Source {
  Compiled_default,
  Environment_variable,
  Configuration_file,
  Command_line,
  User
};

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

  virtual ~Generic_option() {
  }

  void set(const std::string &new_value) {
    set(new_value, Source::User);
  }

  void handle_environment_variable();

  virtual void handle_command_line_input(const std::string &option,
                                         const char *value) = 0;

  virtual std::vector<std::string> get_cmdline_names();

  bool accepts_null_argument() {
    return accept_null;
  }
  bool accepts_no_cmdline_value() {
    return no_cmdline_value;
  }

  std::vector<std::string> get_cmdline_help(std::size_t options_width,
                                            std::size_t help_width);

  const std::string &get_name() {
    return name;
  }

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

  Concrete_option(T *landing_spot, T default_value, const std::string &name,
                  const char *environment_variable,
                  std::vector<std::string> &&command_line_names,
                  const char *help, Validator validator)
      : Generic_option(
            name, environment_variable,
            std::forward<std::vector<std::string> &&>(command_line_names),
            help),
        landing_spot(landing_spot),
        validator(validator) {
    *landing_spot = default_value;
  }

  void set(const std::string &new_value, Source source) override {
    *landing_spot = validator(new_value, source);
    this->source = source;
  }

  void handle_command_line_input(const std::string &option,
                                 const char *value) override {
    if (value == nullptr) {
      if (accept_null)
        value = "1";
      else
        throw std::invalid_argument("Option " + option + " needs value");
    }
    set(value, Source::Compiled_default);
  }

  const T &get() const {
    return *landing_spot;
  }

 protected:
  T *landing_spot;
  Validator validator;
};

/* This class is supposed to handle special cases.
 *
 * 1) Option that has not concrete storage but rather method/function (handler)
 *    that is supposed to be called when new value is available.
 * 2) Option that is there to only appear in help message, but is handled
 *    outside of normal processing e.g. in Options::Custom_cmdline_handler.
 * 3) Deprecated options (use deprecated function to define them.
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

Proxy_option::Handler deprecated(const char *replacement = nullptr);

template <class T, class S>
Proxy_option::Handler assign_value(T *landing_spot, S value) {
  return [landing_spot, value](const std::string &opt, const char *) {
    *landing_spot = value;
  };
}

template <class T>
T convert(const std::string &data) {
  T t;
  std::istringstream iss(data);
  iss >> t;
  if (iss.fail()) {
    iss.clear();
    iss >> std::boolalpha >> t;
    if (iss.fail())
      throw std::invalid_argument("incorrect value");
  } else if (!iss.eof()) {
    throw std::invalid_argument("malformed value");
  }
  return t;
}

template <>
std::string convert(const std::string &data);

/// Validator that does some standard type conversion
template <class T>
struct Basic_type {
  T operator()(const std::string &data, Source UNUSED(source)) {
    return convert<T>(data);
  }
};

/// Wrapper for validator that prohibits user from setting option value
template <class T>
struct Read_only : public Basic_type<T> {
  T operator()(const std::string &data, Source source) {
    if (source == Source::User)
      throw std::logic_error("read only.");
    return Basic_type<T>::operator()(data, source);
  }
};

/// Extension of Basic_type validator to enable range checks
template <class T>
struct Range : public Basic_type<T> {
  Range(T min, T max) : min(min), max(max) {
  }

  T operator()(const std::string &data, Source source) {
    T t = Basic_type<T>::operator()(data, source);
    if (t < min || t > max)
      throw std::out_of_range("value out of range");
    return t;
  }

  T min;
  T max;
};
}  // namespace opts

class Options {
  struct Add_named_option_helper {
    explicit Add_named_option_helper(Options &options) : parent(options) {
    }

    template <class T, class S>
    Add_named_option_helper &operator()(
        T *landing_spot, S &&default_value, const std::string &optname,
        const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>()) {
      return (*this)(landing_spot, std::forward<S>(default_value), optname,
                     nullptr, opts::cmdline(), help, validator);
    }

    template <class T, class S>
    Add_named_option_helper &operator()(
        T *landing_spot, S &&default_value, const std::string &optname,
        std::vector<std::string> &&command_line_names, const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>()) {
      return (*this)(landing_spot, std::forward<S>(default_value), optname,
                     nullptr, std::move(command_line_names), help, validator);
    }

    template <class T, class S>
    Add_named_option_helper &operator()(
        T *landing_spot, S &&default_value, const std::string &optname,
        const char *envname, std::vector<std::string> &&command_line_names,
        const char *help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>()) {
      parent.add_option(new opts::Concrete_option<T>(
          landing_spot, T(std::forward<S>(default_value)), optname, envname,
          std::move(command_line_names), help, validator));
      return *this;
    }

    Options &parent;
  };

  struct Add_startup_option_helper {
    explicit Add_startup_option_helper(Options &options) : parent(options) {
    }

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
  using Custom_cmdline_handler =
      std::function<bool(const char **argv, int *argi)>;

  static int cmdline_arg_with_value(const char **argv, int *argi,
                                    const char *arg, const char *larg,
                                    const char **value,
                                    bool accept_null = false) noexcept;

  Options(bool allow_unregistered_options = true,
          Custom_cmdline_handler custom_cmdline_handler = nullptr);
  virtual ~Options() {
  }

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

  void set(const std::string &name, const std::string &new_value);

  template <class T>
  const T &get(const std::string &option_name) {
    auto it = named_options.find(option_name);
    if (it == named_options.end())
      throw std::invalid_argument("No option registered under name: " +
                                  option_name);
    typename opts::Concrete_option<T> *opt =
        dynamic_cast<typename opts::Concrete_option<T> *>(it->second);
    if (opt == nullptr)
      throw std::invalid_argument(
          option_name +
          " has different type then specified in template argument");
    return opt->get();
  }

  /// Gets values from environment variables to options.
  void handle_environment_options();

  /// Parses command line and stores values in options.
  void handle_cmdline_options(int argc, const char **argv);

  /** Returns formatted help for all defined command line options.
   *
   * @param options_width - max space allowed for option names on single line.
   * @param help_width - max space allowed for help message on single line.
   *
   * Option's help will be broken into multiple lines if it exceeds those limits.
   */
  std::vector<std::string> get_cmdline_help(std::size_t options_width = 28,
                                            std::size_t help_width = 50);

 protected:
  struct Command_line_comparator {
    bool operator()(const std::string &lhs, const std::string &rhs) const;
  };

  void add_option(opts::Generic_option *opt);

  std::vector<std::unique_ptr<opts::Generic_option> > options;
  std::map<std::string, opts::Generic_option *> named_options;
  std::map<std::string, opts::Generic_option *, Command_line_comparator>
      cmdline_options;
  bool allow_unregistered_options = true;
  Custom_cmdline_handler custom_cmdline_handler;
};

} /* namespace shcore */

#endif  // MYSQLSHDK_LIBS_UTILS_OPTIONS_H_"
