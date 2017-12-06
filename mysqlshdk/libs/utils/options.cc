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

#include "mysqlshdk/libs/utils/options.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace shcore {

namespace opts {

Generic_option::Generic_option(const std::string &name,
                               const char *environment_variable,
                               std::vector<std::string> &&command_line_names,
                               const char *help)
    : name(name),
      environment_variable(environment_variable),
      command_line_names(std::move(command_line_names)),
      help(help) {
}

void Generic_option::handle_environment_variable() {
  if (environment_variable != nullptr) {
    const char *val = getenv(environment_variable);
    if (val != nullptr)
      set(val, Source::Environment_variable);
  }
}

std::vector<std::string> Generic_option::get_cmdline_names() {
  std::vector<std::string> res;
  for (const std::string &cmd_name : command_line_names) {
    std::size_t pos = cmd_name.find('=');
    if (pos != std::string::npos) {
      no_cmdline_value = false;
      if (pos != 0 && cmd_name[pos - 1] == '[')
        --pos;
      else
        accept_null = false;
      res.push_back(cmd_name.substr(0, pos));
    } else {
      res.push_back(cmd_name);
    }
  }
  return res;
}

std::vector<std::string> Generic_option::get_cmdline_help(
    std::size_t options_width, std::size_t help_width) const {
  std::vector<std::string> result;
  if (command_line_names.empty() || help == nullptr)
    return result;

  for (const auto &n : command_line_names)
    if (n.length() >= options_width)
      throw std::runtime_error(str_format(
          "Command line option %s too wide for help options_width=%zu",
          n.c_str(), options_width));

  std::string line = command_line_names[0];
  for (std::size_t i = 1; i < command_line_names.size(); i++)
    if (line.length() + 2 + command_line_names[i].length() <= options_width) {
      line += ", " + command_line_names[i];
    } else {
      line.insert(line.end(), options_width - line.length(), ' ');
      result.push_back(line);
      line = command_line_names[i];
    }
  line.insert(line.end(), options_width - line.length(), ' ');
  result.push_back(line);

  std::size_t help_filled = 1;
  if (strlen(help) <= help_width) {
    result[0] += help;
  } else {
    std::vector<std::string> help_lines;
    std::string rem(help);
    while (rem.length() > help_width) {
      std::size_t split_point = help_width - 1;
      while (rem[split_point] != ' ') {
        --split_point;
        assert(split_point > 0);
      }
      help_lines.push_back(rem.substr(0, split_point));
      rem = rem.substr(split_point + 1);
    }
    help_lines.push_back(rem);
    for (std::size_t i = 0; i < help_lines.size(); ++i)
      if (i < result.size())
        result[i] += help_lines[i];
      else
        result.push_back(help_lines[i].insert(0, options_width, ' '));
    help_filled = help_lines.size();
  }

  for (; help_filled < result.size(); help_filled++)
    result[help_filled] += "see above";

  return result;
}

Proxy_option::Proxy_option(const char *environment_variable,
                           std::vector<std::string> &&command_line_names,
                           const char *help, Handler handler)
    : Generic_option("", environment_variable, std::move(command_line_names),
                     help),
      handler(handler) {
  assert(environment_variable == nullptr || handler != nullptr);
}

void Proxy_option::set(const std::string &new_value, Source source) {
  assert(handler != nullptr);
  handler("", new_value.c_str());
  this->source = source;
}

void Proxy_option::handle_command_line_input(const std::string &option,
                                             const char *value) {
  assert(handler != nullptr);
  handler(option, value);
  source = Source::Command_line;
}

std::vector<std::string> Proxy_option::get_cmdline_names() {
  if (handler == nullptr)
    return {};
  return Generic_option::get_cmdline_names();
}

Proxy_option::Handler deprecated(const char *replacement) {
  return [replacement](const std::string &opt, const char *) {
    std::stringstream ss;
    ss << "The " << opt << " option has been deprecated";
    if (replacement != nullptr)
      ss << ", please use " << replacement << " instead.";
    throw std::invalid_argument(ss.str());
  };
}

template <>
std::string convert(const std::string &data) {
  return data;
}
}  // namespace opts

