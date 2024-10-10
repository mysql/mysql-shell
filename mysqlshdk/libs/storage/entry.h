/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_ENTRY_H_
#define MYSQLSHDK_LIBS_STORAGE_ENTRY_H_

#include <functional>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <unordered_set>

namespace mysqlshdk {
namespace storage {

namespace detail {

class Entry_info {
 public:
  Entry_info() = delete;

  /**
   * Constructor is not marked as explicit in order to make working with sets
   * more natural.
   */
  Entry_info(std::string name);

  Entry_info(const Entry_info &) = default;

  Entry_info(Entry_info &&) = default;

  Entry_info &operator=(const Entry_info &) = default;

  Entry_info &operator=(Entry_info &&) = default;

  /**
   * Name of this entry.
   */
  const std::string &name() const { return m_name; }

  bool operator<(const Entry_info &other) const;

  bool operator==(const Entry_info &other) const {
    return name() == other.name();
  }

  friend std::ostream &operator<<(::std::ostream &os, const Entry_info &info) {
    return os << info.name();
  }

 private:
  std::string m_name;
};

}  // namespace detail

class File_info : public detail::Entry_info {
 public:
  File_info() = delete;

  /**
   * Constructor is not marked as explicit in order to make working with sets
   * more natural.
   */
  File_info(std::string name, std::size_t size = 0);

  /**
   * Size is fetched on first usage.
   */
  File_info(std::string name, std::function<std::size_t()> &&get_size);

  File_info(const File_info &) = default;

  File_info(File_info &&) = default;

  File_info &operator=(const File_info &) = default;

  File_info &operator=(File_info &&) = default;

  /**
   * File size.
   */
  std::size_t size() const;

  /**
   * Sets the size.
   */
  void set_size(std::size_t s);

 private:
  std::optional<std::size_t> m_size;
  std::function<std::size_t()> m_get_size;
};

class Directory_info : public detail::Entry_info {
 public:
  using Entry_info::Entry_info;
};

}  // namespace storage
}  // namespace mysqlshdk

namespace std {

template <>
struct hash<mysqlshdk::storage::File_info> {
  size_t operator()(const mysqlshdk::storage::File_info &info) const {
    return std::hash<std::string>()(info.name());
  }
};

template <>
struct hash<mysqlshdk::storage::Directory_info> {
  size_t operator()(const mysqlshdk::storage::Directory_info &info) const {
    return std::hash<std::string>()(info.name());
  }
};

}  // namespace std

namespace mysqlshdk {
namespace storage {

using File_list = std::unordered_set<File_info>;
using Directory_list = std::unordered_set<Directory_info>;

struct Directory_listing {
  File_list files;
  Directory_list directories;
};

/**
 * Returns entry list matching the glob pattern.
 *
 * @param list List to be processed.
 * @param pattern Glob pattern. Available wildcards '*' and '?'. Wildcard
 * escaping available on linux using backslash '\' char.
 *
 * @return Entries which match the pattern.
 */
File_list filter(File_list &&list, const std::string &pattern);
Directory_list filter(Directory_list &&list, const std::string &pattern);
Directory_listing filter(Directory_listing &&list, const std::string &pattern);

using Sorted_file_list = std::set<File_info>;
using Sorted_directory_list = std::set<Directory_info>;

struct Sorted_directory_listing {
  Sorted_file_list files;
  Sorted_directory_list directories;
};

/**
 * Sorts the entries.
 *
 * @param list List to be processed.
 *
 * @returns All entries, sorted.
 */
Sorted_file_list sort(File_list &&list);
Sorted_directory_list sort(Directory_list &&list);
Sorted_directory_listing sort(Directory_listing &&list);

}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_ENTRY_H_
