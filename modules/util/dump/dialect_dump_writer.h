/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DIALECT_DUMP_WRITER_H_
#define MODULES_UTIL_DUMP_DIALECT_DUMP_WRITER_H_

#include <cassert>
#include <cctype>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "modules/util/dump/dump_writer.h"

namespace mysqlsh {
namespace dump {
namespace detail {

struct dialect_traits {};

struct default_traits : public dialect_traits {
  static constexpr std::string_view lines_terminated_by = "\n";
  static constexpr std::string_view fields_escaped_by = "\\";
  static constexpr std::string_view fields_terminated_by = "\t";
  static constexpr std::string_view fields_enclosed_by = "";
  static constexpr bool fields_optionally_enclosed = false;
};

struct json_traits : public dialect_traits {
  static constexpr std::string_view lines_terminated_by = "\n";
  static constexpr std::string_view fields_escaped_by = "";
  static constexpr std::string_view fields_terminated_by = "\n";
  static constexpr std::string_view fields_enclosed_by = "";
  static constexpr bool fields_optionally_enclosed = false;
};

struct csv_traits : public dialect_traits {
  static constexpr std::string_view lines_terminated_by = "\r\n";
  static constexpr std::string_view fields_escaped_by = "\\";
  static constexpr std::string_view fields_terminated_by = ",";
  static constexpr std::string_view fields_enclosed_by = "\"";
  static constexpr bool fields_optionally_enclosed = true;
};

struct tsv_traits : public dialect_traits {
  static constexpr std::string_view lines_terminated_by = "\r\n";
  static constexpr std::string_view fields_escaped_by = "\\";
  static constexpr std::string_view fields_terminated_by = "\t";
  static constexpr std::string_view fields_enclosed_by = "\"";
  static constexpr bool fields_optionally_enclosed = true;
};

struct csv_unix_traits : public dialect_traits {
  static constexpr std::string_view lines_terminated_by = "\n";
  static constexpr std::string_view fields_escaped_by = "\\";
  static constexpr std::string_view fields_terminated_by = ",";
  static constexpr std::string_view fields_enclosed_by = "\"";
  static constexpr bool fields_optionally_enclosed = false;
};

struct csv_rfc_unix_traits : public dialect_traits {
  static constexpr std::string_view lines_terminated_by = "\n";
  static constexpr std::string_view fields_escaped_by = "";
  static constexpr std::string_view fields_terminated_by = ",";
  static constexpr std::string_view fields_enclosed_by = "\"";
  static constexpr bool fields_optionally_enclosed = true;
};

/**
 * This class provides a bit more optimized implementation of Text_dump_writer
 * and intends to only handle dialects supported by import/export utilities. If
 * a new dialect is added and any of the static asserts below are failed, the
 * whole implementation needs to be carefully analyzed.
 */
template <class Traits>
  requires std::is_base_of_v<dialect_traits, Traits>
class Dialect_dump_writer : public Dump_writer {
 public:
  Dialect_dump_writer() {
    static_assert(
        s_lines_terminated_by_length >= 1 && s_lines_terminated_by_length <= 2,
        "Line terminator needs to be 1 or 2 characters long");
    static_assert(s_fields_escaped_by_length <= 1,
                  "FIELDS ESCAPED BY is a single character (can be not set)");
    static_assert(s_fields_terminated_by_length == 1,
                  "Field terminator needs to be 1 character long");
    static_assert(s_fields_enclosed_by_length <= 1,
                  "FIELDS ENCLOSED BY is a single character (can be not set)");
    // NOTE: this is required by the Encoding_type::VECTOR, it's stored via
    //       VECTOR_TO_STRING and contains commas; in order to skip encoding
    //       comma cannot need to be escaped
    static_assert(
        s_fields_enclosed_by_length || (s_fields_terminated_by_length &&
                                        ',' != Traits::fields_terminated_by[0]),
        "FIELDS ENCLOSED BY is set or field terminator is not a comma");
  }

  Dialect_dump_writer(const Dialect_dump_writer &) = delete;
  Dialect_dump_writer(Dialect_dump_writer &&) = default;

