/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/utils/masked_value.h"

#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {

class IDirectory {
 public:
  class File_info final {
   public:
    File_info() = delete;

    /**
     * Constructor is not marked as explicit in order to make
     *    std::[unordered_]set&lt;File_info&gt;::find(std::string)
     * more natural.
     */
    File_info(std::string name, std::size_t size = 0);

    /**
     * Size if fetched on first usage.
     */
    File_info(std::string name, std::function<std::size_t()> &&get_size);

    File_info(const File_info &) = default;

    File_info(File_info &&) = default;

    File_info &operator=(const File_info &) = default;

    File_info &operator=(File_info &&) = default;

    /**
     * Name of the file.
     */
    const std::string &name() const { return m_name; }

    /**
     * File size.
     */
    std::size_t size() const;

    /**
     * Sets the size.
     */
    void set_size(std::size_t s);

    bool operator<(const File_info &other) const;

    bool operator==(const File_info &other) const {
      return name() == other.name();
    }

   private:
    std::string m_name;
    std::optional<std::size_t> m_size;
    std::function<std::size_t()> m_get_size;
  };

  IDirectory() = default;
  IDirectory(const IDirectory &other) = delete;
  IDirectory(IDirectory &&other) = default;

  IDirectory &operator=(const IDirectory &other) = delete;
  IDirectory &operator=(IDirectory &&other) = default;

  virtual ~IDirectory() = default;

  virtual bool exists() const = 0;

  virtual void create() = 0;

  virtual void close() = 0;

  virtual Masked_string full_path() const = 0;

  /**
   * Lists all files in this directory.
   *
   * @returns All files in this directory.
   */
  virtual std::unordered_set<File_info> list_files(
      bool hidden_files = false) const = 0;

  /**
   * Lists all files in this directory in sorted order.
   *
   * @returns All files in this directory, sorted.
   */
  std::set<File_info> list_files_sorted(bool hidden_files = false) const;

  /**
   * Returns file list matching glob pattern.
   *
   * @param pattern Glob pattern. Available wildcards '*' and '?'. Wildcard
   * escaping available on linux using backslash '\' char.
   *
   * @return Files which match the pattern.
   */
  virtual std::unordered_set<File_info> filter_files(
      const std::string &pattern) const = 0;

  /**
   * Returns sorted file list matching glob pattern.
   *
   * @param pattern Glob pattern. Available wildcards '*' and '?'. Wildcard
   * escaping available on linux using backslash '\' char.
   *
   * @return Files which match the pattern, sorted.
   */
  std::set<File_info> filter_files_sorted(const std::string &pattern) const;

  /**
   * Provides handle to the file with the specified name in this directory.
   *
   * @param name Name of the file.
   * @param options File backend specific options.
   *
   * @returns Handle to the file.
   */
  virtual std::unique_ptr<IFile> file(const std::string &name,
                                      const File_options &options = {}) const;

  virtual bool is_local() const = 0;

  virtual std::string join_path(const std::string &a,
                                const std::string &b) const = 0;
};

std::unique_ptr<IDirectory> make_directory(const std::string &path);

std::unique_ptr<IDirectory> make_directory(const std::string &path,
                                           const Config_ptr &config);

}  // namespace storage
}  // namespace mysqlshdk

namespace std {

template <>
struct hash<mysqlshdk::storage::IDirectory::File_info> {
  size_t operator()(
      const mysqlshdk::storage::IDirectory::File_info &info) const {
    return std::hash<std::string>()(info.name());
  }
};

}  // namespace std

#endif  // MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_
