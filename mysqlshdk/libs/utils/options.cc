/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include <rapidjson/error/en.h>
#include <iostream>

#include "mysqlshdk/libs/textui/textui.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace textui = mysqlshdk::textui;

namespace shcore {

namespace opts {

std::string to_string(Source s) {
  switch (s) {
    case Source::Command_line:
      return "Command line";
    case Source::Compiled_default:
      return "Compiled default";
    case Source::Configuration_file:
      return "Configuration file";
    case Source::Environment_variable:
      return "Environment variable";
    case Source::User:
      return "User defined";
  }
  return "";
}

Generic_option::Generic_option(const std::string &name,
                               const char *environment_variable,
                               std::vector<std::string> &&command_line_names,
                               const char *help)
    : name(name),
      environment_variable(environment_variable),
      command_line_names(std::move(command_line_names)),
      help(help) {}

void Generic_option::handle_environment_variable() {
  if (environment_variable != nullptr) {
    const char *val = getenv(environment_variable);
    if (val != nullptr) set(val, Source::Environment_variable);
  }
}

void Generic_option::handle_persisted_value(const char *value) {
  if (value != nullptr) set(value, Source::Configuration_file);
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
  if (command_line_names.empty() || help == nullptr) return result;

  for (const auto &n : command_line_names)
    if (n.length() >= options_width)
      throw std::runtime_error(str_format(
          "Command line option %s too wide for help options_width=%zu",
          n.c_str(), options_width));

  std::string line = command_line_names[0];
  for (std::size_t i = 1; i < command_line_names.size(); i++)
    if (line.length() + 2 + command_line_names[i].length() < options_width) {
      line += ", " + command_line_names[i];
    } else {
      line.insert(line.end(), options_width - line.length(), ' ');
      result.push_back(line);
      line = command_line_names[i];
    }
  line.insert(line.end(), options_width - line.length(), ' ');
  result.push_back(line);

  std::size_t help_filled = 1;
  if (std::strlen(help) <= help_width) {
    result[0] += help;
  } else {
    std::vector<std::string> help_lines =
        shcore::str_break_into_lines(help, help_width);
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
  if (handler == nullptr) return {};
  return Generic_option::get_cmdline_names();
}

bool icomp(const std::string &lhs, const std::string &rhs) {
  return shcore::str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

/**
 * Deprecation handler, to be used for any option being deprecated.
 * @param replacement: Replacement option name if any.
 * @param target: function handler of new option if any.
 * @param def: default value to be set if no value is provided
 * @param map: mapping of old option values to new option values.
 *
 * This function does the deprecation handling, it supports:
 * - Ignoring an option (tho is not desired or it would be better removed)
 * - Renaming an option, for the case a new option replaced the deprecated one
 * - Inserting default value if none specified, in case the deprecated one did
 *   this and the new option does not
 * - Mapping old values to new values, in case the deprecated option uses
 *   different values than new option.
 *
 * For each deprecated option used, a warning will be generated
 *
 */
Proxy_option::Handler deprecated(
    const char *replacement, Proxy_option::Handler target, const char *def,
    const std::map<std::string, std::string> &map) {
  return [replacement, target, def, map](const std::string &opt,
                                         const char *value) {
    std::stringstream ss;
    ss << "The " << opt << " option has been deprecated";
    if (replacement != nullptr)
      ss << ", please use " << replacement << " instead";

    ss << ".";

    if (target) {
      std::string final_val;

      // A value is provided, let's see if a value mapping exists
      if (value) {
        if (!map.empty()) {
          std::map<std::string, std::string,
                   bool (*)(const std::string &, const std::string &)>
              imap(icomp);

          for (const auto &element : map) {
            imap.emplace(element.first, element.second);
          }

          if (imap.find(value) != imap.end())
            final_val = imap.at(value);
          else
            final_val.assign(value);
        } else {
          final_val.assign(value);
        }
      } else {
        // If no value is provided and a default value is, it gets used
        if (def) final_val.assign(def);
      }

      if (replacement) {
        ss << " (Option has been processed as " << replacement;
        if (!final_val.empty()) ss << "=" << final_val;
        ss << ").";
      }

      std::cout << "WARNING: " << ss.str() << std::endl;
      target(replacement ? replacement : opt,
             (value || def) ? final_val.c_str() : value);
    } else {
      ss << " (Option has been ignored).";
      std::cout << "WARNING: " << ss.str() << std::endl;
    }
  };
}

template <>
std::string convert(const std::string &data) {
  return data;
}

template <>
std::string serialize(const std::string &val) {
  return val;
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
// They behave differently:
// --option <value> can take the default argument (def) if def has data and
// value is missing, error if missing and def is not defined
// --option=[value] will get NULL value if value is missing and can
// accept_null(i.e. --option=)
//
// ReturnValue: Returns the # of format found based on the list above, or 0 if
// no valid value was found
Options::Format Options::cmdline_arg_with_value(char **argv, int *argi,
                                                const char *arg,
                                                const char *larg, char **value,
                                                bool accept_null) noexcept {
  Format ret_val = Format::INVALID;
  *value = nullptr;

  if (strcmp(argv[*argi], arg) == 0 ||
      (larg && strcmp(argv[*argi], larg) == 0)) {
    // --option [value] or -o [value]
    ret_val = Format::SEPARATE_VALUE;

    // value can be in next arg and can't start with - which indicates next
    // option
    if (argv[*argi + 1] != NULL && strncmp(argv[*argi + 1], "-", 1) != 0) {
      ++(*argi);
      *value = argv[*argi];
    }
  } else if (larg && strncmp(argv[*argi], larg, strlen(larg)) == 0 &&
             strlen(argv[*argi]) > strlen(larg)) {
    // -o<value>
    ret_val = Format::SHORT;
    *value = argv[*argi] + strlen(larg);
  } else if (strncmp(argv[*argi], arg, strlen(arg)) == 0 &&
             argv[*argi][strlen(arg)] == '=') {
    // --option=[value]
    ret_val = Format::LONG;

    // Value was specified
    if (strlen(argv[*argi]) > (strlen(arg) + 1))
      *value = argv[*argi] + strlen(arg) + 1;
  }

  // Option requires an argument
  if (Format::INVALID != ret_val && (!*value || !**value) && !accept_null) {
    ret_val = Format::MISSING_VALUE;
  }

  return ret_val;
}

Options::Options(const std::string &config_file) : config_file(config_file) {}

void Options::set(const std::string &option_name,
                  const std::string &new_value) {
  auto it = find_option(option_name);
  it->second->set(new_value);
}

std::string Options::get_value_as_string(const std::string &option_name) {
  auto it = find_option(option_name);
  return it->second->get_value_as_string();
}

void Options::handle_environment_options() {
  for (auto &opt : options) opt->handle_environment_variable();
}

static std::unique_ptr<rapidjson::Document> read_json_from_file(
    const std::string &filename) {
  if (filename.empty())
    throw std::runtime_error("Configuration file was not specified");

  std::unique_ptr<rapidjson::Document> d(new rapidjson::Document());

  std::ifstream f(filename, std::ios::binary);
  if (f.is_open()) {
    std::stringstream buffer;
    buffer << f.rdbuf();
    d->Parse(buffer.str().c_str());
    if (d->HasParseError())
      throw std::runtime_error("Error while parsing configuration file '" +
                               filename + "': ." +
                               rapidjson::GetParseError_En(d->GetParseError()));

  } else if (errno != ENOENT) {
    throw std::runtime_error("Unable to open configuration file " + filename +
                             "for reading: " + strerror(errno));
  }

  if (!d->IsObject()) d->SetObject();

  return d;
}

static void write_json_to_file(const std::string &filename,
                               rapidjson::Document *d) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  d->Accept(writer);

  // in case of a tragedy
  std::rename(filename.c_str(), (filename + ".old").c_str());
  std::ofstream f(filename, std::ios::binary);
  if (f.is_open()) {
    f.write(buffer.GetString(), buffer.GetSize());
    if (f.fail())
      throw std::runtime_error(std::string("Error '") + strerror(errno) +
                               "' while writing to file " + filename +
                               ", old file contents is available as file " +
                               filename + ".old");
    else
      f.flush();
  } else {
    throw std::runtime_error("Unable to open file " + filename +
                             " for writing" + ": " + strerror(errno));
  }
}

void Options::save(const std::string &option_name) {
  std::string opval = get_value_as_string(option_name);
  std::unique_ptr<rapidjson::Document> d = read_json_from_file(config_file);
  rapidjson::Value new_value(opval.c_str(), opval.length(), d->GetAllocator());
  auto it = d->FindMember(option_name.c_str());
  if (it != d->MemberEnd())
    it->value = new_value;
  else
    d->AddMember(
        rapidjson::StringRef(option_name.c_str(), option_name.length()),
        new_value, d->GetAllocator());
  write_json_to_file(config_file, d.get());
}

void Options::unsave(const std::string &option_name) {
  find_option(option_name);
  std::unique_ptr<rapidjson::Document> d = read_json_from_file(config_file);
  if (!d->RemoveMember(option_name.c_str()))
    throw std::invalid_argument("Option " + option_name +
                                " not present in file " + config_file);
  write_json_to_file(config_file, d.get());
}

void Options::handle_persisted_options() {
  using rapidjson::Value;
  std::unique_ptr<rapidjson::Document> d = read_json_from_file(config_file);
  for (Value::ConstMemberIterator it = d->MemberBegin(); it != d->MemberEnd();
       ++it) {
    std::string opt_name(it->name.GetString());
    auto op = named_options.find(opt_name);
    if (op == named_options.end())
      throw std::runtime_error("Unrecognized option: " + opt_name +
                               " in configuration file: " + config_file);
    if (!it->value.IsString())
      throw std::runtime_error("Malformed value for option '" + opt_name +
                               "' in configuration file. All values need to be "
                               "represented as strings.");
    try {
      op->second->handle_persisted_value(it->value.GetString());
    } catch (const std::invalid_argument &e) {
      throw std::invalid_argument(
          "Error while reading configuration file, option " + opt_name + ": " +
          e.what());
    }
  }
}

void Options::handle_cmdline_options(
    int argc, char **argv, bool allow_unregistered_options,
    Custom_cmdline_handler custom_cmdline_handler) {
  for (int i = 1; i < argc; i++) {
    if (custom_cmdline_handler && custom_cmdline_handler(argv, &i)) continue;

    std::string opt(argv[i]);
    std::size_t epos = opt.find('=');

    if (epos != std::string::npos) {
      opt = opt.substr(0, epos);
    }

    auto it = cmdline_options.find(opt);

    if (it != cmdline_options.end()) {
      if (it->second->accepts_no_cmdline_value()) {
        if (epos == std::string::npos) {
          it->second->handle_command_line_input(opt, nullptr);
        } else {
          throw std::invalid_argument(std::string(argv[0]) + ": option " + opt +
                                      " does not require an argument");
        }
      } else {
        const std::string &cmd = it->first;
        char *value = nullptr;
        if (Format::MISSING_VALUE !=
            cmdline_arg_with_value(argv, &i, cmd.c_str(),
                                   cmd[1] == '-' ? nullptr : cmd.c_str(),
                                   &value, it->second->accepts_null_argument()))
          it->second->handle_command_line_input(cmd, value);
        else
          throw std::invalid_argument(std::string(argv[0]) + ": option " + opt +
                                      " requires an argument");
      }
    } else if (!allow_unregistered_options) {
      throw std::invalid_argument(argv[0] + std::string(": unknown option ") +
                                  opt);
    }
  }
}

std::vector<std::string> Options::get_cmdline_help(
    std::size_t options_width, std::size_t help_width) const {
  std::vector<std::string> result;
  for (const auto &opt : options) {
    auto oh = opt->get_cmdline_help(options_width, help_width);
    result.insert(result.end(), oh.begin(), oh.end());
  }

  return result;
}

std::vector<std::string> Options::get_options_description(
    bool show_origin) const {
  std::vector<std::string> res;
  std::size_t first_column_width = 0;
  for (const auto &it : named_options) {
    if (first_column_width < it.first.length())
      first_column_width = it.first.length();
  }
  first_column_width += 2;

  for (auto &it : named_options) {
    opts::Generic_option *opt = it.second;
    std::stringstream ss;
    ss << opt->get_name();
    for (std::size_t i = opt->get_name().length(); i < first_column_width; i++)
      ss.put(' ');
    ss << opt->get_value_as_string();
    if (show_origin) ss << " (" << to_string(opt->get_source()) << ")";
    res.emplace_back(ss.str());
  }
  return res;
}

std::string Options::get_named_help(const std::string &filter,
                                    std::size_t output_width,
                                    std::size_t padding) {
  std::string res;

  std::size_t first_column_width = 0;
  for (const auto &it : named_options) {
    if (!filter.empty() && it.first.find(filter) == std::string::npos) continue;
    if (first_column_width < it.first.length())
      first_column_width = it.first.length();
  }
  first_column_width += 2 + padding;
  assert(first_column_width < output_width - 20);
  std::size_t second_column_width = output_width - first_column_width;

  for (const auto &it : named_options) {
    if (!filter.empty() && it.first.find(filter) == std::string::npos) continue;
    opts::Generic_option *opt = it.second;
    const char *help = opt->get_help();
    std::string first_column(first_column_width, ' ');
    first_column.replace(padding, opt->get_name().length(), opt->get_name());
    if (std::strlen(help) <= second_column_width) {
      res.append(first_column + help + "\n");
    } else {
      std::string final = textui::format_markup_text(
          {help}, second_column_width + first_column_width, first_column_width,
          false);
      final.replace(0, first_column_width, first_column);
      res.append(final + "\n");
    }
  }
  return res;
}

bool Options::Command_line_comparator::operator()(
    const std::string &lhs, const std::string &rhs) const {
  // in case of single '-' parameters we have to consider -name<value> format
  if (lhs[1] != '-' && rhs[1] != '-') {
    if (lhs.length() < rhs.length()) return rhs.compare(0, lhs.size(), lhs) > 0;
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

Options::NamedOptions::const_iterator Options::find_option(
    const std::string &option_name) const {
  auto op = named_options.find(option_name);
  if (op == named_options.end())
    throw std::invalid_argument("Unrecognized option: " + option_name + ".");
  return op;
}

} /* namespace shcore */
