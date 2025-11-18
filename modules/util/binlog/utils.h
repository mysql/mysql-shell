/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_BINLOG_UTILS_H_
#define MODULES_UTIL_BINLOG_UTILS_H_

#include <mysql/binlog/event/control_events.h>
#include <mysql/gtids/gtids.h>

#include <string>
#include <string_view>

namespace mysqlsh {
namespace binlog {

mysql::gtids::Gtid to_gtid(const mysql::binlog::event::Gtid_event &gtid_event);

mysql::gtids::Gtid_set to_gtid_set(std::string_view str);

mysql::gtids::Gtid to_gtid(std::string_view str);

std::string to_string(const mysql::gtids::Gtid_set &gtid_set);

std::string to_string(const mysql::gtids::Gtid &gtid);

std::string subtract(std::string_view l, std::string_view r);

std::string subtract(const mysql::gtids::Gtid_set &l,
                     const mysql::gtids::Gtid_set &r);

/**
 * Throws std::bad_alloc if status is not OK, to be used with functions which
 * return error statuses only when memory-related failure happens.
 */
void throw_on_failure(mysql::utils::Return_status status);

}  // namespace binlog
}  // namespace mysqlsh

#endif  // MODULES_UTIL_BINLOG_UTILS_H_
