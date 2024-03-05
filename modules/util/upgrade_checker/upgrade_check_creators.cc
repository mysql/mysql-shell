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

#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include <mysqld_error.h>
#include <forward_list>
#include <regex>
#include <vector>

#include "modules/util/upgrade_checker/feature_life_cycle_check.h"
#include "modules/util/upgrade_checker/manual_check.h"
#include "modules/util/upgrade_checker/sql_upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/parser/mysql_parser_utils.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace upgrade_checker {

std::unique_ptr<Sql_upgrade_check> get_old_temporal_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_old_temporal_check,
      std::vector<std::string>{
          "SELECT table_schema, table_name,column_name,column_type "
          "FROM information_schema.columns WHERE column_type LIKE "
          "'%5.5 binary format%';"},
      Upgrade_issue::ERROR, nullptr,
      std::forward_list<std::string>{"SET show_old_temporals = ON;"},
      std::forward_list<std::string>{"SET show_old_temporals = OFF;"});
}

std::unique_ptr<Sql_upgrade_check> get_reserved_keywords_check(
    const Upgrade_info &info) {
  std::string keywords;
  const auto add_keywords = [&keywords, &info](const char *v, const char *kws) {
    Version kv(v);
    if (info.server_version < kv && info.target_version >= kv) {
      if (!keywords.empty()) keywords += ", ";
      keywords += kws;
    }
  };

  add_keywords("8.0.11",
               "'ADMIN', 'CUBE', 'CUME_DIST', 'DENSE_RANK', 'EMPTY', 'EXCEPT', "
               "'FIRST_VALUE', 'FUNCTION', 'GROUPING', 'GROUPS', 'JSON_TABLE', "
               "'LAG', 'LAST_VALUE', 'LEAD', 'NTH_VALUE', 'NTILE', 'OF', "
               "'OVER', 'PERCENT_RANK', 'PERSIST', 'PERSIST_ONLY', 'RANK', "
               "'RECURSIVE', 'ROW', 'ROWS', 'ROW_NUMBER', 'SYSTEM', 'WINDOW'");
  add_keywords("8.0.14", "'LATERAL'");

  add_keywords("8.0.17", "'ARRAY' ,'MEMBER'");
  add_keywords("8.0.31", "'FULL', 'INTERSECT'");

  keywords = "(" + keywords + ");";
  return std::make_unique<Sql_upgrade_check>(
      ids::k_reserved_keywords_check,
      std::vector<std::string>{
          "select SCHEMA_NAME, 'Schema name' as WARNING from "
          "INFORMATION_SCHEMA.SCHEMATA where <<schema_filter>> and SCHEMA_NAME "
          "in" +
              keywords,
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name' as WARNING FROM "
          "INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE != 'VIEW' and "
          "<<schema_and_table_filter>> and TABLE_NAME in " +
              keywords,
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, COLUMN_TYPE, 'Column "
          "name' as WARNING FROM information_schema.columns WHERE "
          "<<schema_and_table_filter>> and COLUMN_NAME in " +
              keywords,
          "SELECT TRIGGER_SCHEMA, TRIGGER_NAME, 'Trigger name' as WARNING "
          "FROM INFORMATION_SCHEMA.TRIGGERS WHERE "
          "<<schema_and_trigger_filter>> and TRIGGER_NAME in " +
              keywords,
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'View name' as WARNING FROM "
          "INFORMATION_SCHEMA.VIEWS WHERE <<schema_and_table_filter>> and "
          "TABLE_NAME in " +
              keywords,
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, 'Routine name' as WARNING "
          "FROM INFORMATION_SCHEMA.ROUTINES WHERE "
          "<<schema_and_routine_filter>> and ROUTINE_NAME in " +
              keywords,
          "SELECT EVENT_SCHEMA, EVENT_NAME, 'Event name' as WARNING FROM "
          "INFORMATION_SCHEMA.EVENTS WHERE <<schema_and_event_filter>> and "
          "EVENT_NAME in " +
              keywords},
      Upgrade_issue::WARNING);
}

class Routine_syntax_check : public Upgrade_check {
  // This check works through a simple syntax check to find whether reserved
  // keywords are used somewhere unexpected, without checking for specific
  // keywords

  // This checks:
  // routines, view definitions, event definitions and trigger definitions

 public:
  Routine_syntax_check() : Upgrade_check(ids::k_routine_syntax_check) {}

  bool is_runnable() const override { return true; }

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info & /*server_info*/, Checker_cache *cache) override {
    struct Check_info {
      std::string names_query;
      std::string show_query;
      int code_field;
    };
    // Don't need to check views because they get auto-quoted
    const auto &qh = cache->query_helper();

    Check_info object_info[] = {
        {"SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
         " FROM information_schema.routines WHERE ROUTINE_TYPE = 'PROCEDURE'"
         " AND " +
             qh.schema_and_routine_filter(),
         "SHOW CREATE PROCEDURE !.!", 2},
        {"SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
         " FROM information_schema.routines WHERE ROUTINE_TYPE = 'FUNCTION'"
         " AND " +
             qh.schema_and_routine_filter(),
         "SHOW CREATE FUNCTION !.!", 2},
        {"SELECT TRIGGER_SCHEMA, TRIGGER_NAME"
         " FROM information_schema.triggers"
         " WHERE " +
             qh.schema_and_trigger_filter(),
         "SHOW CREATE TRIGGER !.!", 2},
        {"SELECT EVENT_SCHEMA, EVENT_NAME"
         " FROM information_schema.events"
         " WHERE " +
             qh.schema_and_event_filter(),
         "SHOW CREATE EVENT !.!", 3}};

    std::vector<Upgrade_issue> issues;
    for (const auto &obj : object_info) {
      auto result = session->queryf(obj.names_query);

      // fetch all results because we need to query again in process_item()
      result->buffer();

      while (auto row = result->fetch_one()) {
        auto issue =
            process_item(row, obj.show_query, obj.code_field, session.get());
        if (!issue.description.empty()) issues.push_back(std::move(issue));
      }
    }
    return issues;
  }

 protected:
  std::string check_routine_syntax(mysqlshdk::db::ISession *session,
                                   const std::string &show_template,
                                   int show_sql_field,
                                   const std::string &schema,
                                   const std::string &name) {
    auto result = session->queryf(show_template, schema, name);
    if (auto row = result->fetch_one()) {
      std::string sql = row->get_as_string(show_sql_field);
      try {
        sql = "DELIMITER $$$\n" + sql + "$$$\n";

        mysqlshdk::parser::check_sql_syntax(sql);
      } catch (const mysqlshdk::parser::Sql_syntax_error &err) {
        return shcore::str_format("at line %i,%i: unexpected token '%s'",
                                  static_cast<int>(err.line() - 1),
                                  static_cast<int>(err.offset()),
                                  err.token_text().c_str());
      }
    } else {
      log_warning("Upgrade check query %s returned no rows for %s.%s",
                  show_template.c_str(), schema.c_str(), name.c_str());
    }

    return "";
  }

  Upgrade_issue process_item(const mysqlshdk::db::IRow *row,
                             const std::string &show_template,
                             int show_sql_field,
                             mysqlshdk::db::ISession *session) {
    Upgrade_issue issue;

    issue.schema = row->get_as_string(0);
    issue.table = row->get_as_string(1);
    issue.level = Upgrade_issue::ERROR;

    // we need to get routine definitions with the SHOW command
    // because INFORMATION_SCHEMA will eat up things like backslashes
    // Bug#34534696	unparseable code returned in
    // INFORMATION_SCHEMA.ROUTINES.ROUTINE_DEFINITION

    issue.description = check_routine_syntax(
        session, show_template, show_sql_field, issue.schema, issue.table);

    if (issue.description.empty()) return {};
    return issue;
  }
};

