/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/config/config_file.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "my_config.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace config {
Config_file::Config_file(Case group_case, Escape escape)
    : m_group_case(group_case),
      m_escape_characters(escape),
      m_configmap(m_group_case == Case::SENSITIVE ? Config_file::comp
                                                  : Config_file::icomp) {}

bool Config_file::comp(const std::string &lhs, const std::string &rhs) {
  return lhs.compare(rhs) < 0;
}

bool Config_file::icomp(const std::string &lhs, const std::string &rhs) {
  return shcore::str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

Config_file::Option_key Config_file::convert_option_to_key(
    const std::string &option) const {
  Option_key opt_key;
  opt_key.original_option = option;
  opt_key.key = option;

  // Dash (-) and underscore (_) may be used interchangeably
  std::replace(opt_key.key.begin(), opt_key.key.end(), '-', '_');

  // Ignore loose_ prefix
  if (shcore::str_ibeginswith(opt_key.key, "loose_")) {
    opt_key.key = opt_key.key.substr(6);
  } else if (shcore::str_ibeginswith(opt_key.key, "group_replication_")) {
    // if it doesn't start with loose_ and is a group_replication option,
    // prepend the original_option (the one written to the file) with loose_.
    opt_key.original_option = "loose_" + opt_key.original_option;
  }
  return opt_key;
}

void Config_file::get_include_files(const std::string &cnf_path,
                                    std::set<std::string> *out_files,
                                    unsigned int recursive_depth) const {
  // 10 is the max recursion supported on the server when parsing option files
  if (recursive_depth >= 10) {
    log_warning(
        "Skipping option file '%s' when searching for include_files because "
        "maximum depth level was reached.",
        cnf_path.c_str());
    return;  // exit function without doing anything.
  }

  // Verify that the file can be read.
  std::ifstream input_file(cnf_path);
  if (!input_file.good()) {
    throw std::runtime_error("Cannot open file: " + cnf_path);
  }

  // Add the file itself to the resulting list of included files.
  out_files->insert(cnf_path);

  std::string line;
  unsigned int linenum = 0;
  while (std::getline(input_file, line)) {
    ++linenum;
    // Trim line (remove whitespaces)
    std::string trimmed_line = shcore::str_strip(line);

    // Skip empty line (only with whitespaces)
    if (trimmed_line.empty()) continue;

    // Handle line with directives.
    if (trimmed_line[0] == '!') {
      if (shcore::str_beginswith(trimmed_line, "!include ")) {
        std::string include_file_path =
            parse_include_path(trimmed_line, cnf_path);

        // Recursively process the included file if not already included.
        if (out_files->find(include_file_path) == out_files->end()) {
          get_include_files(include_file_path, out_files, recursive_depth + 1);
        }
      } else if (shcore::str_beginswith(trimmed_line, "!includedir ")) {
        std::string include_dir_path =
            parse_include_path(trimmed_line, cnf_path);

        std::vector<std::string> files_to_include =
            get_config_files_in_dir(include_dir_path);

        // Recursively process valid files in directory if not already
        // included.
        for (const std::string &include_file_path : files_to_include) {
          if (out_files->find(include_file_path) == out_files->end()) {
            get_include_files(include_file_path, out_files,
                              recursive_depth + 1);
          }
        }
      } else {
        throw std::runtime_error("Invalid directive at line " +
                                 std::to_string(linenum) + " of file '" +
                                 cnf_path + "': " + line + ".");
      }
    }
  }
}

std::string Config_file::parse_group_line(const std::string &line,
                                          unsigned int line_number,
                                          const std::string &cnf_path) const {
  std::string res;
  if (line[0] == '[') {
    auto pos = line.find(']');
    if (pos == std::string::npos) {
      // Malformed group if it does not end with ']'
      throw std::runtime_error("Invalid group, not ending with ']', at line " +
                               std::to_string(line_number) + " of file '" +
                               cnf_path + "': " + line + ".");
    } else {
      // found a closing bracket, make sure that the next character is a
      // comment otherwise throw error
      std::string group = line.substr(1, pos - 1);
      std::string section_rest = shcore::str_strip(line.substr(pos + 1));
      if (!section_rest.empty() && section_rest[0] != '#') {
        throw std::runtime_error("Invalid group at line " +
                                 std::to_string(line_number) + " of file '" +
                                 cnf_path + "': " + line + ".");
      } else {
        // everything ok
        res = group;
      }
    }
  } else {
    // could not find open bracket
    throw std::runtime_error("Invalid group, not starting with '[' at line " +
                             std::to_string(line_number) + " of file '" +
                             cnf_path + "': " + line + ".");
  }
  return res;
}

std::string Config_file::parse_escape_sequence_from_file(
    const std::string &input, const char quote) const {
  if (m_escape_characters == Escape::NO) {
    return input;
  }

  std::string result;
  char escape = '\0';
  for (char c : input) {
    if (escape == '\\') {
      if (c == quote) {
        result.push_back(quote);
      } else if (c == '\\') {
        result.push_back('\\');
      } else if (c == 'n') {
        result.push_back('\n');
      } else if (c == 't') {
        result.push_back('\t');
      } else if (c == 'b') {
        result.push_back('\b');
      } else if (c == 'r') {
        result.push_back('\r');
      } else if (c == 's') {
        result.push_back(' ');
      } else {
        result.push_back('\\');
        result.push_back(c);
      }
      escape = '\0';
    } else {
      if (c != '\\') result.push_back(c);
      escape = c;
    }
  }
  return result;
}

std::string Config_file::parse_escape_sequence_to_file(const std::string &input,
                                                       const char quote) const {
  if (m_escape_characters == Escape::NO) {
    return input;
  }

  std::string result;
  for (char c : input) {
    if (c == quote) {
      result.push_back('\\');
      result.push_back(quote);
    } else if (c == '\\') {
      result.append("\\\\");
    } else if (c == '\n') {
      result.append("\\n");
    } else if (c == '\t') {
      result.append("\\t");
    } else if (c == '\b') {
      result.append("\\b");
    } else if (c == '\r') {
      result.append("\\r");
    } else {
      result.push_back(c);
    }
  }
  return result;
}

std::string Config_file::convert_value_for_file(
    const std::string &value) const {
  if (value.find_first_of("# '\"") != std::string::npos) {
    if (value.find('\'') == std::string::npos) {
      return "'" + parse_escape_sequence_to_file(value) + "'";
    } else {
      return "\"" + parse_escape_sequence_to_file(value, '"') + "\"";
    }
  } else {
    return parse_escape_sequence_to_file(value);
  }
}

std::tuple<std::string, utils::nullable<std::string>, std::string>
Config_file::parse_option_line(const std::string &line,
                               unsigned int line_number,
                               const std::string &cnf_path) const {
  std::string value, option, after_value;

  std::string::size_type pos_equal = line.find('=');
  if (pos_equal != std::string::npos) {
    // Equal sign found
    option = shcore::str_strip(line.substr(0, pos_equal));

    std::string raw_value = shcore::str_strip(line.substr(pos_equal + 1));
    if (!raw_value.empty()) {
      char quote = '\0';
      // Check if value is enclosed with single or double quotes
      if (raw_value[0] == '\'' || raw_value[0] == '"') quote = raw_value[0];
      if (quote) {
        // Value starts with a quote char, check if has valid closing quote
        std::pair<std::string::size_type, std::string::size_type> quote_pos =
            shcore::get_quote_span(quote, raw_value);
        if (quote_pos.second == std::string::npos) {
          // Error if there is no matching closing quote
          throw std::runtime_error(
              "Invalid option, missing closing quote for option value at "
              "line " +
              std::to_string(line_number) + " of file '" + cnf_path +
              "': " + line + ".");
        } else {
          // Value enclosed with quotes
          // Value can only be followed by a comment, otherwise issue error
          std::string value_rest =
              shcore::str_strip(raw_value.substr(quote_pos.second + 1));
          // NOTE: Only # is used for comments in the middle of lines
          if (!value_rest.empty() && value_rest[0] != '#') {
            throw std::runtime_error(
                "Invalid option, only comments (started with #) are allowed "
                "after a quoted value at line " +
                std::to_string(line_number) + " of file '" + cnf_path +
                "': " + line + ".");
          } else {
            // Valid quoted value found (no invalid characters after value)
            value = raw_value.substr(quote_pos.first + 1,
                                     quote_pos.second - quote_pos.first - 1);
            // Keep any text after the value (e.g., inline comment)
            if (raw_value.size() > quote_pos.second + 1) {
              after_value = raw_value.substr(quote_pos.second + 1);
            }
            return std::make_tuple(
                option, parse_escape_sequence_from_file(value, quote),
                after_value);
          }
        }
      } else {
        // No quotes found, value is whatever is found until the comment
        // character # or end of line.
        std::string::size_type comment_pos = raw_value.find('#');
        value = shcore::str_strip(raw_value.substr(0, comment_pos));

        if (comment_pos != std::string::npos &&
            raw_value.size() >= comment_pos + 1) {
          after_value = " " + raw_value.substr(comment_pos);
        }

        return std::make_tuple(
            option, parse_escape_sequence_from_file(value, '\0'), after_value);
      }
    } else {
      // No value after '=' sign
      // NOTE: return empty string for the value which is different from NULL
      return std::make_tuple(option, utils::nullable<std::string>(""),
                             after_value);
    }
  } else {
    // No equal sign found, meaning no value (null)
    // Name is whatever is found until the comment character # or end of line
    // NOTE: Only # is used for comments in the middle of lines
    std::string::size_type comment_pos = line.find('#');
    option = shcore::str_strip(line.substr(0, comment_pos));

    if (comment_pos != std::string::npos && line.size() >= comment_pos + 1) {
      after_value = " " + line.substr(comment_pos);
    }

    // NOTE: return NULL for the value which is different from empty string
    return std::make_tuple(option, utils::nullable<std::string>(), after_value);
  }
}

void Config_file::read(const std::string &cnf_path,
                       const std::string &group_prefix) {
  container configmap(m_group_case == Case::SENSITIVE ? comp : icomp);
  read_recursive_aux(cnf_path, 0, &configmap, group_prefix);
  // if no exception happened during the read of the files, then replace the
  // current configuration map with the new one.
  m_configmap = std::move(configmap);
}

void Config_file::write(const std::string &cnf_path) const {
  // Check if it is possible to write to the file (whatever it exists or not).
  shcore::check_file_writable_or_throw(cnf_path);

  std::set<std::string> cnf_files_to_write;
  bool update_file = false;
  if (shcore::is_file(cnf_path)) {
    // Update an existing file.
    // Get list of files considering any existing include directives
    get_include_files(cnf_path, &cnf_files_to_write, 0);
    update_file = true;
  } else {
    // Create a new file
    cnf_files_to_write.insert(cnf_path);
  }

  // Create a copy of the configurations map to track options that need
  // to be added to the file.
  auto track_cfg_to_add = m_configmap;
  std::vector<std::pair<std::string, std::string>> files_to_copy;

  bool in_include_file = false;
  for (const std::string &file_path : cnf_files_to_write) {
    // Create temporary file to write to. Do not update the file directly to
    // keep it unchanged in case an error occurs. NOTE: temporary file cannot
    // exist already.
    std::string tmp_file_path = shcore::get_tempfile_path(file_path);

    // Open stream to write configurations (to temp file).
    std::ofstream tmp_file(tmp_file_path);
    if (!tmp_file.good()) {
      throw std::ios::failure{"Cannot write to file: " + tmp_file_path + "."};
    }

    // If the target file exist, only updated (added or removed) options must
    // be changed on the file, keeping other file contents (e.g., comments).
    if (update_file) {
      std::ifstream cfg_file(file_path);
      if (!cfg_file.good()) {
        throw std::ios::failure{"Cannot open file: " + file_path + "."};
      }

      std::string line, in_group;
      unsigned int linenum = 0;
      bool delete_group = false;
      while (std::getline(cfg_file, line)) {
        ++linenum;
        // Trim line (remove whitespaces)
        std::string trimmed_line = shcore::str_strip(line);

        if (trimmed_line.empty() || trimmed_line[0] == '#' ||
            trimmed_line[0] == ';') {
          // Print line as it is if it is empty or comment and not in a group
          // to delete.
          if (!delete_group) tmp_file << line << std::endl;
          continue;
        } else if (trimmed_line[0] == '!') {
          // Keep all directives (print as it is, even if in a group to
          // delete).
          tmp_file << line << std::endl;
          continue;
        } else if (!trimmed_line.empty() && trimmed_line[0] == '[') {
          // Parse group line
          std::string new_group =
              parse_group_line(trimmed_line, linenum, file_path);

          // Before switching the group, add all new options for that group.
          // NOTE: Only add new options in the main option file (not for the
          // files from include directives).
          if (!in_include_file &&
              (track_cfg_to_add.find(in_group) != track_cfg_to_add.end())) {
            if (!track_cfg_to_add.at(in_group).empty()) {
              for (const auto &opt_pair : track_cfg_to_add.at(in_group)) {
                if (opt_pair.second.is_null()) {
                  tmp_file << opt_pair.first.original_option.c_str()
                           << std::endl;
                } else {
                  tmp_file << opt_pair.first.original_option.c_str() << " = "
                           << convert_value_for_file(*opt_pair.second).c_str()
                           << std::endl;
                }
              }
              tmp_file << std::endl;
            }

            // Remove group from tracking map (no more options to add).
            track_cfg_to_add.erase(in_group);
          }
          in_group = new_group;

          // Set group to be deleted if does not exist in the configuration.
          if (m_configmap.find(in_group) == m_configmap.end()) {
            delete_group = true;
            continue;
          } else {
            delete_group = false;
            tmp_file << line << std::endl;
          }
        } else {
          // Parse option line
          std::tuple<std::string, utils::nullable<std::string>, std::string>
              opt_tuple = parse_option_line(trimmed_line, linenum, file_path);

          if (in_group.empty())
            throw std::runtime_error("Group missing before option at line " +
                                     std::to_string(linenum) + " of file '" +
                                     file_path + "': " + line + ".");

          // Delete option (not write it), if group is marked to be deleted.
          if (delete_group) continue;

          Option_key opt_key = convert_option_to_key(std::get<0>(opt_tuple));
          if (m_configmap.at(in_group).find(opt_key) ==
              m_configmap.at(in_group).end()) {
            // Delete option (not write it), if it does not exist in the
            // configurations.
            continue;
          } else {
            // Check if the option value needs to be updated.
            utils::nullable<std::string> value =
                m_configmap.at(in_group).at(opt_key);

            if (value == std::get<1>(opt_tuple)) {
              // Value is the same, keep the line in the file unchanged.
              tmp_file << line << std::endl;
            } else {
              // Value is different, update it.
              auto it = m_configmap.at(in_group).find(opt_key);
              std::string option = it->first.original_option;

              // Update value keeping inline comments.
              if (value.is_null()) {
                tmp_file << option.c_str() << std::get<2>(opt_tuple)
                         << std::endl;
              } else {
                tmp_file << option.c_str() << " = "
                         << convert_value_for_file(*value).c_str()
                         << std::get<2>(opt_tuple) << std::endl;
              }
            }

            // Remove option from tracking map (not a new option).
            if (track_cfg_to_add.find(in_group) != track_cfg_to_add.end() &&
                track_cfg_to_add.at(in_group).find(opt_key) !=
                    track_cfg_to_add.at(in_group).end()) {
              track_cfg_to_add.at(in_group).erase(opt_key);
            }
          }
        }
      }

      // Add all new options for the last group.
      // NOTE: Only add new options in the main option file (not for the
      // files from include directives).
      if (!in_include_file &&
          (track_cfg_to_add.find(in_group) != track_cfg_to_add.end())) {
        if (!track_cfg_to_add.at(in_group).empty()) {
          for (auto opt_pair : track_cfg_to_add.at(in_group)) {
            if (opt_pair.second.is_null()) {
              tmp_file << opt_pair.first.original_option.c_str() << std::endl;
            } else {
              tmp_file << opt_pair.first.original_option.c_str() << " = "
                       << convert_value_for_file(*opt_pair.second).c_str()
                       << std::endl;
            }
          }
          tmp_file << std::endl;
        }

        // Remove group from tracking map (no more options to add).
        track_cfg_to_add.erase(in_group);
      }

      cfg_file.close();
    }

    // Write new groups and respective options to add at the end of the file.
    // NOTE: Only add new options in the main option file (not for the
    // files from include directives).
    if (!in_include_file && !track_cfg_to_add.empty()) {
      tmp_file << std::endl;

      for (const auto &grp_pair : track_cfg_to_add) {
        tmp_file << "[" << grp_pair.first.c_str() << "]" << std::endl;
        for (auto opt_pair : grp_pair.second) {
          if (opt_pair.second.is_null()) {
            tmp_file << opt_pair.first.original_option.c_str() << std::endl;
          } else {
            tmp_file << opt_pair.first.original_option.c_str() << " = "
                     << convert_value_for_file(*opt_pair.second).c_str()
                     << std::endl;
          }
        }

        tmp_file << std::endl;
      }

      // Clear tracking map (all groups and options added).
      track_cfg_to_add.clear();
    }

    // Add written temporary file to the list of files to copy at the end.
    tmp_file.close();
    files_to_copy.emplace_back(tmp_file_path, file_path);

    // Only the first file in the list is the main option file, any remaining
    // one in the list is from an include directive.
    in_include_file = true;
  }

  // Copy temp files to target option files and remove temp files.
  for (const auto &copy_pair : files_to_copy) {
    shcore::copy_file(copy_pair.first, copy_pair.second);
    shcore::delete_file(copy_pair.first, false);
  }
}

std::vector<std::string> Config_file::groups() const {
  std::vector<std::string> res;
  for (const auto &element : m_configmap) {
    res.push_back(element.first);
  }
  return res;
}

bool Config_file::add_group(const std::string &group) {
  // Second value in the pair returned by emplace() indicates if the element
  // was successfully inserted (true) or not (false) because it already
  // exists.
  auto res = m_configmap.emplace(group, Option_map());
  return res.second;
}

bool Config_file::has_group(const std::string &group) const {
  return m_configmap.find(group) != m_configmap.end();
}

bool Config_file::remove_group(const std::string &group) {
  if (has_group(group)) {
    m_configmap.erase(group);
    return true;
  } else {
    return false;
  }
}

std::vector<std::string> Config_file::options(const std::string &group) const {
  if (!has_group(group)) {
    throw std::out_of_range("Group '" + group + "' does not exist.");
  } else {
    std::vector<std::string> res;
    for (auto const &element : m_configmap.at(group)) {
      res.push_back(element.first.original_option);
    }
    return res;
  }
}

bool Config_file::has_option(const std::string &group,
                             const std::string &option) const {
  if (has_group(group)) {
    Option_key opt_key = convert_option_to_key(option);
    return m_configmap.at(group).find(opt_key) != m_configmap.at(group).end();
  } else {
    return false;
  }
}

utils::nullable<std::string> Config_file::get(const std::string &group,
                                              const std::string &option) const {
  if (has_option(group, option)) {
    Option_key opt_key = convert_option_to_key(option);
    return m_configmap.at(group).at(opt_key);
  } else {
    throw std::out_of_range("Option '" + option +
                            "' does not exist in group '" + group + "'.");
  }
}

void Config_file::set(const std::string &group, const std::string &option,
                      const utils::nullable<std::string> &value) {
  if (has_group(group)) {
    Option_key opt_key = convert_option_to_key(option);
    m_configmap[group][opt_key] = value;
  } else {
    throw std::out_of_range("Group '" + group + "' does not exist.");
  }
}

bool Config_file::remove_option(const std::string &group,
                                const std::string &option) {
  if (has_group(group)) {
    if (has_option(group, option)) {
      Option_key opt_key = convert_option_to_key(option);
      m_configmap[group].erase(opt_key);
      return true;
    } else {
      return false;
    }
  } else {
    throw std::out_of_range("Group '" + group + "' does not exist.");
  }
}

void Config_file::clear() { m_configmap.clear(); }

void Config_file::read_recursive_aux(const std::string &cnf_path,
                                     unsigned int recursive_depth,
                                     container *out_map,
                                     const std::string &group_prefix) const {
  // 10 is the max recursion supported on the server when parsing option files
  if (recursive_depth >= 10) {
    log_warning(
        "Skipping parsing of option file '%s' because maximum depth "
        "level was reached.",
        cnf_path.c_str());
    return;  // exit function without doing anything.
  }
#ifdef _WIN32
  const auto wide_cnf_path = shcore::utf8_to_wide(cnf_path);
  std::ifstream input_file(wide_cnf_path);
#else
  std::ifstream input_file(cnf_path);
#endif
  if (!input_file.good()) {
    throw std::runtime_error("Cannot open file: " + cnf_path + ".");
  }

  std::string line, in_group;
  unsigned int linenum = 0;
  while (std::getline(input_file, line)) {
    ++linenum;
    // Trim line (remove whitespaces)
    std::string trimmed_line = shcore::str_strip(line);

    // Skip empty line (only with whitespaces)
    if (trimmed_line.empty()) continue;

    // Ignore comment lines (starting with ';' or '#')
    if (trimmed_line[0] == '#' || trimmed_line[0] == ';') continue;

    // If line is an include directive
    if (trimmed_line[0] == '!') {
      if (shcore::str_beginswith(trimmed_line, "!include ")) {
        std::string include_file_path =
            parse_include_path(trimmed_line, cnf_path);
        read_recursive_aux(include_file_path, recursive_depth + 1, out_map,
                           group_prefix);
        continue;  // after processing the !include skip to the next line
      } else if (shcore::str_beginswith(trimmed_line, "!includedir ")) {
        std::string include_dir_path =
            parse_include_path(trimmed_line, cnf_path);

        std::vector<std::string> files_to_include =
            get_config_files_in_dir(include_dir_path);
        // Recursively process config files in directory
        for (const std::string &include_file_path : files_to_include) {
          read_recursive_aux(include_file_path, recursive_depth + 1, out_map,
                             group_prefix);
        }
        continue;  // after processing the !includedir skip to the next line
      } else {
        throw std::runtime_error("Invalid directive at line " +
                                 std::to_string(linenum) + " of file '" +
                                 cnf_path + "': " + line + ".");
      }
    }
    // A line cannot start with the ',' or '=' values.
    if (trimmed_line[0] == ',' || trimmed_line[0] == '=')
      throw std::runtime_error("Line " + std::to_string(linenum) +
                               " starts with invalid character in file '" +
                               cnf_path + "': " + line + ".");

    // Handle line depending if it matches a group or option.
    if (trimmed_line[0] == '[') {
      // Group line, parse it and store empty map using group name as key
      in_group = parse_group_line(trimmed_line, linenum, cnf_path);

      // Create config map entry for group if it does not exist already.
      // If the group_prefix was provided, only add an entry if it matches.
      // NOTE: Groups can appear repeated in option files (no error is
      // issued by mysqld). If the same option is repeated in the same
      // group the last value read is used.
      if (out_map->find(in_group) == out_map->end() &&
          (group_prefix.empty() ||
           (m_group_case == Case::INSENSITIVE
                ? shcore::str_ibeginswith(in_group, group_prefix)
                : shcore::str_beginswith(in_group, group_prefix)))) {
        out_map->emplace(in_group, Option_map());
      }
    } else {
      // Not a group, handle as an option, try to parse it and save it
      // std::string name, value;
      std::tuple<std::string, utils::nullable<std::string>, std::string> res =
          parse_option_line(trimmed_line, linenum, cnf_path);

      if (in_group.empty())
        throw std::runtime_error("Group missing before option at line " +
                                 std::to_string(linenum) + " of file '" +
                                 cnf_path + "': " + line + ".");

      if (group_prefix.empty() ||
          (m_group_case == Case::INSENSITIVE
               ? shcore::str_ibeginswith(in_group, group_prefix)
               : shcore::str_beginswith(in_group, group_prefix))) {
        auto key = convert_option_to_key(std::get<0>(res));
        (*out_map)[in_group][key] = std::get<1>(res);
      }
    }
  }
  input_file.close();
}

std::vector<std::string> Config_file::get_config_files_in_dir(
    const std::string &dir_path) const {
  // Get list of files to be included.
  std::vector<std::string> files_to_include;
  for (const std::string &filename : shcore::listdir(dir_path)) {
    size_t last_dot = filename.find_last_of('.');
    // check if file has an extension after the .
    if (last_dot != std::string::npos && last_dot + 2 <= filename.size()) {
      std::string extension = filename.substr(last_dot + 1);
      // On Windows included files can have the '.cnf' or '.ini'
      // extension, otherwise only '.cnf'.
#ifdef WIN32
      bool is_option_file = (extension == "cnf" || extension == "ini");
#else
      bool is_option_file = (extension == "cnf");
#endif
      if (is_option_file) {
        files_to_include.push_back(shcore::path::join_path(dir_path, filename));
      }
    }
  }
  return files_to_include;
}

std::string Config_file::parse_include_path(
    const std::string &line, const std::string &base_path) const {
  std::string res;
  if (shcore::str_beginswith(line, "!include ")) {
    // Match !include + space to ensure there are non blank chars after.
    std::string include_file_path = shcore::str_strip(line.substr(9));

    // Get absolute path for included file
    std::string base_dir = shcore::path::dirname(base_path);
    res = shcore::get_absolute_path(include_file_path, base_dir);
  } else if (shcore::str_beginswith(line, "!includedir ")) {
    std::string include_dir_path = shcore::str_strip(line.substr(12));

    // Get absolute path for included dir
    std::string base_dir = shcore::path::dirname(base_path);
    res = shcore::get_absolute_path(include_dir_path, base_dir);
  } else {
    throw std::runtime_error(
        "No !include or !includedir directive found on "
        "line: '" +
        line + "'.");
  }
  return res;
}

std::vector<std::string> get_default_config_paths(shcore::OperatingSystem os) {
  std::vector<std::string> default_paths;
  // NOTE: According to the docs MySQL programs read startup options files
  // according to a predefined location and order. Those my.cnf files for
  // global options should be included in the default locations.
  // See: https://dev.mysql.com/doc/en/option-files.html
  log_info("Getting default config paths for OS %s", to_string(os).c_str());
  std::string sysconfdir;
#ifdef DEFAULT_SYSCONFDIR
  sysconfdir = std::string(DEFAULT_SYSCONFDIR);
#endif
  switch (os) {
    case shcore::OperatingSystem::DEBIAN:
      default_paths.push_back("/etc/my.cnf");
      default_paths.push_back("/etc/mysql/my.cnf");
      if (!sysconfdir.empty()) default_paths.push_back(sysconfdir + "/my.cnf");
      default_paths.push_back("/etc/mysql/mysql.conf.d/mysqld.cnf");
      break;
    case shcore::OperatingSystem::REDHAT:
    case shcore::OperatingSystem::SOLARIS:
    case shcore::OperatingSystem::LINUX:
    case shcore::OperatingSystem::MACOS:
      default_paths.push_back("/etc/my.cnf");
      default_paths.push_back("/etc/mysql/my.cnf");
      if (!sysconfdir.empty()) default_paths.push_back(sysconfdir + "/my.cnf");
      break;
    case shcore::OperatingSystem::WINDOWS: {
      char *program_data_ptr = getenv("PROGRAMDATA");
      if (program_data_ptr) {
        default_paths.push_back(std::string(program_data_ptr) +
                                R"(\MySQL\MySQL Server 5.7\my.ini)");
        default_paths.push_back(std::string(program_data_ptr) +
                                R"(\MySQL\MySQL Server 8.0\my.ini)");
      }
    } break;
    default:
      // The non-handled OS case will keep default_paths and cnfPath empty
      break;
  }
  DBUG_EXECUTE_IF("override_mycnf_default_path", { default_paths.clear(); });
  return default_paths;
}

}  // namespace config
}  // namespace mysqlshdk
