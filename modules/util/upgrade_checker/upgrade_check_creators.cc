/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/upgrade_check_creators.h"

#include <forward_list>
#include <regex>
#include <vector>

#include "modules/util/upgrade_checker/manual_check.h"
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
      "oldTemporalCheck", "Usage of old temporal type",
      std::vector<std::string>{
          "SELECT table_schema, table_name,column_name,column_type "
          "FROM information_schema.columns WHERE column_type LIKE "
          "'%5.5 binary format%';"},
      Upgrade_issue::ERROR,
      "Following table columns use a deprecated and no longer supported "
      "temporal disk storage format. They must be converted to the new format "
      "before upgrading. It can by done by rebuilding the table using 'ALTER "
      "TABLE <table_name> FORCE' command",
      nullptr, std::forward_list<std::string>{"SET show_old_temporals = ON;"},
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
      "reservedKeywordsCheck",
      "Usage of db objects with names conflicting with new reserved keywords",
      std::vector<std::string>{
          "select SCHEMA_NAME, 'Schema name' as WARNING from "
          "INFORMATION_SCHEMA.SCHEMATA where SCHEMA_NAME in " +
              keywords,
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name' as WARNING FROM "
          "INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE != 'VIEW' and "
          "TABLE_NAME in " +
              keywords,
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, COLUMN_TYPE, 'Column "
          "name' as WARNING FROM information_schema.columns WHERE "
          "TABLE_SCHEMA "
          "not in ('information_schema', 'performance_schema') and "
          "COLUMN_NAME in " +
              keywords,
          "SELECT TRIGGER_SCHEMA, TRIGGER_NAME, 'Trigger name' as WARNING "
          "FROM "
          "INFORMATION_SCHEMA.TRIGGERS WHERE TRIGGER_NAME in " +
              keywords,
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'View name' as WARNING FROM "
          "INFORMATION_SCHEMA.VIEWS WHERE TABLE_NAME in " +
              keywords,
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, 'Routine name' as WARNING "
          "FROM INFORMATION_SCHEMA.ROUTINES WHERE ROUTINE_NAME in " +
              keywords,
          "SELECT EVENT_SCHEMA, EVENT_NAME, 'Event name' as WARNING FROM "
          "INFORMATION_SCHEMA.EVENTS WHERE EVENT_NAME in " +
              keywords},
      Upgrade_issue::WARNING,
      "The following objects have names that conflict with new reserved "
      "keywords. Ensure queries sent by your applications use `quotes` when "
      "referring to them or they will result in errors.");
}

class Routine_syntax_check : public Upgrade_check {
  // This check works through a simple syntax check to find whether reserved
  // keywords are used somewhere unexpected, without checking for specific
  // keywords

  // This checks:
  // routines, view definitions, event definitions and trigger definitions

 public:
  Routine_syntax_check() : Upgrade_check("routinesSyntaxCheck") {}

