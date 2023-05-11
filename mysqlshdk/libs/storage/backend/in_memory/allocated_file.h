/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_ALLOCATED_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_ALLOCATED_FILE_H_

#include <deque>
#include <string>

#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

/**
 * Keeps the file in memory until the whole data is read, allocates memory for
 * the file contents once, then dynamically adds blocks of set size when writing
 * to a file. The whole file can be read only once, the blocks are released
 * while reading.
 */
class Allocated_file : public Virtual_fs::IFile {
 public:
  /**
   * Creates file with the given name, read/write operations will use the given
   * allocator.
   *
   * @param name Name of the file.
   * @param allocator Allocator to use.
   * @param consume_if_first Blocks are released as long as reading starts at
   * the first block.
   */
  Allocated_file(const std::string &name, Allocator *allocator,
                 bool consume_if_first = false);

  Allocated_file(const Allocated_file &) = delete;
  Allocated_file(Allocated_file &&) = default;

  Allocated_file &operator=(const Allocated_file &) = delete;
  Allocated_file &operator=(Allocated_file &&) = delete;

  ~Allocated_file() override;

  /**
   * Size of the file.
   */
  std::size_t size() const override { return m_size; }

  /**
   * Whether this file is open.
   */
  bool is_open() const override { return m_is_open; }

  /**
   * Opens the file.
   *
   * @param read_mode Set to true when opening for reading.
   *
   * @throws std::runtime_error If file is already open.
   * @throws std::runtime_error When opening file for writing after it was
   *                            read from.
   */
  void open(bool read_mode) override;

  /**
   * Closes the file.
   *
   * @throws std::runtime_error If file is already closed.
   */
  void close() override;

  /**
   * Moves internal offset of the file to the given position. If offset is
   * greater than size, the size is used instead.
   *
   * @param offset New offset.
   *
   * @throws std::runtime_error When using negative value.
   * @throws std::runtime_error If file is closed.
   * @throws std::runtime_error If offset points to the part of the file which
   *                            was discarded.
   *
   * @returns Offset after the operation.
   */
  off64_t seek(off64_t offset) override;

  /**
   * Provides current offset of the file.
   *
   * @throws std::runtime_error If file is closed.
   */
  off64_t tell() override;

  /**
   * Reads file contents into a buffer.
   *
   * @param buffer Buffer which will hold the data.
   * @param length Number of bytes to read.
   *
   * @throws std::runtime_error If file is closed.
   * @throws std::runtime_error If file is opened for writing.
   *
   * @returns Number of bytes read.
   */
  ssize_t read(void *buffer, std::size_t length) override;

  /**
   * Writes file contents from a buffer.
   *
   * @param buffer Buffer which holds the data.
   * @param length Number of bytes to write.
   *
   * @throws std::runtime_error If file is closed.
   * @throws std::runtime_error If file is opened for reading.
   *
   * @returns Number of bytes written.
   */
  ssize_t write(const void *buffer, std::size_t length) override;

  /**
   * Appends a data block, file takes ownership of the memory.
   *
   * @param block Buffer which holds the data.
   *
   * @throws std::runtime_error If it's not possible to append a new block.
   */
  void append(Scoped_data_block block);

 private:
  Allocator *m_allocator;
  bool m_is_open = false;
  bool m_read_mode = false;
  std::size_t m_size = 0;
  std::size_t m_capacity = 0;
  std::size_t m_offset = 0;
  const std::size_t m_block_size;
  // number of bytes read from the blocks which were released
  std::size_t m_bytes_consumed = 0;
  bool m_reading_from_beginning = false;
  std::deque<char *> m_blocks;
  bool m_consume_if_first;
  bool m_accepts_append = true;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_ALLOCATED_FILE_H_
