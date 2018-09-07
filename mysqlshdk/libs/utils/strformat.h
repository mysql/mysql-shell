/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_STRFORMAT_H_
#define MYSQLSHDK_LIBS_UTILS_STRFORMAT_H_

#include <string>
#include <utility>

namespace mysqlshdk {
namespace utils {

template <typename T>
inline std::pair<std::string, double> scale_value(T n) {
  if (n > 1000000000000)
    return {"T", n / 1000000000000.0};
  else if (n > 1000000000)
    return {"G", n / 1000000000.0};
  else if (n > 1000000)
    return {"M", n / 1000000.0};
  else if (n > 1000)
    return {"K", n / 1000.0};
  return {"", n};
}

std::string format_seconds(double secs);
std::string format_microseconds(double secs);

std::string format_bytes(uint64_t bytes);

std::string format_throughput_items(const std::string &item_name_singular,
                                    const std::string &item_name_plural,
                                    const uint64_t items, double seconds);

std::string format_throughput_bytes(uint64_t bytes, double seconds);

std::string fmttime(const char *fmt);

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_STRFORMAT_H_