  Upgrade_issue::Level get_level() const override {
    return Upgrade_issue::ERROR;
  }
  bool is_runnable() const override { return true; }

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info & /*server_info*/) override {
    struct Check_info {
      const char *names_query;
      const char *show_query;
      int code_field;
    };
    // Don't need to check views because they get auto-quoted
    Check_info object_info[] = {
        {"SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
         " FROM INFORMATION_SCHEMA.ROUTINES WHERE ROUTINE_TYPE = 'PROCEDURE'"
         " AND ROUTINE_SCHEMA <> 'sys'",
         "SHOW CREATE PROCEDURE !.!", 2},
        {"SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
         " FROM INFORMATION_SCHEMA.ROUTINES WHERE ROUTINE_TYPE = 'FUNCTION'"
         " AND ROUTINE_SCHEMA <> 'sys'",
         "SHOW CREATE FUNCTION !.!", 2},
        {"SELECT TRIGGER_SCHEMA, TRIGGER_NAME"
         " FROM INFORMATION_SCHEMA.TRIGGERS"
         " WHERE TRIGGER_SCHEMA <> 'sys'",
         "SHOW CREATE TRIGGER !.!", 2},
        {"SELECT EVENT_SCHEMA, EVENT_NAME"
         " FROM INFORMATION_SCHEMA.EVENTS"
         " WHERE EVENT_SCHEMA <> 'sys'",
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
  const char *get_title_internal() const override {
    return "MySQL syntax check for routine-like objects";
  }

  const char *get_description_internal() const override {
    return "The following objects did not pass a syntax check with the latest "
           "MySQL grammar. A common reason is that they reference names "
           "that conflict with new reserved keywords. You must update these "
           "routine definitions and `quote` any such references before "
           "upgrading.";
  }

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
      "utf8mb3Check", "Usage of utf8mb3 charset",
      std::vector<std::string>{
          "select SCHEMA_NAME, concat('schema''s default character set: ',  "
          "DEFAULT_CHARACTER_SET_NAME) from INFORMATION_SCHEMA.schemata where "
          "SCHEMA_NAME not in ('information_schema', 'performance_schema', "
          "'sys') "
          "and DEFAULT_CHARACTER_SET_NAME in ('utf8', 'utf8mb3');",
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, concat('column''s "
          "default character set: ',CHARACTER_SET_NAME) from "
          "information_schema.columns where CHARACTER_SET_NAME in ('utf8', "
          "'utf8mb3') and TABLE_SCHEMA not in ('sys', 'performance_schema', "
          "'information_schema', 'mysql');"},
      Upgrade_issue::WARNING,
      "The following objects use the utf8mb3 character set. It is recommended "
      "to convert them to use utf8mb4 instead, for improved Unicode "
      "support.");
}

std::unique_ptr<Sql_upgrade_check> get_mysql_schema_check() {
  return std::make_unique<Sql_upgrade_check>(
      "mysqlSchemaCheck",
      "Table names in the mysql schema conflicting with new tables in the "
      "latest MySQL.",
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
      Upgrade_issue::ERROR,
      "The following tables in mysql schema have names that will conflict with "
      "the ones introduced in the latest version. They must be renamed or "
      "removed before upgrading (use RENAME TABLE command). This may also "
      "entail changes to applications that use the affected tables.");
}

std::unique_ptr<Sql_upgrade_check> get_innodb_rowformat_check() {
  return std::make_unique<Sql_upgrade_check>(
      "innodbRowFormatCheck", "InnoDB tables with non-default row format",
      std::vector<std::string>{
          "select table_schema, table_name, row_format from "
          "information_schema.tables where engine = 'innodb' "
          "and row_format != 'Dynamic';"},
      Upgrade_issue::WARNING,
      "The following tables use a non-default InnoDB row format");
}

std::unique_ptr<Sql_upgrade_check> get_zerofill_check() {
  return std::make_unique<Sql_upgrade_check>(
      "zerofillCheck", "Usage of use ZEROFILL/display length type attributes",
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
      Upgrade_issue::NOTICE,
      "The following table columns specify a ZEROFILL/display length "
      "attributes. "
      "Please be aware that they will be ignored in the latest MySQL version.");
}

// Zerofill is still available in 8.0.11
// namespace {
// bool UNUSED_VARIABLE(register_zerofill) =
// Upgrade_check_registry::register_check(
//    std::bind(&get_zerofill_check), "8.0.11");
//}

std::unique_ptr<Sql_upgrade_check> get_nonnative_partitioning_check() {
  return std::make_unique<Sql_upgrade_check>(
      "nonNativePartitioningCheck",
      "Partitioned tables using engines with non native partitioning",
      std::vector<std::string>{
          "select table_schema, table_name, concat(engine, ' engine does not "
          "support native partitioning') from information_schema.Tables where "
          "create_options like '%partitioned%' and upper(engine) not in "
          "('INNODB', 'NDB', 'NDBCLUSTER');"},
      Upgrade_issue::ERROR,
      "In the latest MySQL storage engine is responsible for providing its own "
      "partitioning handler, and the MySQL server no longer provides generic "
      "partitioning support. InnoDB and NDB are the only storage engines that "
      "provide a native partitioning handler that is supported in the latest "
      "MySQL. A partitioned table using any other storage engine must be "
      "altered—either to convert it to InnoDB or NDB, or to remove its "
      "partitioning—before upgrading the server, else it cannot be used "
      "afterwards.");
}

std::unique_ptr<Sql_upgrade_check> get_foreign_key_length_check() {
  return std::make_unique<Sql_upgrade_check>(
      "foreignKeyLengthCheck",
      "Foreign key constraint names longer than 64 characters",
      std::vector<std::string>{
          "select table_schema, table_name, 'Foreign key longer than 64 "
          "characters' as description from information_schema.tables where "
          "table_name in (select left(substr(id,instr(id,'/')+1), "
          "instr(substr(id,instr(id,'/')+1),'_ibfk_')-1) from "
          "information_schema.innodb_sys_foreign where "
          "length(substr(id,instr(id,'/')+1))>64);"},
      Upgrade_issue::ERROR,
      "The following tables must be altered to have constraint names shorter "
      "than 64 characters (use ALTER TABLE).");
}

std::unique_ptr<Sql_upgrade_check> get_maxdb_sql_mode_flags_check() {
  return std::make_unique<Sql_upgrade_check>(
      "maxdbFlagCheck", "Usage of obsolete MAXDB sql_mode flag",
      std::vector<std::string>{
          "select routine_schema, routine_name, concat(routine_type, ' uses "
          "obsolete MAXDB sql_mode') from information_schema.routines where "
          "find_in_set('MAXDB', sql_mode);",
          "select event_schema, event_name, 'EVENT uses obsolete MAXDB "
          "sql_mode' "
          "from information_schema.EVENTS where find_in_set('MAXDB', "
          "sql_mode);",
          "select trigger_schema, trigger_name, 'TRIGGER uses obsolete MAXDB "
          "sql_mode' from information_schema.TRIGGERS where "
          "find_in_set('MAXDB', sql_mode);",
          "select concat('global system variable ', variable_name), 'defined "
          "using obsolete MAXDB option' as reason from "
          "performance_schema.global_variables where variable_name = "
          "'sql_mode' and find_in_set('MAXDB', variable_value);"},
      Upgrade_issue::WARNING,
      "The following DB objects have the obsolete MAXDB option persisted for "
      "sql_mode, which will be cleared during the upgrade. It "
      "can potentially change the datatype DATETIME into TIMESTAMP if it is "
      "used inside object's definition, and this in turn can change the "
      "behavior in case of dates earlier than 1970 or later than 2037. If this "
      "is a concern, please redefine these objects so that they do not rely on "
      "the MAXDB flag before running the upgrade.");
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
    queries.emplace_back(shcore::str_format(
        "select concat('global system variable ', variable_name), 'defined "
        "using obsolete %s option' as reason from "
        "performance_schema.global_variables where variable_name = 'sql_mode' "
        "and find_in_set('%s', variable_value);",
        mode, mode));
  }

  return std::make_unique<Sql_upgrade_check>(
      "sqlModeFlagCheck", "Usage of obsolete sql_mode flags",
      std::move(queries), Upgrade_issue::NOTICE,
      "The following DB objects have obsolete options persisted for sql_mode, "
      "which will be cleared during the upgrade.");
}

class Enum_set_element_length_check : public Sql_upgrade_check {
 public:
  Enum_set_element_length_check()
      : Sql_upgrade_check(
            "enumSetElementLenghtCheck",
            "ENUM/SET column definitions containing elements longer than 255 "
            "characters",
            {"select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, UPPER(DATA_TYPE), "
             "COLUMN_TYPE, CHARACTER_MAXIMUM_LENGTH from "
             "information_schema.columns where data_type in ('enum','set') and "
             "CHARACTER_MAXIMUM_LENGTH > 255 and table_schema not in "
             "('information_schema');"},
            Upgrade_issue::ERROR,
            "The following columns are defined as either ENUM or SET and "
            "contain at least one element longer that 255 characters. They "
            "need to be altered so that all elements fit into the 255 "
            "characters limit.") {}

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
  Check_table_command() : Upgrade_check("checkTableOutput") {}

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override {
    // Needed for warnings related to triggers, incompatible types in 5.7
    if (server_info.server_version < Version(8, 0, 0))
      session->execute("FLUSH LOCAL TABLES;");

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

  Upgrade_issue::Level get_level() const override {
    throw std::runtime_error("Unimplemented");
  }

 protected:
  const char *get_description_internal() const override { return nullptr; }

  const char *get_title_internal() const override {
    return "Issues reported by 'check table x for upgrade' command";
  }
};

std::unique_ptr<Upgrade_check> get_table_command_check() {
  return std::make_unique<Check_table_command>();
}

std::unique_ptr<Sql_upgrade_check>
get_partitioned_tables_in_shared_tablespaces_check(const Upgrade_info &info) {
  return std::make_unique<Sql_upgrade_check>(
      "partitionedTablesInSharedTablespaceCheck",
      "Usage of partitioned tables in shared tablespaces",
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
      Upgrade_issue::ERROR,
      "The following tables have partitions in shared tablespaces. They need "
      "to be moved to file-per-table tablespace before upgrading. "
      "You can do this by running query like 'ALTER TABLE table_name "
      "REORGANIZE PARTITION X INTO (PARTITION X VALUES LESS THAN (30) "
      "TABLESPACE=innodb_file_per_table);'");
}

std::unique_ptr<Sql_upgrade_check> get_circular_directory_check() {
  return std::make_unique<Sql_upgrade_check>(
      "circularDirectoryCheck",
      "Circular directory references in tablespace data file paths",
      std::vector<std::string>{
          "SELECT tablespace_name, concat('circular reference in datafile "
          "path: "
          R"(\'', file_name, '\'') FROM INFORMATION_SCHEMA.FILES where )"
          R"(file_type='TABLESPACE' and (file_name rlike '[^\\.]/\\.\\./' or )"
          R"(file_name rlike '[^\\.]\\\\\\.\\.\\\\');)"},
      Upgrade_issue::ERROR,
      "Following tablespaces contain circular directory references (e.g. the "
      "reference '/../') in data file paths which as of MySQL 8.0.17 are not "
      "permitted by the CREATE TABLESPACE ... ADD DATAFILE clause. An "
      "exception to the restriction exists on Linux, where a circular "
      "directory reference is permitted if the preceding directory is a "
      "symbolic link. To avoid upgrade issues, remove any circular directory "
      "references from tablespace data file paths before upgrading.");
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
            "removedFunctionsCheck", "Usage of removed functions",
            {"select table_schema, table_name, '', 'VIEW', "
             "UPPER(view_definition) from information_schema.views where "
             "table_schema not in "
             "('performance_schema','information_schema','sys','mysql');",
             "select routine_schema, routine_name, '', routine_type, "
             "UPPER(routine_definition) from information_schema.routines where"
             " routine_schema not in "
             "('performance_schema','information_schema','sys','mysql');",
             "select TABLE_SCHEMA,TABLE_NAME,COLUMN_NAME, 'COLUMN'"
             ", UPPER(GENERATION_EXPRESSION) from "
             "information_schema.columns where extra regexp 'generated' and "
             "table_schema not in "
             "('performance_schema','information_schema','sys','mysql');",
             "select TRIGGER_SCHEMA, TRIGGER_NAME, '', 'TRIGGER', "
             "UPPER(ACTION_STATEMENT) from information_schema.triggers where "
             "TRIGGER_SCHEMA not in "
             "('performance_schema','information_schema','sys','mysql');",
             "select event_schema, event_name, '', 'EVENT', "
             "UPPER(EVENT_DEFINITION) from information_schema.events where "
             "event_schema not in "
             "('performance_schema','information_schema','sys','mysql')"},
            Upgrade_issue::ERROR,
            "Following DB objects make use of functions that have "
            "been removed int the latest MySQL version. Please make sure to "
            "update them to use supported alternatives before upgrade.") {}

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
            "groupByAscCheck", "Usage of removed GROUP BY ASC/DESC syntax",
            {"select table_schema, table_name, 'VIEW', "
             "UPPER(view_definition) from information_schema.views where "
             "table_schema not in "
             "('performance_schema','information_schema','sys','mysql') and "
             "(UPPER(view_definition) like '%ASC%' or UPPER(view_definition) "
             "like '%DESC%');",
             "select routine_schema, routine_name, routine_type, "
             "UPPER(routine_definition) from information_schema.routines where "
             "routine_schema not in "
             "('performance_schema','information_schema','sys','mysql') and "
             "(UPPER(routine_definition) like '%ASC%' or "
             "UPPER(routine_definition) like '%DESC%');",
             "select TRIGGER_SCHEMA, TRIGGER_NAME, 'TRIGGER', "
             "UPPER(ACTION_STATEMENT) from information_schema.triggers where "
             "TRIGGER_SCHEMA not in "
             "('performance_schema','information_schema','sys','mysql') and "
             "(UPPER(ACTION_STATEMENT) like '%ASC%' or UPPER(ACTION_STATEMENT) "
             "like '%DESC%');",
             "select event_schema, event_name, 'EVENT', "
             "UPPER(EVENT_DEFINITION) from information_schema.events where "
             "event_schema not in "
             "('performance_schema','information_schema','sys','mysql') and "
             "(UPPER(event_definition) like '%ASC%' or UPPER(event_definition) "
             "like '%DESC%');"},
            Upgrade_issue::ERROR,
            "The following DB objects use removed GROUP BY ASC/DESC syntax. "
            "They need to be altered so that ASC/DESC keyword is removed "
            "from GROUP BY clause and placed in appropriate ORDER BY clause.") {
  }

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

class Removed_sys_var_check : public Sql_upgrade_check {
 public:
  Removed_sys_var_check(const char *name, const char *title,
                        std::map<std::string, const char *> &&removed_vars,
                        const char *problem_description,
                        const char *advice =
                            "Following system variables that were detected "
                            "as being used will be removed",
                        Upgrade_issue::Level level = Upgrade_issue::ERROR,
                        const char *minimal_version = "8.0.11")
      : Sql_upgrade_check(name, title, {}, level, advice, minimal_version),
        m_vars(std::move(removed_vars)),
        m_problem_description(problem_description) {
    assert(!m_vars.empty());
    std::stringstream ss;
    ss << "SELECT VARIABLE_NAME FROM performance_schema.variables_info WHERE "
          "VARIABLE_SOURCE <> 'COMPILED' AND VARIABLE_NAME IN (";
    for (const auto &vp : m_vars) ss << "'" << vp.first << "',";
    ss.seekp(-1, ss.cur);
    ss << ");";
    m_queries.push_back(ss.str());
  }

 protected:
  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    Upgrade_issue out;
    out.schema = row->get_as_string(0);
    out.description = m_problem_description;

    const auto it = m_vars.find(out.schema);
    if (it != m_vars.end() && it->second != nullptr)
      out.description +=
          shcore::str_format(", consider using %s instead", it->second);
    return out;
  }

  const std::map<std::string, const char *> m_vars;
  std::string m_problem_description;
};

