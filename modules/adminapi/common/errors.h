/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MODULES_ADMINAPI_COMMON_ERRORS_H_
#define MODULES_ADMINAPI_COMMON_ERRORS_H_

#include <stdexcept>
#include <string>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlsh {
namespace dba {

inline shcore::Exception make_unsupported_protocol_error() {
  return shcore::Exception::runtime_error(
      "The provided URI uses the X protocol, which is not supported by this "
      "command.");
}

namespace detail {

inline std::string connection_error_msg(const std::exception &e,
                                        const std::string &context) {
  return "Could not open connection to '" + context + "': " + e.what();
}

inline void report_connection_error(const std::exception &e,
                                    const std::string &context) {
  log_warning("Failed to connect to instance: %s", e.what());
  mysqlsh::current_console()->print_error(
      "Unable to connect to the target instance '" + context +
      "'. Please verify the connection settings, make sure the instance is "
      "available and try again.");
}

}  // namespace detail

#define CATCH_AND_THROW_CONNECTION_ERROR(address)                          \
  catch (const shcore::Exception &e) {                                     \
    throw shcore::Exception::error_with_code(                              \
        e.type(), mysqlsh::dba::detail::connection_error_msg(e, address),  \
        e.code());                                                         \
  }                                                                        \
  catch (const shcore::Error &e) {                                         \
    throw shcore::Exception::mysql_error_with_code(                        \
        mysqlsh::dba::detail::connection_error_msg(e, address), e.code()); \
  }                                                                        \
  catch (const std::exception &e) {                                        \
    throw shcore::Exception::runtime_error(                                \
        mysqlsh::dba::detail::connection_error_msg(e, address));           \
  }

#define CATCH_REPORT_AND_THROW_CONNECTION_ERROR(address)                   \
  catch (const shcore::Exception &e) {                                     \
    mysqlsh::dba::detail::report_connection_error(e, address);             \
    throw shcore::Exception::error_with_code(                              \
        e.type(), mysqlsh::dba::detail::connection_error_msg(e, address),  \
        e.code());                                                         \
  }                                                                        \
  catch (const shcore::Error &e) {                                         \
    mysqlsh::dba::detail::report_connection_error(e, address);             \
    throw shcore::Exception::mysql_error_with_code(                        \
        mysqlsh::dba::detail::connection_error_msg(e, address), e.code()); \
  }                                                                        \
  catch (const std::exception &e) {                                        \
    mysqlsh::dba::detail::report_connection_error(e, address);             \
    throw shcore::Exception::runtime_error(                                \
        mysqlsh::dba::detail::connection_error_msg(e, address));           \
  }

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ERRORS_H_
