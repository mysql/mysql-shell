/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQL_SECRET_STORE_INCLUDE_MYSQL_SECRET_STORE_API_H_
#define MYSQL_SECRET_STORE_INCLUDE_MYSQL_SECRET_STORE_API_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mysql {
namespace secret_store {
namespace api {

/**
 * Types of secrets.
 */
enum class Secret_type {
  PASSWORD /**< Secret is a password */
};

/**
 * Specification of a secret, uniquely identifies a secret.
 */
struct Secret_spec {
 public:
  /**
   * Equality operator.
   */
  bool operator==(const Secret_spec &r) const noexcept;

  /**
   * Inequality operator.
   */
  bool operator!=(const Secret_spec &r) const noexcept;

  Secret_type type; /**< Secret's type. */
  std::string url;  /**< Secret's URL. */
};

/**
 * Holds name of the secret store helper.
 */
class Helper_name final {
 public:
  /**
   * Default constructor is deleted.
   */
  Helper_name() = delete;

  /**
   * Copy construction is supported.
   */
  Helper_name(const Helper_name &name) noexcept;

  /**
   * Copy assignment is supported.
   */
  Helper_name &operator=(const Helper_name &r) noexcept;

  /**
   * Destructor.
   */
  ~Helper_name() noexcept;

  /**
   * Provides name which uniquely identifies the secret store helper.
   *
   * @returns Helper name.
   */
  std::string get() const noexcept;

  /**
   * Provides full path of the helper executable.
   *
   * @returns Full path of helper executable.
   */
  std::string path() const noexcept;

 private:
  friend std::vector<Helper_name> get_available_helpers(
      const std::string &dir) noexcept;
  class Helper_name_impl;

  Helper_name(const std::string &name, const std::string &path) noexcept;

  std::unique_ptr<Helper_name_impl> m_impl;
};

/**
 * Provides access to the operations supported by the secret store helper.
 */
class Helper_interface final {
 public:
  /**
   * Default constructor is deleted.
   */
  Helper_interface() = delete;

  /**
   * Copy construction is not supported.
   */
  Helper_interface(const Helper_interface &other) = delete;

  /**
   * Copy assignment is not supported.
   */
  Helper_interface &operator=(const Helper_interface &r) = delete;

  /**
   * Destructor.
   */
  ~Helper_interface() noexcept;

  /**
   * Provides name of the secret store helper.
   *
   * @returns Name of the helper.
   */
  Helper_name name() const noexcept;

  /**
   * Stores the secret identified by spec.
   *
   * @param spec Specification of a secret.
   * @param secret Secret to store.
   *
   * @returns true if secret was successfully stored.
   */
  bool store(const Secret_spec &spec, const std::string &secret) noexcept;

  /**
   * Provides the secret identified by spec.
   *
   * @param spec Specification of a secret.
   * @param secret Retrieved secret.
   *
   * @returns true if secret was successfully retrieved.
   */
  bool get(const Secret_spec &spec, std::string *secret) const noexcept;

  /**
   * Erases the secret identified by spec.
   *
   * @param spec Specification of a secret.
   *
   * @returns true if secret was successfully removed.
   */
  bool erase(const Secret_spec &spec) noexcept;

  /**
   * Provides the list of secret specifications stored by the secret store
   * helper.
   *
   * @param specs Retrieved list of secret specifications.
   *
   * @returns true if list was successfully retrieved.
   */
  bool list(std::vector<Secret_spec> *specs) const noexcept;

  /**
   * Provides error message of the previous operation (store/get/erase/list)
   * which has failed (returned false).
   *
   * @returns Error message or an empty string, if previous operation finished
   *          successfully.
   */
  std::string get_last_error() const noexcept;

 private:
  friend std::unique_ptr<Helper_interface> get_helper(
      const Helper_name &name) noexcept;

  class Helper_interface_impl;

  explicit Helper_interface(const Helper_name &name) noexcept;

  std::unique_ptr<Helper_interface_impl> m_impl;
};

/**
 * Provides secret store helpers which are available (and valid) for the current
 * platform.
 *
 * @param dir Directory with the secret store helpers. If it's empty, directory
 *            containing the binary of the current process is used.
 *
 * @returns Collection of available helpers.
 */
std::vector<Helper_name> get_available_helpers(
    const std::string &dir = "") noexcept;

/**
 * Provides the specified secret store helper.
 *
 * @param name Name of the secret store helper to fetch.
 *
 * @returns Specified secret store helper or nullptr if helper cannot be
 *          fetched.
 */
std::unique_ptr<Helper_interface> get_helper(const Helper_name &name) noexcept;

/**
 * Sets a hook which is going to be used by the API to log messages.
 *
 * @param logger Function to be called when logging a message.
 */
void set_logger(std::function<void(std::string_view)> logger) noexcept;

}  // namespace api
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_INCLUDE_MYSQL_SECRET_STORE_API_H_
