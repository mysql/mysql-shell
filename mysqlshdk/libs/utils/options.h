/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <cstring>
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
                 const std::string &help);

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

  bool is_value_optional() const { return value_optional; }
  bool accepts_no_cmdline_value() const { return no_cmdline_value; }

  std::vector<std::string> get_cmdline_help(std::size_t options_width,
                                            std::size_t help_width) const;

  const std::string &get_name() const { return name; }

  Source get_source() const { return source; }

  const std::string &get_help() const { return help; }

  virtual void set(const std::string &new_value, Source source) = 0;

 protected:
  const std::string name;
  Source source = Source::Compiled_default;
  const char *environment_variable;
  std::vector<std::string> command_line_names;
  bool value_optional = true;
  bool no_cmdline_value = true;
  const std::string help;
};

template <class T>
class Concrete_option : public Generic_option {
 public:
  using Validator =
      typename std::function<T(const std::string &, Source source)>;

  using Serializer = typename std::function<std::string(const T &)>;

  Concrete_option(T *landing_spot_, T default_value_, const std::string &name_,
                  const char *environment_variable_,
                  std::vector<std::string> &&command_line_names_,
                  const std::string &help_, Validator validator_,
                  Serializer serializer_ = nullptr)
      : Generic_option(
            name_, environment_variable_,
            std::forward<std::vector<std::string> &&>(command_line_names_),
            help_),
        landing_spot(landing_spot_),
        default_value(default_value_),
        validator(validator_),
        serializer(serializer_) {
    assert(validator != nullptr);
    *landing_spot = default_value;
  }

  void set(const std::string &new_value, Source source_) override {
    try {
      *landing_spot = validator(new_value, source_);
      this->source = source_;
    } catch (const std::exception &e) {
      if (name.empty() || source == Source::Command_line ||
          strstr(e.what(), name.c_str()))
        throw std::invalid_argument(e.what());
      throw std::invalid_argument(name + ": " + e.what());
    }
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
      if (value_optional)
        value = "";
      else
        throw std::invalid_argument("Option " + option + " needs value");
    } else if (strcmp(value, "") == 0 && !std::is_same<T, std::string>::value) {
      throw std::invalid_argument("Option " + option +
                                  " does not accept empty string as a value");
    }
    try {
      set(value, Source::Command_line);
    } catch (const std::invalid_argument &e) {
      std::string err_msg(e.what());
      std::string::size_type it = std::string::npos;
      if (!name.empty() && (it = err_msg.find(name)) != std::string::npos)
        err_msg.replace(it, name.length(), option);
      else
        err_msg = option + ": " + err_msg;
      throw std::invalid_argument(err_msg);
    }
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
               std::vector<std::string> &&command_line_names,
               const std::string &help, Handler handler = nullptr,
               const std::string &name = "");

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
T convert(const std::string &data, Source s) {
  // assuming that option specification turns it on
  if (data.empty() && s == Source::Command_line) return static_cast<T>(1);
  T t;
  std::istringstream iss(data);
  iss >> t;
  if (iss.fail()) {
    if (std::is_same<T, bool>::value) {
      iss.clear();
      iss >> std::boolalpha >> t;
    }
    if (iss.fail()) throw std::invalid_argument("Incorrect option value.");
  } else if (!iss.eof()) {
    throw std::invalid_argument("Malformed option value.");
  }
  return t;
}

template <>
std::string convert(const std::string &data, Source);

/// Standard serialization mechanism for options
template <class T>
std::string serialize(const T &val) {
  std::ostringstream os;
  if (std::is_same<T, bool>::value) os << std::boolalpha;
  os << val;
  return os.str();
}

template <>
std::string serialize(const std::string &val);

