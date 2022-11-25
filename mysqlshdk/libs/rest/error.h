/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_ERROR_H_
#define MYSQLSHDK_LIBS_REST_ERROR_H_

#include <stdexcept>
#include <string>
#include <string_view>

namespace mysqlshdk {
namespace rest {

/**
 * Detailed information on connection error.
 */
class Connection_error : public std::runtime_error {
 public:
  Connection_error(std::string_view msg, int curl_code)
      : std::runtime_error(std::string{msg} +
                           " (CURLcode = " + std::to_string(curl_code) + ")"),
        m_curl_code(curl_code) {}

  int curl_code() const noexcept { return m_curl_code; }

 private:
  int m_curl_code;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_ERROR_H_