std::unique_ptr<Upgrade_check> get_routine_syntax_check() {
  return std::make_unique<Routine_syntax_check>();
}

/// In this check we are only interested if any such table/database exists
std::unique_ptr<Sql_upgrade_check> get_utf8mb3_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_utf8mb3_check,
      std::vector<std::string>{
          "select SCHEMA_NAME, concat('schema''s default character set: ',  "
          "DEFAULT_CHARACTER_SET_NAME) from INFORMATION_SCHEMA.schemata where "
          "<<schema_filter>> "
          "and DEFAULT_CHARACTER_SET_NAME in ('utf8', 'utf8mb3');",
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, concat('column''s "
          "default character set: ',CHARACTER_SET_NAME) from "
          "information_schema.columns where CHARACTER_SET_NAME in ('utf8', "
          "'utf8mb3') and <<schema_and_table_filter>>;"},
      Upgrade_issue::WARNING);
}

std::unique_ptr<Sql_upgrade_check> get_mysql_schema_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_mysql_schema_check,
      std::vector<std::string>{
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name used in mysql schema.' "
          "as WARNING FROM INFORMATION_SCHEMA.TABLES WHERE "
          "LOWER(TABLE_SCHEMA) = 'mysql' and LOWER(TABLE_NAME) IN "
          "('catalogs', 'character_sets', 'collations', 'column_type_elements',"
          " 'columns', 'dd_properties', 'events', 'foreign_key_column_usage', "
          "'foreign_keys', 'index_column_usage', 'index_partitions', "
          "'index_stats', 'indexes', 'parameter_type_elements', 'parameters', "
          "'routines', 'schemata', 'st_spatial_reference_systems', "
          "'table_partition_values', 'table_partitions', 'table_stats', "
          "'tables', 'tablespace_files', 'tablespaces', 'triggers', "
          "'view_routine_usage', 'view_table_usage', 'component', "
          "'default_roles', 'global_grants', 'innodb_ddl_log', "
          "'innodb_dynamic_metadata', 'password_history', 'role_edges');"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_innodb_rowformat_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_innodb_rowformat_check,
      std::vector<std::string>{
          "select table_schema, table_name, row_format from "
          "information_schema.tables where engine = 'innodb' "
          "and row_format != 'Dynamic';"},
      Upgrade_issue::WARNING);
}

std::unique_ptr<Sql_upgrade_check> get_zerofill_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_zerofill_check,
      std::vector<std::string>{
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, COLUMN_TYPE from "
          "information_schema.columns where TABLE_SCHEMA not in ('sys', "
          "'performance_schema', 'information_schema', 'mysql') and "
          "(COLUMN_TYPE REGEXP 'zerofill' or (COLUMN_TYPE like 'tinyint%' and "
          "COLUMN_TYPE not like 'tinyint(4)%' and COLUMN_TYPE not like "
          "'tinyint(3) unsigned%') or (COLUMN_TYPE like 'smallint%' and "
          "COLUMN_TYPE not like 'smallint(6)%' and COLUMN_TYPE not like "
          "'smallint(5) unsigned%') or (COLUMN_TYPE like 'mediumint%' and "
          "COLUMN_TYPE not like 'mediumint(9)%' and COLUMN_TYPE not like "
          "'mediumint(8) unsigned%') or (COLUMN_TYPE like 'int%' and "
          "COLUMN_TYPE not like 'int(11)%' and COLUMN_TYPE not like 'int(10) "
          "unsigned%') or (COLUMN_TYPE like 'bigint%' and COLUMN_TYPE not like "
          "'bigint(20)%'));"},
      Upgrade_issue::NOTICE);
}

// Zerofill is still available in 8.0.11
// namespace {
// bool UNUSED_VARIABLE(register_zerofill) =
// Upgrade_check_registry::register_check(
//    std::bind(&get_zerofill_check), "8.0.11");
//}

std::unique_ptr<Sql_upgrade_check> get_nonnative_partitioning_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_nonnative_partitioning_check,
      std::vector<std::string>{
          "select table_schema, table_name, concat(engine, ' engine does not "
          "support native partitioning') from information_schema.Tables where "
          "<<schema_and_table_filter>> and create_options like '%partitioned%' "
          "and upper(engine) not in ('INNODB', 'NDB', 'NDBCLUSTER');"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_foreign_key_length_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_foreign_key_length_check,
      std::vector<std::string>{
          "select table_schema, table_name, 'Foreign key longer than 64 "
          "characters' as description from information_schema.tables where "
          "<<schema_and_table_filter>> and table_name in (select "
          "left(substr(id,instr(id,'/')+1), "
          "instr(substr(id,instr(id,'/')+1),'_ibfk_')-1) from "
          "information_schema.innodb_sys_foreign where "
          "length(substr(id,instr(id,'/')+1))>64);"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_maxdb_sql_mode_flags_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_maxdb_sql_mode_flags_check,
      std::vector<std::string>{
          "select routine_schema, routine_name, concat(routine_type, ' uses "
          "obsolete MAXDB sql_mode') from information_schema.routines where "
          "<<schema_and_routine_filter>> and find_in_set('MAXDB', sql_mode);",
          "select event_schema, event_name, 'EVENT uses obsolete MAXDB "
          "sql_mode' from information_schema.EVENTS where "
          "<<schema_and_event_filter>> and find_in_set('MAXDB', sql_mode);",
          "select trigger_schema, trigger_name, 'TRIGGER uses obsolete MAXDB "
          "sql_mode' from information_schema.TRIGGERS where "
          "<<schema_and_trigger_filter>> and find_in_set('MAXDB', sql_mode);",
          "select concat('global system variable ', variable_name), 'defined "
          "using obsolete MAXDB option' as reason from "
          "performance_schema.global_variables where variable_name = "
          "'sql_mode' and find_in_set('MAXDB', variable_value);"},
      Upgrade_issue::WARNING);
}