  Dialect_dump_writer &operator=(const Dialect_dump_writer &) = delete;
  Dialect_dump_writer &operator=(Dialect_dump_writer &&) = default;

  ~Dialect_dump_writer() override = default;

 private:
  void store_row(const mysqlshdk::db::IRow *row) override {
    for (uint32_t idx = 0; idx < m_num_fields; ++idx) {
      store_field(row, idx);
    }

    finish_row();
  }

  void read_metadata(
      const std::vector<mysqlshdk::db::Column> &metadata,
      const std::vector<Encoding_type> &pre_encoded_columns) override {
    m_num_fields = static_cast<uint32_t>(metadata.size());

    m_is_string_type.clear();
    m_is_string_type.resize(m_num_fields);

    m_is_number_type.clear();
    m_is_number_type.resize(m_num_fields);

    m_needs_escape.clear();
    m_needs_escape.resize(m_num_fields);

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

      // find columns that are safe to not escape
      m_needs_escape[i] = Escape_type::FULL;
      if (m_is_number_type[i]) {
        m_needs_escape[i] = Escape_type::NONE;
      } else if (pre_encoded_columns.size() == metadata.size()) {
        if (pre_encoded_columns[i] == Encoding_type::BASE64)
          m_needs_escape[i] = Escape_type::BASE64;
        else if (pre_encoded_columns[i] == Encoding_type::HEX ||
                 pre_encoded_columns[i] == Encoding_type::VECTOR)
          m_needs_escape[i] = Escape_type::NONE;
      }

      if constexpr (!Traits::fields_optionally_enclosed) {
        fixed_length += 2 * s_fields_enclosed_by_length;
      } else {
        if (is_string) {
          fixed_length += 2 * s_fields_enclosed_by_length;
        }
      }
    }

    buffer()->set_fixed_length(fixed_length);
  }

  void store_field(const mysqlshdk::db::IRow *row, uint32_t idx) {
    if (0 != idx) {
      buffer()->append_fixed(Traits::fields_terminated_by[0]);
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
      switch (m_needs_escape[idx]) {
        case Escape_type::FULL:
          store_field(data, length);
          break;
        case Escape_type::BASE64:
          store_base64_field(data, length);
          break;
        case Escape_type::NONE:
          store_unescaped_field(data, length);
          break;
      }
      quote_field(idx);
    }
  }

  inline void store_base64_field(const char *data, std::size_t length) {
    buffer()->write_base64_data(data, length);
  }

  inline void store_field(const char *data, std::size_t length) {
    if constexpr (0 == s_fields_escaped_by_length) {
      store_unescaped_field(data, length);
    } else {
      store_escaped_field(data, length);
    }
  }

  inline void store_unescaped_field(const char *data, std::size_t length) {
    // if FIELDS ESCAPED BY character is not specified, write as is
    buffer()->will_write(length);
    buffer()->append(data, length);
  }

  inline void store_escaped_field(const char *data, std::size_t length) {
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
          if (c == Traits::fields_escaped_by[0] ||
              c == Traits::lines_terminated_by[0] || should_escape(c)) {
            to_write = c;
          }
          break;
      }

      if (0 != to_write) {
        buffer()->append(Traits::fields_escaped_by[0]);
        buffer()->append(to_write);
      } else {
        buffer()->append(c);
      }
    }
  }

  inline bool should_escape(char c) {
    if constexpr (0 == s_fields_enclosed_by_length) {
      // FIELDS ENCLOSED BY character is not specified, escape first character
      // of FIELDS TERMINATED BY
      return c == Traits::fields_terminated_by[0];
    } else {
      // FIELDS ENCLOSED BY character is specified, escape it
      return c == Traits::fields_enclosed_by[0];
    }
  }

  inline void quote_field(uint32_t idx) {
    if constexpr (1 == s_fields_enclosed_by_length) {
      // FIELDS ENCLOSED BY character is specified
      if constexpr (!Traits::fields_optionally_enclosed) {
        // all fields are quoted
        buffer()->append_fixed(Traits::fields_enclosed_by[0]);
      } else {
        // quote conditionally
        if (m_is_string_type[idx]) {
          buffer()->append_fixed(Traits::fields_enclosed_by[0]);
        }
      }
    }
  }

