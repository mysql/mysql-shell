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

#include <sstream>
#include <utility>

#include "modules/util/upgrade_check.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {

std::string to_string(const Upgrade_issue& problem) {
  std::stringstream ss;
  ss << problem.schema;
  if (!problem.table.empty())
    ss << "." << problem.table;
  if (!problem.column.empty())
    ss << "." << problem.column;
  if (!problem.description.empty())
    ss << " - " << problem.description;
  return ss.str();
}

std::string Upgrade_issue::level_to_string(const Upgrade_issue::Level level) {
  switch (level) {
    case Upgrade_issue::ERROR:
      return "Error:";
    case Upgrade_issue::WARNING:
      return "Warning:";
    case Upgrade_issue::NOTICE:
      return "Notice:";
  }
  return "Notice:";
}

std::vector<std::unique_ptr<Upgrade_check> > Upgrade_check::create_checklist(
    const std::string& src_ver, const std::string& dst_ver) {
  using mysqlshdk::utils::Version;
  Version src_version(src_ver);
  Version dst_version(dst_ver);
  if (src_version < Version("5.7.0") || src_version >= Version("8.0"))
    throw std::invalid_argument(
        "Upgrades to MySQL 8.0 requires the target server to be on version "
        "5.7");
  if (dst_version < Version("8.0") || dst_version >= Version("8.1"))
    throw std::invalid_argument(
        "Only Upgrade to MySQL 8.0 from 5.7 is currently supported");

  std::vector<std::unique_ptr<Upgrade_check> > result;
  result.emplace_back(Sql_upgrade_check::get_reserved_keywords_check());
  result.emplace_back(Sql_upgrade_check::get_utf8mb3_check());
  // TODO(konrad): is it necessary?
  // result.emplace_back(Sql_upgrade_check::get_innodb_rowformat_check());
  result.emplace_back(Sql_upgrade_check::get_zerofill_check());
  result.emplace_back(new Check_table_command());
  result.emplace_back(Sql_upgrade_check::get_mysql_schema_check());
  result.emplace_back(Sql_upgrade_check::get_old_temporal_check());
  result.emplace_back(Sql_upgrade_check::get_foreign_key_length_check());
  return result;
}

Sql_upgrade_check::Sql_upgrade_check(
    const char* name, std::vector<std::string>&& queries,
    Upgrade_issue::Level level = Upgrade_issue::WARNING, const char* advice,
    std::forward_list<std::string>&& set_up,
    std::forward_list<std::string>&& clean_up)
    : Upgrade_check(name),
      queries(queries),
      set_up(set_up),
      clean_up(clean_up),
      level(level),
      advice(advice) {
  if (!this->advice.empty())
    this->advice = Upgrade_issue::level_to_string(level) + " " + this->advice;
}

std::vector<Upgrade_issue> Sql_upgrade_check::run(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  for (const auto& stm : set_up)
    session->execute(stm);

  std::vector<Upgrade_issue> issues;
  for (const auto& query : queries) {
    //    puts(query.c_str());
    auto result = session->query(query);
    const mysqlshdk::db::IRow* row = nullptr;
    while ((row = result->fetch_one()) != nullptr)
      issues.push_back(parse_row(row));
  }

  for (const auto& stm : clean_up)
    session->execute(stm);

  return issues;
}

const char* Sql_upgrade_check::get_long_advice() const {
  if (advice.empty())
    return nullptr;
  return advice.c_str();
}

Upgrade_issue Sql_upgrade_check::parse_row(const mysqlshdk::db::IRow* row) {
  Upgrade_issue problem;
  auto fields_count = row->num_fields();
  problem.schema = row->get_as_string(0);
  if (fields_count > 2)
    problem.table = row->get_as_string(1);
  if (fields_count > 3)
    problem.column = row->get_as_string(2);
  if (fields_count > 1)
    problem.description = row->get_as_string(3 - (4 - fields_count));
  problem.level = level;

  return problem;
}