std::unique_ptr<Sql_upgrade_check> get_obsolete_sql_mode_flags_check() {
  const std::array<const char *, 10> modes = {
      {"DB2", "MSSQL", "MYSQL323", "MYSQL40", "NO_AUTO_CREATE_USER",
       "NO_FIELD_OPTIONS", "NO_KEY_OPTIONS", "NO_TABLE_OPTIONS", "ORACLE",
       "POSTGRESQL"}};
  std::vector<std::string> queries;
  for (const char *mode : modes) {
    queries.emplace_back(shcore::str_format(
        "select routine_schema, routine_name, concat(routine_type, ' uses "
        "obsolete %s sql_mode') from information_schema.routines where "
        "<<schema_and_routine_filter>> and find_in_set('%s', sql_mode);",
        mode, mode));
    queries.emplace_back(shcore::str_format(
        "select event_schema, event_name, 'EVENT uses obsolete %s sql_mode' "
        "from information_schema.EVENTS where <<schema_and_event_filter>> and "
        "find_in_set('%s', sql_mode);",
        mode, mode));
    queries.emplace_back(shcore::str_format(
        "select trigger_schema, trigger_name, 'TRIGGER uses obsolete %s "
        "sql_mode' from information_schema.TRIGGERS where "
        "<<schema_and_trigger_filter>> and find_in_set('%s', "
        "sql_mode);",
        mode, mode));
    queries.emplace_back(shcore::str_format(
        "select concat('global system variable ', variable_name), 'defined "
        "using obsolete %s option' as reason from "
        "performance_schema.global_variables where variable_name = 'sql_mode' "
        "and find_in_set('%s', variable_value);",
        mode, mode));
  }

  return std::make_unique<Sql_upgrade_check>(
      ids::k_obsolete_sql_mode_flags_check, std::move(queries),
      Upgrade_issue::NOTICE);
}

class Enum_set_element_length_check : public Sql_upgrade_check {
 public:
  Enum_set_element_length_check()
      : Sql_upgrade_check(
            ids::k_enum_set_element_length_check,
            {"select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, UPPER(DATA_TYPE), "
             "COLUMN_TYPE, CHARACTER_MAXIMUM_LENGTH from "
             "information_schema.columns where <<schema_and_table_filter>> and "
             "data_type in ('enum','set') and "
             "CHARACTER_MAXIMUM_LENGTH > 255 and table_schema not in "
             "('information_schema');"},
            Upgrade_issue::ERROR) {}

  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    Upgrade_issue res;
    std::string type = row->get_as_string(3);
    if (type == "SET") {
      std::string definition{row->get_as_string(4)};
      std::size_t i = 0;
      for (i = 0; i < definition.length(); i++) {
        std::size_t prev = i + 1;
        if (definition[i] == '"')
          i = mysqlshdk::utils::span_quoted_string_dq(definition, i) - 1;
        else if (definition[i] == '\'')
          i = mysqlshdk::utils::span_quoted_string_sq(definition, i) - 1;
        else
          continue;
        if (i - prev > 255) break;
      }
      if (i == definition.length()) return res;
    }

    res.schema = row->get_as_string(0);
    res.table = row->get_as_string(1);
    res.column = row->get_as_string(2);
    res.description = type + " contains element longer than 255 characters";
    res.level = m_level;
    return res;
  }
};

std::unique_ptr<Sql_upgrade_check> get_enum_set_element_length_check() {
  return std::make_unique<Enum_set_element_length_check>();
}

class Check_table_command : public Upgrade_check {
 public:
  Check_table_command() : Upgrade_check(ids::k_table_command_check) {}

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override {
    // Needed for warnings related to triggers, incompatible types in 5.7
    if (server_info.server_version < Version(8, 0, 0)) {
      try {
        session->execute("FLUSH LOCAL TABLES;");
      } catch (const mysqlshdk::db::Error &error) {
        if (error.code() == ER_SPECIFIC_ACCESS_DENIED_ERROR) {
          throw Check_configuration_error(
              "To run this check the RELOAD grant is required.");
        } else {
          throw;
        }
      }
    }

    std::vector<std::pair<std::string, std::string>> tables;
    auto result = session->query(
        "SELECT TABLE_SCHEMA, TABLE_NAME FROM "
        "INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA not in "
        "('information_schema', 'performance_schema', 'sys')");
    {
      const mysqlshdk::db::IRow *pair = nullptr;
      while ((pair = result->fetch_one()) != nullptr) {
        tables.push_back(
            std::make_pair(pair->get_string(0), pair->get_string(1)));
      }
    }

    std::vector<Upgrade_issue> issues;
    for (const auto &pair : tables) {
      const auto query = shcore::sqlstring("CHECK TABLE !.! FOR UPGRADE;", 0)
                         << pair.first << pair.second;
      auto check_result = session->query(query.str_view());
      const mysqlshdk::db::IRow *row = nullptr;
      while ((row = check_result->fetch_one()) != nullptr) {
        if (row->get_string(2) == "status") continue;
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

        // Native partitioning warning has been promoted to error in context of
        // upgrade to 8.0 and is handled by the separate check
        if (issue.description.find("use native partitioning instead.") !=
                std::string::npos &&
            issue.level == Upgrade_issue::WARNING)
          continue;
        issues.push_back(issue);
      }
    }

    return issues;
  }
};

std::unique_ptr<Upgrade_check> get_table_command_check() {
  return std::make_unique<Check_table_command>();
}

std::unique_ptr<Sql_upgrade_check>
get_partitioned_tables_in_shared_tablespaces_check(const Upgrade_info &info) {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_partitioned_tables_in_shared_tablespaces_check,
      std::vector<std::string>{
          info.server_version < Version(8, 0, 0)
              ? "SELECT TABLE_SCHEMA, TABLE_NAME, "
                "concat('Partition ', PARTITION_NAME, "
                "' is in shared tablespace ', TABLESPACE_NAME) "
                "as description FROM "
                "information_schema.PARTITIONS WHERE "
                "PARTITION_NAME IS NOT NULL AND "
                "(TABLESPACE_NAME IS NOT NULL AND "
                "TABLESPACE_NAME!='innodb_file_per_table');"
              : "select SUBSTRING_INDEX(it.name, '/', 1), "
                "SUBSTRING_INDEX(SUBSTRING_INDEX(it.name, '/', -1), '#', 1), "
                "concat('Partition ', SUBSTRING_INDEX(SUBSTRING_INDEX(it.name, "
                "'/', -1), '#', -1), ' is in shared tablespce ', itb.name) "
                "from information_schema.INNODB_TABLES it, "
                "information_schema.INNODB_TABLESPACES itb where it.SPACE = "
                "itb.space and it.name like '%#P#%' and it.space_type != "
                "'Single';"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_circular_directory_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_circular_directory_check,
      std::vector<std::string>{
          "SELECT tablespace_name, concat('circular reference in datafile "
          "path: "
          R"(\'', file_name, '\'') FROM INFORMATION_SCHEMA.FILES where )"
          R"(file_type='TABLESPACE' and (file_name rlike '[^\\.]/\\.\\./' or )"
          R"(file_name rlike '[^\\.]\\\\\\.\\.\\\\');)"},
      Upgrade_issue::ERROR);
}

class Removed_functions_check : public Sql_upgrade_check {
 private:
  const std::unordered_map<std::string, const char *> functions{
      {"ENCODE", "AES_ENCRYPT and AES_DECRYPT"},
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
      {"PASSWORD", nullptr},
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
      {"Y", "ST_Y"}};

