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

class invalid_json : public std::invalid_argument {
 public:
  invalid_json(const std::string &e, std::size_t offs)
      : std::invalid_argument(e), m_offset(offs) {
    m_msg += e;
    m_msg += " at offset ";
    m_msg += std::to_string(m_offset);
  }

  size_t offset() const { return m_offset; }

  const char *what() const noexcept override { return m_msg.c_str(); }

 private:
  size_t m_offset;    //< Byte offset from file beginning
  std::string m_msg;  //< Exception message
};

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

  byte get() {
    byte c = peek();
    ++m_pos;
    ++m_bytes_processed;
    return c;
  }

  size_t offset() { return m_bytes_processed; }

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

/**
 * Find the end of the JSON document in the given buffered input, assuming
 * syntax is correct.
 *
 * This function assumes the input contains valid JSON data and will not perform
 * any validation except for balancing of {} and [] and strings.
 *
 * The purpose is to split buffers containing many JSON documents, which will
 * be processed/parsed later on by an actual JSON parser.
 *
 * @param input Input with JSON document.
 * @return
 */
std::string span_one_maybe_json(shcore::Buffered_input *input);

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_BUFFERED_INPUT_H_
