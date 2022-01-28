/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_CONFIG_FILE_H_
#define MYSQLSHDK_LIBS_AWS_AWS_CONFIG_FILE_H_

#include <optional>
#include <string>
#include <unordered_map>

namespace mysqlshdk {
namespace aws {

/**
 * Handles both credentials and config files.
 */
class Aws_config_file final {
 public:
  struct Profile {
    std::optional<std::string> access_key_id;
    std::optional<std::string> secret_access_key;
    std::optional<std::string> session_token;
    std::unordered_map<std::string, std::string> settings;
  };

  Aws_config_file() = delete;

  explicit Aws_config_file(const std::string &path);

  Aws_config_file(const Aws_config_file &) = default;
  Aws_config_file(Aws_config_file &&) = default;

  Aws_config_file &operator=(const Aws_config_file &) = default;
  Aws_config_file &operator=(Aws_config_file &&) = default;

  ~Aws_config_file() = default;

  /**
   * Loads the configuration from the file.
   *
   * @returns true if the requested file exists
   *
   * @throws runtime_error if file is malformed
   */
  bool load();

  /**
   * Checks if the given profile exists.
   *
   * @param name The name of the profile.
   *
   * @returns true if the requested profile exists
   */
  bool has_profile(const std::string &name) const;

  /**
   * Returns the given profile.
   *
   * @param name The name of the profile.
   *
   * @returns the requested profile or nullptr if it doesn't exist
   */
  const Profile *get_profile(const std::string &name) const;

 private:
  using Profiles = std::unordered_map<std::string, Profile>;

  std::string m_path;
  Profiles m_profiles;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_CONFIG_FILE_H_