/// Validator that does some standard type conversion
template <class T>
struct Basic_type {
  T operator()(const std::string &data, Source s) {
    return convert<T>(data, s);
  }
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
  Range(T min_, T max_) : min(min_), max(max_) { assert(max >= min); }

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
        const std::string &help,
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
        std::vector<std::string> &&command_line_names, const std::string &help,
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
        const std::string &help,
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
        std::vector<std::string> &&command_line_names, const std::string &help,
        typename opts::Concrete_option<T>::Validator validator =
            opts::Basic_type<T>()) {
      return (*this)(landing_spot, std::forward<S>(default_value), nullptr,
                     std::move(command_line_names), help, validator);
    }

    template <class T, class S>
    Add_startup_option_helper &operator()(
        T *landing_spot, S &&default_value, const char *envname,
        std::vector<std::string> &&command_line_names, const std::string &help,
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
        const std::string &help,  // full help text
        opts::Proxy_option::Handler handler = nullptr,
        const std::string &name = "") {
      parent.add_option(new opts::Proxy_option(
          nullptr, std::move(command_line_names), help, handler, name));
      return *this;
    }

    // Proxy options - deprecated/secret option not appearing in help.
    Add_startup_option_helper &operator()(
        std::vector<std::string> &&command_line_names,
        opts::Proxy_option::Handler handler = nullptr) {
      return (*this)(std::move(command_line_names), "", handler);
    }

    Options &parent;
  };

 public:
  class Cmdline_iterator {
   public:
    Cmdline_iterator(int argc_, char const *const *argv_, int start)
        : argv(argv_), argc(argc_), current(start) {}

    /// Return current cmdline argument.
    const char *peek() const { return argv[current]; }

    /// Return current cmdline argument and advance to the next.
    const char *get() { return argv[current++]; }

    /// Go back to previous cmdline argument and return it.
    const char *back() { return argv[--current]; }

    /// Check if iterator points to valid cmdline argument.
    bool valid() const { return current < argc && current >= 0; }

    const char *first() const { return argv[0]; }

   private:
    char const *const *argv;
    int argc;
    int current;
  };

  class Iterator {
   public:
    enum class Type {
      VALUE,          /*!< value which does not begin with '-' */
      NO_VALUE,       /*!< --option or -o but not followed with value */
      SEPARATE_VALUE, /*!< --option <value> or -o <value> */
      SHORT,          /*!< -o<value> */
      LONG            /*!< --option=<value> */
    };

    explicit Iterator(const Options::Cmdline_iterator &iterator);

    bool valid() const { return m_iterator.valid(); }

    Type type() const { return m_type; }

    std::string option() const { return m_option; }

    const char *value() const { return m_value; }

    bool has_non_empty_value() const {
      return nullptr != value() && '\0' != value()[0];
    }

    Options::Cmdline_iterator *iterator() { return &m_iterator; }

    /**
     * Moves iterator to the next option without consuming the value.
     *
     * @throws std::logic_error if iterator is not valid
     * @throws std::logic_error if type() is Type::LONG
     */
    void next_no_value();

    /**
     * Moves iterator to the next option.
     *
     * @throws std::logic_error if iterator is not valid
     */
    void next();

   private:
    void get_data();

    void get_separate_value();

    Options::Cmdline_iterator m_iterator;
    std::string m_option;
    const char *m_value;
    Type m_type;
    size_t m_short_value_pos = 0;
  };

  using Custom_cmdline_handler = std::function<bool(Iterator *)>;

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
    typename opts::Concrete_option<T> *opt =
        dynamic_cast<typename opts::Concrete_option<T> *>(
            &get_option(option_name));
    if (opt == nullptr)
      throw std::invalid_argument(
          option_name +
          " has different type then specified in template argument");
    return opt->get();
  }

  std::string get_value_as_string(const std::string &option_name);
  opts::Source get_option_source(const std::string &option_name);

  /// Gets values from environment variables to options.
  void handle_environment_options();

  void save(const std::string &option_name);
  void unsave(const std::string &option_name);

  /// Reads values persisted in configuration file.
  void handle_persisted_options();

  /// Parses command line and stores values in options.
  void handle_cmdline_options(
      int argc, char const *const *argv, bool allow_unregistered_options = true,
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
  opts::Generic_option &get_option(const std::string &option_name) const;

  std::vector<std::unique_ptr<opts::Generic_option>> options;
  NamedOptions named_options;
  std::map<std::string, opts::Generic_option *, Command_line_comparator>
      cmdline_options;
  std::string config_file;
};

} /* namespace shcore */

#endif  // MYSQLSHDK_LIBS_UTILS_OPTIONS_H_"
