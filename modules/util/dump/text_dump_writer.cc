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

#include "modules/util/dump/text_dump_writer.h"

#include <utility>

namespace mysqlsh {
namespace dump {

Text_dump_writer::Text_dump_writer(
    std::unique_ptr<mysqlshdk::storage::IFile> out,
    const import_table::Dialect &dialect)
    : Dump_writer(std::move(out)), m_dialect(dialect), m_escaped_characters() {
  if (m_dialect.lines_terminated_by.empty()) {
    m_line_terminator = m_dialect.fields_terminated_by;
  } else {
    m_line_terminator = m_dialect.lines_terminated_by;
  }

  m_escape = !m_dialect.fields_escaped_by.empty();

  m_null = m_escape ? m_dialect.fields_escaped_by + "N" : "NULL";

  if (m_escape) {
    m_escape_char = m_dialect.fields_escaped_by[0];

    // escape if character is:
    // - FIELDS ESCAPED BY character
    // - FIELDS ENCLOSED BY character
    // - if the FIELDS ENCLOSED BY is empty, the first character of the FIELDS
    //   TERMINATED BY or LINES TERMINATED BY values
    size_t idx = 0;
    m_escaped_characters[idx++] = m_escape_char;

    if (m_dialect.fields_enclosed_by.empty()) {
      if (!m_dialect.fields_terminated_by.empty()) {
        m_escaped_characters[idx++] = m_dialect.fields_terminated_by[0];
      }

      if (!m_dialect.lines_terminated_by.empty()) {
        m_escaped_characters[idx++] = m_dialect.lines_terminated_by[0];
      }
    } else {
      m_escaped_characters[idx++] = m_dialect.fields_enclosed_by[0];
    }
  }
}

void Text_dump_writer::store_preamble(
    const std::vector<mysqlshdk::db::Column> &metadata) {
  read_metadata(metadata);

  // no preamble
}

void Text_dump_writer::store_row(const mysqlshdk::db::IRow *row) {
  start_row();

  for (uint32_t idx = 0; idx < m_num_fields; ++idx) {
    store_field(row, idx);
  }

  finish_row();
}

void Text_dump_writer::store_postamble() {
  // no postamble
}

void Text_dump_writer::read_metadata(
    const std::vector<mysqlshdk::db::Column> &metadata) {
  m_num_fields = static_cast<uint32_t>(metadata.size());

  m_is_string_type.clear();
  m_is_string_type.resize(m_num_fields);

  m_is_number_type.clear();
  m_is_number_type.resize(m_num_fields);

  std::size_t fixed_length =
      m_dialect.lines_starting_by.length() + m_line_terminator.length();

  if (m_num_fields > 0) {
    fixed_length +=
        (m_num_fields - 1) * m_dialect.fields_terminated_by.length();
  }

  for (uint32_t i = 0; i < m_num_fields; ++i) {
    const auto type = metadata[i].get_type();
    const auto is_string = mysqlshdk::db::is_string_type(type);

    m_is_string_type[i] = is_string;
    // bit fields are transferred in binary format, should not be inspected for
    // any alpha characters, so they are not accidentally converted to NULL
    m_is_number_type[i] = !is_string && mysqlshdk::db::Type::Bit != type;

    if (!m_dialect.fields_optionally_enclosed || m_is_string_type[i]) {
      fixed_length += 2 * m_dialect.fields_enclosed_by.length();
    }
  }

  buffer()->set_fixed_length(fixed_length);
}

void Text_dump_writer::start_row() {
  buffer()->append_fixed(m_dialect.lines_starting_by);
}

void Text_dump_writer::store_field(const mysqlshdk::db::IRow *row,
                                   uint32_t idx) {
  // TODO(pawel): implement a fixed-row format:
  //              https://dev.mysql.com/doc/refman/8.0/en/load-data.html

  if (0 != idx) {
    buffer()->append_fixed(m_dialect.fields_terminated_by);
  }

  quote_field(idx);

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
    if (m_escape) {
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
            if (c == m_escaped_characters[0] || c == m_escaped_characters[1] ||
                c == m_escaped_characters[2]) {
              to_write = c;
            }
            break;
        }

        if (0 != to_write) {
          buffer()->append(m_escape_char);
          buffer()->append(to_write);
        } else {
          buffer()->append(c);
        }
      }
    } else {
      buffer()->will_write(length);
      buffer()->append(data, length);
    }
  }

  quote_field(idx);
}

void Text_dump_writer::quote_field(uint32_t idx) {
  if (!m_dialect.fields_optionally_enclosed || m_is_string_type[idx]) {
    buffer()->append_fixed(m_dialect.fields_enclosed_by);
  }
}

void Text_dump_writer::store_null() {
  const auto length = m_null.length();
  buffer()->will_write(length);
  buffer()->append(m_null.c_str(), length);
}

void Text_dump_writer::finish_row() {
  buffer()->append_fixed(m_line_terminator);
}

}  // namespace dump
}  // namespace mysqlsh
