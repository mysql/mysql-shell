/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_SETTINGS_H_
#define MYSQLSHDK_LIBS_AWS_AWS_SETTINGS_H_

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/aws/aws_profiles.h"

namespace mysqlshdk {
namespace aws {

enum class Setting {
  PROFILE,
  CONFIG_FILE,
  CREDENTIALS_FILE,
  REGION,
  METADATA_SERVICE_TIMEOUT,
  METADATA_SERVICE_NUM_ATTEMPTS,
  EC2_METADATA_SERVICE_ENDPOINT,
  EC2_METADATA_SERVICE_ENDPOINT_MODE,
  LAST,
};

class Settings final {
 public:
  Settings();

  Settings(const Settings &) = delete;
  Settings(Settings &&) = default;

  Settings &operator=(const Settings &) = delete;
  Settings &operator=(Settings &&) = default;

  ~Settings() = default;

  /**
   * Interprets given string as a boolean.
   *
   * @param s String to be interpreted.
   *
   * @returns true if s compares to 'true' (ignoring case)
   */
  static bool as_bool(std::string_view s);

  /**
   * Interprets given string as an integer.
   *
   * @param s String to be interpreted.
   *
   * @returns integer value of the given string
   *
   * @throws std::invalid_argument if string cannot be converted
   */
  static int as_int(std::string_view s);

  /**
   * Adds a user-set value of a setting.
   *
   * NOTE: this is going to take precedence over all other values.
   *
   * @param s Setting to be set.
   * @param value Value to be set.
   *
   * @throws std::runtime_error If called after initialization.
   */
  void add_user_option(Setting s, std::string value);

  /**
   * Initializes the settings.
   *
   * @throws std::runtime_error In case of any errors.
   */
  void initialize();

  /**
   * Checks whether profile was explicitly set by the user.
   *
   * @returns true if profile was explicitly set by the user.
   */
  bool explicit_profile() const noexcept { return m_explicit_profile; }

  /**
   * Gets the current value of the given setting.
   *
   * @param s Setting to be fetched.
   *
   * @returns Value of the given setting, or nullptr if there's none.
   *
   * @throws std::runtime_error If settings were not initialized.
   */
  const std::string *get(Setting s) const;

  /**
   * Gets the current value of the given string setting.
   *
   * @param s Setting to be fetched.
   *
   * @returns String value of the given setting.
   *
   * @throws std::runtime_error If settings were not initialized.
   * @throws std::runtime_error If value is not set.
   */
  const std::string &get_string(Setting s) const;

  /**
   * Gets the current value of the given integer setting.
   *
   * @param s Setting to be fetched.
   *
   * @returns Integer value of the given setting.
   *
   * @throws std::runtime_error If settings were not initialized.
   * @throws std::runtime_error If value is not set.
   * @throws std::runtime_error If value cannot be converted to integer.
   */
  int get_int(Setting s) const;

  /**
   * Gets the current value of the given boolean setting.
   *
   * @param s Setting to be fetched.
   *
   * @returns Boolean value of the given setting.
   *
   * @throws std::runtime_error If settings were not initialized.
   * @throws std::runtime_error If value is not set.
   */
  bool get_bool(Setting s) const;

  /**
   * Returns the given profile from the config file.
   *
   * @param name The name of the profile.
   *
   * @returns the requested profile or nullptr if it doesn't exist
   */
  const Profile *config_profile(const std::string &name) const;

  /**
   * Returns the given profile from the credentials file.
   *
   * @param name The name of the profile.
   *
   * @returns the requested profile or nullptr if it doesn't exist
   */
  const Profile *credentials_profile(const std::string &name) const;

  /**
   * Checks if the given profile has static credentials in either config or
   * credentials file.
   *
   * @param name The name of the profile.
   *
   * @returns true if requested profile exists and has credentials
   */
  bool has_static_credentials(const std::string &name) const;

  /**
   * Provides name of the given setting.
   *
   * @param s Setting to be fetched.
   *
   * @returns Name of the setting.
   */
  inline const char *name(Setting s) const { return info(s).name; }

 private:
  struct Setting_info {
    const char *name;
    std::vector<const char *> env_vars = {};
    std::optional<std::string> default_value = {};
    bool in_config = true;
  };

  inline const Setting_info &info(Setting s) const { return m_settings.at(s); }

  void add_to_cache(Setting s, std::string value);

  void load_profiles();

  bool m_is_initialized = false;

  std::unordered_map<Setting, Setting_info> m_settings;
  Profile m_cache;

  bool m_explicit_profile = false;

  Profiles m_credentials_profiles;

  Profiles m_config_profiles;
  const Profile *m_config_profile = nullptr;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_SETTINGS_H_
