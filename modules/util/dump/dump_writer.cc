/*
 * Copyright (c) 2020, 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_writer.h"

#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/storage/compressed_file.h"

namespace mysqlsh {
namespace dump {

using mysqlshdk::storage::IFile;
using mysqlshdk::storage::Mode;

Dump_write_result &Dump_write_result::operator+=(const Dump_write_result &rhs) {
  m_data_bytes += rhs.m_data_bytes;
  m_bytes_written += rhs.m_bytes_written;

  return *this;
}

Dump_writer::Buffer::Buffer()
    : m_data(std::make_unique<char[]>(m_capacity)), m_ptr(m_data.get()) {}

void Dump_writer::Buffer::append_fixed(const std::string &s) noexcept {
  const auto length = s.length();

  assert(m_fixed_length_remaining >= length);

  append(s.c_str(), length);

  m_fixed_length_remaining -= length;
}

void Dump_writer::Buffer::clear() noexcept {
  m_ptr = m_data.get();
  m_length = 0;
  m_fixed_length_remaining = m_fixed_length;
}

void Dump_writer::Buffer::set_fixed_length(std::size_t fixed_length) {
  will_write(fixed_length);

  m_fixed_length_remaining = m_fixed_length = fixed_length;
}

void Dump_writer::Buffer::will_write(std::size_t bytes) {
  const auto requested_capacity = m_length + m_fixed_length_remaining + bytes;

  if (requested_capacity > m_capacity) {
    resize(requested_capacity);
  }
}

void Dump_writer::Buffer::resize(std::size_t requested_capacity) {
  std::size_t new_capacity = m_capacity;

  while (new_capacity < requested_capacity) {
    new_capacity <<= 1;
  }

  if (new_capacity != m_capacity) {
    auto new_data = std::make_unique<char[]>(new_capacity);
    memcpy(new_data.get(), m_data.get(), m_length);

    m_capacity = new_capacity;
    m_data = std::move(new_data);
    m_ptr = m_data.get() + m_length;
  }
}

Dump_writer::Dump_writer(std::unique_ptr<IFile> out)
    : m_output(std::move(out)), m_buffer(std::make_unique<Buffer>()) {}

void Dump_writer::open() {
  if (!output()->is_open()) {
    output()->open(Mode::WRITE);
  }
}

void Dump_writer::close() {
  if (output()->is_open()) {
    output()->close();
  }

  // Once closed, the writer handle is released
  m_output.reset();
}

Dump_write_result Dump_writer::write_preamble(
    const std::vector<mysqlshdk::db::Column> &metadata) {
  buffer()->clear();
  store_preamble(metadata);
  return write_buffer("preamble");
}

Dump_write_result Dump_writer::write_row(const mysqlshdk::db::IRow *row) {
  buffer()->clear();
  store_row(row);
  return write_buffer("row");
}

Dump_write_result Dump_writer::write_postamble() {
  buffer()->clear();
  store_postamble();
  return write_buffer("postamble");
}

Dump_write_result Dump_writer::write_buffer(const char *context) const {
  Dump_write_result result;

  result.m_data_bytes = buffer()->length();

  if (result.m_data_bytes > 0) {
    using mysqlshdk::storage::Compressed_file;
    const auto compressed = dynamic_cast<Compressed_file *>(output());
    const auto size = compressed ? compressed->file()->tell() : 0;
    const auto bytes_written =
        output()->write(buffer()->data(), result.m_data_bytes);

    if (bytes_written < 0) {
      throw std::runtime_error("Failed to write " + std::string(context) +
                               " into file " + output()->full_path());
    }

    result.m_bytes_written =
        compressed ? compressed->file()->tell() - size : bytes_written;
  }

  return result;
}

}  // namespace dump
}  // namespace mysqlsh
