/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <array>
#include <sstream>
#include <utility>

#include "modules/util/upgrade_check.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
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

std::vector<std::unique_ptr<Upgrade_check>> Upgrade_check::create_checklist(
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

  std::vector<std::unique_ptr<Upgrade_check>> result;
  result.emplace_back(Sql_upgrade_check::get_reserved_keywords_check());
  result.emplace_back(Sql_upgrade_check::get_utf8mb3_check());
  // TODO(konrad): is it necessary?
  // result.emplace_back(Sql_upgrade_check::get_innodb_rowformat_check());
  result.emplace_back(Sql_upgrade_check::get_zerofill_check());
  result.emplace_back(new Check_table_command());
  result.emplace_back(Sql_upgrade_check::get_mysql_schema_check());
  result.emplace_back(Sql_upgrade_check::get_old_temporal_check());
  result.emplace_back(Sql_upgrade_check::get_foreign_key_length_check());
  result.emplace_back(Sql_upgrade_check::get_maxdb_sql_mode_flags_check());
  result.emplace_back(Sql_upgrade_check::get_obsolete_sql_mode_flags_check());
  result.emplace_back(
      Sql_upgrade_check::get_partitioned_tables_in_shared_tablespaces_check());
  result.emplace_back(Sql_upgrade_check::get_removed_functions_check());
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
    while ((row = result->fetch_one()) != nullptr) {
      Upgrade_issue issue = parse_row(row);
      if (!issue.empty())
        issues.emplace_back(std::move(issue));
    }
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
      "than 64 characters (use ALTER TABLE)."));
}

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_maxdb_sql_mode_flags_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of obsolete MAXDB sql_mode flag",
      {"select routine_schema, routine_name, concat(routine_type, ' uses "
       "obsolete MAXDB sql_mode') from information_schema.routines where "
       "find_in_set('MAXDB', sql_mode);",
       "select event_schema, event_name, 'EVENT uses obsolete MAXDB sql_mode' "
       "from information_schema.EVENTS where find_in_set('MAXDB', sql_mode);",
       "select trigger_schema, trigger_name, 'TRIGGER uses obsolete MAXDB "
       "sql_mode' from information_schema.TRIGGERS where find_in_set('MAXDB', "
       "sql_mode);"},
      Upgrade_issue::WARNING,
      "The following DB objects have the obsolete MAXDB option persisted for "
      "sql_mode, which will be cleared during upgrade to 8.0. It "
      "can potentially change the datatype DATETIME into TIMESTAMP if it is "
      "used inside object's definition, and this in turn can change the "
      "behavior in case of dates earlier than 1970 or later than 2037. If this "
      "is a concern, please redefine these objects so that they do not rely on "
      "the MAXDB flag before running the upgrade to 8.0."));
}

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_obsolete_sql_mode_flags_check() {
  const std::array<const char*, 9> modes = {{"DB2",
                                            "MSSQL",
                                            "MYSQL323",
                                            "MYSQL40",
                                            "NO_FIELD_OPTIONS",
                                            "NO_KEY_OPTIONS",
                                            "NO_TABLE_OPTIONS",
                                            "ORACLE",
                                            "POSTGRESQL"}};
  std::vector<std::string> queries;
  for (const char* mode : modes) {
    queries.emplace_back(shcore::str_format(
        "select routine_schema, routine_name, concat(routine_type, ' uses "
        "obsolete %s sql_mode') from information_schema.routines where "
        "find_in_set('%s', sql_mode);",
        mode, mode));
    queries.emplace_back(shcore::str_format(
        "select event_schema, event_name, 'EVENT uses obsolete %s sql_mode' "
        "from information_schema.EVENTS where find_in_set('%s', sql_mode);",
        mode, mode));
    queries.emplace_back(shcore::str_format(
        "select trigger_schema, trigger_name, 'TRIGGER uses obsolete %s "
        "sql_mode' from information_schema.TRIGGERS where find_in_set('%s', "
        "sql_mode);",
        mode, mode));
  }

  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of obsolete sql_mode flags", std::move(queries),
      Upgrade_issue::NOTICE,
      "The following DB objects have obsolete options persisted for sql_mode, "
      "which will be cleared during upgrade to 8.0."));
}

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_partitioned_tables_in_shared_tablespaces_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Sql_upgrade_check(
      "Usage of partitioned tables in shared tablespaces",
      {"SELECT TABLE_SCHEMA, TABLE_NAME, concat('Partition ', PARTITION_NAME, "
       "' is in shared tablespace ', TABLESPACE_NAME) as description FROM "
       "information_schema.PARTITIONS WHERE PARTITION_NAME IS NOT NULL AND "
       "(TABLESPACE_NAME IS NOT NULL AND "
       "TABLESPACE_NAME!='innodb_file_per_table');"},
      Upgrade_issue::ERROR,
      "The following tables have partitions in shared tablespaces. Before "
      "upgrading to 8.0 they need to be moved to file-per-table tablespace. "
      "You can do this by running query like 'ALTER TABLE table_name "
      "REORGANIZE PARTITION X INTO (PARTITION X VALUES LESS THAN (30) "
      "TABLESPACE=innodb_file_per_table);'"));
}

