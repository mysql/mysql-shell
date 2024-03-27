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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_PROFILES_H_
#define MYSQLSHDK_LIBS_AWS_AWS_PROFILES_H_

#include <optional>
#include <string>
#include <unordered_map>

namespace mysqlshdk {
namespace aws {

class Profile final {
 public:
  Profile() = delete;

  explicit Profile(std::string name);

  Profile(const Profile &) = delete;
  Profile(Profile &&) = default;

  Profile &operator=(const Profile &) = delete;
  Profile &operator=(Profile &&) = default;

  ~Profile() = default;

  const std::string &name() const noexcept { return m_name; }

  /**
   * Updates the given setting.
   *
   * @param name The name of the setting.
   * @param value The new value of the setting.
   */
  void set(std::string name, std::string value);

  /**
   * Returns the given setting.
   *
   * NOTE: Credential-related settings are not available here.
   *
   * @param name The name of the setting.
   *
   * @returns the requested setting or nullptr if it doesn't exist
   */
  const std::string *get(const std::string &name) const;

  /**
   * Checks if this profile has the given setting.
   *
   * @param name The name of the setting.
   *
   * @return true if the given setting exists
   */
  bool has(const std::string &name) const;

  /**
   * Checks if this profile has credentials (access key and secret key).
   *
   * @return true if profile has credentials
   */
  inline bool has_credentials() const noexcept {
    return m_access_key_id.has_value() && m_secret_access_key.has_value();
  }

  /**
   * Provides the access key.
   */
  inline const std::optional<std::string> &access_key_id() const noexcept {
    return m_access_key_id;
  }

  /**
   * Provides the secret key.
   */
  inline const std::optional<std::string> &secret_access_key() const noexcept {
    return m_secret_access_key;
  }

  /**
   * Provides the session token.
   */
  inline const std::optional<std::string> &session_token() const noexcept {
    return m_session_token;
  }

 private:
  std::string m_name;
  std::unordered_map<std::string, std::string> m_settings;
  std::optional<std::string> m_access_key_id;
  std::optional<std::string> m_secret_access_key;
  std::optional<std::string> m_session_token;
};

class Profiles final {
 public:
  Profiles() = default;

  Profiles(const Profiles &) = delete;
  Profiles(Profiles &&) = default;

  Profiles &operator=(const Profiles &) = delete;
  Profiles &operator=(Profiles &&) = default;

  ~Profiles() = default;

  /**
   * Checks if the given profile exists.
   *
   * @param name The name of the profile.
   *
   * @returns true if the requested profile exists
   */
  bool exists(const std::string &name) const;

  /**
   * Returns the given profile.
   *
   * @param name The name of the profile.
   *
   * @returns the requested profile or nullptr if it doesn't exist
   */
  const Profile *get(const std::string &name) const;

  /**
   * Returns the given profile, creates it if it doesn't exist.
   *
   * @param name The name of the profile.
   *
   * @returns the requested profile
   */
  Profile *get_or_create(const std::string &name);

 private:
  std::unordered_map<std::string, Profile> m_profiles;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_PROFILES_H_