// This class enables checking server configuration file for defined/undefined
// system variables that have been removed, deprecated etc.
class Config_check : public Upgrade_check {
 public:
  Config_check(const char *name, std::map<std::string, const char *> &&vars,
               Config_mode mode = Config_mode::DEFINED,
               Upgrade_issue::Level level = Upgrade_issue::ERROR,
               const char *problem_description = "is set and will be removed",
               const char *title = "", const char *advice = "")
      : Upgrade_check(name),
        m_vars(std::move(vars)),
        m_mode(mode),
        m_level(level),
        m_problem_description(problem_description),
        m_title(title),
        m_advice(Upgrade_issue::level_to_string(level)) {
    if (advice && strlen(advice) > 0) m_advice = m_advice + ": " + advice;
  }

  std::vector<Upgrade_issue> run(
      [[maybe_unused]] const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override {
    if (server_info.config_path.empty())
      throw Check_configuration_error(
          "To run this check requires full path to MySQL server configuration "
          "file to be specified at 'configPath' key of options dictionary");

    mysqlshdk::config::Config_file cf;
    cf.read(server_info.config_path);

    std::vector<Upgrade_issue> res;
    for (const auto &group : cf.groups()) {
      for (const auto &option : cf.options(group)) {
        auto standardized = shcore::str_replace(option, "-", "_");
        const auto it = m_vars.find(standardized);
        if (it == m_vars.end()) continue;
        if (m_mode == Config_mode::DEFINED) {
          Upgrade_issue issue;
          issue.schema = option;
          issue.description = m_problem_description;
          if (it->second != nullptr)
            issue.description +=
                shcore::str_format(", consider using %s instead", it->second);
          issue.level = m_level;
          res.push_back(issue);
        }
        m_vars.erase(it);
      }
    }

    if (m_mode == Config_mode::UNDEFINED) {
      for (const auto &var : m_vars) {
        Upgrade_issue issue;
        issue.schema = var.first;
        issue.description = m_problem_description;
        if (var.second != nullptr) {
          issue.description += " ";
          issue.description += var.second;
        }
        issue.level = m_level;
        res.push_back(issue);
      }
    }

    return res;
  }

 protected:
  const char *get_description_internal() const override {
    return m_advice.empty() ? nullptr : m_advice.c_str();
  }
  const char *get_title_internal() const override {
    return m_title.empty() ? nullptr : m_title.c_str();
  }
  Upgrade_issue::Level get_level() const override { return m_level; }

  std::map<std::string, const char *> m_vars;
  Config_mode m_mode;
  const Upgrade_issue::Level m_level;
  std::string m_problem_description;
  std::string m_title;
  std::string m_advice;
};

std::unique_ptr<Upgrade_check> get_config_check(
    const char *name, std::map<std::string, const char *> &&vars,
    Config_mode mode, Upgrade_issue::Level level,
    const char *problem_description, const char *title, const char *advice) {
  return std::make_unique<Config_check>(name, std::move(vars), mode, level,
                                        problem_description, title, advice);
}

std::unique_ptr<Upgrade_check> get_removed_sys_log_vars_check(
    const Upgrade_info &info) {
  const char *name = "removedSysLogVars";
  const char *title =
      "Removed system variables for error logging to the system log "
      "configuration";
  const char *advice =
      "System variables related to logging using OS facilities (the Event Log "
      "on Windows, and syslog on Unix and Unix-like systems) have been "
      "removed. Where appropriate, the removed system variables were replaced "
      "with new system variables managed by the log_sink_syseventlog error log "
      "component. Installations that used the old system variable names must "
      "update their configuration to use the new variable names.";
  const char *problem_description = "is set and will be removed";
  std::map<std::string, const char *> vars{
      {"log_syslog_facility", "syseventlog.facility"},
      {"log_syslog_include_pid", "syseventlog.include_pid"},
      {"log_syslog_tag", "syseventlog.tag"},
      {"log_syslog", nullptr}};

  if (info.server_version < mysqlshdk::utils::Version(8, 0, 0) &&
      info.server_version >= mysqlshdk::utils::Version(5, 7, 0))
    return std::make_unique<Config_check>(
        name, std::move(vars), Config_mode::DEFINED, Upgrade_issue::ERROR,
        problem_description, title, advice);

  return std::make_unique<Removed_sys_var_check>(name, title, std::move(vars),
                                                 problem_description, advice,
                                                 Upgrade_issue::ERROR);
}

std::unique_ptr<Upgrade_check> get_removed_sys_vars_check(
    const Upgrade_info &info) {
  const auto &ver = info.server_version;
  const auto &target = info.target_version;
  std::map<std::string, const char *> vars;
  if (ver < mysqlshdk::utils::Version(8, 0, 11))
    vars.insert(
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

  if (ver < Version(8, 0, 13) && target >= Version(8, 0, 13))
    vars.insert({{"metadata_locks_cache_size", nullptr},
                 {"metadata_locks_hash_instances", nullptr}});

  if (ver < Version(8, 0, 16) && target >= Version(8, 0, 16))
    vars.insert({{"internal_tmp_disk_storage_engine", nullptr}});

  const char *name = "removedSysVars";
  const char *title = "Removed system variables";
  const char *problem_description = "is set and will be removed";
  const char *advice =
      "Following system variables that were detected as being used will be "
      "removed. Please update your system to not rely on them before the "
      "upgrade.";

  if (ver < Version(8, 0, 0))
    return std::make_unique<Config_check>(
        name, std::move(vars), Config_mode::DEFINED, Upgrade_issue::ERROR,
        problem_description, title, advice);
  return std::make_unique<Removed_sys_var_check>(name, title, std::move(vars),
                                                 problem_description, advice);
}

std::unique_ptr<Upgrade_check> get_sys_vars_new_defaults_check() {
  return std::make_unique<Config_check>(
      "sysVarsNewDefaults",
      std::map<std::string, const char *>{
          {"character_set_server", "from latin1 to utf8mb4"},
          {"collation_server", "from latin1_swedish_ci to utf8mb4_0900_ai_ci"},
          {"explicit_defaults_for_timestamp", "from OFF to ON"},
          {"optimizer_trace_max_mem_size", "from 16KB to 1MB"},
          {"back_log", nullptr},
          {"max_allowed_packet", "from 4194304 (4MB) to 67108864 (64MB)"},
          {"max_error_count", "from 64 to 1024"},
          {"event_scheduler", "from OFF to ON"},
          {"table_open_cache", "from 2000 to 4000"},
          {"log_error_verbosity", "from 3 (Notes) to 2 (Warning)"},
          {"innodb_undo_tablespaces", "from 0 to 2"},
          {"innodb_undo_log_truncate", "from OFF to ON"},
          {"innodb_flush_method",
           "from NULL to fsync (Unix), unbuffered (Windows)"},
          {"innodb_autoinc_lock_mode",
           "from 1 (consecutive) to 2 (interleaved)"},
          {"innodb_flush_neighbors", "from 1 (enable) to 0 (disable)"},
          {"innodb_max_dirty_pages_pct_lwm", "from_0 (%) to 10 (%)"},
          {"innodb_max_dirty_pages_pct", "from 75 (%)  90 (%)"},
          {"performance_schema_consumer_events_transactions_current",
           "from OFF to ON"},
          {"performance_schema_consumer_events_transactions_history",
           "from OFF to ON"},
          {"log_bin", "from OFF to ON"},
          {"server_id", "from 0 to 1"},
          {"log_slave_updates", "from OFF to ON"},
          {"master_info_repository", "from FILE to TABLE"},
          {"relay_log_info_repository", "from FILE to TABLE"},
          {"transaction_write_set_extraction", "from OFF to XXHASH64"},
          {"slave_rows_search_algorithms",
           "from 'INDEX_SCAN, TABLE_SCAN' to 'INDEX_SCAN, HASH_SCAN'"}},
      Config_mode::UNDEFINED, Upgrade_issue::WARNING,
      "default value will change", "System variables with new default values",
      "Following system variables that are not defined in your configuration "
      "file will have new default values. Please review if you rely on their "
      "current values and if so define them before performing upgrade.");
}

std::unique_ptr<Sql_upgrade_check> get_zero_dates_check() {
  return std::make_unique<Sql_upgrade_check>(
      "zeroDatesCheck", "Zero Date, Datetime, and Timestamp values",
      std::vector<std::string>{
          "select 'global.sql_mode', 'does not contain either NO_ZERO_DATE or "
          "NO_ZERO_IN_DATE which allows insertion of zero dates' from (SELECT "
          "@@global.sql_mode like '%NO_ZERO_IN_DATE%' and @@global.sql_mode "
          "like '%NO_ZERO_DATE%' as zeroes_enabled) as q where "
          "q.zeroes_enabled = 0;",
          "select 'session.sql_mode', concat(' of ', q.thread_count, ' "
          "session(s) does not contain either NO_ZERO_DATE or NO_ZERO_IN_DATE "
          "which allows insertion of zero dates') FROM (select "
          "count(thread_id) as thread_count from "
          "performance_schema.variables_by_thread WHERE variable_name = "
          "'sql_mode' and (variable_value not like '%NO_ZERO_IN_DATE%' or "
          "variable_value not like '%NO_ZERO_DATE%')) as q where "
          "q.thread_count > 0;",
          "select TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, concat('column has "
          "zero default value: ', COLUMN_DEFAULT) from "
          "information_schema.columns where TABLE_SCHEMA not in "
          "('performance_schema','information_schema','sys','mysql') and "
          "DATA_TYPE in ('timestamp', 'datetime', 'date') and COLUMN_DEFAULT "
          "like '0000-00-00%';"},
      Upgrade_issue::WARNING,
      "By default zero date/datetime/timestamp values are no longer allowed in "
      "MySQL, as of 5.7.8 NO_ZERO_IN_DATE and NO_ZERO_DATE are included in "
      "SQL_MODE by default. These modes should be used with strict mode as "
      "they will be merged with strict mode in a future release. If you do not "
      "include these modes in your SQL_MODE setting, you are able to insert "
      "date/datetime/timestamp values that contain zeros. It is strongly "
      "advised to replace zero values with valid ones, as they may not work "
      "correctly in the future.");
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
      "schemaInconsistencyCheck",
      "Schema inconsistencies resulting from file removal or corruption",
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
      Upgrade_issue::ERROR,
      "Following tables show signs that either table datadir directory or frm "
      "file was removed/corrupted. Please check server logs, examine datadir "
      "to detect the issue and fix it before upgrade");
}
// clang-format on

std::unique_ptr<Sql_upgrade_check> get_fts_in_tablename_check(
    const Upgrade_info &info) {
  if (info.target_version >= Version(8, 0, 18) ||
      shcore::str_beginswith(info.server_os, "WIN"))
    throw Check_not_needed();

  return std::make_unique<Sql_upgrade_check>(
      "ftsTablenameCheck", "Table names containing 'FTS'",
      std::vector<std::string>{
          "select table_schema, table_name , 'table name contains ''FTS''' "
          "from information_schema.tables where table_name like binary "
          "'%FTS%';"},
      Upgrade_issue::ERROR,
      "Upgrading from 5.7 to the latest MySQL version does not support tables "
      "with name containing 'FTS' character string. The workaround is to "
      "rename the table for the upgrade - e.g. it is enough to change any "
      "letter of the 'FTS' part to a lower case. It can be renamed back again "
      "after the upgrade.");
}

// clang-format off
std::unique_ptr<Sql_upgrade_check> get_engine_mixup_check() {
  return std::make_unique<Sql_upgrade_check>(
      "engineMixupCheck",
      "Tables recognized by InnoDB that belong to a different engine",
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
          "where a.engine != 'Innodb';"},
      Upgrade_issue::ERROR);
}
// clang-format on