 public:
  Removed_functions_check()
      : Sql_upgrade_check(
            ids::k_removed_functions_check,
            {"select table_schema, table_name, '', 'VIEW', "
             "UPPER(view_definition) from information_schema.views where "
             "<<schema_and_table_filter>>",
             "select routine_schema, routine_name, '', routine_type, "
             "UPPER(routine_definition) from information_schema.routines where"
             " <<schema_and_routine_filter>>",
             "select TABLE_SCHEMA,TABLE_NAME,COLUMN_NAME, 'COLUMN'"
             ", UPPER(GENERATION_EXPRESSION) from "
             "information_schema.columns where extra regexp 'generated' and "
             "<<schema_and_table_filter>>",
             "select TRIGGER_SCHEMA, TRIGGER_NAME, '', 'TRIGGER', "
             "UPPER(ACTION_STATEMENT) from information_schema.triggers where "
             "<<schema_and_trigger_filter>>",
             "select event_schema, event_name, '', 'EVENT', "
             "UPPER(EVENT_DEFINITION) from information_schema.events where "
             "<<schema_and_event_filter>>"},
            Upgrade_issue::ERROR) {}

 protected:
  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    Upgrade_issue res;
    std::vector<std::pair<std::string, const char *>> flagged_functions;
    std::string definition = row->get_as_string(4);
    mysqlshdk::utils::SQL_iterator it(definition);
    std::string func;
    while (!(func = it.next_sql_function()).empty()) {
      auto i = functions.find(func);
      if (i != functions.end()) flagged_functions.emplace_back(*i);
    }

    if (flagged_functions.empty()) return res;
    std::stringstream ss;
    ss << row->get_as_string(3) << " uses removed function";
    if (flagged_functions.size() > 1) ss << "s";
    for (std::size_t i = 0; i < flagged_functions.size(); ++i) {
      ss << (i > 0 ? ", " : " ") << flagged_functions[i].first;
      if (flagged_functions[i].second != nullptr)
        ss << " (consider using " << flagged_functions[i].second << " instead)";
    }

    res.schema = row->get_as_string(0);
    res.table = row->get_as_string(1);
    res.column = row->get_as_string(2);
    res.description = ss.str();
    res.level = m_level;
    return res;
  }
};

std::unique_ptr<Sql_upgrade_check> get_removed_functions_check() {
  return std::make_unique<Removed_functions_check>();
}

class Groupby_asc_syntax_check : public Sql_upgrade_check {
 public:
  Groupby_asc_syntax_check()
      : Sql_upgrade_check(
            ids::k_groupby_asc_syntax_check,
            {"select table_schema, table_name, 'VIEW', "
             "UPPER(view_definition) from information_schema.views where "
             "<<schema_and_table_filter>> and "
             "(UPPER(view_definition) like '%ASC%' or UPPER(view_definition) "
             "like '%DESC%');",
             "select routine_schema, routine_name, routine_type, "
             "UPPER(routine_definition) from information_schema.routines where "
             "<<schema_and_routine_filter>> and "
             "(UPPER(routine_definition) like '%ASC%' or "
             "UPPER(routine_definition) like '%DESC%');",
             "select TRIGGER_SCHEMA, TRIGGER_NAME, 'TRIGGER', "
             "UPPER(ACTION_STATEMENT) from information_schema.triggers where "
             "<<schema_and_trigger_filter>> and "
             "(UPPER(ACTION_STATEMENT) like '%ASC%' or UPPER(ACTION_STATEMENT) "
             "like '%DESC%');",
             "select event_schema, event_name, 'EVENT', "
             "UPPER(EVENT_DEFINITION) from information_schema.events where "
             "<<schema_and_event_filter>> and "
             "(UPPER(event_definition) like '%ASC%' or UPPER(event_definition) "
             "like '%DESC%');"},
            Upgrade_issue::ERROR) {}

  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    Upgrade_issue res;
    std::string definition = row->get_as_string(3);
    mysqlshdk::utils::SQL_iterator it(definition);
    bool gb_found = false;
    std::string token;

    while (!(token = it.next_token()).empty()) {
      if (token == "GROUP") {
        auto pos = it.position();
        if (it.next_token() == "BY")
          gb_found = true;
        else
          it.set_position(pos);
      } else if (token.back() == ';') {
        gb_found = false;
      } else if (token == "ORDER") {
        auto pos = it.position();
        if (it.next_token() == "BY")
          gb_found = false;
        else
          it.set_position(pos);
      } else if (gb_found && token == "ASC") {
        res.description =
            row->get_as_string(2) + " uses removed GROUP BY ASC syntax";
        break;
      } else if (gb_found && token == "DESC") {
        res.description =
            row->get_as_string(2) + " uses removed GROUP BY DESC syntax";
        break;
      }
    }

    if (!res.description.empty()) {
      res.schema = row->get_as_string(0);
      res.table = row->get_as_string(1);
      res.level = m_level;
    }

    return res;
  }
};

std::unique_ptr<Sql_upgrade_check> get_groupby_asc_syntax_check() {
  return std::make_unique<Groupby_asc_syntax_check>();
}

std::unique_ptr<Upgrade_check> get_removed_sys_log_vars_check(
    const Upgrade_info &info) {
  auto check = std::make_unique<Removed_sys_var_check>(
      ids::k_removed_sys_log_vars_check, info);

  check->add_sys_var(Version(8, 0, 13),
                     {{"log_syslog_facility", "syseventlog.facility"},
                      {"log_syslog_include_pid", "syseventlog.include_pid"},
                      {"log_syslog_tag", "syseventlog.tag"},
                      {"log_syslog", nullptr}});

  return check;
}

