/*
 * Copyright (c) 2018, 2021 Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQL_SECRET_STORE_INCLUDE_HELPER_H_
#define MYSQL_SECRET_STORE_INCLUDE_HELPER_H_

#include <stdexcept>
#include <string>
#include <vector>

namespace mysql {
namespace secret_store {
namespace common {

constexpr auto k_scheme_name_file = "file";
constexpr auto k_scheme_name_ssh = "ssh";

/**
 * Exception thrown by helpers.
 */
class Helper_exception : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/**
 * Codes used to translate errors to Helper_exceptions.
 */
enum class Helper_exception_code {
  NO_SUCH_SECRET /**< Requested secret does not exist. */
};

/**
 * Provides Helper_exception with appropriate error message for the given
 * exception code.
 *
 * @param code Code of the exception.
 */
Helper_exception get_helper_exception(Helper_exception_code code);

/**
 * Uniquely identifies a secret.
 */
struct Secret_id {
 public:
  /**
   * Equality operator.
   */
  bool operator==(const Secret_id &r) const noexcept;

  /**
   * Inequality operator.
   */
  bool operator!=(const Secret_id &r) const noexcept;

  std::string secret_type; /**< Secret's type. */
  std::string url;         /**< Secret's URL. */
};

/**
 * Stores a secret.
 */
struct Secret {
 public:
  /**
   * Initializes secret with provided values.
   */
  Secret(const std::string &s, const Secret_id &secret_id) noexcept;

  std::string secret; /**< Secret. */
  Secret_id id;       /**< Secret's ID. */
};

/**
 * An interface for helper operations.
 */
class Helper {
 public:
  /**
   * Initializes the helper-related information.
   *
   * @param name Name of the secret store helper.
   * @param version Version of the secret store helper.
   * @param copyright Copyright of the secret store helper.
   */
  Helper(const std::string &name, const std::string &version,
         const std::string &copyright) noexcept;

  /**
   * Destructor.
   */
  virtual ~Helper() noexcept = default;

  /**
   * Provides full name of the helper.
   *
   * @returns Name of the helper.
   */
  std::string name() const noexcept;

  /**
   * Provides version of the helper.
   *
   * @returns Version of the helper.
   */
  std::string version() const noexcept;

  /**
   * Provides copyright of the helper.
   *
   * @returns Copyright of the helper.
   */
  std::string copyright() const noexcept;

  /**
   * Checks if requirements of this helper are fulfilled, i.e. required
   * application is installed.
   *
   * @throws Helper_exception if requirements are not fulfilled.
   */
  virtual void check_requirements() = 0;

  /**
   * Stores the specified secret.
   *
   * @param secret Secret to store.
   *
   * @throws Helper_exception in case of an error.
   */
  virtual void store(const Secret &secret) = 0;

  /**
   * Retrieves the specified secret.
   *
   * @param id Unique ID of the secret to retrieve.
   * @param secret Retrieved secret.
   *
   * @throws Helper_exception in case of an error.
   */
  virtual void get(const Secret_id &id, std::string *secret) = 0;

  /**
   * Erases the specified secret.
   *
   * @param id Unique ID of the secret to erase.
   *
   * @throws Helper_exception in case of an error.
   */
  virtual void erase(const Secret_id &id) = 0;

  /**
   * Retrieves all secrets stored by this secret store helper.
   *
   * @param secrets Retrieved IDs of stored secrets.
   *
   * @throws Helper_exception in case of an error.
   */
  virtual void list(std::vector<Secret_id> *secrets) = 0;

 private:
  std::string m_name;
  std::string m_version;
  std::string m_copyright;
};

}  // namespace common
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_INCLUDE_HELPER_H_
