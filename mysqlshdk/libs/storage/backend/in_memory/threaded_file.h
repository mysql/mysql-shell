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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_THREADED_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_THREADED_FILE_H_

#include <atomic>
#include <condition_variable>
#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "mysqlshdk/libs/utils/synchronized_queue.h"

#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/ifile.h"

#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

class Allocator;

/**
 * Configuration used to initialize the threaded file.
 */
struct Threaded_file_config {
  /// Path to the file.
  std::string file_path;
  /// Optional configuration of the file system.
  Config_ptr config;
  /// Number of threads used to fetch the file.
  std::size_t threads = 4;
  /// Maximum amount of memory to be allocated at once. Will be rounded up.
  std::size_t max_memory = 1024 * 1024;
  /// Allocator used to manage the allocated memory.
  Allocator *allocator = nullptr;
};

/**
 * Fetches the file using multiple threads, only for reading.
 */
class Threaded_file final : public IFile {
 public:
  Threaded_file(const Threaded_file &) = delete;
  Threaded_file(Threaded_file &&) = delete;

  Threaded_file &operator=(const Threaded_file &) = delete;
  Threaded_file &operator=(Threaded_file &&) = delete;

  ~Threaded_file() override;

  void open(Mode m) override;

  bool is_open() const override { return m_file->is_open(); }

  int error() const override { return m_file->error(); }

  void close() override;

  size_t file_size() const override { return m_file_size; }

  Masked_string full_path() const override { return m_file->full_path(); }

  std::string filename() const override { return m_file->filename(); }

  bool exists() const override { return m_file->exists(); }

  std::unique_ptr<IDirectory> parent() const override {
    throw std::logic_error("Threaded_file::parent() - not supported");
  }

  off64_t seek(off64_t) override {
    throw std::logic_error("Threaded_file::seek() - not supported");
  }

  off64_t tell() const override {
    throw std::logic_error("Threaded_file::tell() - not supported");
  }

  ssize_t read(void *buffer, size_t length) override;

  ssize_t write(const void *, size_t) override {
    throw std::logic_error("Threaded_file::write() - not supported");
  }

  bool flush() override {
    throw std::logic_error("Threaded_file::flush() - not supported");
  }

  bool is_compressed() const override { return m_file->is_compressed(); }

  bool is_local() const override { return m_file->is_local(); }

  void rename(const std::string &) override {
    throw std::logic_error("Threaded_file::rename() - not supported");
  }

  void remove() override {
    throw std::logic_error("Threaded_file::remove() - not supported");
  }

  Data_block peek();

  void pop_front();

  Scoped_data_block extract();

 private:
  struct Block {
    Data_block data;
    std::size_t offset_in_file = 0;
    std::size_t consumed = 0;
    std::unique_ptr<std::atomic_bool> ready =
        std::make_unique<std::atomic_bool>(false);
  };

  friend std::unique_ptr<Threaded_file> threaded_file(Threaded_file_config);

  /**
   * Initializes the file.
   *
   * @param config Configuration.
   *
   * @throws std::invalid_argument if configuration is not valid
   */
  explicit Threaded_file(Threaded_file_config config);

  auto lock() { return std::unique_lock{m_mutex}; }

  void handle_exception() const;

  bool schedule_block(bool force = false);

  void close_impl();

  Block *front();

  Threaded_file_config m_config;

  std::unique_ptr<IFile> m_file;
  bool m_compressed;
  std::size_t m_file_size;
  std::size_t m_offset = 0;
  // signals that EOF was read by the background thread
  std::atomic_bool m_eof = false;
  // EOF was reached by the main thread
  bool m_read_eof = false;

  std::list<Block> m_blocks;
  std::size_t m_block_size;

  std::vector<std::thread> m_workers;
  std::unique_ptr<shcore::Synchronized_queue<Block *>> m_tasks;
  std::atomic_bool m_has_exception = false;
  std::exception_ptr m_worker_exception;

  std::mutex m_mutex;
  std::condition_variable m_cv;
};

std::unique_ptr<Threaded_file> threaded_file(Threaded_file_config config);

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_THREADED_FILE_H_
