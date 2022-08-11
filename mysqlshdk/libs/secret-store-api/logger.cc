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

#include "mysqlshdk/libs/secret-store-api/logger.h"

namespace mysql {
namespace secret_store {
namespace api {
namespace logger {

namespace {

// We have to use a static local method, otherwise we have an
// initialization-order-fiasco because this logger is used with statically
// initialized tests (e.g.: mysql_secret_store_t.cc:1299)

std::function<void(std::string_view)> *log_cb() {
  static std::function<void(std::string_view)> cb;
  return &cb;
}

}  // namespace

void log(std::string_view msg) {
  if (auto cb = log_cb(); *cb) (*cb)(msg);
}

void set_logger(std::function<void(std::string_view)> cb) {
  *log_cb() = std::move(cb);
}

}  // namespace logger
}  // namespace api
}  // namespace secret_store
}  // namespace mysql
