/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_UPGRADE_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECK_H_

#include <forward_list>
#include <memory>
#include <string>
#include <vector>

namespace mysqlshdk {
namespace db {
class ISession;
class IRow;
}  // namespace db
}  // namespace mysqlshdk

namespace mysqlsh {

struct Upgrade_issue {
  enum Level { ERROR = 0, WARNING, NOTICE };
  static std::string level_to_string(const Upgrade_issue::Level level);

  std::string schema;
  std::string table;
  std::string column;
  std::string description;
  Level level = ERROR;

  bool empty() { return schema.empty(); }
};

std::string to_string(const Upgrade_issue &problem);

class Upgrade_check {
 public:
  static std::vector<std::unique_ptr<Upgrade_check>> create_checklist(
      const std::string &src_ver, const std::string &dst_ver);

  explicit Upgrade_check(const char *name) : name(name) {}
  virtual ~Upgrade_check() {}

  const char *get_name() const { return name; }

  virtual const char *get_long_advice() const { return nullptr; }

  virtual std::vector<Upgrade_issue> run(
      std::shared_ptr<mysqlshdk::db::ISession> session) = 0;

 protected:
  const char *name;
};

class Sql_upgrade_check : public Upgrade_check {
 public:
  static std::unique_ptr<Sql_upgrade_check> get_reserved_keywords_check();
  static std::unique_ptr<Sql_upgrade_check> get_utf8mb3_check();
  static std::unique_ptr<Sql_upgrade_check> get_innodb_rowformat_check();
  static std::unique_ptr<Sql_upgrade_check> get_zerofill_check();
  static std::unique_ptr<Sql_upgrade_check> get_mysql_schema_check();
  static std::unique_ptr<Sql_upgrade_check> get_old_temporal_check();
  static std::unique_ptr<Sql_upgrade_check> get_foreign_key_length_check();
  static std::unique_ptr<Sql_upgrade_check> get_maxdb_sql_mode_flags_check();
  static std::unique_ptr<Sql_upgrade_check> get_obsolete_sql_mode_flags_check();
  static std::unique_ptr<Sql_upgrade_check>
  get_partitioned_tables_in_shared_tablespaces_check();
  static std::unique_ptr<Sql_upgrade_check> get_removed_functions_check();

  Sql_upgrade_check(const char *name, std::vector<std::string> &&queries,
                    Upgrade_issue::Level level, const char *advice = "",
                    std::forward_list<std::string> &&set_up =
                        std::forward_list<std::string>(),
                    std::forward_list<std::string> &&clean_up =
                        std::forward_list<std::string>());

  std::vector<Upgrade_issue> run(
      std::shared_ptr<mysqlshdk::db::ISession> session) override;

  const char *get_long_advice() const override;

 protected:
  virtual Upgrade_issue parse_row(const mysqlshdk::db::IRow *row);
  std::vector<std::string> queries;
  std::forward_list<std::string> set_up;
  std::forward_list<std::string> clean_up;
  const Upgrade_issue::Level level;
  std::string advice;
};

class Check_table_command : public Upgrade_check {
 public:
  Check_table_command();

  std::vector<Upgrade_issue> run(
      std::shared_ptr<mysqlshdk::db::ISession> session) override;
};

} /* namespace mysqlsh */

#endif  // MODULES_UTIL_UPGRADE_CHECK_H_
