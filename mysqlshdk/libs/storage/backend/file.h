/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_FILE_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

enum class Mmap_preference {
  OFF,      // mmap not allowed even if requested and possible
  ON,       // mmap used if requested, fallback if not possible
  REQUIRED  // mmap used if requested, error if not possible
};

Mmap_preference to_mmap_preference(const std::string &s);

class File : public IFile {
 public:
  struct Options {
    Mmap_preference mmap;

    Options() : mmap(Mmap_preference::OFF) {}
  };

  File() = delete;
  explicit File(const std::string &filename,
                const Options &options = Options());
  File(const File &other) = delete;
  File(File &&other) = default;

  File &operator=(const File &other) = delete;
  File &operator=(File &&other) = default;

  ~File() override;

  void open(Mode m) override;
  bool is_open() const override;
  int error() const override;
  void close() override;

  size_t file_size() const override;
  Masked_string full_path() const override;
  std::string filename() const override;
  bool exists() const override;
  std::unique_ptr<IDirectory> parent() const override;

  off64_t seek(off64_t offset) override;
  off64_t tell() const override;
  ssize_t read(void *buffer, size_t length) override;
  ssize_t write(const void *buffer, size_t length) override;
  bool flush() override;

  void rename(const std::string &new_name) override;
  void remove() override;

  bool is_local() const override { return true; }

 public:
  bool mmapped() const { return m_mmap_ptr != nullptr; }
  /**
   * Extend the file by the given amount, relative to the current position
   * and return a pointer. Use this for writing to a mmapped file.
   *
   * @param length minimum number of bytes to reserve
   * @param out_avail if not null, contains the number of bytes available to be
   * written
   * @return pointer to memory area where that can be written to
   *
   * This method can only be used on files open for writing.
   */
  char *mmap_will_write(size_t length, size_t *out_avail = nullptr);

  /**
   * Indicate how many bytes were written to the area reserved by mmap_reserve()
   *
   * @param length number of bytes written
   * @param out_avail if not null, contains the number of bytes available to be
   * written
   * @return pointer to memory area to continue writing to buffer
   *
   * This method can only be used on files open for writing.
   */
  char *mmap_did_write(size_t length, size_t *out_avail = nullptr);

  /**
   * mmap() the file for reading and return pointer to memory area.
   *
   * @param out_avail if not null, contains number of bytes available
   * @return pointer to memory area that can be used to access file data.
   *
   * If file is already mmapped, just returns the current position in the area,
   * which is incremented by mmap_advanced().
   *
   * File must be open for reading. Reading more then file_size() bytes will
   * trigger a BUS error.
   */
  const char *mmap_will_read(size_t *out_avail = nullptr);

  /**
   * Advance offset within.
   *
   * @param length number of bytes to advance
   * @param out_avail if not null, contains number of bytes left available
   * @return number of bytes advanced, 0 if past EOF
   *
   * File must be open for reading.
   */
  const char *mmap_did_read(size_t length, size_t *out_avail = nullptr);

 private:
  void do_close();
#ifndef _WIN32
  bool init_mmap_read();
#endif

  FILE *m_file = nullptr;

  std::string m_filepath;

  Mmap_preference m_use_mmap = Mmap_preference::OFF;
  bool m_writing = false;

  char *m_mmap_ptr = nullptr;
  size_t m_mmap_offset = 0;
  size_t m_mmap_used = 0;
  size_t m_mmap_available = 0;
};

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_FILE_H_
