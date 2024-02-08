/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/parser/base/symbol-info.h"

#include <iterator>
#include <map>
#include <utility>

#include "mysqlshdk/libs/parser/server/keyword_list57.h"

#include "mysqlshdk/libs/parser/server/keyword_diff80.h"
#include "mysqlshdk/libs/parser/server/keyword_diff81.h"
#include "mysqlshdk/libs/parser/server/keyword_diff82.h"
#include "mysqlshdk/libs/parser/server/keyword_diff83.h"

#include "mysqlshdk/libs/parser/server/system-functions.h"

namespace base {

//----------------------------------------------------------------------------------------------------------------------

std::set<std::string> const &MySQLSymbolInfo::systemFunctionsForVersion(
    MySQLVersion version) {
  static std::set<std::string> k_empty;

  switch (version) {
    case MySQLVersion::MySQL57:
      return systemFunctions57;

    case MySQLVersion::MySQL80:
    case MySQLVersion::MySQL81:
    case MySQLVersion::MySQL82:
    case MySQLVersion::MySQL83:
    case MySQLVersion::Highest:
      return systemFunctions80;

    default:
      return k_empty;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<MySQLVersion, std::set<std::string_view>> keywords;
static std::map<MySQLVersion, std::set<std::string_view>> reservedKeywords;

std::set<std::string_view> const &MySQLSymbolInfo::keywordsForVersion(
    MySQLVersion version) {
  auto &result = keywords[version];

  if (result.empty()) {
    if (version >= MySQLVersion::MySQL57) {
      std::set<std::string_view> list;
      std::set<std::string_view> reserved_list;

      for (auto item : keyword_list_57) {
        if (item.reserved != 0) reserved_list.emplace(item.word);
        list.emplace(item.word);
      }

      result = std::move(list);
      reservedKeywords[version] = std::move(reserved_list);
    }

    const auto add = [&](auto begin, auto end) {
      for (auto it = begin; it != end; ++it) {
        if (it->reserved != 0) reservedKeywords[version].emplace(it->word);
        result.emplace(it->word);
      }
    };

    const auto remove = [&](auto begin, auto end) {
      for (auto it = begin; it != end; ++it) {
        if (it->reserved != 0) reservedKeywords[version].erase(it->word);
        result.erase(it->word);
      }
    };

    const auto apply = [&](auto &&added, auto &&removed) {
      remove(std::begin(removed), std::end(removed));
      add(std::begin(added), std::end(added));
    };

    if (version >= MySQLVersion::MySQL80) {
      apply(keyword_diff_80::added, keyword_diff_80::removed);
    }

    if (version >= MySQLVersion::MySQL81) {
      apply(keyword_diff_81::added, keyword_diff_81::removed);
    }

    if (version >= MySQLVersion::MySQL82) {
      apply(keyword_diff_82::added, keyword_diff_82::removed);
    }

    if (version >= MySQLVersion::MySQL83) {
      apply(keyword_diff_83::added, keyword_diff_83::removed);
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool MySQLSymbolInfo::isReservedKeyword(std::string_view identifier,
                                        MySQLVersion version) {
  keywordsForVersion(version);
  return reservedKeywords[version].count(identifier) > 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * For both, reserved and non-reserved keywords.
 */
bool MySQLSymbolInfo::isKeyword(std::string_view identifier,
                                MySQLVersion version) {
  return keywordsForVersion(version).count(identifier) > 0;
}

//----------------------------------------------------------------------------------------------------------------------

MySQLVersion MySQLSymbolInfo::numberToVersion(uint32_t version) {
  uint32_t major = version / 10000, minor = (version / 100) % 100;

  if (major < 5) return MySQLVersion::Unknown;
  if (major > 8) return MySQLVersion::Highest;

  switch (major) {
    case 5:
      return MySQLVersion::MySQL57;

    case 8:
      switch (minor) {
        case 0:
          return MySQLVersion::MySQL80;

        case 1:
          return MySQLVersion::MySQL81;

        case 2:
          return MySQLVersion::MySQL82;

        case 3:
          return MySQLVersion::MySQL83;

        default:
          return MySQLVersion::Highest;
      }

    default:
      return MySQLVersion::Unknown;
  }
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace base
