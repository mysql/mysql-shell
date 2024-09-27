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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FS_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FS_H_

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/utils/atomic_flag.h"

#include "mysqlshdk/libs/storage/backend/in_memory/allocator.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

/**
 * In-memory file system.
 */
class Virtual_fs final {
 public:
  class Directory;

  /**
   * In-memory file.
   */
  class IFile {
   public:
    /**
     * Creates file with the given name.
     *
     * @param name Name of the file.
     */
    explicit IFile(const std::string &name) : m_name(name) {}

    IFile(const IFile &) = delete;
    IFile(IFile &&) = default;

    IFile &operator=(const IFile &) = delete;
    IFile &operator=(IFile &&) = delete;

    virtual ~IFile() = default;

    /**
     * Name of the file.
     */
    inline const std::string &name() const { return m_name; }

    /**
     * Size of the file.
     */
    virtual std::size_t size() const = 0;

    /**
     * Whether this file is open.
     */
    virtual bool is_open() const = 0;

    /**
     * Opens the file.
     *
     * @param read_mode Set to true when opening for reading.
     */
    virtual void open(bool read_mode) = 0;

    /**
     * Closes the file.
     *
     * @throws std::runtime_error If file is already closed.
     */
    virtual void close() = 0;

    /**
     * Moves internal offset of the file to the given position. If offset is
     * greater than size, the size is used instead.
     *
     * @param offset New offset.
     *
     * @returns Offset after the operation.
     */
    virtual off64_t seek(off64_t offset) = 0;

    /**
     * Provides current offset of the file.
     *
     * @throws std::runtime_error If file is closed.
     */
    virtual off64_t tell() = 0;

    /**
     * Reads file contents into a buffer.
     *
     * @param buffer Buffer which will hold the data.
     * @param length Number of bytes to read.
     *
     * @returns Number of bytes read.
     */
    virtual ssize_t read(void *buffer, std::size_t length) = 0;

    /**
     * Writes file contents from a buffer.
     *
     * @param buffer Buffer which holds the data.
     * @param length Number of bytes to write.
     *
     * @returns Number of bytes written.
     */
    virtual ssize_t write(const void *buffer, std::size_t length) = 0;

   private:
    friend class Virtual_fs::Directory;

    void rename(const std::string &new_name) { m_name = new_name; }

    std::string m_name;
  };

  /**
   * In-memory directory.
   */
  class Directory final {
   public:
    /**
     * Creates directory with the given name.
     *
     * @param name Name of the directory.
     * @param fs Virtual file system.
     */
    Directory(const std::string &name, Virtual_fs *fs);

    Directory(const Directory &) = delete;
    Directory(Directory &&) = delete;

    Directory &operator=(const Directory &) = delete;
    Directory &operator=(Directory &&) = delete;

    ~Directory() = default;

    /**
     * Name of the directory.
     */
    const std::string &name() const { return m_name; }

    /**
     * Fetches a file with the given name.
     *
     * @param name Name of the file to fetch.
     *
     * @returns The requested file or nullptr if file does not exist.
     */
    IFile *file(const std::string &name) const;

    /**
     * Creates the file with the given name. File is not visible in this
     * directory until it is published using publish_created_file().
     *
     * @param name Name of the file to create.
     *
     * @throws std::runtime_error If name is invalid.
     * @throws std::runtime_error If file already exists.
     *
     * @returns The created file.
     */
    IFile *create_file(const std::string &name);

    /**
     * Lists files in this directory.
     *
     * @param pattern List only files matching this glob pattern.
     *
     * @returns List of all or matching files.
     */
    std::unordered_set<IDirectory::File_info> list_files(
        const std::string &pattern = {}) const;

    /**
     * Renames a file.
     *
     * @param old_name Current name of the file.
     * @param new_name New name of the file.
     *
     * @throws std::runtime_error If new name is invalid.
     * @throws std::runtime_error If old_name file does not exist.
     * @throws std::runtime_error If new_name file already exists.
     */
    void rename_file(const std::string &old_name, const std::string &new_name);

    /**
     * Removes a file.
     *
     * @param name Name of the file.
     *
     * @throws std::runtime_error If file does not exist.
     * @throws std::runtime_error If file is currently open.
     */
    void remove_file(const std::string &name);

    /**
     * Publishes a file. A created file is not visible until it is published.
     * Calling this method multiple times with the same name is a NO-OP.
     *
     * @param name Name of the file.
     */
    void publish_created_file(const std::string &name);

   private:
    Virtual_fs *m_fs;
    std::string m_name;
    std::unordered_map<std::string, std::unique_ptr<IFile>> m_files;
    // we keep files which were created but not yet closed in a separate map,
    // this ensures that we're not going to read from an unfinished file
    std::unordered_map<std::string, std::unique_ptr<IFile>> m_created_files;
    mutable std::mutex m_mutex;
  };

  /**
   * Initializes the virtual file system with the given allocator parameters.
   *
   * @param page_size Size of a single memory page.
   * @param block_size Size of a single block in the memory page.
   */
  explicit Virtual_fs(std::size_t page_size, std::size_t block_size = 8192);

  Virtual_fs(const Virtual_fs &) = delete;
  Virtual_fs(Virtual_fs &&) = delete;

  Virtual_fs &operator=(const Virtual_fs &) = delete;
  Virtual_fs &operator=(Virtual_fs &&) = delete;

  ~Virtual_fs() = default;

  /**
   * Joins two path segments.
   *
   * @param a Left segment.
   * @param b Right segment.
   *
   * @returns Joined path.
   */
  static std::string join_path(const std::string &a, const std::string &b);

  /**
   * Splits the path into segments.
   *
   * @param path A path to be split.
   *
   * @returns Path segments.
   */
  static std::vector<std::string> split_path(const std::string &path);

  /**
   * Fetches the directory with the given name.
   *
   * @param name Name of the directory to fetch.
   *
   * @returns The requested directory or nullptr if directory does not exist.
   */
  Directory *directory(const std::string &name) const;

  /**
   * Creates a directory with the given name.
   *
   * @param name Name of the directory to create.
   *
   * @throws std::runtime_error If name is invalid.
   * @throws std::runtime_error If directory already exists.
   *
   * @returns The created directory.
   */
  Directory *create_directory(const std::string &name);

  /**
   * Signals that the I/O operations should be interrupted.
   */
  void interrupt();

  /**
   * Sets a callback which returns true if a file with the given name should
   * use synchronized I/O operations.
   */
  void set_uses_synchronized_io(std::function<bool(std::string_view)> callback);

 private:
  Allocator m_allocator;
  std::unordered_map<std::string, std::unique_ptr<Directory>> m_dirs;
  mutable std::mutex m_mutex;
  shcore::atomic_flag m_interrupted;
  std::function<bool(std::string_view)> m_uses_synchronized_io;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FS_H_