std::unique_ptr<Sql_upgrade_check> get_old_geometry_types_check(
    const Upgrade_info &info) {
  if (info.target_version >= Version(8, 0, 24)) throw Check_not_needed();
  return std::make_unique<Sql_upgrade_check>(
      "oldGeometryCheck", "Spatial data columns created in MySQL 5.6",
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
      : Sql_upgrade_check(
            "changedFunctionsInGeneratedColumnsCheck",
            "Indexes on functions with changed semantics",
            {"SELECT s.table_schema, s.table_name, s.column_name,"
             "    UPPER(c.generation_expression)"
             "  FROM information_schema.columns c"
             "  JOIN information_schema.statistics s"
             "    ON c.table_schema = s.table_schema"
             "    AND c.table_name = s.table_name"
             "    AND c.column_name = s.column_name"
             "  WHERE s.table_schema not in "
             "    ('performance_schema','information_schema','sys','mysql')"
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
get_changed_functions_generated_columns_check(const Upgrade_info &info) {
  if (info.target_version && (info.target_version < Version(8, 0, 28) ||
                              info.server_version >= Version(8, 0, 28)))
    throw Check_not_needed();

  return std::make_unique<Changed_functions_in_generated_columns_check>();
}

// this check is applicable to versions up to 8.0.12, starting with 8.0.13
// these types can have default values, specified as an expression
class Columns_which_cannot_have_defaults_check : public Sql_upgrade_check {
 public:
  Columns_which_cannot_have_defaults_check()
      : Sql_upgrade_check(
            "columnsWhichCannotHaveDefaultsCheck",
            "Columns which cannot have default values",
            {"SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, DATA_TYPE FROM "
             "INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA NOT IN "
             "('information_schema', 'performance_schema', 'sys') AND "
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
      "mysqlInvalid57NamesCheck",
      "Check for invalid table names and schema names used in 5.7",
      std::vector<std::string>{
          "SELECT SCHEMA_NAME, 'Schema name' AS WARNING FROM "
          "INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME LIKE '#mysql50#%';",
          "SELECT TABLE_SCHEMA, TABLE_NAME, 'Table name' AS WARNING FROM "
          "INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME LIKE '#mysql50#%';"},
      Upgrade_issue::ERROR,
      "The following tables and/or schemas have invalid names. In order to fix "
      "them use the mysqlcheck utility as follows:\n"
      "\n  $ mysqlcheck --check-upgrade --all-databases"
      "\n  $ mysqlcheck --fix-db-names --fix-table-names --all-databases"
      "\n\nOR via mysql client, for eg:\n"
      "\n  ALTER DATABASE `#mysql50#lost+found` UPGRADE DATA DIRECTORY NAME;");
}

std::unique_ptr<Sql_upgrade_check> get_orphaned_routines_check() {
  return std::make_unique<Sql_upgrade_check>(
      "mysqlOrphanedRoutinesCheck", "Check for orphaned routines in 5.7",
      std::vector<std::string>{
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, 'is orphaned' AS WARNING "
          "FROM information_schema.routines WHERE NOT EXISTS "
          "(SELECT SCHEMA_NAME FROM information_schema.schemata "
          "WHERE ROUTINE_SCHEMA=SCHEMA_NAME);"},
      Upgrade_issue::ERROR,
      "The following routines have been orphaned. Schemas that they are "
      "referencing no longer exists.\nThey have to be cleaned up or the "
      "upgrade will fail.");
}

std::unique_ptr<Sql_upgrade_check> get_dollar_sign_name_check() {
  return std::make_unique<Sql_upgrade_check>(
      "mysqlDollarSignNameCheck",
      "Check for deprecated usage of single dollar signs in object names",
      std::vector<std::string>{
          "SELECT SCHEMA_NAME, 'name starts with a $ sign.' FROM "
          "information_schema.schemata WHERE SCHEMA_NAME LIKE '$_%' AND "
          "SCHEMA_NAME NOT LIKE '$_%$';",
          "SELECT TABLE_SCHEMA, TABLE_NAME, ' name starts with $ sign.' FROM "
          "information_schema.tables WHERE TABLE_SCHEMA NOT IN ('sys', "
          "'performance_schema', 'mysql', 'information_schema') AND TABLE_NAME "
          "LIKE '$_%' AND TABLE_NAME NOT LIKE '$_%$';",
          "SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, ' name starts with $ "
          "sign.' FROM information_schema.columns WHERE TABLE_SCHEMA NOT IN "
          "('sys', 'performance_schema', 'mysql', 'information_schema') AND "
          "COLUMN_NAME LIKE ('$_%') AND COLUMN_NAME NOT LIKE ('$%_$') ORDER BY "
          "TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME;",
          "SELECT TABLE_SCHEMA, TABLE_NAME, INDEX_NAME, ' starts with $ sign.' "
          "FROM information_schema.statistics WHERE TABLE_SCHEMA NOT IN "
          "('sys', 'mysql', 'information_schema', 'performance_schema') AND "
          "INDEX_NAME LIKE '$_%' AND INDEX_NAME NOT LIKE '$%_$' ORDER BY "
          "TABLE_SCHEMA, TABLE_NAME, INDEX_NAME;",
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, ' name starts with $ sign.' "
          "FROM information_schema.routines WHERE ROUTINE_SCHEMA NOT IN "
          "('sys', 'performance_schema', 'mysql', 'information_schema') AND "
          "ROUTINE_NAME LIKE '$_%' AND ROUTINE_NAME NOT LIKE '$%_$';",
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, ' body contains identifiers "
          "that start with $ sign.' FROM information_schema.routines WHERE "
          "ROUTINE_SCHEMA NOT IN ('sys', 'performance_schema', 'mysql', "
          "'information_schema') AND ROUTINE_DEFINITION REGEXP "
          "'[[:blank:],.;\\\\)\\\\(]\\\\$[A-Za-z0-9\\\\_\\\\$]*[A-Za-z0-9\\\\_]"
          "([[:blank:],;\\\\)]|(\\\\.[^0-9]))';"},

      Upgrade_issue::WARNING,
      "The following objects have names with deprecated usage of dollar sign "
      "($) at the begining of the identifier. To correct this warning, ensure, "
      "that names starting with dollar sign, also end with it, similary "
      "to quotes ($example$). ");
}

std::unique_ptr<Sql_upgrade_check> get_index_too_large_check() {
  return std::make_unique<Sql_upgrade_check>(
      "mysqlIndexTooLargeCheck",
      "Check for indexes that are too large to work on higher versions of "
      "MySQL Server than 5.7",
      std::vector<std::string>{
          "select s.table_schema, s.table_name, s.index_name, 'index too "
          "large.' as warning from information_schema.statistics s, "
          "information_schema.columns c, information_schema.innodb_sys_tables "
          "i where s.table_name=c.table_name and s.table_schema=c.table_schema "
          "and c.column_name=s.column_name and concat(s.table_schema, '/', "
          "s.table_name) = i.name and i.row_format in ('Redundant', 'Compact') "
          "and (s.sub_part is null or s.sub_part > 255) and "
          "c.character_octet_length > 767;"},
      Upgrade_issue::ERROR,
      "The following indexes ware made too large for their format in an older "
      "version of MySQL (older than 5.7.34). Normally those indexes within "
      "tables with compact or redundant row formats shouldn't be larger than "
      "767 bytes. To fix this problem those indexes should be dropped before "
      "upgrading or those tables will be inaccessible.");
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
      "mysqlEmptyDotTableSyntaxCheck",
      "Check for deprecated '.<table>' syntax used in routines.",
      std::vector<std::string>{
          "SELECT ROUTINE_SCHEMA, ROUTINE_NAME, ' routine body contains "
          "deprecated identifiers.' FROM information_schema.routines WHERE "
          "ROUTINE_SCHEMA NOT IN ('sys', 'performance_schema', 'mysql', "
          "'information_schema') AND ROUTINE_DEFINITION REGEXP '" +
              regex + "';",
          "SELECT EVENT_SCHEMA, EVENT_NAME, ' event body contains deprecated "
          "identifiers.' FROM information_schema.events WHERE EVENT_SCHEMA NOT "
          "IN ('sys', 'performance_schema', 'mysql', 'information_schema') AND "
          "EVENT_DEFINITION REGEXP '" +
              regex + "';",
          "SELECT TRIGGER_SCHEMA, TRIGGER_NAME, ' trigger body contains "
          "deprecated identifiers.' FROM information_schema.triggers WHERE "
          "TRIGGER_SCHEMA NOT IN ('sys', 'performance_schema', 'mysql', "
          "'information_schema') AND ACTION_STATEMENT REGEXP '" +
              regex + "';"},
      Upgrade_issue::ERROR,
      "The following routines contain identifiers in deprecated identifier "
      "syntax (\".<table>\"), and should be corrected before upgrade:\n");
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

class Deprecated_auth_method_check : public Sql_upgrade_check {
 public:
  explicit Deprecated_auth_method_check(mysqlshdk::utils::Version target_ver)
      : Sql_upgrade_check(
            "deprecatedAuthMethod",
            "Check for deprecated or invalid user authentication methods.",
            std::vector<std::string>{
                "SELECT CONCAT(user, '@', host), plugin FROM "
                "mysql.user WHERE plugin IN (" +
                deprecated_auth_funcs::get_auth_list() +
                ") AND user NOT LIKE 'mysql_router%';"},
            Upgrade_issue::WARNING),
        m_target_version(target_ver) {
    m_advice =
        "The following users are using a deprecated authentication method:\n";
  }

  ~Deprecated_auth_method_check() = default;

  bool is_multi_lvl_check() const override { return true; }

 protected:
  Upgrade_issue parse_row(const mysqlshdk::db::IRow *row) override {
    auto item = row->get_as_string(0);
    auto auth = row->get_as_string(1);

    return deprecated_auth_funcs::parse_item(item, auth, m_target_version);
  }

 private:
  const mysqlshdk::utils::Version m_target_version;
};

class Deprecated_default_auth_check : public Sql_upgrade_check {
 public:
  Deprecated_default_auth_check(mysqlshdk::utils::Version target_ver)
      : Sql_upgrade_check(
            "deprecatedDefaultAuth",
            "Check for deprecated or invalid default authentication methods in "
            "system variables.",
            std::vector<std::string>{
                "show variables where variable_name = "
                "'default_authentication_plugin' AND value IN (" +
                    deprecated_auth_funcs::get_auth_list() + ");",
                "show variables where variable_name = 'authentication_policy' "
                "AND (" +
                    deprecated_auth_funcs::get_auth_or_like_list("value") +
                    ");"},
            Upgrade_issue::WARNING),
        m_target_version(target_ver) {
    m_advice =
        "The following variables have problems with their set "
        "authentication method:\n";
  }

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

std::unique_ptr<Sql_upgrade_check> get_deprecated_auth_method_check(
    const Upgrade_info &info) {
  return std::make_unique<Deprecated_auth_method_check>(info.target_version);
}

std::unique_ptr<Sql_upgrade_check> get_deprecated_default_auth_check(
    const Upgrade_info &info) {
  return std::make_unique<Deprecated_default_auth_check>(info.target_version);
}

std::unique_ptr<Sql_upgrade_check> get_deprecated_router_auth_method_check(
    const Upgrade_info &info) {
  return std::make_unique<Sql_upgrade_check>(
      "deprecatedRouterAuthMethod",
      "Check for deprecated or invalid authentication methods in use by MySQL "
      "Router internal accounts.",
      std::vector<std::string>{
          "SELECT CONCAT(user, '@', host), ' - router user "
          "with deprecated authentication method.' FROM "
          "mysql.user WHERE plugin IN (" +
          deprecated_auth_funcs::get_auth_list() +
          ") AND user LIKE 'mysql_router%';"},
      info.target_version >= Version(8, 4, 0) ? Upgrade_issue::ERROR
                                              : Upgrade_issue::WARNING,
      "The following accounts are MySQL Router accounts that use a deprecated "
      "authentication method.\n"
      "Those accounts are automatically created at bootstrap time when the "
      "Router is not instructed to use an existing account. Please upgrade "
      "MySQL Router to the latest version to ensure deprecated authentication "
      "methods are no longer used.\n"
      "Since version 8.0.19 it's also possible to instruct MySQL Router to use "
      "a dedicated account. That account can be created using the AdminAPI.");
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
            "deprecatedTemporalDelimiter",
            "Check for deprecated temporal delimiters in table partitions.",
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
  (not p.partition_description REGEXP ')" +
                    deprecated_delimiter_funcs::k_time_regex_multi_str + R"(') 
  and (c.column_type = 'datetime' or c.column_type = 'timestamp');)"},
            Upgrade_issue::ERROR,
            "The following columns are referenced in partitioning function "
            "using custom temporal delimiters.\n"
            "Custom temporal delimiters were deprecated since 8.0.29. "
            "Partitions using them will make partitioned tables unaccessible "
            "after upgrade.\n"
            "These partitions need to be updated to standard temporal "
            "delimiters before the upgrade.") {}

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

}  // namespace upgrade_checker
}  // namespace mysqlsh
