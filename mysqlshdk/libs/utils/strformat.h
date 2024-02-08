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

#ifndef MYSQLSHDK_LIBS_UTILS_STRFORMAT_H_
#define MYSQLSHDK_LIBS_UTILS_STRFORMAT_H_

#include <cstdint>
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

std::string format_seconds(double secs, bool show_fractional_seconds = true);
std::string format_microseconds(double secs);

std::string format_items(const std::string &full,
                         const std::string &abbreviation, uint64_t items,
                         bool space_before_item = true, bool compact = false);

std::string format_bytes(uint64_t bytes);

std::string format_throughput_items(const std::string &item_name_singular,
                                    const std::string &item_name_plural,
                                    const uint64_t items, double seconds,
                                    bool space_before_item = true,
                                    bool compact = false);

std::string format_throughput_bytes(uint64_t bytes, double seconds);

enum class Time_type { LOCAL, GMT };

std::string fmttime(const char *fmt, Time_type type = Time_type::LOCAL,
                    const time_t *time_ptr = nullptr);

std::string isotime(const time_t *time_ptr = nullptr);

size_t expand_to_bytes(const std::string &number);

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_STRFORMAT_H_
