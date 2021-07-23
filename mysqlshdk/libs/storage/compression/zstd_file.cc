/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/compression/zstd_file.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "mysqlshdk/libs/storage/backend/file.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace storage {
namespace compression {

Zstd_file::Zstd_file(std::unique_ptr<IFile> file)
    : Compressed_file(std::move(file)) {}

Zstd_file::~Zstd_file() {
  try {
    if (is_open()) do_close();
  } catch (const std::runtime_error &e) {
    log_error("Failed to close zstd compressed file: %s", e.what());
  }
}

Zstd_file::Buf_view Zstd_file::peek(const size_t length) {
  const auto avail = m_buffer.size();
  if (avail < length) {
    const auto want = align(std::max<size_t>(length, 4 * 1024 * 1024));
    extend_to_fit(want);
    uint8_t *p = &m_buffer[avail];
    const auto bytes_read = file()->read(p, want);
    if (bytes_read < 0) {
      // read error
      m_buffer.resize(avail);  // revert extent_to_fit
    } else {
      m_buffer.resize(avail + bytes_read);
    }
  }
  return Buf_view{m_buffer.data(), m_buffer.size()};
}

ssize_t Zstd_file::read(void *buffer, size_t length) {
  ZSTD_outBuffer obuf;
  obuf.dst = buffer;
  obuf.size = length;
  obuf.pos = 0;

  return (*this.*m_read_f)(&obuf);
}

ssize_t Zstd_file::do_read(ZSTD_outBuffer *obuf) {
  while (obuf->pos < obuf->size) {
    const auto input_buf = peek(m_decompress_read_size);
    if (input_buf.length == 0) {
      break;
    }

    ZSTD_inBuffer ibuf;
    ibuf.size = input_buf.length;
    ibuf.src = input_buf.ptr;
    ibuf.pos = 0;

    size_t status = ZSTD_decompressStream(m_dctx, obuf, &ibuf);

    if (ZSTD_isError(status))
      throw std::runtime_error(std::string("zstd.read: ") +
                               ZSTD_getErrorName(status));
    if (ibuf.pos > 0) {
      consume(ibuf.pos);
    }
  }

  m_offset += obuf->pos;

  // number of bytes being returned
  return obuf->pos;
}

ssize_t Zstd_file::do_read_mmap(ZSTD_outBuffer *obuf) {
  while (obuf->pos < obuf->size) {
    auto *mfile = dynamic_cast<backend::File *>(file());

    ZSTD_inBuffer ibuf;
    ibuf.src = mfile->mmap_will_read(&ibuf.size);
    ibuf.pos = 0;

    if (ibuf.size == 0) break;

    size_t status = ZSTD_decompressStream(m_dctx, obuf, &ibuf);

    if (ZSTD_isError(status))
      throw std::runtime_error(std::string("zstd.read: ") +
                               ZSTD_getErrorName(status));
    if (ibuf.pos > 0) {
      mfile->mmap_did_read(ibuf.pos);
    }
  }

  m_offset += obuf->pos;

  // number of bytes being returned
  return obuf->pos;
}

ssize_t Zstd_file::write(const void *buffer, size_t length) {
  ZSTD_inBuffer ibuf;
  ibuf.size = length;
  ibuf.pos = 0;
  ibuf.src = buffer;

  m_offset += length;

  return (*this.*m_write_f)(&ibuf, ZSTD_e_continue);
}

bool Zstd_file::flush() {
  ZSTD_inBuffer ibuf;
  ibuf.size = 0;
  ibuf.pos = 0;
  ibuf.src = nullptr;

  (*this.*m_write_f)(&ibuf, ZSTD_e_flush);

  return file()->flush();
}

void Zstd_file::write_finish() {
  ZSTD_inBuffer ibuf;
  ibuf.size = 0;
  ibuf.pos = 0;
  ibuf.src = nullptr;

  (*this.*m_write_f)(&ibuf, ZSTD_e_end);
}

ssize_t Zstd_file::do_write(ZSTD_inBuffer *ibuf, ZSTD_EndDirective op) {
  ZSTD_outBuffer obuf;

  obuf.dst = &m_buffer[0];
  obuf.size = m_buffer.size();
  obuf.pos = 0;
  bool done;

  size_t status;
  do {
    status = ZSTD_compressStream2(m_cctx, &obuf, ibuf, op);
    if (ZSTD_isError(status)) {
      throw std::runtime_error(std::string("zstd.write: ") +
                               ZSTD_getErrorName(status));
    } else {
      // make sure everything is written out
      ssize_t r;

      r = file()->write(static_cast<const char *>(obuf.dst), obuf.pos);
      if (r < 0)
        throw std::runtime_error("zstd.write: error writing compressed data");

      obuf.pos = 0;
    }
    // make sure the whole input buffer is consumed
    done = (op == ZSTD_e_end) ? (status == 0) : ibuf->pos == ibuf->size;
  } while (!done);

  return ibuf->size;
}

ssize_t Zstd_file::do_write_mmap(ZSTD_inBuffer *ibuf, ZSTD_EndDirective op) {
  ZSTD_outBuffer obuf;
  auto *mfile = static_cast<backend::File *>(file());

  obuf.dst =
      mfile->mmap_will_write(ibuf->size + ZSTD_CStreamOutSize(), &obuf.size);
  if (!obuf.dst) {
    throw std::runtime_error(
        std::string("Error reserving space on mmapped file"));
  }
  obuf.pos = 0;

  bool done;

  size_t status;
  do {
    status = ZSTD_compressStream2(m_cctx, &obuf, ibuf, op);
    if (ZSTD_isError(status)) {
      throw std::runtime_error(std::string("zstd.write: ") +
                               ZSTD_getErrorName(status));
    } else {
      obuf.dst = mfile->mmap_did_write(obuf.pos, &obuf.size);
      obuf.pos = 0;
    }
    // make sure the whole input buffer is consumed
    done = (op == ZSTD_e_end) ? (status == 0) : ibuf->pos == ibuf->size;
  } while (!done);

  return ibuf->size;
}

void Zstd_file::init_write() {
  if (!m_cctx) {
    m_cctx = ZSTD_createCStream();
    if (!m_cctx) {
      throw std::runtime_error("zstd compression context init failed");
    }
    ZSTD_initCStream(m_cctx, m_clevel);

    auto *mfile = dynamic_cast<backend::File *>(file());

    // try to enable mmap if available
    if (mfile && mfile->mmap_will_write(0, nullptr)) {
      log_debug("mmap() enabled for file %s", mfile->full_path().c_str());
      m_write_f = &Zstd_file::do_write_mmap;
    } else {
      m_write_f = &Zstd_file::do_write;
      m_buffer.resize(ZSTD_CStreamOutSize());
    }
  }
}

void Zstd_file::init_read() {
  if (!m_dctx) {
    m_dctx = ZSTD_createDStream();
    if (!m_dctx) {
      throw std::runtime_error("zstd decompression context init failed");
    }
    m_decompress_read_size = ZSTD_initDStream(m_dctx);

    auto *mfile = dynamic_cast<backend::File *>(file());
    // try to enable mmap if available
    if (mfile && mfile->mmap_will_read(nullptr)) {
      log_debug("mmap() enabled for file %s", mfile->full_path().c_str());
      m_read_f = &Zstd_file::do_read_mmap;
    } else {
      m_read_f = &Zstd_file::do_read;
    }
  }
}

void Zstd_file::open(Mode m) {
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
      throw std::invalid_argument("append not supported for zstd file");
  }

  m_open_mode = m;
  m_offset = 0;
}

bool Zstd_file::is_open() const {
  return !m_open_mode.is_null() && file()->is_open();
}

void Zstd_file::close() { do_close(); }

void Zstd_file::do_close() {
  switch (*m_open_mode) {
    case Mode::READ:
      if (m_dctx) ZSTD_freeDStream(m_dctx);
      m_dctx = nullptr;
      m_read_f = nullptr;
      break;

    case Mode::WRITE:
      write_finish();
      if (m_cctx) ZSTD_freeCStream(m_cctx);
      m_cctx = nullptr;
      m_write_f = nullptr;
      break;

    case Mode::APPEND:
      break;
  }

  m_open_mode.reset();
  m_buffer.resize(0);

  if (file()->is_open()) {
    file()->close();
  }
}

}  // namespace compression
}  // namespace storage
}  // namespace mysqlshdk
