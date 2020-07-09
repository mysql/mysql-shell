/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DIALECT_DUMP_WRITER_H_
#define MODULES_UTIL_DUMP_DIALECT_DUMP_WRITER_H_

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_general.h"

#include "modules/util/dump/dump_writer.h"

namespace mysqlsh {
namespace dump {
namespace detail {

struct dialect_traits {};

struct default_traits : public dialect_traits {
  static constexpr char lines_terminated_by[] = "\n";
  static constexpr char fields_escaped_by[] = "\\";
  static constexpr char fields_terminated_by[] = "\t";
  static constexpr char fields_enclosed_by[] = "";
  static constexpr bool fields_optionally_enclosed = false;
};

struct json_traits : public dialect_traits {
  static constexpr char lines_terminated_by[] = "\n";
  static constexpr char fields_escaped_by[] = "";
  static constexpr char fields_terminated_by[] = "\n";
  static constexpr char fields_enclosed_by[] = "";
  static constexpr bool fields_optionally_enclosed = false;
};

struct csv_traits : public dialect_traits {
  static constexpr char lines_terminated_by[] = "\r\n";
  static constexpr char fields_escaped_by[] = "\\";
  static constexpr char fields_terminated_by[] = ",";
  static constexpr char fields_enclosed_by[] = "\"";
  static constexpr bool fields_optionally_enclosed = true;
};

struct tsv_traits : public dialect_traits {
  static constexpr char lines_terminated_by[] = "\r\n";
  static constexpr char fields_escaped_by[] = "\\";
  static constexpr char fields_terminated_by[] = "\t";
  static constexpr char fields_enclosed_by[] = "\"";
  static constexpr bool fields_optionally_enclosed = true;
};

struct csv_unix_traits : public dialect_traits {
  static constexpr char lines_terminated_by[] = "\n";
  static constexpr char fields_escaped_by[] = "\\";
  static constexpr char fields_terminated_by[] = ",";
  static constexpr char fields_enclosed_by[] = "\"";
  static constexpr bool fields_optionally_enclosed = false;
};

/**
 * This class provides a bit more optimized implementation of Text_dump_writer
 * and intends to only handle dialects supported by import/export utilities. If
 * a new dialect is added and any of the static asserts below are failed, the
 * whole implementation needs to be carefully analyzed.
 */
template <class T,
          std::enable_if_t<std::is_base_of<dialect_traits, T>::value, int> = 0>
class Dialect_dump_writer : public Dump_writer {
 public:
  Dialect_dump_writer() = delete;
  explicit Dialect_dump_writer(std::unique_ptr<mysqlshdk::storage::IFile> out)
      : Dump_writer(std::move(out)) {
    static_assert(
        s_lines_terminated_by_length >= 1 && s_lines_terminated_by_length <= 2,
        "Line terminator needs to be 1 or 2 characters long");
    static_assert(s_fields_escaped_by_length <= 1,
                  "FIELDS ESCAPED BY is a single character (can be not set)");
    static_assert(s_fields_terminated_by_length == 1,
                  "Field terminator needs to be 1 character long");
    static_assert(s_fields_enclosed_by_length <= 1,
                  "FIELDS ENCLOSED BY is a single character (can be not set)");
  }

  Dialect_dump_writer(const Dialect_dump_writer &) = delete;
  Dialect_dump_writer(Dialect_dump_writer &&) = default;

  Dialect_dump_writer &operator=(const Dialect_dump_writer &) = delete;
  Dialect_dump_writer &operator=(Dialect_dump_writer &&) = default;

  ~Dialect_dump_writer() override = default;

 private:
  void store_preamble(
      const std::vector<mysqlshdk::db::Column> &metadata) override {
    read_metadata(metadata);

    // no preamble
  }

  void store_row(const mysqlshdk::db::IRow *row) override {
    for (uint32_t idx = 0; idx < m_num_fields; ++idx) {
      store_field(row, idx);
    }

    finish_row();
  }

  void store_postamble() override {
    // no postamble
  }