std::unique_ptr<Sql_upgrade_check> Sql_upgrade_check::get_old_temporal_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of old temporal type",
      {"SELECT table_schema, table_name,column_name,column_type "
       "FROM information_schema.columns WHERE column_type LIKE "
       "'timestamp /* 5.5 binary format */';"},
      Upgrade_issue::ERROR,
      "Following table columns use a deprecated and no longer supported "
      "timestamp disk storage format. They must be converted to the new format "
      "before upgrading. It can by done by rebuilding the table using 'ALTER "
      "TABLE <table_name> FORCE' command",
      {"SET show_old_temporals = ON;"}, {"SET show_old_temporals = OFF;"}));
}

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_reserved_keywords_check() {
  std::string keywords =
      "('ADMIN', 'BUCKETS', 'CLONE', 'COMPONENT', 'CUBE', 'CUME_DIST', "
      "'DENSE_RANK', 'EXCEPT', 'EMPTY', 'EXCLUDE', 'FIRST_VALUE', 'FOLLOWING', "
      "'GROUPING', 'GROUPS', 'HISTOGRAM', 'INVISIBLE', 'LAG', 'LAST_VALUE', "
      "'LEAD', 'LOCKED', 'NOWAIT', 'NTH_VALUE', 'NTILE', 'NULLS', 'OF', "
      "'OTHERS', 'OVER', 'PERCENT_RANK', 'PERSIST', 'PERSIST_ONLY', "
      "'PRECEDING', 'RANK', 'RECURSIVE', 'REMOTE', 'RESPECT', 'ROLE', "
      "'ROW_NUMBER', 'SKIP', 'TIES', 'UNBOUNDED', 'VISIBLE', 'WINDOW');";
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of db objects with names conflicting with reserved keywords "
      "in 8.0",
      {"select SCHEMA_NAME, 'Schema name' as WARNING from "
       "INFORMATION_SCHEMA.SCHEMATA where SCHEMA_NAME in " +
           keywords,
       "SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name' as WARNING FROM "
       "INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE != 'VIEW' and "
       "TABLE_NAME "
       "in " +
           keywords,
       "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, COLUMN_TYPE, 'Column "
       "name' as WARNING FROM information_schema.columns WHERE "
       "TABLE_SCHEMA "
       "not in ('information_schema', 'performance_schema') and "
       "COLUMN_NAME "
       "in " +
           keywords,
       "SELECT TRIGGER_SCHEMA, TRIGGER_NAME, 'Trigger name' as WARNING "
       "FROM "
       "INFORMATION_SCHEMA.TRIGGERS WHERE TRIGGER_NAME in " +
           keywords,
       "SELECT TABLE_SCHEMA, TABLE_NAME, 'View name' as WARNING FROM "
       "INFORMATION_SCHEMA.VIEWS WHERE TABLE_NAME in " +
           keywords,
       "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, 'Routine name' as WARNING "
       "FROM "
       "INFORMATION_SCHEMA.ROUTINES WHERE ROUTINE_NAME in " +
           keywords,
       "SELECT EVENT_SCHEMA, EVENT_NAME, 'Event name' as WARNING FROM "
       "INFORMATION_SCHEMA.EVENTS WHERE EVENT_NAME in " +
           keywords},
      Upgrade_issue::WARNING,
      "The following objects have names that conflict with reserved keywords "
      "that are new to 8.0. Ensure queries sent by your applications use "
      "`quotes` when referring to them or they will result in errors."));
}

/// In this check we are only interested if any such table/database exists
std::unique_ptr<Sql_upgrade_check> Sql_upgrade_check::get_utf8mb3_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of utf8mb3 charset",
      {"select SCHEMA_NAME, concat(\"schema's default character set: \",  "
       "DEFAULT_CHARACTER_SET_NAME) from INFORMATION_SCHEMA.schemata where "
       "SCHEMA_NAME not in ('information_schema', 'performance_schema', 'sys') "
       "and DEFAULT_CHARACTER_SET_NAME in ('utf8', 'utf8mb3');",
       "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, concat(\"column's "
       "default character set: \",CHARACTER_SET_NAME) from "
       "information_schema.columns where CHARACTER_SET_NAME in ('utf8', "
       "'utf8mb3') and TABLE_SCHEMA not in ('sys', 'performance_schema', "
       "'information_schema', 'mysql');"},
      Upgrade_issue::WARNING,
      "The following objects use the utf8mb3 character set. It is recommended "
      "to convert them to use utf8mb4 instead, for improved Unicode "
      "support."));
}