  inline void store_null() {
    if constexpr (0 == s_fields_escaped_by_length) {
      // if FIELDS ESCAPED BY character is not specified, write "NULL"
      constexpr size_t length = 4;
      buffer()->will_write(length);
      buffer()->append("NULL", length);
    } else {
      // FIELDS ESCAPED BY character is specified, write "\N"
      buffer()->will_write(2);
      buffer()->append(Traits::fields_escaped_by[0]);
      buffer()->append('N');
    }
  }

  inline void finish_row() {
    buffer()->append_fixed(Traits::lines_terminated_by[0]);

    if constexpr (2 == s_lines_terminated_by_length) {
      // LINES TERMINATED BY is two characters long
      buffer()->append_fixed(Traits::lines_terminated_by[1]);
    }
  }

  static constexpr auto s_lines_terminated_by_length =
      Traits::lines_terminated_by.length();

  static constexpr auto s_fields_escaped_by_length =
      Traits::fields_escaped_by.length();

  static constexpr auto s_fields_terminated_by_length =
      Traits::fields_terminated_by.length();

  static constexpr auto s_fields_enclosed_by_length =
      Traits::fields_enclosed_by.length();

  uint32_t m_num_fields;

  // not using vectors of bool here, as they are not very efficient on access
  std::vector<uint8_t> m_is_string_type;

  std::vector<uint8_t> m_is_number_type;

  std::vector<Escape_type> m_needs_escape;
};

/**
 * Writes output escaping just the ENCLOSED BY character.
 */
template <class Traits>
  requires std::is_base_of_v<dialect_traits, Traits>
class Fast_dialect_dump_writer : public Dump_writer {
 public:
  Fast_dialect_dump_writer() {
    static_assert(1 == Traits::lines_terminated_by.length(),
                  "Line terminator needs to be one character");
    static_assert(1 == Traits::fields_terminated_by.length(),
                  "Field terminator needs to be one character");
    static_assert(Traits::fields_escaped_by.empty(),
                  "FIELDS ESCAPED BY cannot be set");
    static_assert(1 == Traits::fields_enclosed_by.length(),
                  "FIELDS ENCLOSED BY needs to be set");
  }

  Fast_dialect_dump_writer(const Fast_dialect_dump_writer &) = delete;
  Fast_dialect_dump_writer(Fast_dialect_dump_writer &&) = default;

  Fast_dialect_dump_writer &operator=(const Fast_dialect_dump_writer &) =
      delete;
  Fast_dialect_dump_writer &operator=(Fast_dialect_dump_writer &&) = default;

  ~Fast_dialect_dump_writer() override = default;

 private:
  void store_row(const mysqlshdk::db::IRow *row) override {
    for (uint32_t idx = 0; idx < m_num_fields; ++idx) {
      store_field(row, idx);
    }

    finish_row();
  }

  void read_metadata(
      const std::vector<mysqlshdk::db::Column> &metadata,
      const std::vector<Encoding_type> &pre_encoded_columns) override {
    m_num_fields = static_cast<uint32_t>(metadata.size());

    m_needs_escape.clear();
    m_needs_escape.resize(m_num_fields);

    m_is_numeric.clear();
    m_is_numeric.resize(m_num_fields);

    std::size_t fixed_length = Traits::lines_terminated_by.length();

    if (m_num_fields > 0) {
      fixed_length +=
          (m_num_fields - 1) * Traits::fields_terminated_by.length();
    }

    bool is_numeric;
    Escape_type needs_escape;
    const auto has_encoding_type_info =
        pre_encoded_columns.size() == metadata.size();

    for (uint32_t i = 0; i < m_num_fields; ++i) {
      is_numeric = metadata[i].is_numeric();
      needs_escape = is_numeric ? Escape_type::NONE : Escape_type::FULL;

      if (has_encoding_type_info) {
        switch (pre_encoded_columns[i]) {
          case Encoding_type::BASE64:
            needs_escape = Escape_type::BASE64;
            break;

          case Encoding_type::HEX:
            needs_escape = Escape_type::NONE;
            break;

          case Encoding_type::VECTOR:
            // this is encoded using VECTOR_TO_STRING and contains commas
            if constexpr (',' == k_fields_terminated_by) {
              needs_escape = Escape_type::FULL;
            } else {
              needs_escape = Escape_type::NONE;
            }
            break;

          case Encoding_type::NONE:
            // not encoded, depends on column type
            break;
        }
      }

      m_is_numeric[i] = is_numeric;
      m_needs_escape[i] = needs_escape;

      if constexpr (!Traits::fields_optionally_enclosed) {
        fixed_length += 2 * Traits::fields_enclosed_by.length();
      } else {
        if (Escape_type::FULL == needs_escape) {
          fixed_length += 2 * Traits::fields_enclosed_by.length();
        }
      }
    }

    buffer()->set_fixed_length(fixed_length);
  }

