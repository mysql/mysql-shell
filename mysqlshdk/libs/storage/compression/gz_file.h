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

#ifndef MYSQLSHDK_LIBS_STORAGE_COMPRESSION_GZ_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_COMPRESSION_GZ_FILE_H_

#include <zlib.h>
#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace storage {
namespace compression {

class Gz_file : public Compressed_file {
 public:
  Gz_file() = delete;

  explicit Gz_file(std::unique_ptr<IFile> file);

  Gz_file(const Gz_file &other) = delete;
  Gz_file(Gz_file &&other) = default;

  Gz_file &operator=(const Gz_file &other) = delete;
  Gz_file &operator=(Gz_file &&other) = default;

  ~Gz_file() override;

  void open(Mode m) override;
  bool is_open() const override;
  void close() override;

  off64_t seek(off64_t) override {
    throw std::logic_error("Gz_file::seek() - not supported");
  }

  off64_t tell() const override {
    return std::max(m_stream.total_in, m_stream.total_out);
  }

  ssize_t read(void *buffer, size_t length) override;
  ssize_t write(const void *buffer, size_t length) override;

 private:
  struct Buf_view {
    uint8_t *ptr;
    size_t length;
  };

  static constexpr const size_t CHUNK = 1 << 15;

  static constexpr bool is_power_of_2(size_t x) {
    return ((x - 1) & x) == 0 && (x != 0);
  }

  static constexpr int align(int x) {
    static_assert(is_power_of_2(CHUNK), "CHUNK must be a power of 2.");
    return (x + (CHUNK - 1)) & (~(CHUNK - 1));
  }

  void extend_to_fit(size_t size) {
    assert(m_source.size() <= std::numeric_limits<size_t>::max() - size);
    m_source.resize(size + m_source.size());
  }

  void init_read();
  void init_write();
  inline ssize_t do_write(void *buffer, size_t length, int flag);
  void write_finish();
  void do_close();

  inline Buf_view peek(const size_t length);

  void consume(const size_t length) {
    m_source.erase(m_source.begin(), m_source.begin() + length);
  }

  z_stream m_stream;
  std::vector<uint8_t> m_source;
  mysqlshdk::utils::nullable<Mode> m_open_mode{nullptr};
};

Gz_file::Buf_view Gz_file::peek(const size_t length) {
  const auto avail = m_source.size();
  if (avail < length) {
    const auto want = align(length);
    extend_to_fit(want);
    uint8_t *p = &m_source[avail];
    const auto bytes_read = file()->read(p, want);
    if (bytes_read < 0) {
      // read error
      m_source.resize(avail);  // revert extent_to_fit
    } else {
      m_source.resize(avail + bytes_read);
    }
  }
  return Gz_file::Buf_view{m_source.data(), m_source.size()};
}

ssize_t Gz_file::do_write(void *buffer, size_t length, int flag) {
  Bytef buff[CHUNK];
  m_stream.next_in = static_cast<Bytef *>(buffer);
  m_stream.avail_in = length;
  do {
    m_stream.next_out = buff;
    m_stream.avail_out = CHUNK;

    const auto ret = deflate(&m_stream, flag);
    if (ret == Z_STREAM_ERROR) {
      throw std::runtime_error(std::string("deflate: stream error (") +
                               (m_stream.msg ? m_stream.msg : "unknown error") +
                               ")");
    }
    ssize_t have = CHUNK - m_stream.avail_out;
    const auto write_bytes = file()->write(buff, have);
    if (write_bytes != have || write_bytes < 0) {
      throw std::runtime_error("deflate: cannot write");
    }
    if (ret == Z_STREAM_END || ret == Z_BUF_ERROR) {
      break;
    }
  } while (m_stream.avail_out == 0);
  return length;
}

}  // namespace compression
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_COMPRESSION_GZ_FILE_H_
