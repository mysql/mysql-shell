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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_SYNCHRONIZED_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_SYNCHRONIZED_FILE_H_

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>

#include "mysqlshdk/libs/utils/synchronized_queue.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

/**
 * I/O operations are synchronized, write waits for a read operation in order
 * to write directly to the buffer. Only a single reader and a single writer are
 * allowed at a time. The reader and the writer must be in separate threads.
 */
class Synchronized_file : public Virtual_fs::IFile {
 public:
  /**
   * Creates file with the given name.
   *
   * @param name Name of the file.
   * @param interrupted Callback which signals that I/O should be aborted.
   */
  explicit Synchronized_file(const std::string &name,
                             std::atomic<bool> *interrupted);

  Synchronized_file(const Synchronized_file &) = delete;
  Synchronized_file(Synchronized_file &&) = delete;

  Synchronized_file &operator=(const Synchronized_file &) = delete;
  Synchronized_file &operator=(Synchronized_file &&) = delete;

  ~Synchronized_file() override = default;

  /**
   * Size of the file.
   */
  std::size_t size() const override { return m_size; }

  /**
   * Whether this file is open.
   */
  bool is_open() const override;

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
   * Seek is not supported.
   */
  off64_t seek(off64_t) override {
    throw std::logic_error("Synchronized_file::seek() - not implemented");
  }

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
   * Provides the size of the buffer for the pending write operation.
   */
  std::size_t pending_write_size() const { return m_pending_write; }

 private:
  struct Read_request {
    void *buffer = nullptr;
    std::size_t length = 0;
    ssize_t written = 0;
  };

  struct Read_response {
    ssize_t length = 0;
  };

  bool is_interrupted() const;

  std::atomic<bool> *m_interrupted;

  std::mutex m_open_close_mutex;
  std::atomic<bool> m_reading = false;
  std::atomic<bool> m_writing = false;

  std::atomic<std::size_t> m_size = 0;
  std::atomic<std::size_t> m_pending_write = 0;

  shcore::Synchronized_queue<bool> m_read_requests;
  shcore::Synchronized_queue<Read_response> m_read_responses;

  // access to this variable is protected by the two queues
  Read_request m_request;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_SYNCHRONIZED_FILE_H_
