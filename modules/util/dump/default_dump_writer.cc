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

#include "modules/util/dump/default_dump_writer.h"

#include <utility>

namespace mysqlsh {
namespace dump {

namespace {

constexpr auto k_fields_escaped_by = '\\';
constexpr auto k_field_terminator = '\t';
constexpr auto k_line_terminator = '\n';

constexpr auto k_escaped_characters = "\\\t\n";

constexpr auto k_null = "\\N";
constexpr size_t k_null_length = 2;

}  // namespace

Default_dump_writer::Default_dump_writer(
    std::unique_ptr<mysqlshdk::storage::IFile> out)
    : Dump_writer(std::move(out)) {}

void Default_dump_writer::store_preamble(
    const std::vector<mysqlshdk::db::Column> &metadata) {
  read_metadata(metadata);

  // no preamble
}

void Default_dump_writer::store_row(const mysqlshdk::db::IRow *row) {
  for (uint32_t idx = 0; idx < m_num_fields; ++idx) {
    store_field(row, idx);
  }

  finish_row();
}

void Default_dump_writer::store_postamble() {
  // no postamble
}

void Default_dump_writer::read_metadata(
    const std::vector<mysqlshdk::db::Column> &metadata) {
  m_num_fields = static_cast<uint32_t>(metadata.size());

  m_is_string_type.clear();
  m_is_string_type.resize(m_num_fields);

  m_is_number_type.clear();
  m_is_number_type.resize(m_num_fields);

  std::size_t fixed_length = 1;  // k_line_terminator is one char

  if (m_num_fields > 0) {
    // k_field_terminator is one char
    fixed_length += m_num_fields - 1;
  }

  for (uint32_t i = 0; i < m_num_fields; ++i) {
    const auto type = metadata[i].get_type();
    const auto is_string = mysqlshdk::db::is_string_type(type);

    m_is_string_type[i] = is_string;
    // bit fields are transferred in binary format, should not be inspected for
    // any alpha characters, so they are not accidentally converted to NULL
    m_is_number_type[i] = !is_string && mysqlshdk::db::Type::Bit != type;

    // fields are not enclosed
  }

  buffer()->set_fixed_length(fixed_length);
}

void Default_dump_writer::store_field(const mysqlshdk::db::IRow *row,
                                      uint32_t idx) {
  if (0 != idx) {
    buffer()->append_fixed(k_field_terminator);
  }

  const char *data = nullptr;
  std::size_t length = 0;
  row->get_raw_data(idx, &data, &length);

  bool is_null = nullptr == data;

  if (!is_null) {
    if (m_is_number_type[idx]) {
      if ((length > 0 && std::isalpha(data[0])) ||
          (length > 1 && '-' == data[0] && std::isalpha(data[1]))) {
        // convert any strings ("inf", "-inf", "nan") into NULL
        is_null = true;
      }
      // handle numeric types just like strings, if '.' is i.e. FIELDS
      // ENCLOSED BY character then it needs to be escaped
    }
  }

  if (is_null) {
    store_null();
  } else {
    buffer()->will_write(2 * length);
    const auto end = data + length;

    for (auto p = data; p != end; ++p) {
      const auto c = *p;
      char to_write = 0;

      // note: this doesn't produce output consistent with SELECT .. INTO
      // OUTFILE (i.e. tabs are escaped), but LOAD DATA INFILE handles
      // this correctly and escaping i.e. carriage return characters helps
      // with readability

      switch (c) {
        case '\0':
          to_write = '0';
          break;

        case '\b':
          to_write = 'b';
          break;

        case '\n':
          to_write = 'n';
          break;

        case '\r':
          to_write = 'r';
          break;

        case '\t':
          to_write = 't';
          break;

        case 0x1A:  // ASCII 26
          to_write = 'Z';
          break;

        default:
          if (c == k_escaped_characters[0] || c == k_escaped_characters[1] ||
              c == k_escaped_characters[2]) {
            to_write = c;
          }
          break;
      }

      if (0 != to_write) {
        buffer()->append(k_fields_escaped_by);
        buffer()->append(to_write);
      } else {
        buffer()->append(c);
      }
    }
  }
}

void Default_dump_writer::store_null() {
  buffer()->will_write(k_null_length);
  buffer()->append(k_null, k_null_length);
}

void Default_dump_writer::finish_row() {
  buffer()->append_fixed(k_line_terminator);
}

}  // namespace dump
}  // namespace mysqlsh
