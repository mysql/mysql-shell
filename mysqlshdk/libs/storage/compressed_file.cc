/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/storage/compressed_file.h"

#include <utility>

#include "mysqlshdk/libs/storage/compression/gz_file.h"
#include "mysqlshdk/libs/storage/compression/zstd_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {

namespace {

#define COMPRESSIONS     \
  X(NONE, "none", "")    \
  X(GZIP, "gzip", ".gz") \
  X(ZSTD, "zstd", ".zstd")

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

std::string Compressed_file::full_path() const { return m_file->full_path(); }

bool Compressed_file::exists() const { return m_file->exists(); }

bool Compressed_file::flush() { return m_file->flush(); }

void Compressed_file::rename(const std::string &new_name) {
  m_file->rename(new_name);
}

void Compressed_file::remove() { m_file->remove(); }

std::string Compressed_file::filename() const { return m_file->filename(); }

Compression to_compression(const std::string &c) {
#define X(value, name, ext) \
  if (c == name) return Compression::value;

  COMPRESSIONS

#undef X

  throw std::invalid_argument("Unknown compression type: " + c);
}

std::string to_string(Compression c) {
#define X(value, name, ext) \
  case Compression::value:  \
    return name;

  switch (c) { COMPRESSIONS }

#undef X

  throw std::logic_error("Shouldn't happen, but compiler complains");
}

std::string get_extension(Compression c) {
#define X(value, name, ext) \
  case Compression::value:  \
    return ext;

  switch (c) { COMPRESSIONS }

#undef X

  throw std::logic_error("Shouldn't happen, but compiler complains");
}

Compression from_extension(const std::string &e) {
#define X(value, name, ext) \
  if (e == ext) return Compression::value;

  COMPRESSIONS

#undef X

  throw std::invalid_argument("Unknown compression type: " + e);
}

std::unique_ptr<IFile> make_file(std::unique_ptr<IFile> file, Compression c) {
  std::unique_ptr<IFile> result;

  switch (c) {
    case Compression::NONE:
      result = std::move(file);
      break;

    case Compression::GZIP:
      result = std::make_unique<compression::Gz_file>(std::move(file));
      break;

    case Compression::ZSTD:
      result = std::make_unique<compression::Zstd_file>(std::move(file));
      break;

    default:
      throw std::logic_error("Unhandled compression type: " + to_string(c));
  }

  return result;
}

}  // namespace storage
}  // namespace mysqlshdk