  inline void store_field(const mysqlshdk::db::IRow *row, uint32_t idx) {
    if (0 != idx) [[likely]] {
      buffer()->append_fixed(k_fields_terminated_by);
    }

    const char *data = nullptr;
    std::size_t length = 0;
    row->get_raw_data(idx, &data, &length);

    bool is_null = nullptr == data;

    if (!is_null && m_is_numeric[idx]) {
      if ((length > 0 && std::isalpha(data[0])) ||
          (length > 1 && '-' == data[0] && std::isalpha(data[1])))
          [[unlikely]] {
        // convert any strings ("inf", "-inf", "nan") into NULL
        is_null = true;
      }
    }

    if (is_null) {
      store_null();
    } else {
      quote_field();

      switch (m_needs_escape[idx]) {
        case Escape_type::FULL:
          store_escaped_field(data, length);
          break;

        case Escape_type::BASE64:
          store_base64_field(data, length);
          break;

        case Escape_type::NONE:
          store_unescaped_field(data, length);
          break;
      }

      quote_field();
    }
  }

  inline void store_null() {
    // FIELDS ESCAPED BY character is not specified, write "NULL"
    buffer()->will_write(k_null.length());
    buffer()->append(k_null.data(), k_null.length());
  }

  inline void quote_field() {
    if constexpr (!Traits::fields_optionally_enclosed) {
      // all fields are quoted
      store_fields_enclosed_by();
    }
  }

  inline void store_escaped_field(const char *data, std::size_t length) {
    if (!length) {
      // empty field, don't write anything
      return;
    }

    // this method escapes the ENCLOSED BY character by doubling it

    buffer()->will_write(2 * length);

    [[maybe_unused]] auto enclose = false;
    const auto end = data + length;
    auto ptr = data;
    const auto append_block = [this, &data, &ptr]() {
      buffer()->append(data, ptr - data);
    };

    if constexpr (Traits::fields_optionally_enclosed) {
      // check if field needs to be enclosed, it does if it contains: field
      // terminator, line terminator or ENCLOSED BY character
      ptr = find_any_of(data, length, k_fields_enclosed_by,
                        k_fields_terminated_by, k_lines_terminated_by);

      if (ptr < end) {
        // field needs to be enclosed
        enclose = true;

        // if this is not the ENCLOSED BY character, search again
        if (k_fields_enclosed_by != *ptr) {
          ++ptr;
          ptr = find_enclosed_by(ptr, end - ptr);
        }
      } else {
        // field needs to be enclosed if this is a literal NULL string
        if (k_null.length() == length) {
          enclose = 0 == strncmp(k_null.data(), data, k_null.length());
        } else {
          enclose = false;
        }
      }

      if (enclose) {
        // field needs to be enclosed, store starting quote
        store_fields_enclosed_by();
      }
    } else {
      // search for the ENCLOSED BY character
      ptr = find_enclosed_by(data, length);
    }

    while (ptr < end) {
      // include the ENCLOSED BY character
      ++ptr;
      append_block();

      // double the ENCLOSED BY character
      buffer()->append(k_fields_enclosed_by);

      // find the next ENCLOSED BY character
      data = ptr;
      ptr = find_enclosed_by(ptr, end - ptr);
    }

    // append the leftover
    assert(ptr == end);
    append_block();

    if constexpr (Traits::fields_optionally_enclosed) {
      if (enclose) {
        // store ending quote
        store_fields_enclosed_by();
      }
    }
  }