std::unique_ptr<Removed_sys_var_check> get_removed_sys_vars_check(
    const Upgrade_info &info) {
  auto check = std::make_unique<Removed_sys_var_check>(
      ids::k_removed_sys_vars_check, info);

  check->add_sys_var(
      Version(8, 0, 11),
      {{"date_format", nullptr},
       {"datetime_format", nullptr},
       {"group_replication_allow_local_disjoint_gtids_join", nullptr},
       {"have_crypt", nullptr},
       {"ignore_builtin_innodb", nullptr},
       {"ignore_db_dirs", nullptr},
       {"innodb_checksums", "innodb_checksum_algorithm"},
       {"innodb_disable_resize_buffer_pool_debug", nullptr},
       {"innodb_file_format", nullptr},
       {"innodb_file_format_check", nullptr},
       {"innodb_file_format_max", nullptr},
       {"innodb_large_prefix", nullptr},
       {"innodb_locks_unsafe_for_binlog", nullptr},
       {"innodb_stats_sample_pages", "innodb_stats_transient_sample_pages"},
       {"innodb_support_xa", nullptr},
       {"innodb_undo_logs", "innodb_rollback_segments"},
       {"log_warnings", "log_error_verbosity"},
       {"log_builtin_as_identified_by_password", nullptr},
       {"log_error_filter_rules", nullptr},
       {"max_tmp_tables", nullptr},
       {"multi_range_count", nullptr},
       {"old_passwords", nullptr},
       {"query_cache_limit", nullptr},
       {"query_cache_min_res_unit", nullptr},
       {"query_cache_size", nullptr},
       {"query_cache_type", nullptr},
       {"query_cache_wlock_invalidate", nullptr},
       {"secure_auth", nullptr},
       {"show_compatibility_56", nullptr},
       {"sync_frm", nullptr},
       {"time_format", nullptr},
       {"tx_isolation", "transaction_isolation"},
       {"tx_read_only", "transaction_read_only"}});

  check->add_sys_var(Version(8, 0, 13),
                     {{"metadata_locks_cache_size", nullptr},
                      {"metadata_locks_hash_instances", nullptr}});

  check->add_sys_var(Version(8, 0, 16),
                     {{"internal_tmp_disk_storage_engine", nullptr}});

  check->add_sys_var(Version(8, 2, 0), {{"expire_logs_days", nullptr}});

  check->add_sys_var(Version(8, 3, 0),
                     {{"slave_rows_search_algorithms", nullptr},
                      {"log_bin_use_v1_row_events", nullptr},
                      {"relay_log_info_file", nullptr},
                      {"master_info_file", nullptr},
                      {"transaction_write_set_extraction", nullptr},
                      {"relay_log_info_repository", nullptr},
                      {"master_info_repository", nullptr},
                      {"group_replication_ip_whitelist", nullptr}});

  check->add_sys_var(
      Version(8, 4, 0),
      {
          {"default_authentication_plugin", "authentication_policy"},
          {"sha256_password_auto_generate_rsa_keys", nullptr},
          {"sha256_password_private_key_path", nullptr},
          {"sha256_password_proxy_users", nullptr},
          {"sha256_password_public_key_path", nullptr},
          {"mysql_native_password_proxy_users", nullptr},
          {"old", nullptr},
          {"new", nullptr},
          {"binlog_transaction_dependency_tracking", nullptr},
          {"group_replication_recovery_complete_at", nullptr},
          {"profiling", nullptr},
          {"profiling_history_size", nullptr},
          {"avoid_temporal_upgrade", nullptr},
          {"show_old_temporals", nullptr},
      });

  return check;
}

std::unique_ptr<Upgrade_check> get_sys_vars_new_defaults_check(
    const Upgrade_info &server_info) {
  return std::make_unique<Sysvar_new_defaults>(server_info);
}

std::unique_ptr<Sql_upgrade_check> get_zero_dates_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_zero_dates_check,
      std::vector<std::string>{
          "select 'global.sql_mode', 'does not contain either NO_ZERO_DATE "
          "or "
          "NO_ZERO_IN_DATE which allows insertion of zero dates' from "
          "(SELECT "
          "@@global.sql_mode like '%NO_ZERO_IN_DATE%' and @@global.sql_mode "
          "like '%NO_ZERO_DATE%' as zeroes_enabled) as q where "
          "q.zeroes_enabled = 0;",
          "select 'session.sql_mode', concat(' of ', q.thread_count, ' "
          "session(s) does not contain either NO_ZERO_DATE or "
          "NO_ZERO_IN_DATE "
          "which allows insertion of zero dates') FROM (select "
          "count(thread_id) as thread_count from "
          "performance_schema.variables_by_thread WHERE variable_name = "
          "'sql_mode' and (variable_value not like '%NO_ZERO_IN_DATE%' or "
          "variable_value not like '%NO_ZERO_DATE%')) as q where "
          "q.thread_count > 0;",
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, concat('column has "
          "zero default value: ', COLUMN_DEFAULT) from "
          "information_schema.columns where <<schema_and_table_filter>> and "
          "DATA_TYPE in ('timestamp', 'datetime', 'date') and COLUMN_DEFAULT "
          "like '0000-00-00%';"},
      Upgrade_issue::WARNING);
}

#define replace_in_SQL(string)                                                 \
  "replace(replace(replace(replace(replace(replace(replace(replace(replace("   \
  "replace(replace(replace(" string                                            \
  ", '@@@', ''), '@002d', '-'), '@003a', ':'), '@002e', '.'), '@0024', '$'), " \
  "'@0021', '!'), '@003f', '?'), '@0025', '%'), '@0023', '#'), '@0026', "      \
  "'&'), '@002a', '*'), '@0040', '@') "

// clang-format off
std::unique_ptr<Sql_upgrade_check>
get_schema_inconsistency_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_schema_inconsistency_check,
      std::vector<std::string>{
       "select A.schema_name, A.table_name, 'present in INFORMATION_SCHEMA''s "
       "INNODB_SYS_TABLES table but missing from TABLES table' from (select "
       "distinct "
       replace_in_SQL("substring_index(NAME, '/',1)")
       " as schema_name, "
       replace_in_SQL("substring_index(substring_index(NAME, '/',-1),if(locate(BINARY '#P#', substring_index(NAME, '/',-1)), '#P#','#p#'),1)")
       " as table_name from "
       "information_schema.innodb_sys_tables where NAME like '%/%') A left "
       "join information_schema.tables I on A.table_name = I.table_name and "
       "A.schema_name = I.table_schema where A.table_name not like 'FTS_0%' "
       "and (I.table_name IS NULL or I.table_schema IS NULL) and A.table_name "
       "not REGEXP '@[0-9]' and A.schema_name not REGEXP '@[0-9]';"},
      Upgrade_issue::ERROR);
}
// clang-format on

std::unique_ptr<Sql_upgrade_check> get_fts_in_tablename_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_fts_in_tablename_check,
      std::vector<std::string>{
          "select table_schema, table_name , 'table name contains ''FTS''' "
          "from information_schema.tables where <<schema_and_table_filter>> "
          "and table_name like binary '%FTS%';"},
      Upgrade_issue::ERROR);
}

// clang-format off
std::unique_ptr<Sql_upgrade_check> get_engine_mixup_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_engine_mixup_check,
      std::vector<std::string>{
          "select a.table_schema, a.table_name, concat('recognized by the "
          "InnoDB engine but belongs to ', a.engine) from "
          "information_schema.tables a join (select "
          replace_in_SQL("substring_index(NAME, '/',1)")
          " as table_schema, "
          replace_in_SQL("substring_index(substring_index(NAME, '/',-1),'#',1)")
          " as table_name from "
          "information_schema.innodb_sys_tables where NAME like '%/%') b on "
          "a.table_schema = b.table_schema and a.table_name = b.table_name "
          "where <<a.schema_and_table_filter>> and a.engine != 'Innodb';"},
      Upgrade_issue::ERROR);
}
// clang-format on

std::unique_ptr<Sql_upgrade_check> get_old_geometry_types_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_old_geometry_types_check,
      std::vector<std::string>{
          R"(select t.table_schema, t.table_name, c.column_name,
              concat(c.data_type, " column") as 'advice'
              from information_schema.innodb_sys_tables as st,
              information_schema.innodb_sys_columns as sc,
              information_schema.tables as t,
              information_schema.columns as c
              where sc.mtype=5 and
                sc.table_id = st.table_id and
                st.name =  concat(t.table_schema, '/', t.table_name) and
                c.table_schema = t.table_schema and
                c.table_name = t.table_name and
                c.column_name = sc.name and
                c.data_type in ('point', 'geometry', 'polygon', 'linestring',
                  'multipoint', 'multilinestring', 'multipolygon',
                  'geometrycollection');)"},
      Upgrade_issue::ERROR);
}