  void read_metadata(const std::vector<mysqlshdk::db::Column> &metadata) {
    m_num_fields = static_cast<uint32_t>(metadata.size());

    m_is_string_type.clear();
    m_is_string_type.resize(m_num_fields);

    m_is_number_type.clear();
    m_is_number_type.resize(m_num_fields);

    std::size_t fixed_length = s_lines_terminated_by_length;

    if (m_num_fields > 0) {
      // field terminator is always one char
      fixed_length += m_num_fields - 1;
    }

    for (uint32_t i = 0; i < m_num_fields; ++i) {
      const auto type = metadata[i].get_type();
      const auto is_string = mysqlshdk::db::is_string_type(type);

      m_is_string_type[i] = is_string;
      // bit fields are transferred in binary format, should not be inspected
      // for any alpha characters, so they are not accidentally converted to
      // NULL
      m_is_number_type[i] = !is_string && mysqlshdk::db::Type::Bit != type;

      if (!T::fields_optionally_enclosed || is_string) {
        fixed_length += 2 * s_fields_enclosed_by_length;
      }
    }

    buffer()->set_fixed_length(fixed_length);
  }

  void store_field(const mysqlshdk::db::IRow *row, uint32_t idx) {
    if (0 != idx) {
      buffer()->append_fixed(T::fields_terminated_by[0]);
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
      quote_field(idx);
      store_field(data, length);
      quote_field(idx);
    }
  }

  inline void store_field(const char *data, std::size_t length) {
    store_field<s_fields_escaped_by_length>(data, length);
  }

  template <int N, std::enable_if_t<0 == N, int> = 0>
  inline void store_field(const char *data, std::size_t length) {
    // if FIELDS ESCAPED BY character is not specified, write as is
    buffer()->will_write(length);
    buffer()->append(data, length);
  }

  template <int N, std::enable_if_t<1 == N, int> = 0>
  inline void store_field(const char *data, std::size_t length) {
    // FIELDS ESCAPED BY character is specified, escape the string
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
          if (c == T::fields_escaped_by[0] || c == T::fields_terminated_by[0] ||
              c == T::lines_terminated_by[0] || should_escape(c)) {
            to_write = c;
          }
          break;
      }

      if (0 != to_write) {
        buffer()->append(T::fields_escaped_by[0]);
        buffer()->append(to_write);
      } else {
        buffer()->append(c);
      }
    }
  }

  inline bool should_escape(char c) {
    return should_escape<s_fields_enclosed_by_length>(c);
  }

  template <int N, std::enable_if_t<0 == N, int> = 0>
  inline bool should_escape(char) {
    // FIELDS ENCLOSED BY character is not specified, nothing to do
    return false;
  }

  template <int N, std::enable_if_t<1 == N, int> = 0>
  inline bool should_escape(char c) {
    // FIELDS ENCLOSED BY character is specified, escape it
    return c == T::fields_enclosed_by[0];
  }

  inline void quote_field(uint32_t idx) {
    quote_field<s_fields_enclosed_by_length>(idx);
  }

  template <int N, std::enable_if_t<0 == N, int> = 0>
  inline void quote_field(uint32_t) {
    // FIELDS ENCLOSED BY character is not specified, nothing to do
  }

  template <int N, std::enable_if_t<1 == N, int> = 0>
  inline void quote_field(uint32_t idx) {
    // FIELDS ENCLOSED BY character is specified, write it conditionally
    if (!T::fields_optionally_enclosed || m_is_string_type[idx]) {
      buffer()->append_fixed(T::fields_enclosed_by[0]);
    }
  }

  inline void store_null() { store_null<s_fields_escaped_by_length>(); }

  template <int N, std::enable_if_t<0 == N, int> = 0>
  inline void store_null() {
    // if FIELDS ESCAPED BY character is not specified, write "NULL"
    constexpr size_t length = 4;
    buffer()->will_write(length);
    buffer()->append("NULL", length);
  }

  template <int N, std::enable_if_t<1 == N, int> = 0>
  inline void store_null() {
    // FIELDS ESCAPED BY character is specified, write "\N"
    buffer()->will_write(2);
    buffer()->append(T::fields_escaped_by[0]);
    buffer()->append('N');
  }

  inline void finish_row() { finish_row<s_lines_terminated_by_length>(); }

