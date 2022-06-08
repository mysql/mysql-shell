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

#include "mysqlshdk/libs/parser/base/symbol-info.h"

#include <map>
#include <utility>

#include "mysqlshdk/libs/parser/server/keyword_list57.h"
#include "mysqlshdk/libs/parser/server/keyword_list80.h"
#include "mysqlshdk/libs/parser/server/system-functions.h"

namespace base {

//----------------------------------------------------------------------------------------------------------------------

static std::set<std::string> empty;

std::set<std::string> const &MySQLSymbolInfo::systemFunctionsForVersion(
    MySQLVersion version) {
  switch (version) {
    case MySQLVersion::MySQL57:
      return systemFunctions57;
    case MySQLVersion::MySQL80:
      return systemFunctions80;

    default:
      return empty;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<MySQLVersion, std::set<std::string>> keywords;
static std::map<MySQLVersion, std::set<std::string>> reservedKeywords;

std::set<std::string> const &MySQLSymbolInfo::keywordsForVersion(
    MySQLVersion version) {
  auto &result = keywords[version];

  if (result.empty()) {
    std::set<std::string> list;
    std::set<std::string> reservedList;

    switch (version) {
      case MySQLVersion::MySQL57: {
        for (const auto &keyword : keyword_list57) {
          std::string word = keyword.word;
          if (keyword.reserved != 0) reservedList.emplace(word);
          list.emplace(std::move(word));
        }
        break;
      }

      case MySQLVersion::MySQL80: {
        for (const auto &keyword : keyword_list80) {
          std::string word = keyword.word;
          if (keyword.reserved != 0) reservedList.emplace(word);
          list.emplace(std::move(word));
        }
        break;
      }

      default:
        break;
    }

    result = std::move(list);
    reservedKeywords[version] = std::move(reservedList);
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLSymbolInfo::isReservedKeyword(std::string const &identifier,
                                        MySQLVersion version) {
  keywordsForVersion(version);
  return reservedKeywords[version].count(identifier) > 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * For both, reserved and non-reserved keywords.
 */
bool MySQLSymbolInfo::isKeyword(std::string const &identifier,
                                MySQLVersion version) {
  return keywordsForVersion(version).count(identifier) > 0;
}

//----------------------------------------------------------------------------------------------------------------------

MySQLVersion MySQLSymbolInfo::numberToVersion(uint32_t version) {
  uint32_t major = version / 10000, minor = (version / 100) % 100;

  if (major < 5 || major > 8) return MySQLVersion::Unknown;

  if (major == 8) return MySQLVersion::MySQL80;

  if (major != 5) return MySQLVersion::Unknown;

  switch (minor) {
    case 7:
      return MySQLVersion::MySQL57;
    default:
      return MySQLVersion::Unknown;
  }
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace base
