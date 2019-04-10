/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_CONFIG_CONFIG_FILE_H_
#define MYSQLSHDK_LIBS_CONFIG_CONFIG_FILE_H_

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace config {

enum class Case { SENSITIVE, INSENSITIVE };

enum class Escape { YES, NO };

class Config_file {
 public:
  /**
   * Constructors
   */
  explicit Config_file(Case group_case = Case::INSENSITIVE,
                       Escape escape = Escape::YES);
  Config_file(const Config_file &cnf_obj) = default;

  bool operator==(const Config_file &other) const {
    return m_configmap == other.m_configmap;
  }

  /**
   * Read an existing MySQL option file (my.cnf).
   *
   * This function reads the contents of an existing MySQL option file and
   * load its configurations into the Config_file object. If no 'group_prefix'
   * is specified then all configurations are loaded, otherwise only the
   * configuration belonging to groups with a prefix that match the specified
   * one will be loaded.
   *
   * For more information about MySQL option files, see:
   * https://dev.mysql.com/doc/en/option-files.html
   *
   * @param cnf_path string with the path to the MySQL option file to read.
   * @param group_prefix optional string with the prefix of the group to
   *        read.
   */
  void read(const std::string &cnf_path, const std::string &group_prefix = "");

  /**
   * Write the configuration to the specified option file.
   *
   * If the specified option file already exists, it saves the changed
   * configuration to the file, preserving the order of the existing groups and
   * options, as well as existing comments (except inline comments of deleted
   * options or groups).
   *
   * @param cnf_path string with the path to the target output file to write
   *        the configurations.
   */
  void write(const std::string &cnf_path) const;

  /**
   * Get the list of all groups.
   *
   * @return vector with the name of the groups (string).
   */
  std::vector<std::string> groups() const;

  /**
   * Add a new (empty) group.
   *
   * An empty group is inserted if it does not already exists returning true,
   * otherwise no changes are made and false is returned.
   *
   * NOTE: The specified group name is converted to lower case. Group names are
   *       case insensitive.
   *
   * @param group string with the name of the group to add.
   * @return true if the group was added (does not exist), false if it already
   *         exists.
   */
  bool add_group(const std::string &group);

  /**
   * Check if the specified group exists.
   *
   * NOTE: The specified group name is converted to lower case. Group names are
   *       case insensitive.
   *
   * @param group string with the name of the group to check.
   * @return boolean, true if the group exists, otherwise false.
   */
  bool has_group(const std::string &group) const;

  /**
   * Remove the specified group.
   *
   * If the group exists and it is sucessfully removed then true is returned,
   * otherwise false.
   *
   * @param group string with the name of the group to check.
   * @return boolean, true if the group exists and is removed, otherwise false.
   */
  bool remove_group(const std::string &group);

  /**
   * Get the list of all options in the specified group.
   *
   * @param group string with the name of the group to retrieve all options.
   * @return a vector of strings with the name of all options belonging to the
   *         specified group.
   * @throw a out_of_range exception if the specified group does not exist.
   */
  std::vector<std::string> options(const std::string &group) const;

  /**
   * Check if the specified option exists.
   *
   * @param group string with the name of the group the option belongs to.
   * @param option string with the name of the option to check.
   * @return boolean, true if the option exists in the specified group,
   *         otherwise false.
   */
  bool has_option(const std::string &group, const std::string &option) const;

  /**
   * Get the value for the specified option.
   *
   * @param group string with the name of the group the option belongs to.
   * @param option string with the name of the option to get its value.
   * @return nullable string with the value for the specified option.
   * @throw a out_of_range exception if the specified option (and group) does
   *        not exist.
   */
  utils::nullable<std::string> get(const std::string &group,
                                   const std::string &option) const;

  /**
   * Set the value for the specified option in an existing group.
   *
   * If the specified option already exists its value will be replaced by the
   * specified one. If the option does not exist then a new option will be
   * inserted with the specified value. However, if the specified group does not
   * exists an exception will be raised.
   *
   * @param group string with the name of the group the option belongs to.
   * @param option string with the name of the option to set.
   * @param value nullable string value that will be set.
   * @throw a out_of_range exception if the specified group does not exist.
   */
  void set(const std::string &group, const std::string &option,
           const utils::nullable<std::string> &value);

  /**
   * Remove the specified option.
   *
   * If the option exists and it is successfully removed true is returned,
   * otherwise false. However, if the specified group does not exists an
   * exception will be raised.
   *
   * @param group string with the name of the group the option belongs to.
   * @param option string with the name of the option to remove.
   * @return boolean, true if the option exists and is removed, otherwise false.
   * @throw a out_of_range exception if the specified group does not exist.
   */
  bool remove_option(const std::string &group, const std::string &option);

  /**
   * Remove all elements from the Config_file.
   */
  void clear();

 private:
  /**
   * Custom option key structure that hold the normalized key value, but
   * also keep the original option name.
   *
   * This is needed because dash (-) and underscore (_) may be used
   * interchangeably for the option names, and option with the "loose-" prefix
   * should match the option without that prefix. The original option value is
   * maintained so that it can be written to a file as initially read/set.
   */
  struct Option_key {
    std::string key;
    std::string original_option;

    bool operator<(const Option_key &other) const { return key < other.key; }

    bool operator==(const Option_key &other) const {
      return key == other.key && original_option == other.original_option;
    }
  };

  Case m_group_case;

  Escape m_escape_characters;

  typedef std::map<Option_key, mysqlshdk::utils::nullable<std::string>>
      Option_map;
  typedef bool (*compare_func)(const std::string &, const std::string &);
  typedef std::map<std::string, Option_map, compare_func> container;

  typedef container::const_iterator const_iterator;
  typedef container::iterator iterator;

  // Map holding all options and respective values for all groups
  container m_configmap;

  static bool comp(const std::string &lhs, const std::string &rhs);
  static bool icomp(const std::string &lhs, const std::string &rhs);

  /**
   * Auxiliary function to create an OptionKey to be used internally.
   *
   * @param option string with the option name to convert to a key.
   * @return an OptionKey object to use internally in the configuration map.
   */
  Option_key convert_option_to_key(const std::string &option) const;

  /**
   * Auxiliary function to get the list of other included files to read.
   *
   * Other options files can be included using the !include and !includedir.
   * This function retrieves the valid list of those files.
   *
   * NOTE: This function is recursive.
   *
   * @param cnf_path string with the path to the MySQL option file to search of
   *        other included option files using the !include and !includedir
   *        directives.
   * @param out_files resulting set with the path (string) of all the other
   *        included option files.
   * @param recursive_depth recursive call depth.
   */
  void get_include_files(const std::string &cnf_path,
                         std::set<std::string> *out_files,
                         unsigned int recursive_depth) const;

  /**
   * Auxiliary method to parse a group line.
   * Note: This function assumes the input string is trimmed.
   *
   * @param line string with the group line to parse.
   * @param line_number integer with the number of the line being parsed.
   * @param cnf_path string with the path of the option file being parsed.
   * @return a string with the group name (if successfully parsed).
   *
   * @throw std::runtime_error if the group line cannot be parsed (invalid
   *        group line).
   */
  std::string parse_group_line(const std::string &line,
                               unsigned int line_number,
                               const std::string &cnf_path) const;

  /**
   * Auxiliary function to parse escape sequences from an option file.
   *
   * This function converts a string value with supported escape sequences
   * from MySQL option files to its correct string representation.
   *
   * @param input string with the input value to convert.
   * @param quote character used as quotation mark (to enclose the value), by
   *              default no quotation mark used.
   * @return string with all supported escape sequence from MySQL option files
   *         converted to the appropriate character.
   */
  std::string parse_escape_sequence_from_file(const std::string &input,
                                              const char quote = '\0') const;

  /**
   * Auxiliary function to parse escape sequences to an option file.
   *
   * This function converts a string value with supported escape sequences
   * to its correct string representation to write in a MySQL option files.
   *
   * @param input string with the input value to convert.
   * @param quote character used as quotation mark (to enclose the value), by
   *              default no quotation mark used.
   * @return string with all supported escape sequence converted to the
   *         appropriate characters to write to a MySQL option files.
   */
  std::string parse_escape_sequence_to_file(const std::string &input,
                                            const char quote = '\0') const;

  /**
   * Auxiliary function to convert an option value to be written to an option
   * file.
   *
   * This function quotes the value with the appropriate quotation marks if
   * needed and converts all supported special characters to their respective
   * escape sequence.
   *
   * @param value string with the value to convert.
   * @return string with the value properly converted to be written to a MySQL
   *         option file.
   */
  std::string convert_value_for_file(const std::string &value) const;

  /**
   * Auxiliary method to parse an option line.
   *
   * @param line string with the option line to parse.
   * @param line_number integer with the number of the line being parsed.
   * @param cnf_path string with the path of the option file being parsed.
   * @return a tuple with the option name, the corresponding value (if
   *         successfully parsed), and remaining text after the value (e.g.,
   *         inline comment; to be able to preserve it when writing to a file).
   * @throw std::runtime_error if the option line cannot be parsed (invalid
   *        option line).
   */
  std::tuple<std::string, utils::nullable<std::string>, std::string>
  parse_option_line(const std::string &line, unsigned int line_number,
                    const std::string &cnf_path) const;

  /**
   * Auxiliary method that returns a vector with all the configuration files
   * found inside a given directory.
   * @param dir_path path of the directory to search for config files
   * @return a vector with all the config files found.
   */
  std::vector<std::string> get_config_files_in_dir(
      const std::string &dir_path) const;

  /**
   * Auxiliary method to read a configuration file
   * @param cnf_path string with the path to the MySQL option file to start
   *        reading from. This function will  recursively call itself for any
   *        included option files via the !include or !includedir directives.
   * @param recursive_depth recursive call depth.
   * @param out_map resulting map with the options and respective values read
   *        from the config file.
   * @param group_prefix optional string with the prefix of the group to
   *        read.
   */
  void read_recursive_aux(const std::string &cnf_path,
                          unsigned int recursive_depth, container *out_map,
                          const std::string &group_prefix = "") const;

  /**
   * Auxiliary function parse the path from the !include and !includedir
   * directives.
   * @param line String with the line with the !include(dir) directive
   * @param base_path base path used to calculate an absolute path if the
   *        !include(dir) directive has a relative path
   * @return string with the absolute path of the !include(dir) directive
   * @throws runtime_error if neither directive is found on line.
   */
  std::string parse_include_path(const std::string &line,
                                 const std::string &base_path = "") const;
};

/**
 * Gets a list of the default config file paths according to the OS
 * @param os OperatingSystem enum
 * @return vector with the default config file paths of the OS
 */
std::vector<std::string> get_default_config_paths(shcore::OperatingSystem os);

}  // namespace config
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_CONFIG_CONFIG_FILE_H_