  inline void store_base64_field(const char *data, std::size_t length) {
    // we strip new lines from the BASE64 output, so it's safe to store it
    // without quotes
    buffer()->write_base64_data(data, length);
  }

  inline void store_unescaped_field(const char *data, std::size_t length) {
    // write as is
    buffer()->will_write(length);
    buffer()->append(data, length);
  }

  inline void finish_row() { buffer()->append_fixed(k_lines_terminated_by); }

  inline void store_fields_enclosed_by() {
    buffer()->append_fixed(k_fields_enclosed_by);
  }

  template <typename... T>
    requires(std::same_as<T, char> && ...)
  static inline const
      char *find_any_of(const char *data, std::size_t length, T... needles) {
    auto s = data;

    while (length) {
      if (((*s == needles) || ...)) {
        break;
      }

      --length;
      ++s;
    }

    return s;
  }

  static inline const char *find_enclosed_by(const char *data,
                                             std::size_t length) {
    return find_any_of(data, length, k_fields_enclosed_by);
  }

  static constexpr auto k_lines_terminated_by = Traits::lines_terminated_by[0];
  static constexpr auto k_fields_terminated_by =
      Traits::fields_terminated_by[0];
  static constexpr auto k_fields_enclosed_by = Traits::fields_enclosed_by[0];

  static constexpr std::string_view k_null = "NULL";

  uint32_t m_num_fields;

  std::vector<uint8_t> m_is_numeric;

  // which field needs to be escaped
  std::vector<Escape_type> m_needs_escape;
};

}  // namespace detail

class Default_dump_writer
    : public detail::Dialect_dump_writer<detail::default_traits> {
 public:
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
  using Dialect_dump_writer::Dialect_dump_writer;

  Json_dump_writer(const Json_dump_writer &) = delete;
  Json_dump_writer(Json_dump_writer &&) = default;

  Json_dump_writer &operator=(const Json_dump_writer &) = delete;
  Json_dump_writer &operator=(Json_dump_writer &&) = default;

  ~Json_dump_writer() override = default;
};

class Csv_dump_writer : public detail::Dialect_dump_writer<detail::csv_traits> {
 public:
  using Dialect_dump_writer::Dialect_dump_writer;

  Csv_dump_writer(const Csv_dump_writer &) = delete;
  Csv_dump_writer(Csv_dump_writer &&) = default;

  Csv_dump_writer &operator=(const Csv_dump_writer &) = delete;
  Csv_dump_writer &operator=(Csv_dump_writer &&) = default;

  ~Csv_dump_writer() override = default;
};

class Tsv_dump_writer : public detail::Dialect_dump_writer<detail::tsv_traits> {
 public:
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
  using Dialect_dump_writer::Dialect_dump_writer;

  Csv_unix_dump_writer(const Csv_unix_dump_writer &) = delete;
  Csv_unix_dump_writer(Csv_unix_dump_writer &&) = default;

  Csv_unix_dump_writer &operator=(const Csv_unix_dump_writer &) = delete;
  Csv_unix_dump_writer &operator=(Csv_unix_dump_writer &&) = default;

  ~Csv_unix_dump_writer() override = default;
};

class Csv_rfc_unix_dump_writer
    : public detail::Fast_dialect_dump_writer<detail::csv_rfc_unix_traits> {
 public:
  using Fast_dialect_dump_writer::Fast_dialect_dump_writer;

  Csv_rfc_unix_dump_writer(const Csv_rfc_unix_dump_writer &) = delete;
  Csv_rfc_unix_dump_writer(Csv_rfc_unix_dump_writer &&) = default;

  Csv_rfc_unix_dump_writer &operator=(const Csv_rfc_unix_dump_writer &) =
      delete;
  Csv_rfc_unix_dump_writer &operator=(Csv_rfc_unix_dump_writer &&) = default;

  ~Csv_rfc_unix_dump_writer() override = default;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DIALECT_DUMP_WRITER_H_
