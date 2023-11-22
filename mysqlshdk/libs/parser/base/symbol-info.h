/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#pragma once

// Provides MySQL symbol information for various tasks, like symbol tables,
// identifier quoting + code completion.

#include <cstdint>
#include <set>
#include <string>
#include <string_view>

namespace base {

// Currently supported MySQL versions.
enum class MySQLVersion {
  Unknown,
  MySQL57,
  MySQL80,
  MySQL81,
  MySQL82,
  MySQL83,
  Highest,
};

class MySQLSymbolInfo {
 public:
  static std::set<std::string> const &systemFunctionsForVersion(
      MySQLVersion version);

  static std::set<std::string_view> const &keywordsForVersion(
      MySQLVersion version);

  static bool isReservedKeyword(std::string_view identifier,
                                MySQLVersion version);

  static bool isKeyword(std::string_view identifier, MySQLVersion version);

  static MySQLVersion numberToVersion(uint32_t version);
};

}  // namespace base