class Removed_functions_check : public Sql_upgrade_check {
 private:
  const std::array<std::pair<std::string, const char*>, 71> functions{
      {{"ENCODE", "AES_ENCRYPT and AES_DECRYPT"},
       {"DECODE", "AES_ENCRYPT and AES_DECRYPT"},
       {"ENCRYPT", "SHA2"},
       {"DES_ENCRYPT", "AES_ENCRYPT and AES_DECRYPT"},
       {"DES_DECRYPT", "AES_ENCRYPT and AES_DECRYPT"},
       {"AREA", "ST_AREA"},
       {"ASBINARY", "ST_ASBINARY"},
       {"ASTEXT", "ST_ASTEXT"},
       {"ASWKB", "ST_ASWKB"},
       {"ASWKT", "ST_ASWKT"},
       {"BUFFER", "ST_BUFFER"},
       {"CENTROID", "ST_CENTROID"},
       {"CONTAINS", "MBRCONTAINS"},
       {"CROSSES", "ST_CROSSES"},
       {"DIMENSION", "ST_DIMENSION"},
       {"DISJOINT", "MBRDISJOINT"},
       {"DISTANCE", "ST_DISTANCE"},
       {"ENDPOINT", "ST_ENDPOINT"},
       {"ENVELOPE", "ST_ENVELOPE"},
       {"EQUALS", "MBREQUALS"},
       {"EXTERIORRING", "ST_EXTERIORRING"},
       {"GEOMCOLLFROMTEXT", "ST_GEOMCOLLFROMTEXT"},
       {"GEOMCOLLFROMWKB", "ST_GEOMCOLLFROMWKB"},
       {"GEOMETRYCOLLECTIONFROMTEXT", "ST_GEOMETRYCOLLECTIONFROMTEXT"},
       {"GEOMETRYCOLLECTIONFROMWKB", "ST_GEOMETRYCOLLECTIONFROMWKB"},
       {"GEOMETRYFROMTEXT", "ST_GEOMETRYFROMTEXT"},
       {"GEOMETRYFROMWKB", "ST_GEOMETRYFROMWKB"},
       {"GEOMETRYN", "ST_GEOMETRYN"},
       {"GEOMETRYTYPE", "ST_GEOMETRYTYPE"},
       {"GEOMFROMTEXT", "ST_GEOMFROMTEXT"},
       {"GEOMFROMWKB", "ST_GEOMFROMWKB"},
       {"GLENGTH", "ST_LENGTH"},
       {"INTERIORRINGN", "ST_INTERIORRINGN"},
       {"INTERSECTS", "MBRINTERSECTS"},
       {"ISCLOSED", "ST_ISCLOSED"},
       {"ISEMPTY", "ST_ISEMPTY"},
       {"ISSIMPLE", "ST_ISSIMPLE"},
       {"LINEFROMTEXT", "ST_LINEFROMTEXT"},
       {"LINEFROMWKB", "ST_LINEFROMWKB"},
       {"LINESTRINGFROMTEXT", "ST_LINESTRINGFROMTEXT"},
       {"LINESTRINGFROMWKB", "ST_LINESTRINGFROMWKB"},
       {"MBREQUAL", "MBREQUALS"},
       {"MLINEFROMTEXT", "ST_MLINEFROMTEXT"},
       {"MLINEFROMWKB", "ST_MLINEFROMWKB"},
       {"MPOINTFROMTEXT", "ST_MPOINTFROMTEXT"},
       {"MPOINTFROMWKB", "ST_MPOINTFROMWKB"},
       {"MPOLYFROMTEXT", "ST_MPOLYFROMTEXT"},
       {"MPOLYFROMWKB", "ST_MPOLYFROMWKB"},
       {"MULTILINESTRINGFROMTEXT", "ST_MULTILINESTRINGFROMTEXT"},
       {"MULTILINESTRINGFROMWKB", "ST_MULTILINESTRINGFROMWKB"},
       {"MULTIPOINTFROMTEXT", "ST_MULTIPOINTFROMTEXT"},
       {"MULTIPOINTFROMWKB", "ST_MULTIPOINTFROMWKB"},
       {"MULTIPOLYGONFROMTEXT", "ST_MULTIPOLYGONFROMTEXT"},
       {"MULTIPOLYGONFROMWKB", "ST_MULTIPOLYGONFROMWKB"},
       {"NUMGEOMETRIES", "ST_NUMGEOMETRIES"},
       {"NUMINTERIORRINGS", "ST_NUMINTERIORRINGS"},
       {"NUMPOINTS", "ST_NUMPOINTS"},
       {"OVERLAPS", "MBROVERLAPS"},
       {"POINTFROMTEXT", "ST_POINTFROMTEXT"},
       {"POINTFROMWKB", "ST_POINTFROMWKB"},
       {"POINTN", "ST_POINTN"},
       {"POLYFROMTEXT", "ST_POLYFROMTEXT"},
       {"POLYFROMWKB", "ST_POLYFROMWKB"},
       {"POLYGONFROMTEXT", "ST_POLYGONFROMTEXT"},
       {"POLYGONFROMWKB", "ST_POLYGONFROMWKB"},
       {"SRID", "ST_SRID"},
       {"STARTPOINT", "ST_STARTPOINT"},
       {"TOUCHES", "ST_TOUCHES"},
       {"WITHIN", "MBRWITHIN"},
       {"X", "ST_X"},
       {"Y", "ST_Y"}}};