std::unique_ptr<Sql_upgrade_check> Sql_upgrade_check::get_mysql_schema_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Table names in the mysql schema conflicting with new tables in 8.0",
      {"SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name used in mysql schema "
       "in "
       "8.0' as WARNING FROM INFORMATION_SCHEMA.TABLES WHERE "
       "LOWER(TABLE_SCHEMA) = 'mysql' and LOWER(TABLE_NAME) IN "
       "('catalogs', 'character_sets', 'collations', 'column_type_elements', "
       "'columns', "
       "'dd_properties', 'events', 'foreign_key_column_usage', "
       "'foreign_keys', 'index_column_usage', 'index_partitions', "
       "'index_stats', "
       "'indexes', 'parameter_type_elements', 'parameters', 'routines', "
       "'schemata', "
       "'st_spatial_reference_systems', 'table_partition_values', "
       "'table_partitions', 'table_stats', 'tables', 'tablespace_files', "
       "'tablespaces', 'triggers', 'view_routine_usage', "
       "'view_table_usage', 'component', 'default_roles', 'global_grants', "
       "'innodb_ddl_log', 'innodb_dynamic_metadata', 'password_history', "
       "'role_edges');"},
      Upgrade_issue::ERROR,
      "The following tables in mysql schema have names that will conflict with "
      "the ones introduced in 8.0 version. They must be renamed or removed "
      "before upgrading (use RENAME TABLE command). This may also entail "
      "changes to applications that use the affected tables."));
}

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_innodb_rowformat_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "InnoDB tables with non-default row format",
      {"select table_schema, table_name, row_format from "
       "information_schema.tables where engine = 'innodb' "
       "and row_format != 'Dynamic';"},
      Upgrade_issue::WARNING,
      "The following tables use a non-default InnoDB row format"));
}

std::unique_ptr<Sql_upgrade_check> Sql_upgrade_check::get_zerofill_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of use ZEROFILL/display length type attributes",
      {"select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, COLUMN_TYPE from "
       "information_schema.columns "
       "where TABLE_SCHEMA not in ('sys', 'performance_schema', "
       "'information_schema', 'mysql') and (COLUMN_TYPE REGEXP 'zerofill' or "
       "(COLUMN_TYPE like 'tinyint%' and COLUMN_TYPE not like 'tinyint(4)%' "
       "and COLUMN_TYPE not like 'tinyint(3) unsigned%') or"
       "(COLUMN_TYPE like 'smallint%' and COLUMN_TYPE not like "
       "'smallint(6)%' and COLUMN_TYPE not like 'smallint(5) unsigned%') or "
       "(COLUMN_TYPE like 'mediumint%' and COLUMN_TYPE not like "
       "'mediumint(9)%' and COLUMN_TYPE not like 'mediumint(8) "
       "unsigned%') or "
       "(COLUMN_TYPE like 'int%' and COLUMN_TYPE not like "
       "'int(11)%' and COLUMN_TYPE not like 'int(10) "
       "unsigned%') or "
       "(COLUMN_TYPE like 'bigint%' and COLUMN_TYPE not like "
       "'bigint(20)%'));"},
      Upgrade_issue::NOTICE,
      "The following table columns specify a ZEROFILL/display length "
      "attributes. "
      "Please be aware that they will be ignored in MySQL 8.0"));
}

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_foreign_key_length_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Foreign key constraint names longer than 64 characters",
      {"select table_schema, table_name, 'Foreign key longer than 64 "
       "characters' as description from information_schema.tables where "
       "table_name in (select left(substr(id,instr(id,'/')+1), "
       "instr(substr(id,instr(id,'/')+1),'_ibfk_')-1) from "
       "information_schema.innodb_sys_foreign where "
       "length(substr(id,instr(id,'/')+1))>64);"},
      Upgrade_issue::ERROR,
      "The following tables must be altered to have constraint names shorter "
      "than 64 characters (use ALTER TABLE). "));
}

Check_table_command::Check_table_command()
    : Upgrade_check("Issues reported by 'check table x for upgrade' command") {
}

std::vector<Upgrade_issue> Check_table_command::run(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  // Needed for warnings related to triggers
  session->execute("FLUSH TABLES;");

  std::vector<std::pair<std::string, std::string> > tables;
  auto result = session->query(
      "SELECT TABLE_SCHEMA, TABLE_NAME FROM "
      "INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA not in "
      "('information_schema', 'performance_schema')");
  const mysqlshdk::db::IRow* pair = nullptr;
  while ((pair = result->fetch_one()) != nullptr)
    tables.push_back(std::pair<std::string, std::string>(pair->get_string(0),
                                                         pair->get_string(1)));

  std::vector<Upgrade_issue> issues;
  for (const auto& pair : tables) {
    auto check_result =
        session->query(shcore::sqlstring("CHECK TABLE !.! FOR UPGRADE;", 0)
                       << pair.first << pair.second);
    const mysqlshdk::db::IRow* row = nullptr;
    while ((row = check_result->fetch_one()) != nullptr) {
      if (row->get_string(2) == "status")
        continue;
      Upgrade_issue issue;
      std::string type = row->get_string(2);
      if (type == "warning")
        issue.level = Upgrade_issue::WARNING;
      else if (type == "error")
        issue.level = Upgrade_issue::ERROR;
      else
        issue.level = Upgrade_issue::NOTICE;
      issue.schema = pair.first;
      issue.table = pair.second;
      issue.description = row->get_string(3);
      issues.push_back(issue);
    }
  }

  return issues;
}

} /* namespace mysqlsh */
