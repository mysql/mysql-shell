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

#ifndef MODULES_UTIL_DUMP_DUMP_WRITER_H_
#define MODULES_UTIL_DUMP_DUMP_WRITER_H_

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlsh {
namespace dump {

enum class Escape_type { NONE, FULL, BASE64 };

class Dump_write_result final {
 public:
  Dump_write_result() = default;

  Dump_write_result(const Dump_write_result &) = default;
  Dump_write_result(Dump_write_result &&) = default;

  Dump_write_result &operator=(const Dump_write_result &) = default;
  Dump_write_result &operator=(Dump_write_result &&) = default;

  ~Dump_write_result() = default;

  Dump_write_result &operator+=(const Dump_write_result &rhs);

  void reset() noexcept { m_data_bytes = m_bytes_written = m_rows_written = 0; }

  void write_data(uint64_t bytes) noexcept { m_data_bytes += bytes; }

  uint64_t data_bytes() const noexcept { return m_data_bytes; }

  void write_bytes(uint64_t bytes) noexcept { m_bytes_written += bytes; }

  uint64_t bytes_written() const noexcept { return m_bytes_written; }

  void write_row() noexcept { ++m_rows_written; }

  uint64_t rows_written() const noexcept { return m_rows_written; }

 private:
  uint64_t m_data_bytes = 0;
  uint64_t m_bytes_written = 0;
  uint64_t m_rows_written = 0;
};

class Dump_writer {
 public:
  enum class Encoding_type { NONE, BASE64, HEX };

  Dump_writer();

  Dump_writer(const Dump_writer &) = delete;
  Dump_writer(Dump_writer &&) = default;

  Dump_writer &operator=(const Dump_writer &) = delete;
  Dump_writer &operator=(Dump_writer &&) = default;

  virtual ~Dump_writer();

  void set_output_file(mysqlshdk::storage::IFile *output);

  void set_index_file(std::unique_ptr<mysqlshdk::storage::IFile> index);

  void open();

  void close();

  Dump_write_result write_preamble(
      const std::vector<mysqlshdk::db::Column> &metadata,
      const std::vector<Encoding_type> &pre_encoded_columns = {});

  Dump_write_result write_row(const mysqlshdk::db::IRow *row);

  Dump_write_result write_postamble();

 protected:
  class Buffer final {
   public:
    Buffer();

    Buffer(const Buffer &) = delete;
    Buffer(Buffer &&) = default;

    Buffer &operator=(const Buffer &) = delete;
    Buffer &operator=(Buffer &&) = default;

    ~Buffer() = default;

    inline std::size_t length() const noexcept { return m_length; }

    inline const char *data() const noexcept { return m_data.get(); }

    inline void append_fixed(char c) noexcept {
      assert(m_fixed_length_remaining >= 1);

      append(c);

      --m_fixed_length_remaining;
    }

    inline void append(char c) noexcept {
      *m_ptr++ = c;
      ++m_length;
    }

    void append_fixed(const std::string &s) noexcept;

    inline void append(const char *data, std::size_t length) noexcept {
      memcpy(m_ptr, data, length);
      m_ptr += length;
      m_length += length;
    }

    void write_base64_data(const char *data, std::size_t length);

    void clear() noexcept;

    void set_fixed_length(std::size_t fixed_length);

    void will_write(std::size_t bytes);

   private:
    void resize(std::size_t requested_capacity);

    std::size_t m_capacity = 1024;
    std::size_t m_length = 0;
    std::size_t m_fixed_length = 0;
    std::size_t m_fixed_length_remaining = 0;
    std::unique_ptr<char[]> m_data;
    char *m_ptr = nullptr;
  };

  inline Buffer *buffer() const noexcept { return m_buffer.get(); }

 private:
  virtual void store_preamble(
      const std::vector<mysqlshdk::db::Column> &metadata,
      const std::vector<Encoding_type> &pre_encoded_columns) = 0;

  virtual void store_row(const mysqlshdk::db::IRow *row) = 0;

  virtual void store_postamble() = 0;

  Dump_write_result write_buffer(const char *context, bool row = false) const;

  void write_index();

  mysqlshdk::storage::IFile *m_output;

  std::unique_ptr<mysqlshdk::storage::IFile> m_index;

  std::unique_ptr<Buffer> m_buffer;

  mysqlshdk::storage::Compressed_file *m_compressed = nullptr;

  uint64_t m_bytes_written = 0;

  uint64_t m_bytes_written_per_idx = 0;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_WRITER_H_
