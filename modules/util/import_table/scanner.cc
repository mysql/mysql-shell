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

#include "modules/util/import_table/scanner.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

namespace mysqlsh {
namespace import_table {

namespace {

constexpr const int k_not_used = std::numeric_limits<int>::max();
constexpr const int k_end_of_block = std::numeric_limits<int>::min();

int first_char(const std::string &s) { return s.length() ? s[0] : k_not_used; }

inline bool used(int c) noexcept { return k_not_used != c; }

}  // namespace

Scanner::Sequence::Sequence(const std::string &s) {
  first = first_char(s);

  if (used()) {
    ptr = s.c_str();
    length = s.length();
  } else {
    ptr = nullptr;
    length = 0;
  }
}

bool Scanner::Sequence::used() const noexcept {
  return import_table::used(first);
}

Scanner::Scanner(const Dialect &dialect, uint64_t skip_rows)
    : m_skip_rows(skip_rows),
      m_fields_terminated_by(dialect.fields_terminated_by),
      m_lines_starting_by(dialect.lines_starting_by),
      m_lines_terminated_by(dialect.lines_terminated_by),
      m_enclosed_char(first_char(dialect.fields_enclosed_by)),
      m_escaped_char(first_char(dialect.fields_escaped_by)) {
  if (dialect.lines_terminated_by.empty() ||
      dialect.lines_terminated_by == dialect.fields_terminated_by) {
    throw std::invalid_argument("Scanner: unsupported LINES TERMINATED BY: '" +
                                dialect.lines_terminated_by + "'");
  }

  // we need to be able to store terminator + FIELDS ENCLOSED BY character
  m_stack.resize(
      std::max({m_fields_terminated_by.length + 1, m_lines_starting_by.length,
                m_lines_terminated_by.length + 1}));
  m_stack_bottom = m_stack_position = m_stack.data();
}

int64_t Scanner::scan(const char *data, std::size_t length) noexcept {
  assert(data);
  assert(length);

  m_data = data;
  m_length = length;
  m_end_of_block = false;

  static constexpr int64_t k_row_not_found = -1;

  while (m_skip_rows > 0) {
    if (skip_row()) {
      --m_skip_rows;
    } else {
      return k_row_not_found;
    }
  }

  int64_t first_row = k_row_not_found;

  while (m_length) {
    switch (m_status) {
      case Row_status::BEGIN:
        if (k_row_not_found == first_row) {
          first_row = length - m_length;
        }

        m_status = Row_status::PREFIX;
        break;

      case Row_status::PREFIX:
        if (!skip_line_start()) {
          return first_row;
        }

        m_status = Row_status::BEGIN_OF_FIELD;
        break;

      case Row_status::BEGIN_OF_FIELD: {
        const auto chr = get();

        if (chr == m_enclosed_char) {
          m_found_enclosed_char = chr;
        } else {
          m_found_enclosed_char = k_not_used;
          // field is not enclosed, store the character for further processing
          unget(chr);
        }

        m_status = Row_status::FIELD;
        break;
      }

      case Row_status::FIELD:
        switch (scan_field()) {
          case Field_status::CONTINUED:
            // no-op
            break;

          case Field_status::END_OF_FIELD:
            m_status = Row_status::BEGIN_OF_FIELD;
            break;

          case Field_status::END_OF_LINE:
            m_status = Row_status::BEGIN;
            break;
        }
        break;
    }
  }

  return first_row;
}

int Scanner::get() noexcept {
  if (m_stack_position != m_stack_bottom) {
    return *--m_stack_position;
  } else {
    if (m_length > 0) {
      --m_length;
      return *m_data++;
    } else {
      m_end_of_block = true;
      return k_end_of_block;
    }
  }
}

void Scanner::unget(char c) noexcept {
  assert(m_stack_position < m_stack_bottom + m_stack.size());
  *m_stack_position++ = c;
}

bool Scanner::contains(const Sequence &s) noexcept {
  int chr = 0;
  std::size_t i;
  auto ptr = s.ptr;

  for (i = 1; i < s.length; ++i) {
    chr = get();

    if (chr != *++ptr) {
      break;
    }
  }

  if (s.length == i) {
    return true;
  }

  // if we reach the end of block while looking for the sequence, we don't want
  // to store the end of block marker
  if (!m_end_of_block) {
    unget(chr);
  }

  while (i-- > 1) {
    unget(*--ptr);
  }

  return false;
}

bool Scanner::skip_row() noexcept {
  int chr;

  while (m_length) {
    chr = get();

    // check for escaped LINES TERMINATED BY sequences
    if (chr == m_escaped_char) {
      chr = get();

      if (m_end_of_block) {
        unget(m_escaped_char);
      }

      continue;
    }

    if (chr == m_lines_terminated_by.first && contains(m_lines_terminated_by)) {
      return true;
    }

    if (m_end_of_block) {
      // we've reached the end of block, restore the full sequence for the next
      // scan
      unget(chr);
    }
  }

  return false;
}

bool Scanner::skip_line_start() noexcept {
  if (!m_lines_starting_by.used()) {
    return true;
  }

  int chr;

  while (m_length) {
    chr = get();

    if (chr == m_lines_starting_by.first && contains(m_lines_starting_by)) {
      return true;
    }

    if (m_end_of_block) {
      // we've reached the end of block, restore the full sequence for the next
      // scan
      unget(chr);
    }
  }

  return false;
}

Scanner::Field_status Scanner::scan_field() noexcept {
  int chr;

// if block has ended, store the last character of this block, so it can be used
// with the next block
#define HANDLE_END_OF_BLOCK(...)      \
  do {                                \
    if (m_end_of_block) {             \
      unget(__VA_ARGS__);             \
      return Field_status::CONTINUED; \
    }                                 \
  } while (false)

  while (m_length) {
    chr = get();

    if (chr == m_escaped_char) {
      chr = get();

      HANDLE_END_OF_BLOCK(m_escaped_char);

      // when ESCAPED BY == ENCLOSED BY, server only allows for doubled-up
      // escape characters, ignoring any other escape sequences
      if (m_escaped_char != m_enclosed_char || chr == m_escaped_char) {
        continue;
      }

      // this was not an escape sequence, continue with processing
      unget(chr);
      chr = m_escaped_char;
    }

    if (!used(m_found_enclosed_char) && chr == m_lines_terminated_by.first) {
      if (contains(m_lines_terminated_by)) {
        return Field_status::END_OF_LINE;
      }

      HANDLE_END_OF_BLOCK(chr);
    }

    if (chr == m_found_enclosed_char) {
      chr = get();

      HANDLE_END_OF_BLOCK(m_found_enclosed_char);

      // doubled ENCLOSED BY character
      if (chr == m_found_enclosed_char) {
        continue;
      }

      if (chr == m_lines_terminated_by.first) {
        if (contains(m_lines_terminated_by)) {
          return Field_status::END_OF_LINE;
        }

        // we need to restore both characters, because we want to end up here
        // when we are scanning the next block
        HANDLE_END_OF_BLOCK(chr, m_found_enclosed_char);
      }

      if (chr == m_fields_terminated_by.first) {
        if (contains(m_fields_terminated_by)) {
          return Field_status::END_OF_FIELD;
        }

        // same as above
        HANDLE_END_OF_BLOCK(chr, m_found_enclosed_char);
      }

      // this was not an end of field, continue with processing
      unget(chr);
    } else if (!used(m_found_enclosed_char) &&
               chr == m_fields_terminated_by.first) {
      if (contains(m_fields_terminated_by)) {
        return Field_status::END_OF_FIELD;
      }

      HANDLE_END_OF_BLOCK(chr);
    }
  }

#undef HANDLE_END_OF_BLOCK

  return Field_status::CONTINUED;
}

}  // namespace import_table
}  // namespace mysqlsh
