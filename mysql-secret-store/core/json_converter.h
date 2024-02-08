/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQL_SECRET_STORE_CORE_JSON_CONVERTER_H_
#define MYSQL_SECRET_STORE_CORE_JSON_CONVERTER_H_

#include <string>
#include <vector>

#include "mysql-secret-store/include/helper.h"

namespace mysql {
namespace secret_store {
namespace core {
namespace converter {

common::Secret to_secret(const std::string &secret);

common::Secret_id to_secret_id(const std::string &secret_id);

std::string to_string(const common::Secret &secret);

std::string to_string(const common::Secret_id &secret_id);

std::string to_string(const std::vector<common::Secret_id> &secrets);

}  // namespace converter
}  // namespace core
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_CORE_JSON_CONVERTER_H_
