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

#ifndef MYSQLSHDK_LIBS_UTILS_RATE_LIMIT_H_
#define MYSQLSHDK_LIBS_UTILS_RATE_LIMIT_H_

#include <sys/types.h>
#include <chrono>
#include <cstdint>

namespace mysqlshdk {
namespace utils {

class Rate_limit final {
 public:
  Rate_limit() = default;

  explicit Rate_limit(int64_t limit)
      : m_bytes_limit(limit), m_unused_bytes(0), m_now() {
    m_last = std::chrono::high_resolution_clock::now();
  }

  Rate_limit(const Rate_limit &other) = default;
  Rate_limit(Rate_limit &&other) = default;

  Rate_limit &operator=(const Rate_limit &other) = default;
  Rate_limit &operator=(Rate_limit &&other) = default;

  ~Rate_limit() = default;

  bool enabled() { return m_bytes_limit > 0; }

  void throttle(int64_t bytes);

 private:
  int64_t m_bytes_limit = 0;
  int64_t m_unused_bytes = 0;
  std::chrono::high_resolution_clock::time_point m_now{};
  std::chrono::high_resolution_clock::time_point m_last{};
};

} /* namespace utils */
} /* namespace mysqlshdk */

#endif /* MYSQLSHDK_LIBS_UTILS_RATE_LIMIT_H_ */
