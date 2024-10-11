/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/compressed_file.h"

#include <cassert>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/storage/compression/gz_file.h"
#include "mysqlshdk/libs/storage/compression/zstd_file.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {

namespace {

void ensure_no_compression_options(const Compression_options &opts, void *) {
  if (!opts.empty())
    throw std::invalid_argument("Compression options not supported");
}

#define COMPRESSIONS                                                      \
  X(NONE, "none", "", ensure_no_compression_options)                      \
  X(GZIP, "gzip", ".gz", compression::Gz_file::parse_compression_options) \
  X(ZSTD, "zstd", ".zst", compression::Zstd_file::parse_compression_options)

}  // namespace

Compressed_file::Compressed_file(std::unique_ptr<IFile> file)
    : m_file(std::move(file)) {}

void Compressed_file::open(Mode m) { m_file->open(m); }

bool Compressed_file::is_open() const { return m_file->is_open(); }

int Compressed_file::error() const { return m_file->error(); }

void Compressed_file::close() { m_file->close(); }

// TODO(alfredo) - this should return the uncompressed size, but there's no way
// to do that
size_t Compressed_file::file_size() const { return m_file->file_size(); }

Masked_string Compressed_file::full_path() const { return m_file->full_path(); }

bool Compressed_file::exists() const { return m_file->exists(); }

std::unique_ptr<IDirectory> Compressed_file::parent() const {
  return m_file->parent();
}

bool Compressed_file::flush() { return m_file->flush(); }

void Compressed_file::rename(const std::string &new_name) {
  m_file->rename(new_name);
}

void Compressed_file::remove() { m_file->remove(); }

std::string Compressed_file::filename() const { return m_file->filename(); }

bool Compressed_file::is_local() const { return m_file->is_local(); }

size_t Compressed_file::latest_io_size() const {
  return m_io_finished ? m_io_size : 0;
}

void Compressed_file::start_io() {
  m_io_size = 0;
  m_io_finished = false;
}

void Compressed_file::update_io(size_t bytes) {
  assert(!m_io_finished);
  m_io_size += bytes;
}

void Compressed_file::finish_io() { m_io_finished = true; }

Compression to_compression(const std::string &c,
                           Compression_options *out_compression_options) {
  std::string ctype;

  if (auto p = c.find(';'); p != std::string::npos) {
    ctype = c.substr(0, p);
    if (out_compression_options) {
      for (const auto &opt : shcore::str_split(c.substr(p + 1), ";")) {
        auto [n, v] = shcore::str_partition(opt, "=");
        out_compression_options->emplace(std::move(n), std::move(v));
      }
    }
  } else {
    ctype = c;
  }

#define X(value, name, ext, check)                                         \
  if (ctype == name) {                                                     \
    if (out_compression_options) check(*out_compression_options, nullptr); \
    return Compression::value;                                             \
  }

  COMPRESSIONS

#undef X

  throw std::invalid_argument("Unknown compression type: " + c);
}

std::string to_string(Compression c) {
#define X(value, name, ext, check) \
  case Compression::value:         \
    return name;

  switch (c) { COMPRESSIONS }

#undef X

  throw std::logic_error("Shouldn't happen, but compiler complains");
}

std::string get_extension(Compression c) {
#define X(value, name, ext, check) \
  case Compression::value:         \
    return ext;

  switch (c) { COMPRESSIONS }

#undef X

  throw std::logic_error("Shouldn't happen, but compiler complains");
}

Compression from_extension(const std::string &e) {
#define X(value, name, ext, check) \
  if (e == ext) return Compression::value;

  COMPRESSIONS

#undef X

  return Compression::NONE;
}

Compression from_file_path(const std::string &file_path) {
  return from_extension(std::get<1>(shcore::path::split_extension(file_path)));
}

bool is_compressed(const std::string &file_path) {
  return Compression::NONE != from_file_path(file_path);
}

std::unique_ptr<IFile> make_file(
    std::unique_ptr<IFile> file, Compression c,
    const Compression_options &compression_options) {
  std::unique_ptr<IFile> result;

  switch (c) {
    case Compression::NONE:
      ensure_no_compression_options(compression_options, nullptr);
      result = std::move(file);
      break;

    case Compression::GZIP:
      result = std::make_unique<compression::Gz_file>(std::move(file),
                                                      compression_options);

      break;

    case Compression::ZSTD:
      result = std::make_unique<compression::Zstd_file>(std::move(file),
                                                        compression_options);

      break;

    default:
      throw std::logic_error("Unhandled compression type: " + to_string(c));
  }

  return result;
}

}  // namespace storage
}  // namespace mysqlshdk