// Will verify if the argument is the one specified by arg or larg and return
// its associated value
// The function has different behaviors:
//
// Options come in three flavors
// 1) --option [value] or -o [value]
// 2) --option or -o
// 3) --option=[value]
//
// They behavie differently:
// --option <value> can take the default argumemt (def) if def has data and
// value is missing, error if missing and def is not defined
// --option=[value] will get NULL value if value is missing and can
// accept_null(i.e. --option=)
//
// ReturnValue: Returns the # of format found based on the list above, or 0 if
// no valid value was found
int Options::cmdline_arg_with_value(char **argv, int *argi, const char *arg,
                                    const char *larg, char **value,
                                    bool accept_null) noexcept {
  int ret_val = 0;
  *value = nullptr;

  if (strcmp(argv[*argi], arg) == 0 ||
      (larg && strcmp(argv[*argi], larg) == 0)) {
    // --option [value] or -o [value]
    ret_val = 1;

    // value can be in next arg and can't start with - which indicates next
    // option
    if (argv[*argi + 1] != NULL && strncmp(argv[*argi + 1], "-", 1) != 0) {
      ++(*argi);
      *value = argv[*argi];
    }
  } else if (larg && strncmp(argv[*argi], larg, strlen(larg)) == 0 &&
             strlen(argv[*argi]) > strlen(larg)) {
    // -o<value>
    ret_val = 2;
    *value = argv[*argi] + strlen(larg);
  } else if (strncmp(argv[*argi], arg, strlen(arg)) == 0 &&
             argv[*argi][strlen(arg)] == '=') {
    // --option=[value]
    ret_val = 3;

    // Value was specified
    if (strlen(argv[*argi]) > (strlen(arg) + 1))
      *value = argv[*argi] + strlen(arg) + 1;
  }

  // Option requires an argument
  if (ret_val && !*value && !accept_null)
    ret_val = 0;

  return ret_val;
}

Options::Options(bool allow_unregistered_options,
                 Custom_cmdline_handler custom_cmdline_handler)
    : allow_unregistered_options(allow_unregistered_options),
      custom_cmdline_handler(custom_cmdline_handler) {
}

void Options::set(const std::string &name, const std::string &new_value) {
  auto it = named_options.find(name);
  if (it == named_options.end())
    throw std::invalid_argument("No option defined under name: " + name);
  it->second->set(new_value);
}

void Options::handle_environment_options() {
  for (auto &opt : options)
    opt->handle_environment_variable();
}

void Options::handle_cmdline_options(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (custom_cmdline_handler && custom_cmdline_handler(argv, &i))
      continue;

    std::string opt(argv[i]);
    std::size_t epos = opt.find('=');
    auto it = cmdline_options.find(
        epos == std::string::npos ? opt : opt.substr(0, epos));
    if (it != cmdline_options.end()) {
      if (it->second->accepts_no_cmdline_value()) {
        if (epos != std::string::npos)
          throw std::invalid_argument(std::string(argv[0]) + ": option " +
                                      it->first + " expects no argument.");
        it->second->handle_command_line_input(opt, nullptr);
      } else {
        const std::string &cmd = it->first;
        char *value = nullptr;
        if (it->second->accepts_no_cmdline_value() ||
            cmdline_arg_with_value(argv, &i, cmd.c_str(),
                                   cmd[1] == '-' ? nullptr : cmd.c_str(),
                                   &value, it->second->accepts_null_argument()))
          it->second->handle_command_line_input(cmd, value);
        else
          throw std::invalid_argument(std::string(argv[0]) + ": option " +
                                      argv[i] + " requires an argument");
      }
    } else if (!allow_unregistered_options) {
      throw std::invalid_argument(argv[0] + std::string(": unknown option ") +
                                  argv[i]);
    }
  }
}

std::vector<std::string> Options::get_cmdline_help(
    std::size_t options_width, std::size_t help_width) const {
  std::vector<std::string> result;
  for (auto &opt : options) {
    auto oh = opt->get_cmdline_help(options_width, help_width);
    result.insert(result.end(), oh.begin(), oh.end());
  }

  return result;
}

bool Options::Command_line_comparator::operator()(
    const std::string &lhs, const std::string &rhs) const {
  // in case of single '-' parameters we have to consider -name<value> format
  if (lhs[1] != '-' && rhs[1] != '-') {
    if (lhs.length() < rhs.length())
      return rhs.compare(0, lhs.size(), lhs) > 0;
    if (lhs.length() > rhs.length())
      return lhs.compare(0, rhs.length(), rhs) < 0;
  }
  return lhs < rhs;
}

using opts::Generic_option;

void Options::add_option(Generic_option *option) {
  std::unique_ptr<Generic_option> opt(option);

  auto cmdline_names = opt->get_cmdline_names();
  for (const auto &cmd_name : cmdline_names) {
    auto it = cmdline_options.find(cmd_name);
    if (it != cmdline_options.end())
      throw std::invalid_argument(
          "Error while adding option " + opt->get_name() +
          " its cmdline name '" + cmd_name +
          "' clashes with option: " + it->second->get_name());
  }

  if (opt->get_name().length() != 0) {
    auto it = named_options.find(opt->get_name());
    if (it != named_options.end())
      throw std::invalid_argument(opt->get_name() + ": option already defined");
    named_options.emplace_hint(it, opt->get_name(), opt.get());
  }

  for (const auto &cmd_name : cmdline_names)
    cmdline_options.emplace(cmd_name, opt.get());

  options.emplace_back(std::move(opt));
}

} /* namespace shcore */