 public:
  Removed_functions_check()
      : Sql_upgrade_check(
            "Usage of removed functions",
            {"select routine_schema, routine_name, '', routine_type, "
             "UPPER(routine_definition) from information_schema.routines;",
             "select TABLE_SCHEMA,TABLE_NAME,COLUMN_NAME, 'COLUMN'"
             ", UPPER(GENERATION_EXPRESSION) from "
             "information_schema.columns where extra regexp 'generated';",
             "select TRIGGER_SCHEMA, TRIGGER_NAME, '', 'TRIGGER', "
             "UPPER(ACTION_STATEMENT) from information_schema.triggers;"},
            Upgrade_issue::ERROR,
            "Following DB objects make use of functions that have "
            "been removed in version 8.0. Please make sure to update them to "
            "use supported alternatives before upgrade.") {
  }

 protected:
  std::size_t find_function(const std::string& str, const std::string& function,
                            std::size_t it = 0) {
    std::size_t pos = 0;
    do {
      pos = str.find(function, it);
      if (pos == std::string::npos)
        return pos;
      if (pos != 0 && std::isalnum(str[pos - 1])) {
        it = pos + 1;
      } else {
        std::size_t after = pos + function.size();
        if (after < str.size()) {
          while (std::isspace(str[after]))
            ++after;
          if (str[after] != '(')
            it = after;
        }
      }
    } while (it > pos);

    return pos;
  }

  Upgrade_issue parse_row(const mysqlshdk::db::IRow* row) override {
    Upgrade_issue res;
    std::vector<const std::pair<std::string, const char*>*> flagged_functions;
    std::string definition = row->get_as_string(4);
    for (const auto& func : functions) {
      std::size_t pos = find_function(definition, func.first);

      std::size_t it = 0;
      while (pos != std::string::npos && it < pos) {
        switch (definition[it]) {
          case '\'':
            it = mysqlshdk::utils::span_quoted_string_sq(definition, it);
            break;
          case '"':
            it = mysqlshdk::utils::span_quoted_string_dq(definition, it);
            break;
          case '/':
            if (definition[it + 1] == '*')
              it = mysqlshdk::utils::span_cstyle_comment(definition, it);
            else
              ++it;
            break;
          case '#':
            ++it;
            while (it < definition.size() && definition[it] != '\n')
              ++it;
            break;
          case '-':
            ++it;
            if (it + 1 < definition.size() && definition[it] == '-' &&
                std::isspace(definition[it + 1])) {
              ++it;
              while (it < definition.size() && definition[it] != '\n')
                ++it;
            }
            break;
          default:
            ++it;
        }
        if (it > pos)
          pos = find_function(definition, func.first, it);
      }

      if (pos != std::string::npos)
        flagged_functions.push_back(&func);
    }

    if (flagged_functions.empty())
      return res;

    std::stringstream ss;
    ss << row->get_as_string(3) << " uses removed function";
    if (flagged_functions.size() > 1)
      ss << "s";
    for (std::size_t i = 0; i < flagged_functions.size(); ++i)
      ss << (i > 0 ? ", " : " ") << flagged_functions[i]->first
         << " (consider using " << flagged_functions[i]->second << " instead)";

    res.schema = row->get_as_string(0);
    res.table = row->get_as_string(1);
    res.column = row->get_as_string(2);
    res.description = ss.str();
    res.level = level;
    return res;
  }
};

std::unique_ptr<Sql_upgrade_check>
Sql_upgrade_check::get_removed_functions_check() {
  return std::unique_ptr<Sql_upgrade_check>(new Removed_functions_check());
}

Check_table_command::Check_table_command()
    : Upgrade_check("Issues reported by 'check table x for upgrade' command") {
}

std::vector<Upgrade_issue> Check_table_command::run(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  // Needed for warnings related to triggers
  session->execute("FLUSH TABLES;");

  std::vector<std::pair<std::string, std::string>> tables;
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
