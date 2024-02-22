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

#include "mysqlshdk/libs/utils/rate_limit.h"

#include <ratio>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace utils {

constexpr int k_micro = 1000000;

void Rate_limit::throttle(int64_t bytes) {
  m_now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::micro> diff = m_now - m_last;
  if (diff.count() < 0) {
    return;
  }

  int64_t allowed_bytes =
      m_unused_bytes + (diff.count() * m_bytes_limit / k_micro);
  m_last = m_now;

  if (bytes <= allowed_bytes) {
    m_unused_bytes = allowed_bytes - bytes;
    return;
  }

  auto over_sent = bytes - allowed_bytes;
  auto sleep_us = k_micro * over_sent / m_bytes_limit;

  m_last += std::chrono::duration<long, std::micro>(sleep_us);

  shcore::sleep_ms(sleep_us / 1000);
}
} /* namespace utils */
} /* namespace mysqlshdk */
