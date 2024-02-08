/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_TEXT_DUMP_WRITER_H_
#define MODULES_UTIL_DUMP_TEXT_DUMP_WRITER_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/util/dump/dump_writer.h"
#include "modules/util/import_table/dialect.h"

namespace mysqlsh {
namespace dump {

class Text_dump_writer : public Dump_writer {
 public:
  Text_dump_writer() = default;
  explicit Text_dump_writer(const import_table::Dialect &dialect);

  Text_dump_writer(const Text_dump_writer &) = delete;
  Text_dump_writer(Text_dump_writer &&) = default;

  Text_dump_writer &operator=(const Text_dump_writer &) = delete;
  Text_dump_writer &operator=(Text_dump_writer &&) = default;

  ~Text_dump_writer() override = default;

 private:
  void store_preamble(
      const std::vector<mysqlshdk::db::Column> &metadata,
      const std::vector<Encoding_type> &pre_encoded_columns) override;

  void store_row(const mysqlshdk::db::IRow *row) override;

  void store_postamble() override;

  void read_metadata(const std::vector<mysqlshdk::db::Column> &metadata,
                     const std::vector<Encoding_type> &pre_encoded_columns);

  void start_row();

  void store_field(const mysqlshdk::db::IRow *row, uint32_t idx);

  void quote_field(uint32_t idx);

  void store_null();

  void finish_row();

  import_table::Dialect m_dialect;

  std::string m_line_terminator;

  std::string m_null;

  bool m_escape;

  char m_escaped_characters[4];

  char m_escape_char;

  bool m_double_enclosed_by = false;

  Escape_type m_numbers_need_escape = Escape_type::NONE;
  Escape_type m_hex_need_escape = Escape_type::NONE;
  Escape_type m_base64_need_escape = Escape_type::BASE64;

  uint32_t m_num_fields;

  // not using vectors of bool here, as they are not very efficient on access
  std::vector<int> m_is_string_type;

  std::vector<int> m_is_number_type;

  std::vector<Escape_type> m_needs_escape;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_TEXT_DUMP_WRITER_H_