namespace {
class Changed_functions_in_generated_columns_check : public Sql_upgrade_check {
 private:
  const std::unordered_set<std::string> functions{"CAST", "CONVERT"};

 public:
  Changed_functions_in_generated_columns_check()
      : Sql_upgrade_check(ids::k_changed_functions_generated_columns_check,
                          {"SELECT s.table_schema, s.table_name, s.column_name,"
                           "    UPPER(c.generation_expression)"
                           "  FROM information_schema.columns c"
                           "  JOIN information_schema.statistics s"
                           "    ON c.table_schema = s.table_schema"
                           "    AND c.table_name = s.table_name"
                           "    AND c.column_name = s.column_name"
                           "  WHERE <<s.schema_and_table_filter>>"
                           "    AND c.generation_expression <> ''"},
                          Upgrade_issue::WARNING) {}

 protected:
  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    Upgrade_issue res;
    bool match = false;
    std::string definition = row->get_as_string(3);
    mysqlshdk::utils::SQL_iterator it(definition);
    std::string func;
    while (!(func = it.next_sql_function()).empty()) {
      if (functions.find(func) != functions.end()) {
        match = true;
        break;
      }
    }

    if (!match) return res;

    res.schema = row->get_as_string(0);
    res.table = row->get_as_string(1);
    res.column = row->get_as_string(2);
    res.description = definition;
    res.level = m_level;
    return res;
  }
};

}  // namespace

std::unique_ptr<Sql_upgrade_check>
get_changed_functions_generated_columns_check() {
  return std::make_unique<Changed_functions_in_generated_columns_check>();
}

// this check is applicable to versions up to 8.0.12, starting with 8.0.13
// these types can have default values, specified as an expression
class Columns_which_cannot_have_defaults_check : public Sql_upgrade_check {
 public:
  Columns_which_cannot_have_defaults_check()
      : Sql_upgrade_check(
            ids::k_columns_which_cannot_have_defaults_check,
            {"SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, DATA_TYPE FROM "
             "INFORMATION_SCHEMA.COLUMNS WHERE <<schema_and_table_filter>> AND "
             "COLUMN_DEFAULT IS NOT NULL AND LOWER(DATA_TYPE) IN ('point', "
             "'linestring', 'polygon', 'geometry', 'multipoint', "
             "'multilinestring', 'multipolygon', 'geometrycollection', "
             "'geomcollection', 'json', 'tinyblob', 'blob', 'mediumblob', "
             "'longblob', 'tinytext', 'text', 'mediumtext', 'longtext')"},
            Upgrade_issue::ERROR) {}
};

std::unique_ptr<Sql_upgrade_check>
get_columns_which_cannot_have_defaults_check() {
  return std::make_unique<Columns_which_cannot_have_defaults_check>();
}

std::unique_ptr<Sql_upgrade_check> get_invalid_57_names_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_invalid_57_names_check,
      std::vector<std::string>{
          "SELECT SCHEMA_NAME, 'Schema name' AS WARNING FROM "
          "INFORMATION_SCHEMA.SCHEMATA WHERE <<schema_filter>> AND SCHEMA_NAME "
          "LIKE '#mysql50#%';",
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name' AS WARNING FROM "
          "INFORMATION_SCHEMA.TABLES WHERE <<schema_and_table_filter>> AND "
          "TABLE_NAME LIKE '#mysql50#%';"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_orphaned_routines_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_orphaned_routines_check,
      std::vector<std::string>{
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, 'is orphaned' AS WARNING "
          "FROM information_schema.routines WHERE "
          "<<schema_and_routine_filter>> AND NOT EXISTS "
          "(SELECT SCHEMA_NAME FROM information_schema.schemata "
          "WHERE ROUTINE_SCHEMA=SCHEMA_NAME);"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_dollar_sign_name_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_dollar_sign_name_check,
      std::vector<std::string>{
          "SELECT SCHEMA_NAME, 'name starts with a $ sign.' FROM "
          "information_schema.schemata WHERE SCHEMA_NAME LIKE '$_%' AND "
          "SCHEMA_NAME NOT LIKE '$_%$' AND <<schema_filter>>",
          "SELECT TABLE_SCHEMA, TABLE_NAME, ' name starts with $ sign.' FROM "
          "information_schema.tables WHERE <<schema_and_table_filter>> AND "
          "TABLE_NAME LIKE '$_%' AND TABLE_NAME NOT LIKE '$_%$';",
          "SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, ' name starts with $ "
          "sign.' FROM information_schema.columns WHERE "
          "<<schema_and_table_filter>> AND "
          "COLUMN_NAME LIKE ('$_%') AND COLUMN_NAME NOT LIKE ('$%_$') ORDER BY "
          "TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME;",
          "SELECT TABLE_SCHEMA, TABLE_NAME, INDEX_NAME, ' starts with $ sign.' "
          "FROM information_schema.statistics WHERE "
          "<<schema_and_table_filter>> AND "
          "INDEX_NAME LIKE '$_%' AND INDEX_NAME NOT LIKE '$%_$' ORDER BY "
          "TABLE_SCHEMA, TABLE_NAME, INDEX_NAME;",
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, ' name starts with $ sign.' "
          "FROM information_schema.routines WHERE "
          "<<schema_and_routine_filter>> AND "
          "ROUTINE_NAME LIKE '$_%' AND ROUTINE_NAME NOT LIKE '$%_$';",
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, ' body contains identifiers "
          "that start with $ sign.' FROM information_schema.routines WHERE "
          "<<schema_and_routine_filter>> AND ROUTINE_DEFINITION REGEXP "
          "'[[:blank:],.;\\\\)\\\\(]\\\\$[A-Za-z0-9\\\\_\\\\$]*[A-Za-z0-9\\\\_]"
          "([[:blank:],;\\\\)]|(\\\\.[^0-9]))';"});
}

