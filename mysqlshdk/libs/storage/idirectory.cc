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

#include "mysqlshdk/libs/storage/idirectory.h"

#include <iterator>
#include <stdexcept>

#include "mysqlshdk/libs/storage/backend/directory.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/natural_compare.h"

namespace mysqlshdk {
namespace storage {

namespace {

std::set<IDirectory::File_info> sort(
    std::unordered_set<IDirectory::File_info> &&input) {
  std::set<IDirectory::File_info> output;

  std::move(input.begin(), input.end(), std::inserter(output, output.begin()));

  return output;
}

}  // namespace

IDirectory::File_info::File_info(std::string name, std::size_t size)
    : m_name(std::move(name)), m_size(size) {}

IDirectory::File_info::File_info(std::string name,
                                 std::function<std::size_t()> &&get_size)
    : m_name(std::move(name)), m_get_size(std::move(get_size)) {}

std::size_t IDirectory::File_info::size() const {
  if (!m_size) {
    assert(m_get_size);
    const_cast<File_info *>(this)->set_size(m_get_size());
  }

  return m_size.value();
}

void IDirectory::File_info::set_size(std::size_t s) {
  m_size = s;
  m_get_size = nullptr;
}

bool IDirectory::File_info::operator<(const File_info &other) const {
  return shcore::natural_compare(name().begin(), name().end(),
                                 other.name().begin(), other.name().end());
}

std::unique_ptr<IFile> IDirectory::file(const std::string &name,
                                        const File_options &options) const {
  return make_file(join_path(full_path().real(), name), options);
}

std::set<IDirectory::File_info> IDirectory::list_files_sorted(
    bool hidden_files) const {
  return sort(list_files(hidden_files));
}

std::set<IDirectory::File_info> IDirectory::filter_files_sorted(
    const std::string &pattern) const {
  return sort(filter_files(pattern));
}

std::unique_ptr<IDirectory> make_directory(const std::string &path) {
  const auto scheme = utils::get_scheme(path);
  if (scheme.empty() || utils::scheme_matches(scheme, "file")) {
    return std::make_unique<backend::Directory>(path);
  }

  throw std::invalid_argument("Directory handling for " + scheme +
                              " protocol is not supported.");
}

std::unique_ptr<IDirectory> make_directory(const std::string &path,
                                           const Config_ptr &config) {
  if (config && config->valid()) {
    return config->make_directory(path);
  } else {
    return make_directory(path);
  }
}

}  // namespace storage
}  // namespace mysqlshdk
