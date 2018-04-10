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

#include "mysqlshdk/shellcore/provider_sql.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>

#include "modules/devapi/base_resultset.h"  // TODO(alfredo) remove when ISession
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace completer {

namespace {

extern std::vector<std::string> k_sorted_keywords;
}

void add_matches_ci(const std::vector<std::string> &options,
                    Completion_list *out_list, const std::string &prefix,
                    bool back_quote = false) {
  auto iter =
      std::lower_bound(options.begin(), options.end(), prefix,
                       [](const std::string &a, const std::string &b) -> bool {
                         return shcore::str_casecmp(a, b) < 0;
                       });
  while (iter != options.end()) {
    if (shcore::str_ibeginswith(*iter, prefix)) {
      if (back_quote)
        out_list->push_back(shcore::quote_identifier(*iter, '`'));
      else
        out_list->push_back(*iter);
      ++iter;
    } else {
      break;
    }
  }
}

Completion_list Provider_sql::complete_schema(const std::string &prefix) {
  Completion_list list;
  add_matches_ci(schema_names_, &list, prefix);
  return list;
}

/**
 * Return list of possible completions for the provided text.
 *
 * @param  text         full text to be completed
 * @param  compl_offset offset in text where the completion should start
 *                      on return, assigned to the start position being
 *                      considered for completion
 * @return              list of completion strings
 *
 * compl_offset is expected to be last string token in the text. Tokens
 * are broken at one of =+-/\\*?\"'`&<>;|@{([])}
 *
 * By default, all matching options are considered.
 * The following types of options are used:
 * - reserved keywords and built-in function names (k_sorted_keywords)
 * - system schema names (k_sorted_schemas)
 * - schema names from the connected server
 * - table names of the current schema
 * - column names of the current schema
 *
 * The limited completion filter will strip down the list of options
 * based on the following contextual information:
 * - if the character before the token is `, only DB names will be used
 * - if there's a . in the token, treat it as a DB name
 */
Completion_list Provider_sql::complete(const std::string &text,
                                       size_t *compl_offset) {
  Completion_list options;
  size_t offset = *compl_offset;
  bool db_names_only = false;
  bool back_quote = false;

  if (text[0] == '\\') return options;

  // look for .
  auto dot_pos = text.find('.', offset);
  if (dot_pos != std::string::npos) {
    db_names_only = true;
  } else if (offset > 0 && text[offset - 1] == '`') {
    --*compl_offset;
    back_quote = true;
    db_names_only = true;
  }

  std::string prefix = text.substr(offset);

  // if backtick quoted, skip keywords lookup
  if (!db_names_only) {
    add_matches_ci(k_sorted_keywords, &options, prefix);
  }

  // add DB objects
  add_matches_ci(schema_names_, &options, prefix, back_quote);

  if (dot_pos != std::string::npos) {
    add_matches_ci(object_dot_names_, &options, prefix, back_quote);
  }
  add_matches_ci(object_names_, &options, prefix, back_quote);
  return options;
}

void Provider_sql::interrupt_rehash() { cancelled_ = true; }

void Provider_sql::refresh_schema_cache(
    std::shared_ptr<mysqlsh::ShellBaseSession> session) {
  schema_names_.clear();
  schema_names_.reserve(100);

  auto res = session->raw_execute_sql("show schemas");
  auto row = res->call("fetchOne", shcore::Argument_list());
  while (row && !cancelled_) {
    std::string column =
        row.as_object<mysqlsh::Row>()->get_member(0).as_string();
    schema_names_.push_back(column);
    row = res->call("fetchOne", shcore::Argument_list());
  }
  std::sort(schema_names_.begin(), schema_names_.end(),
            [](const std::string &a, const std::string &b) -> bool {
              return shcore::str_casecmp(a, b) < 0;
            });
}

void Provider_sql::refresh_name_cache(
    std::shared_ptr<mysqlsh::ShellBaseSession> session,
    const std::string &current_schema,
    const std::vector<std::string> *table_names, bool rehash_all) {
  cancelled_ = false;
  default_schema_ = current_schema;

  object_names_.clear();
  object_dot_names_.clear();

  // cache schema names if not done yet
  if (schema_names_.empty() || rehash_all) {
    refresh_schema_cache(session);
  }

  if (!current_schema.empty() && !cancelled_) {
    // fetch table names, if one was not provided
    std::vector<std::string> schema_tables;
    if (table_names == nullptr) {
      table_names = &schema_tables;

      auto res = session->raw_execute_sql(
          shcore::sqlstring("show tables from !", 0) << current_schema);
      auto row = res->call("fetchOne", shcore::Argument_list());
      while (row && !cancelled_) {
        std::string table =
            row.as_object<mysqlsh::Row>()->get_member(0).as_string();
        object_names_.push_back(table);
        schema_tables.push_back(table);
        row = res->call("fetchOne", shcore::Argument_list());
      }
    }

    // fetch column names for each table
    for (const std::string &t : *table_names) {
      if (cancelled_) break;
      std::vector<std::string> column_names;
      auto res = session->raw_execute_sql(
          shcore::sqlstring("show columns from !.!", 0) << current_schema << t);
      auto row = res->call("fetchOne", shcore::Argument_list());
      while (row && !cancelled_) {
        std::string column =
            row.as_object<mysqlsh::Row>()->get_member(0).as_string();
        // FIXME add quoting
        object_names_.push_back(column);
        object_dot_names_.push_back(t + "." + column);
        row = res->call("fetchOne", shcore::Argument_list());
      }
    }
    std::sort(object_names_.begin(), object_names_.end(),
              [](const std::string &a, const std::string &b) -> bool {
                return shcore::str_casecmp(a, b) < 0;
              });
    auto last = std::unique(object_names_.begin(), object_names_.end());
    object_names_.erase(last, object_names_.end());

    std::sort(object_dot_names_.begin(), object_dot_names_.end(),
              [](const std::string &a, const std::string &b) -> bool {
                return shcore::str_casecmp(a, b) < 0;
              });
    last = std::unique(object_dot_names_.begin(), object_dot_names_.end());
    object_dot_names_.erase(last, object_dot_names_.end());
  }
}

namespace {
// TODO refresh from latest 8.0
/* generated 2006-12-28.  Refresh occasionally from lexer. */
std::vector<std::string> k_sorted_keywords = {"ABS",
                                              "ACOS",
                                              "ACTION",
                                              "ADD",
                                              "ADDDATE",
                                              "ADDTIME",
                                              "AES_DECRYPT",
                                              "AES_ENCRYPT",
                                              "AFTER",
                                              "AGAINST",
                                              "AGGREGATE",
                                              "ALGORITHM",
                                              "ALL",
                                              "ALTER",
                                              "ANALYZE",
                                              "AND",
                                              "ANY",
                                              "AREA",
                                              "AS",
                                              "ASBINARY",
                                              "ASC",
                                              "ASCII",
                                              "ASENSITIVE",
                                              "ASIN",
                                              "ASTEXT",
                                              "ASWKB",
                                              "ASWKT",
                                              "ATAN",
                                              "ATAN2",
                                              "AUTO_INCREMENT",
                                              "AVG_ROW_LENGTH",
                                              "AVG",
                                              "BACKUP",
                                              "BDB",
                                              "BEFORE",
                                              "BEGIN",
                                              "BENCHMARK",
                                              "BERKELEYDB",
                                              "BETWEEN",
                                              "BIGINT",
                                              "BIN",
                                              "BINARY",
                                              "BINLOG",
                                              "BIT_AND",
                                              "BIT_COUNT",
                                              "BIT_LENGTH",
                                              "BIT_OR",
                                              "BIT_XOR",
                                              "BIT",
                                              "BLOB",
                                              "BOOL",
                                              "BOOLEAN",
                                              "BOTH",
                                              "BTREE",
                                              "BY",
                                              "BYTE",
                                              "CACHE",
                                              "CALL",
                                              "CASCADE",
                                              "CASCADED",
                                              "CASE",
                                              "CAST",
                                              "CEIL",
                                              "CEILING",
                                              "CENTROID",
                                              "CHAIN",
                                              "CHANGE",
                                              "CHANGED",
                                              "CHAR_LENGTH",
                                              "CHAR",
                                              "CHARACTER_LENGTH",
                                              "CHARACTER",
                                              "CHARSET",
                                              "CHECK",
                                              "CHECKSUM",
                                              "CIPHER",
                                              "CLIENT",
                                              "CLOSE",
                                              "COALESCE",
                                              "CODE",
                                              "COERCIBILITY",
                                              "COLLATE",
                                              "COLLATION",
                                              "COLUMN",
                                              "COLUMNS",
                                              "COMMENT",
                                              "COMMIT",
                                              "COMMITTED",
                                              "COMPACT",
                                              "COMPRESS",
                                              "COMPRESSED",
                                              "CONCAT_WS",
                                              "CONCAT",
                                              "CONCURRENT",
                                              "CONDITION",
                                              "CONNECTION_ID",
                                              "CONNECTION",
                                              "CONSISTENT",
                                              "CONSTRAINT",
                                              "CONTAINS",
                                              "CONTINUE",
                                              "CONV",
                                              "CONVERT_TZ",
                                              "CONVERT",
                                              "COS",
                                              "COT",
                                              "COUNT",
                                              "CRC32",
                                              "CREATE",
                                              "CROSS",
                                              "CROSSES",
                                              "CUBE",
                                              "CURDATE",
                                              "CURRENT_DATE",
                                              "CURRENT_TIME",
                                              "CURRENT_TIMESTAMP",
                                              "CURRENT_USER",
                                              "CURSOR",
                                              "CURTIME",
                                              "DATA",
                                              "DATABASE",
                                              "DATABASES",
                                              "DATE_ADD",
                                              "DATE_FORMAT",
                                              "DATE_SUB",
                                              "DATE",
                                              "DATEDIFF",
                                              "DATETIME",
                                              "DAY_HOUR",
                                              "DAY_MICROSECOND",
                                              "DAY_MINUTE",
                                              "DAY_SECOND",
                                              "DAY",
                                              "DAYNAME",
                                              "DAYOFMONTH",
                                              "DAYOFWEEK",
                                              "DAYOFYEAR",
                                              "DEALLOCATE",
                                              "DEC",
                                              "DECIMAL",
                                              "DECLARE",
                                              "DECODE",
                                              "DEFAULT",
                                              "DEFINER",
                                              "DEGREES",
                                              "DELAY_KEY_WRITE",
                                              "DELAYED",
                                              "DELETE",
                                              "DES_DECRYPT",
                                              "DES_ENCRYPT",
                                              "DES_KEY_FILE",
                                              "DESC",
                                              "DESCRIBE",
                                              "DETERMINISTIC",
                                              "DIMENSION",
                                              "DIRECTORY",
                                              "DISABLE",
                                              "DISCARD",
                                              "DISJOINT",
                                              "DISTINCT",
                                              "DISTINCTROW",
                                              "DIV",
                                              "DO",
                                              "DOUBLE",
                                              "DROP",
                                              "DUAL",
                                              "DUMPFILE",
                                              "DUPLICATE",
                                              "DYNAMIC",
                                              "EACH",
                                              "ELSE",
                                              "ELSEIF",
                                              "ELT",
                                              "ENABLE",
                                              "ENCLOSED",
                                              "ENCODE",
                                              "ENCRYPT",
                                              "END",
                                              "ENDPOINT",
                                              "ENGINE",
                                              "ENGINES",
                                              "ENUM",
                                              "ENVELOPE",
                                              "EQUALS",
                                              "ERRORS",
                                              "ESCAPE",
                                              "ESCAPED",
                                              "EVENTS",
                                              "EXECUTE",
                                              "EXISTS",
                                              "EXIT",
                                              "EXP",
                                              "EXPANSION",
                                              "EXPLAIN",
                                              "EXPORT_SET",
                                              "EXTENDED",
                                              "EXTERIORRING",
                                              "EXTRACT",
                                              "FALSE",
                                              "FAST",
                                              "FETCH",
                                              "FIELD",
                                              "FIELDS",
                                              "FILE",
                                              "FIND_IN_SET",
                                              "FIRST",
                                              "FIXED",
                                              "FLOAT",
                                              "FLOAT4",
                                              "FLOAT8",
                                              "FLOOR",
                                              "FLUSH",
                                              "FOR",
                                              "FORCE",
                                              "FOREIGN",
                                              "FORMAT",
                                              "FOUND_ROWS",
                                              "FOUND",
                                              "FROM_DAYS",
                                              "FROM_UNIXTIME",
                                              "FROM",
                                              "FULL",
                                              "FULLTEXT",
                                              "FUNCTION",
                                              "GEOMCOLLFROMTEXT",
                                              "GEOMCOLLFROMWKB",
                                              "GEOMETRY",
                                              "GEOMETRYCOLLECTION",
                                              "GEOMETRYCOLLECTIONFROMTEXT",
                                              "GEOMETRYCOLLECTIONFROMWKB",
                                              "GEOMETRYFROMTEXT",
                                              "GEOMETRYFROMWKB",
                                              "GEOMETRYN",
                                              "GEOMETRYTYPE",
                                              "GEOMFROMTEXT",
                                              "GEOMFROMWKB",
                                              "GET_FORMAT",
                                              "GET_LOCK",
                                              "GLENGTH",
                                              "GLOBAL",
                                              "GRANT",
                                              "GRANTS",
                                              "GREATEST",
                                              "GROUP_CONCAT",
                                              "GROUP_UNIQUE_USERS",
                                              "GROUP",
                                              "HANDLER",
                                              "HASH",
                                              "HAVING",
                                              "HELP",
                                              "HEX",
                                              "HIGH_PRIORITY",
                                              "HOSTS",
                                              "HOUR_MICROSECOND",
                                              "HOUR_MINUTE",
                                              "HOUR_SECOND",
                                              "HOUR",
                                              "IDENTIFIED",
                                              "IF",
                                              "IFNULL",
                                              "IGNORE",
                                              "IMPORT",
                                              "IN",
                                              "INDEX",
                                              "INDEXES",
                                              "INET_ATON",
                                              "INET_NTOA",
                                              "INFILE",
                                              "INNER",
                                              "INNOBASE",
                                              "INNODB",
                                              "INOUT",
                                              "INSENSITIVE",
                                              "INSERT_METHOD",
                                              "INSERT",
                                              "INSTR",
                                              "INT",
                                              "INT1",
                                              "INT2",
                                              "INT3",
                                              "INT4",
                                              "INT8",
                                              "INTEGER",
                                              "INTERIORRINGN",
                                              "INTERSECTS",
                                              "INTERVAL",
                                              "INTO",
                                              "INVOKER",
                                              "IO_THREAD",
                                              "IS_FREE_LOCK",
                                              "IS_USED_LOCK",
                                              "IS",
                                              "ISCLOSED",
                                              "ISEMPTY",
                                              "ISNULL",
                                              "ISOLATION",
                                              "ISSIMPLE",
                                              "ISSUER",
                                              "ITERATE",
                                              "JOIN",
                                              "JSON_ARRAY_APPEND",
                                              "JSON_ARRAY",
                                              "JSON_CONTAINS_PATH",
                                              "JSON_CONTAINS",
                                              "JSON_DEPTH",
                                              "JSON_EXTRACT",
                                              "JSON_INSERT",
                                              "JSON_KEYS",
                                              "JSON_LENGTH",
                                              "JSON_MERGE",
                                              "JSON_QUOTE",
                                              "JSON_REPLACE",
                                              "JSON_ROWOBJECT",
                                              "JSON_SEARCH",
                                              "JSON_SET",
                                              "JSON_TYPE",
                                              "JSON_UNQUOTE",
                                              "JSON_VALID",
                                              "KEY",
                                              "KEYS",
                                              "KILL",
                                              "LANGUAGE",
                                              "LAST_DAY",
                                              "LAST_INSERT_ID",
                                              "LAST",
                                              "LCASE",
                                              "LEADING",
                                              "LEAST",
                                              "LEAVE",
                                              "LEAVES",
                                              "LEFT",
                                              "LENGTH",
                                              "LEVEL",
                                              "LIKE",
                                              "LIMIT",
                                              "LINEFROMTEXT",
                                              "LINEFROMWKB",
                                              "LINES",
                                              "LINESTRING",
                                              "LINESTRINGFROMTEXT",
                                              "LINESTRINGFROMWKB",
                                              "LN",
                                              "LOAD_FILE",
                                              "LOAD",
                                              "LOCAL",
                                              "LOCALTIME",
                                              "LOCALTIMESTAMP",
                                              "LOCATE",
                                              "LOCK",
                                              "LOCKS",
                                              "LOG",
                                              "LOG10",
                                              "LOG2",
                                              "LOGS",
                                              "LONG",
                                              "LONGBLOB",
                                              "LONGTEXT",
                                              "LOOP",
                                              "LOW_PRIORITY",
                                              "LOWER",
                                              "LPAD",
                                              "LTRIM",
                                              "MAKE_SET",
                                              "MAKEDATE",
                                              "MAKETIME",
                                              "MASTER_CONNECT_RETRY",
                                              "MASTER_HOST",
                                              "MASTER_LOG_FILE",
                                              "MASTER_LOG_POS",
                                              "MASTER_PASSWORD",
                                              "MASTER_PORT",
                                              "MASTER_POS_WAIT",
                                              "MASTER_SERVER_ID",
                                              "MASTER_SSL_CA",
                                              "MASTER_SSL_CAPATH",
                                              "MASTER_SSL_CERT",
                                              "MASTER_SSL_CIPHER",
                                              "MASTER_SSL_KEY",
                                              "MASTER_SSL",
                                              "MASTER_TLS_VERSION",
                                              "MASTER_USER",
                                              "MASTER",
                                              "MATCH",
                                              "MAX_CONNECTIONS_PER_HOUR",
                                              "MAX_QUERIES_PER_HOUR",
                                              "MAX_ROWS",
                                              "MAX_UPDATES_PER_HOUR",
                                              "MAX_USER_CONNECTIONS",
                                              "MAX",
                                              "MBRCONTAINS",
                                              "MBRDISJOINT",
                                              "MBREQUAL",
                                              "MBRINTERSECTS",
                                              "MBROVERLAPS",
                                              "MBRTOUCHES",
                                              "MBRWITHIN",
                                              "MD5",
                                              "MEDIUM",
                                              "MEDIUMBLOB",
                                              "MEDIUMINT",
                                              "MEDIUMTEXT",
                                              "MERGE",
                                              "MICROSECOND",
                                              "MID",
                                              "MIDDLEINT",
                                              "MIGRATE",
                                              "MIN_ROWS",
                                              "MIN",
                                              "MINUTE_MICROSECOND",
                                              "MINUTE_SECOND",
                                              "MINUTE",
                                              "MLINEFROMTEXT",
                                              "MLINEFROMWKB",
                                              "MOD",
                                              "MODE",
                                              "MODIFIES",
                                              "MODIFY",
                                              "MONTH",
                                              "MONTHNAME",
                                              "MPOINTFROMTEXT",
                                              "MPOINTFROMWKB",
                                              "MPOLYFROMTEXT",
                                              "MPOLYFROMWKB",
                                              "MULTILINESTRING",
                                              "MULTILINESTRINGFROMTEXT",
                                              "MULTILINESTRINGFROMWKB",
                                              "MULTIPOINT",
                                              "MULTIPOINTFROMTEXT",
                                              "MULTIPOINTFROMWKB",
                                              "MULTIPOLYGON",
                                              "MULTIPOLYGONFROMTEXT",
                                              "MULTIPOLYGONFROMWKB",
                                              "MUTEX",
                                              "NAME_CONST",
                                              "NAME",
                                              "NAMES",
                                              "NATIONAL",
                                              "NATURAL",
                                              "NCHAR",
                                              "NDB",
                                              "NDBCLUSTER",
                                              "NEW",
                                              "NEXT",
                                              "NO_WRITE_TO_BINLOG",
                                              "NO",
                                              "NONE",
                                              "NOT",
                                              "NOW",
                                              "NULL",
                                              "NULLIF",
                                              "NUMERIC",
                                              "NUMGEOMETRIES",
                                              "NUMINTERIORRINGS",
                                              "NUMPOINTS",
                                              "NVARCHAR",
                                              "OCT",
                                              "OCTET_LENGTH",
                                              "OFFSET",
                                              "ON",
                                              "ONE_SHOT",
                                              "ONE",
                                              "OPEN",
                                              "OPTIMIZE",
                                              "OPTION",
                                              "OPTIONALLY",
                                              "OR",
                                              "ORD",
                                              "ORDER",
                                              "OUT",
                                              "OUTER",
                                              "OUTFILE",
                                              "OVERLAPS",
                                              "PACK_KEYS",
                                              "PARTIAL",
                                              "PASSWORD",
                                              "PERIOD_ADD",
                                              "PERIOD_DIFF",
                                              "PHASE",
                                              "PI",
                                              "POINT",
                                              "POINTFROMTEXT",
                                              "POINTFROMWKB",
                                              "POINTN",
                                              "POLYFROMTEXT",
                                              "POLYFROMWKB",
                                              "POLYGON",
                                              "POLYGONFROMTEXT",
                                              "POLYGONFROMWKB",
                                              "POSITION",
                                              "POW",
                                              "POWER",
                                              "PRECISION",
                                              "PREPARE",
                                              "PREV",
                                              "PRIMARY",
                                              "PRIVILEGES",
                                              "PROCEDURE",
                                              "PROCESS",
                                              "PROCESSLIST",
                                              "PURGE",
                                              "QUARTER",
                                              "QUERY",
                                              "QUICK",
                                              "QUOTE",
                                              "RADIANS",
                                              "RAND",
                                              "READ",
                                              "READS",
                                              "REAL",
                                              "RECOVER",
                                              "REDUNDANT",
                                              "REFERENCES",
                                              "REGEXP",
                                              "RELAY_LOG_FILE",
                                              "RELAY_LOG_POS",
                                              "RELAY_THREAD",
                                              "RELEASE_LOCK",
                                              "RELEASE",
                                              "RELOAD",
                                              "RENAME",
                                              "REPAIR",
                                              "REPEAT",
                                              "REPEATABLE",
                                              "REPLACE",
                                              "REPLICATION",
                                              "REQUIRE",
                                              "RESET",
                                              "RESTORE",
                                              "RESTRICT",
                                              "RESUME",
                                              "RETURN",
                                              "RETURNS",
                                              "REVERSE",
                                              "REVOKE",
                                              "RIGHT",
                                              "RLIKE",
                                              "ROLLBACK",
                                              "ROLLUP",
                                              "ROUND",
                                              "ROUTINE",
                                              "ROW_COUNT",
                                              "ROW_FORMAT",
                                              "ROW",
                                              "ROWS",
                                              "RPAD",
                                              "RTREE",
                                              "RTRIM",
                                              "SAVEPOINT",
                                              "SCHEMA",
                                              "SCHEMAS",
                                              "SEC_TO_TIME",
                                              "SECOND_MICROSECOND",
                                              "SECOND",
                                              "SECURITY",
                                              "SELECT",
                                              "SENSITIVE",
                                              "SEPARATOR",
                                              "SERIAL",
                                              "SERIALIZABLE",
                                              "SESSION_USER",
                                              "SESSION",
                                              "SET",
                                              "SHA",
                                              "SHA1",
                                              "SHARE",
                                              "SHOW",
                                              "SHUTDOWN",
                                              "SIGN",
                                              "SIGNED",
                                              "SIMPLE",
                                              "SIN",
                                              "SLAVE",
                                              "SLEEP",
                                              "SMALLINT",
                                              "SNAPSHOT",
                                              "SOME",
                                              "SONAME",
                                              "SOUNDEX",
                                              "SOUNDS",
                                              "SPACE",
                                              "SPATIAL",
                                              "SPECIFIC",
                                              "SQL_BIG_RESULT",
                                              "SQL_BUFFER_RESULT",
                                              "SQL_CACHE",
                                              "SQL_CALC_FOUND_ROWS",
                                              "SQL_NO_CACHE",
                                              "SQL_SMALL_RESULT",
                                              "SQL_THREAD",
                                              "SQL_TSI_DAY",
                                              "SQL_TSI_HOUR",
                                              "SQL_TSI_MINUTE",
                                              "SQL_TSI_MONTH",
                                              "SQL_TSI_QUARTER",
                                              "SQL_TSI_SECOND",
                                              "SQL_TSI_WEEK",
                                              "SQL_TSI_YEAR",
                                              "SQL",
                                              "SQLEXCEPTION",
                                              "SQLSTATE",
                                              "SQLWARNING",
                                              "SQRT",
                                              "SRID",
                                              "SSL",
                                              "START",
                                              "STARTING",
                                              "STARTPOINT",
                                              "STATUS",
                                              "STD",
                                              "STDDEV_POP",
                                              "STDDEV_SAMP",
                                              "STDDEV",
                                              "STOP",
                                              "STORAGE",
                                              "STR_TO_DATE",
                                              "STRAIGHT_JOIN",
                                              "STRCMP",
                                              "STRING",
                                              "STRIPED",
                                              "SUBDATE",
                                              "SUBJECT",
                                              "SUBSTR",
                                              "SUBSTRING_INDEX",
                                              "SUBSTRING",
                                              "SUBTIME",
                                              "SUM",
                                              "SUPER",
                                              "SUSPEND",
                                              "SYSDATE",
                                              "SYSTEM_USER",
                                              "TABLE",
                                              "TABLES",
                                              "TABLESPACE",
                                              "TAN",
                                              "TEMPORARY",
                                              "TEMPTABLE",
                                              "TERMINATED",
                                              "TEXT",
                                              "THEN",
                                              "TIME_FORMAT",
                                              "TIME_TO_SEC",
                                              "TIME",
                                              "TIMEDIFF",
                                              "TIMESTAMP",
                                              "TIMESTAMPADD",
                                              "TIMESTAMPDIFF",
                                              "TINYBLOB",
                                              "TINYINT",
                                              "TINYTEXT",
                                              "TO_DAYS",
                                              "TO",
                                              "TOUCHES",
                                              "TRAILING",
                                              "TRANSACTION",
                                              "TRIGGER",
                                              "TRIGGERS",
                                              "TRIM",
                                              "TRUE",
                                              "TRUNCATE",
                                              "TYPE",
                                              "TYPES",
                                              "UCASE",
                                              "UNCOMMITTED",
                                              "UNCOMPRESS",
                                              "UNCOMPRESSED_LENGTH",
                                              "UNDEFINED",
                                              "UNDO",
                                              "UNHEX",
                                              "UNICODE",
                                              "UNION",
                                              "UNIQUE_USERS",
                                              "UNIQUE",
                                              "UNIX_TIMESTAMP",
                                              "UNKNOWN",
                                              "UNLOCK",
                                              "UNSIGNED",
                                              "UNTIL",
                                              "UPDATE",
                                              "UPGRADE",
                                              "UPPER",
                                              "USAGE",
                                              "USE_FRM",
                                              "USE",
                                              "USER_RESOURCES",
                                              "USER",
                                              "USING",
                                              "UTC_DATE",
                                              "UTC_TIME",
                                              "UTC_TIMESTAMP",
                                              "UUID",
                                              "VALUE",
                                              "VALUES",
                                              "VAR_POP",
                                              "VAR_SAMP",
                                              "VARBINARY",
                                              "VARCHAR",
                                              "VARCHARACTER",
                                              "VARIABLES",
                                              "VARIANCE",
                                              "VARYING",
                                              "VERSION",
                                              "VIEW",
                                              "WARNINGS",
                                              "WEEK",
                                              "WEEKDAY",
                                              "WEEKOFYEAR",
                                              "WHEN",
                                              "WHERE",
                                              "WHILE",
                                              "WITH",
                                              "WITHIN",
                                              "WORK",
                                              "WRITE",
                                              "X",
                                              "X509",
                                              "XA",
                                              "XOR",
                                              "Y",
                                              "YEAR_MONTH",
                                              "YEAR",
                                              "YEARWEEK",
                                              "ZEROFILL"};
}  // namespace

}  // namespace completer
}  // namespace shcore
