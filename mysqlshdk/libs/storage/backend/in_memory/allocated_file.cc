/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/in_memory/allocated_file.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace mysqlshdk {
namespace storage {
namespace in_memory {

Allocated_file::Allocated_file(const std::string &name, Allocator *allocator,
                               bool consume_if_first)
    : IFile(name),
      m_allocator(allocator),
      m_block_size(allocator->block_size()),
      m_consume_if_first(consume_if_first) {}

Allocated_file::~Allocated_file() {
  m_allocator->free(m_blocks.begin(), m_blocks.end());
}

void Allocated_file::open(bool read_mode) {
  if (is_open()) {
    throw std::runtime_error("Unable to open file: " + name() +
                             ", it is already open");
  }

  if (m_bytes_consumed && !read_mode) {
    throw std::runtime_error("Unable to open file: " + name() +
                             " for writing, it was already read from");
  }

  m_is_open = true;
  m_read_mode = read_mode;
  m_accepts_append = false;
  seek(0);
}

void Allocated_file::close() {
  if (!is_open()) {
    throw std::runtime_error("Unable to close file: " + name() +
                             ", it is already closed");
  }

  m_is_open = false;
}

off64_t Allocated_file::seek(off64_t offset) {
  if (offset < 0) {
    throw std::runtime_error("Unable to seek file: " + name() +
                             ", using disallowed negative value");
  }

  if (!is_open()) {
    throw std::runtime_error("Unable to seek file: " + name() +
                             ", it is not open");
  }

  const auto new_offset = std::min<std::size_t>(offset, m_size);

  if (new_offset < m_bytes_consumed) {
    throw std::runtime_error("Unable to seek file: " + name() +
                             ", requested offset has already been consumed");
  }

  m_offset = new_offset;
  m_reading_from_beginning =
      m_offset == m_bytes_consumed ||
      (m_consume_if_first && m_offset < m_bytes_consumed + m_block_size);

  return tell();
}

off64_t Allocated_file::tell() {
  if (!is_open()) {
    throw std::runtime_error("Unable to fetch offset of file: " + name() +
                             ", it is not open");
  }

  return m_offset;
}

ssize_t Allocated_file::read(void *buffer, std::size_t length) {
  if (!is_open()) {
    throw std::runtime_error("Unable to read from file: " + name() +
                             ", it is not open");
  }

  if (!m_read_mode) {
    throw std::runtime_error("Unable to read from file: " + name() +
                             ", it is opened for writing");
  }

  if (m_offset == m_size) {
    return 0;
  }

  auto block_number = (m_offset - m_bytes_consumed) / m_block_size;
  auto block_offset = m_offset - m_bytes_consumed - block_number * m_block_size;
  decltype(block_offset) to_read = 0;
  auto char_buffer = static_cast<char *>(buffer);
  ssize_t result = 0;

  while (length > 0 && m_offset != m_size) {
    to_read = std::min(std::min(length, m_block_size - block_offset),
                       m_size - m_offset);

    ::memcpy(char_buffer, m_blocks[block_number] + block_offset, to_read);

    result += to_read;
    m_offset += to_read;
    char_buffer += to_read;
    length -= to_read;

    if (m_reading_from_beginning &&
        (block_offset + to_read == m_block_size || m_offset == m_size)) {
      // we're reading from the beginning and the whole block has been read, it
      // can be discarded
      m_bytes_consumed += m_block_size;
      m_allocator->free(m_blocks.front());
      m_blocks.pop_front();
    } else {
      ++block_number;
    }

    block_offset = 0;
  }

  return result;
}

ssize_t Allocated_file::write(const void *buffer, std::size_t length) {
  if (!is_open()) {
    throw std::runtime_error("Unable to write to file: " + name() +
                             ", it is not open");
  }

  if (m_read_mode) {
    throw std::runtime_error("Unable to write to file: " + name() +
                             ", it is opened for reading");
  }

  if (m_capacity - m_offset < length) {
    for (auto block : m_allocator->allocate(length - (m_capacity - m_offset))) {
      m_blocks.emplace_back(block);
      m_capacity += m_block_size;
    }
  }

  auto block_number = m_offset / m_block_size;
  auto block_offset = m_offset - block_number * m_block_size;
  decltype(block_offset) to_write = 0;
  auto char_buffer = static_cast<const char *>(buffer);
  const ssize_t result = length;

  while (length > 0) {
    to_write = std::min(length, m_block_size - block_offset);

    ::memcpy(m_blocks[block_number] + block_offset, char_buffer, to_write);

    char_buffer += to_write;
    length -= to_write;
    block_offset = 0;
    ++block_number;
  }

  m_offset += result;

  if (m_offset > m_size) {
    m_size = m_offset;
  }

  return result;
}

void Allocated_file::append(Scoped_data_block block) {
  if (!m_accepts_append) {
    throw std::runtime_error("Unable to append a block to file: " + name() +
                             ", not allowed");
  }

  m_blocks.emplace_back(block->memory);
  m_size += block->size;
  m_capacity += m_block_size;

  // it's no longer possible to append a new block after an incomplete block was
  // added, this ensures the continuity of the blocks
  m_accepts_append = block->size == m_block_size;

  block.relinquish();
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
