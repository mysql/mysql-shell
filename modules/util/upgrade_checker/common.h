/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_COMMON_H_
#define MODULES_UTIL_UPGRADE_CHECKER_COMMON_H_

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

// This map should be updated with the latest version of each series to enable
// gatting the latest version available in case a partial version is provided
// as the taget value, so for example, not needed when patch version is 0 in
// the last version of a series
extern std::unordered_map<std::string, Version> k_latest_versions;

enum class Config_mode { DEFINED, UNDEFINED };

enum class Target {
  INNODB_INTERNALS,
  OBJECT_DEFINITIONS,
  ENGINES,
  TABLESPACES,
  SYSTEM_VARIABLES,
  AUTHENTICATION_PLUGINS,
  MDS_SPECIFIC,
  LAST,
};

using Target_flags = mysqlshdk::utils::Enum_set<Target, Target::LAST>;

struct Upgrade_info {
  mysqlshdk::utils::Version server_version;
  std::string server_version_long;
  mysqlshdk::utils::Version target_version;
  std::string server_os;
  std::string config_path;
  bool explicit_target_version;
};

struct Upgrade_issue {
  enum Level { ERROR = 0, WARNING, NOTICE };
  static const char *level_to_string(const Upgrade_issue::Level level);

  std::string schema;
  std::string table;
  std::string column;
  std::string description;
  Level level = ERROR;

  bool empty() const {
    return schema.empty() && table.empty() && column.empty() &&
           description.empty();
  }
  std::string get_db_object() const;
};

std::string upgrade_issue_to_string(const Upgrade_issue &problem);

class Check_configuration_error : public std::runtime_error {
 public:
  explicit Check_configuration_error(const char *what)
      : std::runtime_error(what) {}
};

class Checker_cache {
 public:
  struct Table_info {
    std::string schema_name;
    std::string name;
    std::string engine;
  };

  const Table_info *get_table(const std::string &schema_table);

  void cache_tables(mysqlshdk::db::ISession *session);

 private:
  std::unordered_map<std::string, Table_info> tables_;
};

const std::string &get_translation(const char *item);

using Token_definitions =
    std::vector<std::pair<std::string_view, std::string_view>>;

std::string resolve_tokens(const std::string &text,
                           const Token_definitions &replacements);

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_COMMON_H_