std::unique_ptr<Sql_upgrade_check> get_index_too_large_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_index_too_large_check,
      std::vector<std::string>{
          "select s.table_schema, s.table_name, s.index_name, 'index too "
          "large.' as warning from information_schema.statistics s, "
          "information_schema.columns c, information_schema.innodb_sys_tables "
          "i where <<s.schema_and_table_filter>> and s.table_name=c.table_name "
          "and s.table_schema=c.table_schema "
          "and c.column_name=s.column_name and concat(s.table_schema, '/', "
          "s.table_name) = i.name and i.row_format in ('Redundant', 'Compact') "
          "and (s.sub_part is null or s.sub_part > 255) and "
          "c.character_octet_length > 767;"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_empty_dot_table_syntax_check() {
  using namespace std::string_literals;

  // regex: match any thats start with a blank space,
  //        followed by a dot, followed by a string
  //        of unicode characters, range 0001-007F (ANSI)
  //        plus 0080-FFFF for supported extended chars,
  //        this should include special chars ,'"_$;
  auto regex = "[[:blank:]]\\\\.[\\\\x0001-\\\\xFFFF]+"s;

  return std::make_unique<Sql_upgrade_check>(
      ids::k_empty_dot_table_syntax_check,
      std::vector<std::string>{
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, ' routine body contains "
          "deprecated identifiers.' FROM information_schema.routines WHERE "
          "<<schema_and_routine_filter>> AND ROUTINE_DEFINITION REGEXP '" +
              regex + "';",
          "SELECT EVENT_SCHEMA, EVENT_NAME, ' event body contains deprecated "
          "identifiers.' FROM information_schema.events WHERE "
          "<<schema_and_event_filter>> AND EVENT_DEFINITION REGEXP '" +
              regex + "';",
          "SELECT TRIGGER_SCHEMA, TRIGGER_NAME, ' trigger body contains "
          "deprecated identifiers.' FROM information_schema.triggers WHERE "
          "<<schema_and_trigger_filter>> AND ACTION_STATEMENT REGEXP '" +
              regex + "';"},
      Upgrade_issue::ERROR);
}

namespace deprecated_auth_funcs {
constexpr std::array<std::string_view, 3> k_auth_list = {
    "sha256_password", "mysql_native_password", "authentication_fido"};

std::string get_auth_list() {
  return shcore::str_join(k_auth_list, ", ", [](const auto &item) {
    return shcore::sqlformat("?", item);
  });
}

std::string get_auth_or_like_list(const std::string &value_name) {
  return shcore::str_join(k_auth_list, " OR ", [&](const auto &item) {
    return value_name + " LIKE '%" + std::string(item) + "%'";
  });
}

bool is_auth_method(const std::string &auth) {
  return std::find(k_auth_list.begin(), k_auth_list.end(), auth) !=
         k_auth_list.end();
}

void setup_fido(const Version &target_version, Upgrade_issue *problem) {
  if (target_version < Version(8, 0, 27)) {
    problem->description =
        "authentication_fido was introduced in 8.0.27 release and is not "
        "supported in lower versions - this must be corrected.";
    problem->level = Upgrade_issue::ERROR;
  } else if (target_version < Version(8, 2, 0)) {
    problem->description =
        "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
        "in a future release. Consider using authentication_webauthn instead.";
    problem->level = Upgrade_issue::NOTICE;
  } else if (target_version < Version(8, 4, 0)) {
    problem->description =
        "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
        "in a future release. Use authentication_webauthn instead.";
    problem->level = Upgrade_issue::WARNING;
  } else {
    problem->description =
        "authentication_fido was replaced by authentication_webauthn as of "
        "8.4.0 release - this must be corrected.";
    problem->level = Upgrade_issue::ERROR;
  }
}

void setup_other(const std::string &plugin, const Version &target_version,
                 Upgrade_issue *problem) {
  if (target_version < Version(8, 4, 0)) {
    problem->description =
        plugin +
        " authentication method is deprecated and it should be considered to "
        "correct this before upgrading to 8.4.0 release.";
    problem->level = Upgrade_issue::WARNING;
  } else {
    problem->description =
        plugin +
        " authentication method was removed and it must be corrected before "
        "upgrading to 8.4.0 release.";
    problem->level = Upgrade_issue::ERROR;
  }
}

Upgrade_issue parse_fido(const std::string &item,
                         const Version &target_version) {
  Upgrade_issue problem;
  problem.schema = item;

  setup_fido(target_version, &problem);

  return problem;
}

Upgrade_issue parse_other(const std::string &item, const std::string &plugin,
                          const Version &target_version) {
  Upgrade_issue problem;
  problem.schema = item;

  setup_other(plugin, target_version, &problem);

  return problem;
}

Upgrade_issue parse_item(const std::string &item, const std::string &auth,
                         const Version &target_version) {
  if (auth == "authentication_fido") {
    return parse_fido(item, target_version);
  } else {
    return parse_other(item, auth, target_version);
  }
}
}  // namespace deprecated_auth_funcs

std::unique_ptr<Upgrade_check> get_auth_method_usage_check(
    const Upgrade_info &info) {
  return std::make_unique<Auth_method_usage_check>(info);
}

std::unique_ptr<Upgrade_check> get_plugin_usage_check(
    const Upgrade_info &info) {
  return std::make_unique<Plugin_usage_check>(info);
}

class Deprecated_default_auth_check : public Sql_upgrade_check {
 public:
  Deprecated_default_auth_check(mysqlshdk::utils::Version target_ver)
      : Sql_upgrade_check(
            ids::k_deprecated_default_auth_check,
            std::vector<std::string>{
                "show variables where variable_name = "
                "'default_authentication_plugin' AND value IN (" +
                    deprecated_auth_funcs::get_auth_list() + ");",
                "show variables where variable_name = 'authentication_policy' "
                "AND (" +
                    deprecated_auth_funcs::get_auth_or_like_list("value") +
                    ");"},
            Upgrade_issue::WARNING),
        m_target_version(target_ver) {}

  ~Deprecated_default_auth_check() override = default;

  void parse_var(const std::string &item, const std::string &auth,
                 std::vector<Upgrade_issue> *issues) {
    auto list = shcore::split_string(auth, ",", true);
    if (list.empty()) return;

    for (auto &list_item : list) {
      auto value = shcore::str_strip(list_item);
      if (!deprecated_auth_funcs::is_auth_method(value)) continue;

      auto issue =
          deprecated_auth_funcs::parse_item(item, value, m_target_version);
      if (!issue.empty()) issues->emplace_back(std::move(issue));
    }
  }

  bool is_multi_lvl_check() const override { return true; }

 protected:
  void add_issue(const mysqlshdk::db::IRow *row,
                 std::vector<Upgrade_issue> *issues) override {
    auto item = row->get_as_string(0);
    auto auth = row->get_as_string(1);

    parse_var(item, auth, issues);
  }

 private:
  const mysqlshdk::utils::Version m_target_version;
};

std::unique_ptr<Sql_upgrade_check> get_deprecated_default_auth_check(
    const Upgrade_info &info) {
  return std::make_unique<Deprecated_default_auth_check>(info.target_version);
}

std::unique_ptr<Sql_upgrade_check> get_deprecated_router_auth_method_check(
    const Upgrade_info &info) {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_deprecated_router_auth_method_check,
      std::vector<std::string>{
          "SELECT CONCAT(user, '@', host), ' - router user "
          "with deprecated authentication method.' FROM "
          "mysql.user WHERE plugin IN (" +
          deprecated_auth_funcs::get_auth_list() +
          ") AND user LIKE 'mysql_router%';"},
      info.target_version >= Version(8, 4, 0) ? Upgrade_issue::ERROR
                                              : Upgrade_issue::WARNING);
}

