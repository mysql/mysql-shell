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

#include "mysqlshdk/libs/storage/compression/gz_file.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace storage {
namespace compression {

Gz_file::Gz_file(std::unique_ptr<IFile> file)
    : Compressed_file(std::move(file)) {}

Gz_file::~Gz_file() {
  try {
    if (is_open()) close();
  } catch (const std::runtime_error &e) {
    log_error("Failed to close GZ file: %s", e.what());
  }
}

ssize_t Gz_file::read(void *buffer, size_t length) {
  m_stream.next_out = static_cast<Bytef *>(buffer);
  m_stream.avail_out = length;
  int result = Z_STREAM_END;

  while (m_stream.avail_out) {
    const auto input_buf = peek(CHUNK);
    if (input_buf.length == 0) {
      break;
    }
    const size_t avail =
        std::min(std::numeric_limits<size_t>::max(), input_buf.length);
    m_stream.next_in = input_buf.ptr;
    m_stream.avail_in = avail;

    result = inflate(&m_stream, Z_NO_FLUSH);
    switch (result) {
      case Z_NEED_DICT:
        throw std::runtime_error(
            std::string("inflate: input data requires custom dictionary (") +
            m_stream.msg + ")");
      case Z_STREAM_ERROR:
        throw std::runtime_error(std::string("inflate: stream error (") +
                                 m_stream.msg + ")");
      case Z_DATA_ERROR:
        throw std::runtime_error(std::string("inflate: data error (") +
                                 m_stream.msg + ")");
      case Z_MEM_ERROR:
        throw std::runtime_error(
            std::string("inflate: cannot allocate memory (") + m_stream.msg +
            ")");
    }
    const auto consume_bytes = avail - m_stream.avail_in;
    if (consume_bytes > 0) {
      consume(consume_bytes);
    }
    if (result == Z_STREAM_END || result == Z_BUF_ERROR) {
      break;
    }
  }
  const auto have = length - m_stream.avail_out;
  return have;
}

ssize_t Gz_file::write(const void *buffer, size_t length) {
  return do_write(static_cast<Bytef *>(const_cast<void *>(buffer)), length,
                  Z_NO_FLUSH);
}

void Gz_file::write_finish() {
  // deflate() may return Z_STREAM_ERROR if next_in is NULL
  char c = 0;
  (void)do_write(&c, 0, Z_FINISH);
}

void Gz_file::init_read() {
  m_stream.zalloc = nullptr;
  m_stream.zfree = nullptr;
  m_stream.opaque = nullptr;

  m_stream.avail_in = 0;
  m_stream.next_in = nullptr;

  const int gzip_window_bits = 15 + 16;
  int result = inflateInit2(&m_stream, gzip_window_bits);
  if (result != Z_OK) {
    throw std::runtime_error(std::string("inflate init failed: ") +
                             m_stream.msg);
  }
  m_source.resize(0);
}

void Gz_file::init_write() {
  m_stream.zalloc = nullptr;
  m_stream.zfree = nullptr;
  m_stream.opaque = nullptr;

  m_stream.avail_in = 0;
  m_stream.next_in = nullptr;

  const int gzip_window_bits = 15 + 16;
  const int mem_level = 8;
  int result = deflateInit2(&m_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                            gzip_window_bits, mem_level, Z_DEFAULT_STRATEGY);
  if (result != Z_OK) {
    throw std::runtime_error(std::string("deflate init failed: ") +
                             m_stream.msg);
  }
}

void Gz_file::open(Mode m) {
  if (!file()->is_open()) {
    file()->open(m);
  }

  switch (m) {
    case Mode::READ:
      init_read();
      break;
    case Mode::WRITE:
      init_write();
      break;
    case Mode::APPEND:
      break;
  }

  m_open_mode = m;
}

bool Gz_file::is_open() const {
  return !m_open_mode.is_null() && file()->is_open();
}

void Gz_file::close() {
  switch (*m_open_mode) {
    case Mode::READ: {
      auto result = inflateEnd(&m_stream);
      (void)result;
      assert(result == Z_OK);
    } break;
    case Mode::WRITE: {
      write_finish();
      auto result = deflateEnd(&m_stream);
      (void)result;
      assert(result == Z_OK);
    } break;
    case Mode::APPEND:
      break;
  }
  if (file()->is_open()) {
    file()->close();
  }
  m_open_mode.reset();
}

}  // namespace compression
}  // namespace storage
}  // namespace mysqlshdk
