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

#ifndef MYSQLSHDK_LIBS_STORAGE_COMPRESSION_ZSTD_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_COMPRESSION_ZSTD_FILE_H_

#include <zstd.h>
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

class Zstd_file : public Compressed_file {
 public:
  Zstd_file() = delete;

  explicit Zstd_file(std::unique_ptr<IFile> file);

  Zstd_file(const Zstd_file &other) = delete;
  Zstd_file(Zstd_file &&other) = default;

  Zstd_file &operator=(const Zstd_file &other) = delete;
  Zstd_file &operator=(Zstd_file &&other) = default;

  ~Zstd_file() override;

  void open(Mode m) override;
  bool is_open() const override;
  void close() override;

  off64_t seek(off64_t) override {
    throw std::logic_error("Zstd_file::seek() - not supported");
  }

  off64_t tell() const override { return m_offset; }

  bool flush() override;

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
    assert(m_buffer.size() <= std::numeric_limits<size_t>::max() - size);
    m_buffer.resize(size + m_buffer.size());
  }

  Buf_view peek(const size_t length);

  void consume(const size_t length) {
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + length);
  }

  void init_read();
  void init_write();
  void write_finish();

  void do_close();
  ssize_t do_write(ZSTD_inBuffer *ibuf, ZSTD_EndDirective op);

  size_t m_offset = 0;

  ZSTD_CStream *m_cctx = nullptr;
  ZSTD_DStream *m_dctx = nullptr;
  int m_clevel = 1;
  std::vector<uint8_t> m_buffer;
  size_t m_decompress_read_size = 0;
  mysqlshdk::utils::nullable<Mode> m_open_mode{nullptr};
};

}  // namespace compression
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_COMPRESSION_ZSTD_FILE_H_
