/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/sigint_event.h"

#include <chrono>

namespace shcore {

Sigint_event &Sigint_event::get() {
  static Sigint_event instance;
  return instance;
}

void Sigint_event::interrupt() {
  std::unique_lock<std::mutex> lock(m_mutex);

  if (m_threads > 0) {
    m_interrupted = true;
    m_condition.notify_all();
  }
}

void Sigint_event::wait(uint32_t ms) {
  std::unique_lock<std::mutex> lock(m_mutex);

  ++m_threads;

  m_condition.wait_for(lock, std::chrono::milliseconds(ms),
                       [this]() { return m_interrupted; });

  if (0 == --m_threads) {
    m_interrupted = false;
  }
}

}  // namespace shcore
