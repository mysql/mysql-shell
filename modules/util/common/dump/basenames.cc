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

#include "modules/util/common/dump/basenames.h"

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {
namespace common {

Basenames::Basenames(std::size_t max_basename_length)
    : m_max_basename_length(max_basename_length) {}

std::string Basenames::get(const std::string &name) {
  const auto wbasename = shcore::utf8_to_wide(name);
  const auto wtruncated = shcore::truncate(wbasename, m_max_basename_length);

  if (wbasename.length() != wtruncated.length()) {
    const auto truncated = shcore::wide_to_utf8(wtruncated);
    const auto ordinal = m_truncated_basenames[truncated]++;
    return truncated + std::to_string(ordinal);
  } else {
    return name;
  }
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
