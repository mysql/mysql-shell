/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/entry.h"

#include <cassert>
#include <utility>

#include "mysqlshdk/libs/utils/natural_compare.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace storage {

namespace {

template <typename T>
inline std::unordered_set<T> filter_entries(std::unordered_set<T> &&input,
                                            const std::string &pattern) {
  std::unordered_set<T> output;

  while (!input.empty()) {
    auto entry = input.extract(input.begin());

    if (pattern.empty() || shcore::match_glob(pattern, entry.value().name())) {
      output.emplace(std::move(entry.value()));
    }
  }

  return output;
}

template <typename T>
inline std::set<T> sort_entries(std::unordered_set<T> &&input) {
  std::set<T> output;

  while (!input.empty()) {
    output.emplace(std::move(input.extract(input.begin()).value()));
  }

  return output;
}

}  // namespace

namespace detail {

Entry_info::Entry_info(std::string name) : m_name(std::move(name)) {}

bool Entry_info::operator<(const Entry_info &other) const {
  return shcore::natural_compare(name().begin(), name().end(),
                                 other.name().begin(), other.name().end());
}

}  // namespace detail

File_info::File_info(std::string name, std::size_t size)
    : Entry_info(std::move(name)), m_size(size) {}

File_info::File_info(std::string name, std::function<std::size_t()> &&get_size)
    : Entry_info(std::move(name)), m_get_size(std::move(get_size)) {}

std::size_t File_info::size() const {
  if (!m_size.has_value()) {
    assert(m_get_size);
    const_cast<File_info *>(this)->set_size(m_get_size());
  }

  return m_size.value();
}

void File_info::set_size(std::size_t s) {
  m_size = s;
  m_get_size = nullptr;
}

File_list filter(File_list &&list, const std::string &pattern) {
  return filter_entries(std::move(list), pattern);
}

Directory_list filter(Directory_list &&list, const std::string &pattern) {
  return filter_entries(std::move(list), pattern);
}

Directory_listing filter(Directory_listing &&list, const std::string &pattern) {
  return {filter(std::move(list.files), pattern),
          filter(std::move(list.directories), pattern)};
}

Sorted_file_list sort(File_list &&list) {
  return sort_entries(std::move(list));
}

Sorted_directory_list sort(Directory_list &&list) {
  return sort_entries(std::move(list));
}

Sorted_directory_listing sort(Directory_listing &&list) {
  return {sort(std::move(list.files)), sort(std::move(list.directories))};
}

}  // namespace storage
}  // namespace mysqlshdk