namespace deprecated_delimiter_funcs {
const std::string k_date_regex_str = R"(\'[0-9]{4}\-[0-9]{2}\-[0-9]{2}\')";
const std::string k_time_regex_str =
    R"(\'[0-9]{4}\-[0-9]{2}\-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]*)?\')";

// to match 'date','date','date' instances
const std::string k_date_regex_multi_str =
    "^" + k_date_regex_str + "(," + k_date_regex_str + ")*$";
const std::string k_time_regex_multi_str =
    "^" + k_time_regex_str + "(," + k_time_regex_str + ")*$";

std::vector<std::string> split_string_custom(
    std::string_view input, const char quote,
    std::function<size_t(std::string_view, size_t)> span_func) {
  std::vector<std::string> ret_val;

  constexpr std::string_view separator = ",";
  const auto to_find = std::string(separator) + quote;

  size_t index = 0, new_find = 0;

  while (new_find != std::string_view::npos) {
    new_find = input.find_first_of(to_find, index);

    while (new_find != std::string_view::npos && input[new_find] == quote) {
      // got quote, find its end
      auto after_quote_pos = span_func(input, new_find);
      if (after_quote_pos == std::string::npos) {
        // invalid quote, fallback, try find separator
        new_find = input.find(separator, index);
      } else {
        // quote spanned, find next token
        new_find = input.find_first_of(to_find, after_quote_pos);
      }
    }

    if (new_find != std::string_view::npos) {
      ret_val.emplace_back(input.substr(index, new_find - index));

      index = new_find + separator.length();
    } else {
      ret_val.emplace_back(input.substr(index));
    }
  }

  return ret_val;
}

std::vector<std::string> split_quoted_ids(std::string_view input) {
  return split_string_custom(input, '`',
                             mysqlshdk::utils::span_quoted_sql_identifier_bt);
}

std::vector<std::string> split_quoted(std::string_view input) {
  return split_string_custom(input, '\'',
                             mysqlshdk::utils::span_quoted_string_sq);
}
}  // namespace deprecated_delimiter_funcs

class Deprecated_partition_temporal_delimiter_check : public Sql_upgrade_check {
 public:
  Deprecated_partition_temporal_delimiter_check()
      : Sql_upgrade_check(
            ids::k_deprecated_temporal_delimiter_check,
            std::vector<std::string>{
                R"(select 
  p.table_schema, 
  p.table_name, 
  column_name, 
  CONCAT(' - partition ', 
    p.partition_name, ' uses deprecated temporal delimiters') AS message, 
  p.partition_expression, 
  p.partition_description 
from 
  information_schema.partitions p left join information_schema.columns c on 
    (p.partition_expression LIKE concat('%`', c.column_name, '`%') and 
  p.table_name = c.table_name and p.table_schema = c.table_schema) 
where 
  <<p.schema_and_table_filter>> and
  (not partition_description REGEXP ')" +
                    deprecated_delimiter_funcs::k_date_regex_multi_str +
                    R"(') and c.column_type = 'date';)",
                R"(select 
  p.table_schema, 
  p.table_name, 
  column_name, 
  CONCAT(' - partition ', 
    p.partition_name, ' uses deprecated temporal delimiters') AS message, 
  p.partition_expression, 
  p.partition_description 
from 
  information_schema.partitions p left join information_schema.columns c on 
    (partition_expression LIKE concat('%`', c.column_name, '`%') and 
    p.table_name = c.table_name and p.table_schema = c.table_schema) 
where 
  <<p.schema_and_table_filter>> and
  (not p.partition_description REGEXP ')" +
                    deprecated_delimiter_funcs::k_time_regex_multi_str + R"(') 
  and (c.column_type = 'datetime' or c.column_type = 'timestamp');)"},
            Upgrade_issue::ERROR) {}

  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    static const auto date_regex =
        std::regex(deprecated_delimiter_funcs::k_date_regex_str);
    static const auto time_regex =
        std::regex(deprecated_delimiter_funcs::k_time_regex_str);
    constexpr std::string_view max_value_str = "MAXVALUE";

    Upgrade_issue problem;
    problem.schema = row->get_as_string(0);
    problem.table = row->get_as_string(1);
    problem.column = row->get_as_string(2);
    problem.description = row->get_as_string(3);
    problem.level = m_level;

    auto part_expr = row->get_as_string(4);
    auto part_desc = row->get_as_string(5);

    if (auto col_list = deprecated_delimiter_funcs::split_quoted_ids(part_expr);
        col_list.size() > 1) {
      auto it = std::ranges::find(col_list, "`" + problem.column + "`");

      // didn't found column - false positive
      if (it == col_list.end()) return {};

      auto desc_list = deprecated_delimiter_funcs::split_quoted(part_desc);

      // column and description lists don't match - false positive
      if (col_list.size() != desc_list.size()) return {};

      auto desc = desc_list.at(it - col_list.begin());

      if (desc == max_value_str || std::regex_match(desc, date_regex) ||
          std::regex_match(desc, time_regex)) {
        // false positive
        return {};
      }

    } else if (part_desc == max_value_str) {
      // MAXVALUE is valid for partitions - false positive
      return {};
    }

    return problem;
  }
};

std::unique_ptr<Sql_upgrade_check>
get_deprecated_partition_temporal_delimiter_check() {
  return std::make_unique<Deprecated_partition_temporal_delimiter_check>();
}

std::unique_ptr<Sql_upgrade_check> get_column_definition_check() {
  return std::make_unique<Sql_upgrade_check>(
      ids::k_column_definition,
      std::vector<std::string>{
          "SELECT table_schema,table_name,column_name,concat('##', "
          "column_type, 'AutoIncrement') as tag FROM "
          "information_schema.columns WHERE <<schema_and_table_filter>> AND "
          "column_type IN ('float', 'double') and extra = 'auto_increment'"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Sql_upgrade_check> get_partitions_with_prefix_keys_check(
    const Upgrade_info &info) {
  // Conditions on partition expression
  std::vector<std::string> pe_conditions = {
      "INSTR(p.partition_expression,CONCAT('`',s.column_name,'`'))>0",
      "p.partition_expression IS NULL"};

  if (info.server_version < Version(8, 0, 0)) {
    pe_conditions.push_back("p.partition_expression = ''");
  }

  return std::make_unique<Sql_upgrade_check>(
      ids::k_partitions_with_prefix_keys,
      std::vector<std::string>{
          "SELECT s.table_schema, s.table_name, group_concat(distinct "
          "s.column_name) AS COLUMNS FROM information_schema.statistics s "
          "INNER JOIN information_schema.partitions p ON s.table_schema = "
          "p.table_schema AND s.table_name = p.table_name WHERE s.sub_part IS "
          "NOT NULL AND p.partition_method='KEY' AND "
          "(" +
          shcore::str_join(pe_conditions, " OR ") +
          ") AND <<s.schema_and_table_filter>> GROUP BY s.table_schema, "
          "s.table_name"},
      Upgrade_issue::ERROR);
}

std::unique_ptr<Upgrade_check> get_sys_var_allowed_values_check(
    const Upgrade_info &info) {
  return std::make_unique<Sys_var_allowed_values_check>(info);
}

std::unique_ptr<Upgrade_check> get_invalid_privileges_check(
    const Upgrade_info &info) {
  return std::make_unique<Invalid_privileges_check>(info);
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
