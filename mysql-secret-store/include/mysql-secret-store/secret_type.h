/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQL_SECRET_STORE_INCLUDE_MYSQL_SECRET_STORE_SECRET_TYPE_H_
#define MYSQL_SECRET_STORE_INCLUDE_MYSQL_SECRET_STORE_SECRET_TYPE_H_

namespace mysql {
namespace secret_store {
namespace api {

/**
 * Types of secrets.
 */
enum class Secret_type {
  PASSWORD, /**< Secret is a password, Secret's ID is expected to be a URL. */
  GENERIC,  /**< Generic secret. */
};

}  // namespace api
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_INCLUDE_MYSQL_SECRET_STORE_SECRET_TYPE_H_
