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

#include "mysql-secret-store/include/helper.h"

namespace mysql {
namespace secret_store {
namespace common {

Helper_exception get_helper_exception(Helper_exception_code code) {
  switch (code) {
    case Helper_exception_code::NO_SUCH_SECRET:
      return Helper_exception{"Could not find the secret"};
  }

  throw std::runtime_error{"Unknown exception code"};
}

bool Secret_id::operator==(const Secret_id &r) const noexcept {
  return secret_type == r.secret_type && url == r.url;
}

bool Secret_id::operator!=(const Secret_id &r) const noexcept {
  return !operator==(r);
}

Secret::Secret(const std::string &s, const Secret_id &secret_id) noexcept
    : secret{s}, id(secret_id) {}

Helper::Helper(const std::string &name, const std::string &version,
               const std::string &copyright) noexcept
    : m_name{name}, m_version{version}, m_copyright{copyright} {}

std::string Helper::name() const noexcept {
  return std::string{"mysql-secret-store-"} + m_name;
}

std::string Helper::version() const noexcept { return m_version; }

std::string Helper::copyright() const noexcept { return m_copyright; }

}  // namespace common
}  // namespace secret_store
}  // namespace mysql
