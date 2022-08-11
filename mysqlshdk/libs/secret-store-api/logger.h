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

#ifndef MYSQLSHDK_LIBS_SECRET_STORE_API_LOGGER_H_
#define MYSQLSHDK_LIBS_SECRET_STORE_API_LOGGER_H_

#include <functional>
#include <string_view>

namespace mysql {
namespace secret_store {
namespace api {
namespace logger {

void log(std::string_view msg);

void set_logger(std::function<void(std::string_view)> cb);

}  // namespace logger
}  // namespace api
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQLSHDK_LIBS_SECRET_STORE_API_LOGGER_H_
