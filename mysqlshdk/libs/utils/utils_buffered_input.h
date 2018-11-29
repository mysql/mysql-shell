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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_BUFFERED_INPUT_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_BUFFERED_INPUT_H_

#include <string.h>
#include <string>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace shcore {

/**
 * Forward read only buffered input.
 */
class Buffered_input {
  using byte = unsigned char;

 public:
  Buffered_input() = default;
  explicit Buffered_input(const std::string &filepath) { open(filepath); }

  Buffered_input(const Buffered_input &other) = delete;
  Buffered_input(Buffered_input &&other) = delete;

  Buffered_input &operator=(const Buffered_input &other) = delete;
  Buffered_input &operator=(Buffered_input &&other) = delete;

  ~Buffered_input() { close(); }

  void open(const std::string &filepath_);

  bool eof() { return m_eof; }

  byte peek() {
    if (m_pos == m_end) {
      fill_buffer();
    }
    return *m_pos;
  }

  void seek(byte *pos) { m_pos = pos > m_end ? m_end : pos; }

  byte get() {
    byte c = peek();
    ++m_pos;
    ++m_bytes_processed;
    return c;
  }

  size_t offset() { return m_bytes_processed; }
  byte *pos() const { return m_pos; }
  byte *end() const { return m_end; }

  void skip_whitespaces() {
    while (::isspace(peek())) {
      get();
    }
  }

  std::string get_double_quoted_string();

 private:
  void close();

  void fill_buffer();

  static constexpr const size_t BUFFER_SIZE = 1 << 16;
  int m_fd = 0;
  bool m_eof = false;
  byte m_buffer[BUFFER_SIZE];
  byte *m_pos = m_buffer;
  byte *m_end = m_buffer;
  size_t m_bytes_processed = 0;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_BUFFERED_INPUT_H_