  template <int N, std::enable_if_t<1 == N, int> = 0>
  inline void finish_row() {
    // LINES TERMINATED BY is one character long
    buffer()->append_fixed(T::lines_terminated_by[0]);
  }

  template <int N, std::enable_if_t<2 == N, int> = 0>
  inline void finish_row() {
    // LINES TERMINATED BY is two characters long
    buffer()->append_fixed(T::lines_terminated_by[0]);
    buffer()->append_fixed(T::lines_terminated_by[1]);
  }

  // array size includes the null termination byte
  static constexpr size_t s_lines_terminated_by_length =
      shcore::array_size(T::lines_terminated_by) - 1;

  static constexpr size_t s_fields_escaped_by_length =
      shcore::array_size(T::fields_escaped_by) - 1;

  static constexpr size_t s_fields_terminated_by_length =
      shcore::array_size(T::fields_terminated_by) - 1;

  static constexpr size_t s_fields_enclosed_by_length =
      shcore::array_size(T::fields_enclosed_by) - 1;

  uint32_t m_num_fields;

  // not using vectors of bool here, as they are not very efficient on access
  std::vector<int> m_is_string_type;

  std::vector<int> m_is_number_type;
};

}  // namespace detail

class Default_dump_writer
    : public detail::Dialect_dump_writer<detail::default_traits> {
 public:
  Default_dump_writer() = delete;
  using Dialect_dump_writer::Dialect_dump_writer;

  Default_dump_writer(const Default_dump_writer &) = delete;
  Default_dump_writer(Default_dump_writer &&) = default;

  Default_dump_writer &operator=(const Default_dump_writer &) = delete;
  Default_dump_writer &operator=(Default_dump_writer &&) = default;

  ~Default_dump_writer() override = default;
};

class Json_dump_writer
    : public detail::Dialect_dump_writer<detail::json_traits> {
 public:
  Json_dump_writer() = delete;
  using Dialect_dump_writer::Dialect_dump_writer;

  Json_dump_writer(const Json_dump_writer &) = delete;
  Json_dump_writer(Json_dump_writer &&) = default;

  Json_dump_writer &operator=(const Json_dump_writer &) = delete;
  Json_dump_writer &operator=(Json_dump_writer &&) = default;

  ~Json_dump_writer() override = default;
};

class Csv_dump_writer : public detail::Dialect_dump_writer<detail::csv_traits> {
 public:
  Csv_dump_writer() = delete;
  using Dialect_dump_writer::Dialect_dump_writer;

  Csv_dump_writer(const Csv_dump_writer &) = delete;
  Csv_dump_writer(Csv_dump_writer &&) = default;

  Csv_dump_writer &operator=(const Csv_dump_writer &) = delete;
  Csv_dump_writer &operator=(Csv_dump_writer &&) = default;

  ~Csv_dump_writer() override = default;
};

class Tsv_dump_writer : public detail::Dialect_dump_writer<detail::tsv_traits> {
 public:
  Tsv_dump_writer() = delete;
  using Dialect_dump_writer::Dialect_dump_writer;

  Tsv_dump_writer(const Tsv_dump_writer &) = delete;
  Tsv_dump_writer(Tsv_dump_writer &&) = default;

  Tsv_dump_writer &operator=(const Tsv_dump_writer &) = delete;
  Tsv_dump_writer &operator=(Tsv_dump_writer &&) = default;

  ~Tsv_dump_writer() override = default;
};

class Csv_unix_dump_writer
    : public detail::Dialect_dump_writer<detail::csv_unix_traits> {
 public:
  Csv_unix_dump_writer() = delete;
  using Dialect_dump_writer::Dialect_dump_writer;

  Csv_unix_dump_writer(const Csv_unix_dump_writer &) = delete;
  Csv_unix_dump_writer(Csv_unix_dump_writer &&) = default;

  Csv_unix_dump_writer &operator=(const Csv_unix_dump_writer &) = delete;
  Csv_unix_dump_writer &operator=(Csv_unix_dump_writer &&) = default;

  ~Csv_unix_dump_writer() override = default;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DIALECT_DUMP_WRITER_H_
