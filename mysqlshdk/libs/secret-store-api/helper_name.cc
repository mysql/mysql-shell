/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-secret-store/include/mysql-secret-store/api.h"

namespace mysql {
namespace secret_store {
namespace api {

class Helper_name::Helper_name_impl {
 public:
  Helper_name_impl(const std::string &name, const std::string &path) noexcept
      : m_name{name}, m_path{path} {}

  std::string name() const noexcept { return m_name; }

  std::string path() const noexcept { return m_path; }

 private:
  std::string m_name;
  std::string m_path;
};

Helper_name::Helper_name(const std::string &name,
                         const std::string &path) noexcept
    : m_impl{new Helper_name_impl{name, path}} {}

Helper_name::Helper_name(const Helper_name &name) noexcept { *this = name; }

Helper_name::~Helper_name() noexcept = default;

std::string Helper_name::get() const noexcept { return m_impl->name(); }

std::string Helper_name::path() const noexcept { return m_impl->path(); }

Helper_name &Helper_name::operator=(const Helper_name &r) noexcept {
  if (this != &r) {
    m_impl = std::unique_ptr<Helper_name_impl>{new Helper_name_impl{*r.m_impl}};
  }

  return *this;
}

}  // namespace api
}  // namespace secret_store
}  // namespace mysql
