/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/in_memory/synchronized_file.h"

#include <chrono>
#include <cstring>

namespace mysqlshdk {
namespace storage {
namespace in_memory {

namespace {

using namespace std::chrono_literals;

constexpr auto k_sleep_interval = 100ms;

}  // namespace

Synchronized_file::Synchronized_file(const std::string &name,
                                     shcore::atomic_flag *interrupted)
    : IFile(name), m_interrupted(interrupted) {}

bool Synchronized_file::is_open() const { return m_reading || m_writing; }

void Synchronized_file::open(bool read_mode) {
  std::lock_guard lock{m_open_close_mutex};

  if ((read_mode && m_reading) || (!read_mode && m_writing)) {
    throw std::runtime_error("Unable to open file: " + name() +
                             ", it is already open");
  }

  if (read_mode) {
    m_reading = true;
  } else {
    m_writing = true;
  }
}

void Synchronized_file::close() {
  std::lock_guard lock{m_open_close_mutex};

  if (!is_open()) {
    throw std::runtime_error("Unable to close file: " + name() +
                             ", it is already closed");
  }

  // first close request has to come from the writer, as reader is still
  // waiting for the input
  if (m_writing) {
    if (auto r = m_read_requests.try_pop(1ms);
        r.value_or(false) && m_request.written > 0) {
      // if there's a read operation pending, signal that it's finished
      m_read_responses.push({m_request.written});
    }

    // signal EOF to the reader
    m_read_responses.push({0});

    m_writing = false;
  } else {
    m_reading = false;
  }
}

off64_t Synchronized_file::tell() {
  if (!is_open()) {
    throw std::runtime_error("Unable to fetch offset of file: " + name() +
                             ", it is not open");
  }

  return m_size;
}

ssize_t Synchronized_file::read(void *buffer, std::size_t length) {
  if (!is_open()) {
    throw std::runtime_error("Unable to read from file: " + name() +
                             ", it is not open");
  }

  if (!m_reading) {
    throw std::runtime_error("Unable to read from file: " + name() +
                             ", it is opened for writing");
  }

  m_request.buffer = buffer;
  m_request.length = length;
  m_request.written = 0;
  m_read_requests.push(true);

  while (true) {
    if (is_interrupted()) {
      return 0;
    }

    if (const auto response = m_read_responses.try_pop(k_sleep_interval);
        response.has_value()) {
      return response->length;
    }
  }
}

ssize_t Synchronized_file::write(const void *buffer, std::size_t length) {
  if (!is_open()) {
    throw std::runtime_error("Unable to write to file: " + name() +
                             ", it is not open");
  }

  if (!m_writing) {
    throw std::runtime_error("Unable to write to file: " + name() +
                             ", it is opened for reading");
  }

  m_pending_write = length;

  while (true) {
    if (is_interrupted()) {
      return 0;
    }

    if (auto r = m_read_requests.try_pop(k_sleep_interval); r.value_or(false)) {
      if (length > m_request.length) {
        // write buffer is longer than the read buffer, signal that it is full
        auto response = m_request.written;

        if (0 == m_request.written) {
          // the read buffer is too small, signal this to the reader, it is
          // either going to provide a bigger one, or abort the operation
          response = -1;
        }

        m_read_responses.push({response});
      } else {
        ::memcpy(m_request.buffer, buffer, length);

        m_size += length;
        m_request.buffer = static_cast<char *>(m_request.buffer) + length;
        m_request.written += length;
        m_request.length -= length;

        m_pending_write = 0;

        if (0 == m_request.length) {
          // read buffer is full
          m_read_responses.push({m_request.written});
        } else {
          // push the read buffer back to the queue
          m_read_requests.push(true);
        }

        return length;
      }
    }
  }
}

bool Synchronized_file::is_interrupted() const {
  return m_interrupted && m_interrupted->test();
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
