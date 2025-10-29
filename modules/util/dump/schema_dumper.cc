/*
 * Copyright (c) 2000, 2025, Oracle and/or its affiliates.
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

// Dump a table's contents and format to a text file.
// Adapted from mysqldump.cc

#include "include/mysh_config.h"

#include "modules/util/dump/schema_dumper.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "my_sys.h"
#include "mysql/strings/m_ctype.h"
#include "mysqld_error.h"

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/filtering_options.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/common/dump/constants.h"
#include "modules/util/common/dump/dump_version.h"
#include "modules/util/dump/dump_errors.h"

namespace mysqlsh {
namespace dump {

/**
  Name of the information schema database.
*/
#define INFORMATION_SCHEMA_DB_NAME "information_schema"

/**
  Name of the performance schema database.
*/
#define PERFORMANCE_SCHEMA_DB_NAME "performance_schema"

/**
  Name of the sys schema database.
*/
#define SYS_SCHEMA_DB_NAME "sys"

/* index into 'show fields from table' */

#define SHOW_FIELDNAME 0
#define SHOW_TYPE 1
#define SHOW_NULL 2
#define SHOW_DEFAULT 4
#define SHOW_EXTRA 5

/* Size of buffer for dump's select query */
#define QUERY_LENGTH 1536

/* Size of comment buffer. */
#define COMMENT_LENGTH 2048

/* ignore table flags */
#define IGNORE_NONE 0x00 /* no ignore */
#define IGNORE_DATA 0x01 /* don't dump data for this table */

#define MYSQL_UNIVERSAL_CLIENT_CHARSET "utf8mb4"

/* Maximum number of fields per table */
#define MAX_FIELDS 4000

namespace {

const std::size_t k_max_innodb_columns = 1017;

using mysqlshdk::utils::Version;

const std::unordered_set<std::string> k_system_schemas = {
    "information_schema",
    "mysql",
    "performance_schema",
    "sys",
};

const std::unordered_set<std::string> k_mhs_allowed_authentication_plugins = {
    "authentication_oci",
    "caching_sha2_password",
    "mysql_native_password",
    "sha256_password",
};

struct Version_info {
  Version deprecated;
  std::optional<Version> removed;
};

const std::unordered_map<std::string, Version_info>
    k_mhs_deprecated_authentication_plugins = {
        {"authentication_fido", {Version{8, 2, 0}, Version{8, 4, 0}}},
        {"mysql_native_password", {Version{8, 0, 34}, Version{9, 0, 0}}},
        {"sha256_password", {Version{8, 0, 16}, std::nullopt}},
};

[[maybe_unused]] void use_unsupported_collation(std::string *statement) {
  static constexpr std::string_view k_unsupported_collation =
      "utf8mb4_uca1400_polish_nopad_ai_cs";

  *statement = shcore::str_replace(*statement, "utf8mb4_pl_0900_as_cs",
                                   k_unsupported_collation);
  *statement = shcore::str_replace(*statement, "utf8mb4_0900_ai_ci",
                                   k_unsupported_collation);
}

/**
 * NOTE: this returns true if collation name is empty, this helps dealing with
 *       i.e. non-string columns which do not have collation set.
 */
bool is_supported_collation(std::string_view collation) {
  static const auto k_all_collations = []() {
    // dummy call, will initialize the all_charsets array
    [[maybe_unused]] const auto k_unused = get_charset(0, 0);

    std::unordered_set<std::string_view> collations;

    for (int i = 0; i < MY_ALL_CHARSETS_SIZE; ++i) {
      if (const auto charset = all_charsets[i]) {
        collations.emplace(charset->m_coll_name);
      }
    }

    return collations;
  }();

  if (collation.empty()) {
    return true;
  }

  if (shcore::str_beginswith(collation, "utf8_")) {
    // this is a 5.7 alias for utf8mb3_
    std::string alias = "utf8mb3_";
    alias += collation.substr(5);

    return k_all_collations.contains(alias);
  }

  return k_all_collations.contains(collation);
}

const std::string &map_collation(std::string_view collation) {
  static shcore::heterogeneous_map<std::string, std::string> k_collation_cache;
  static std::string k_unknown_collation;

#define RETURN_IF_FOUND(it) \
  if (const auto c = (it); k_collation_cache.end() != c) return c->second

  {
    const auto &needle =
#if __cpp_lib_generic_unordered_lookup
        collation
#else
        std::string{collation}
#endif
        ;

    RETURN_IF_FOUND(k_collation_cache.find(needle));
  }

  const auto candidate = [collation](std::string &&c) {
    if (is_supported_collation(c)) {
      return k_collation_cache.emplace(std::string{collation}, std::move(c))
          .first;
    } else {
      return k_collation_cache.end();
    }
  };

  const auto contains = [collation](std::string_view needle) {
    return std::string::npos != collation.find(needle);
  };

  if (contains("_nopad_")) {
    RETURN_IF_FOUND(candidate(shcore::str_replace(collation, "_nopad", "")));
  }

  if (contains("_mysql561_")) {
    RETURN_IF_FOUND(candidate(shcore::str_replace(collation, "_mysql561", "")));
  }

  if (contains("general1400_as_ci")) {
    RETURN_IF_FOUND(candidate(
        shcore::str_replace(collation, "general1400_as_ci", "general_ci")));
  }

  static constexpr std::string_view k_uca_1400 = "uca1400";

  if (contains(k_uca_1400)) {
    static constexpr std::string_view k_utf8mb4 = "utf8mb4";

    auto parts = shcore::str_split(collation, "_");
    std::size_t idx = 0;

    // erase 'nopad' part
    std::erase(parts, "nopad");

    // if collation does not begin with 'utf8mb4', insert it
    if (parts.size() > idx && k_utf8mb4 != parts[idx]) {
      parts.emplace(parts.begin() + idx, k_utf8mb4);
    }

    // move to the next element
    ++idx;

    // we should now be at 'uca1400', erase it
    if (parts.size() > idx && k_uca_1400 == parts[idx]) {
      parts.erase(parts.begin() + idx);

      std::string original_language;

      // if there are more than two parts left, we're now at the language name
      if (parts.size() > idx + 2) {
        static const std::unordered_map<std::string_view, std::string_view>
            k_language_to_subtag = {
                {"croatian", "hr"},
                {"czech", "cs"},
                {"danish", "da"},
                {"esperanto", "eo"},
                {"estonian", "et"},
                {"german2", "de_pb"},  // German phone book order
                {"hungarian", "hu"},
                {"icelandic", "is"},
                {"latvian", "lv"},
                {"lithuanian", "lt"},
                {"persian", "fa"},
                {"polish", "pl"},
                {"romanian", "ro"},
                {"roman", "la"},  // Classical Latin
                {"sinhala", "si"},
                {"slovak", "sk"},
                {"slovenian", "sl"},
                {"spanish2", "es_trad"},  // Traditional Spanish
                {"spanish", "es"},        // Modern Spanish
                {"swedish", "sv"},
                {"turkish", "tr"},
                {"vietnamese", "vi"},
            };

        original_language = std::move(parts[idx]);

        if (const auto it = k_language_to_subtag.find(original_language);
            k_language_to_subtag.end() != it) {
          // we have a mapping, use the subtag
          parts[idx] = it->second;
        } else {
          // unknown language, use the original value
          parts[idx] = original_language;
        }

        // move to the next element
        ++idx;
      }

      // insert the version number
      parts.emplace(parts.begin() + idx, "0900");

      RETURN_IF_FOUND(candidate(shcore::str_join(parts, "_")));

      // convert accent and case sensitivity pairs keeping the case sensitivity
      if (auto it = parts.rbegin(); "cs" == *it && "ai" == *++it) {
        *it = "as";
        RETURN_IF_FOUND(candidate(shcore::str_join(parts, "_")));
      }

      if (auto it = parts.rbegin(); "ci" == *it && "as" == *++it) {
        *it = "ai";
        RETURN_IF_FOUND(candidate(shcore::str_join(parts, "_")));
      }

      // last resort, try original language and case insensitive collation
      if (!original_language.empty()) {
        RETURN_IF_FOUND(candidate("utf8mb4_" + original_language + "_ci"));
      }
    }
  }

#undef RETURN_IF_FOUND

  return k_unknown_collation;
}

namespace detail {

void handle_collation(
    Compatibility_issue::Object_type object_type,
    const std::string &object_name, const char *collation_type,
    std::string_view collation,
    const std::function<void(std::string_view, std::string_view)>
        &replace_collation,
    std::vector<Compatibility_issue> *issues) {
  if (is_supported_collation(collation)) {
    return;
  }

  if (const auto &mapped = map_collation(collation); mapped.empty()) {
    issues->emplace_back(
        Compatibility_issue::error::object_unsupported_collation(
            object_type, object_name, collation_type, collation));
  } else {
    replace_collation(collation, mapped);

    issues->emplace_back(
        Compatibility_issue::warning::object_collation_replaced(
            object_type, object_name, collation_type, collation, mapped));
  }
}

}  // namespace detail

void handle_collation_update_statement(
    Compatibility_issue::Object_type object_type,
    const std::string &object_name, const char *collation_type,
    std::string_view collation, std::string *statement,
    std::vector<Compatibility_issue> *issues) {
  detail::handle_collation(
      object_type, object_name, collation_type, collation,
      [statement](std::string_view from, std::string_view to) {
        compatibility::replace_keyword(*statement, from, to, statement);
      },
      issues);
}

void handle_collation_update_variable(
    Compatibility_issue::Object_type object_type,
    const std::string &object_name, const char *collation_type,
    std::string *collation, std::vector<Compatibility_issue> *issues) {
  DBUG_EXECUTE_IF("dumper_unsupported_collation",
                  { use_unsupported_collation(collation); });

  std::string new_collation;

  detail::handle_collation(
      object_type, object_name, collation_type, *collation,
      [&new_collation](std::string_view, std::string_view to) {
        new_collation = to;
      },
      issues);

  if (!new_collation.empty()) {
    *collation = std::move(new_collation);
  }
}

inline bool is_system_schema(const std::string &s) {
  return k_system_schemas.end() != k_system_schemas.find(s);
}

inline bool is_system_schema_or_ndb(const std::string &s) {
  return is_system_schema(s) || "ndbinfo" == s;
}

/* general_log or slow_log tables under mysql database */
inline bool general_log_or_slow_log_tables(const std::string &db,
                                           const std::string &table) {
  return (db == "mysql") && (table == "general_log" || table == "slow_log");
}

/*
 slave_master_info,slave_relay_log_info and gtid_executed tables under
 mysql database
*/
inline bool replication_metadata_tables(const std::string &db,
                                        const std::string &table) {
  return (db == "mysql") &&
         (table == "slave_master_info" || table == "slave_relay_log_info" ||
          table == "gtid_executed");
}

/**
  Check if the table is innodb stats table in mysql database.

   @param [in] db           Database name
   @param [in] table        Table name

  @return
    @retval true if it is innodb stats table else false
*/
inline bool innodb_stats_tables(const std::string &db,
                                const std::string &table) {
  return (db == "mysql" &&
          (table == "innodb_table_stats" || table == "innodb_index_stats" ||
           table == "innodb_dynamic_metadata" || table == "innodb_ddl_log"));
}

void switch_db_collation(IFile *sql_file, const std::string &db_name,
                         const char *delimiter,
                         const std::string &current_db_cl_name,
                         const std::string &required_db_cl_name,
                         int *db_cl_altered) {
  if (current_db_cl_name != required_db_cl_name) {
    auto db_cl = get_charset_by_name(required_db_cl_name.c_str(), MYF(0));

    if (!db_cl) {
      THROW_ERROR(SHERR_DUMP_SD_CHARSET_NOT_FOUND, required_db_cl_name.c_str());
    }

    fprintf(sql_file, "ALTER DATABASE %s CHARACTER SET %s COLLATE %s %s\n",
            shcore::quote_identifier(db_name).c_str(), db_cl->csname,
            db_cl->m_coll_name, delimiter);

    *db_cl_altered = 1;
    return;
  }
  *db_cl_altered = 0;
}

void restore_db_collation(IFile *sql_file, const std::string &db_name,
                          const char *delimiter, const char *db_cl_name) {
  std::string quoted_db_name = shcore::quote_identifier(db_name);

  CHARSET_INFO *db_cl = get_charset_by_name(db_cl_name, MYF(0));

  if (!db_cl) {
    THROW_ERROR(SHERR_DUMP_SD_CHARSET_NOT_FOUND, db_cl_name);
  }

  fprintf(sql_file, "ALTER DATABASE %s CHARACTER SET %s COLLATE %s %s\n",
          quoted_db_name.c_str(), db_cl->csname, db_cl->m_coll_name, delimiter);
}

void switch_cs_variables(IFile *sql_file, const char *delimiter,
                         const char *character_set_client,
                         const char *character_set_results,
                         const char *collation_connection) {
  fprintf(sql_file,
          "/*!50003 SET @saved_cs_client      = @@character_set_client */ %s\n"
          "/*!50003 SET @saved_cs_results     = @@character_set_results */ %s\n"
          "/*!50003 SET @saved_col_connection = @@collation_connection */ %s\n"
          "/*!50003 SET character_set_client  = %s */ %s\n"
          "/*!50003 SET character_set_results = %s */ %s\n"
          "/*!50003 SET collation_connection  = %s */ %s\n",
          delimiter, delimiter, delimiter,

          character_set_client, delimiter,

          character_set_results, delimiter,

          collation_connection, delimiter);
}

void restore_cs_variables(IFile *sql_file, const char *delimiter) {
  fprintf(sql_file,
          "/*!50003 SET character_set_client  = @saved_cs_client */ %s\n"
          "/*!50003 SET character_set_results = @saved_cs_results */ %s\n"
          "/*!50003 SET collation_connection  = @saved_col_connection */ %s\n",
          (const char *)delimiter, (const char *)delimiter,
          (const char *)delimiter);
}

void switch_sql_mode(IFile *sql_file, const char *delimiter,
                     const char *sql_mode) {
  fprintf(sql_file,
          "/*!50003 SET @saved_sql_mode       = @@sql_mode */ %s\n"
          "/*!50003 SET sql_mode              = '%s' */ %s\n",
          (const char *)delimiter,

          (const char *)sql_mode, (const char *)delimiter);
}

void restore_sql_mode(IFile *sql_file, const char *delimiter) {
  fprintf(sql_file,
          "/*!50003 SET sql_mode              = @saved_sql_mode */ %s\n",
          (const char *)delimiter);
}

void switch_time_zone(IFile *sql_file, const char *delimiter,
                      const char *time_zone) {
  fprintf(sql_file,
          "/*!50003 SET @saved_time_zone      = @@time_zone */ %s\n"
          "/*!50003 SET time_zone             = '%s' */ %s\n",
          (const char *)delimiter,

          (const char *)time_zone, (const char *)delimiter);
}

void restore_time_zone(IFile *sql_file, const char *delimiter) {
  fprintf(sql_file,
          "/*!50003 SET time_zone             = @saved_time_zone */ %s\n",
          (const char *)delimiter);
}

void check_io(IFile *file) {
  if (file->error() || errno == 5) {
    THROW_ERROR(SHERR_DUMP_SD_WRITE_FAILED, errno);
  }
}

std::string fixup_event_ddl(const std::string &create_event) {
  // CREATE DEFINER=`root`@`localhost` EVENT `event_name` ON ...
  // -->
  // CREATE DEFINER=`root`@`localhost` EVENT IF NOT EXISTS `event_name` ON ...
  const char *ddl = create_event.c_str();
  size_t skip = 0;

  skip += 7;  // skip CREATE
  skip += 8;  // skip DEFINER=
  // skip `user`
  if (ddl[skip] == '`')
    skip = mysqlshdk::utils::span_quoted_sql_identifier_bt(ddl, skip);
  else if (ddl[skip] == '"')
    skip = mysqlshdk::utils::span_quoted_sql_identifier_dquote(ddl, skip);
  else
    throw std::logic_error("CREATE EVENT in unexpected format: " +
                           create_event);
  skip++;  // skip @
  // skip `host`
  if (ddl[skip] == '`')
    skip = mysqlshdk::utils::span_quoted_sql_identifier_bt(ddl, skip);
  else if (ddl[skip] == '"')
    skip = mysqlshdk::utils::span_quoted_sql_identifier_dquote(ddl, skip);
  else
    throw std::logic_error("CREATE EVENT in unexpected format: " +
                           create_event);

  skip += 7;  // skip _EVENT_

  return create_event.substr(0, skip) + "IF NOT EXISTS " +
         create_event.substr(skip);
}

/**
  @brief Accepts object names and prefixes them with "-- " wherever
         end-of-line character ('\n') is found.

  @param[in]  object_name   object name list (concatenated string)
  @param[out] freemem       should buffer be released after usage

  @return
    @retval                 pointer to a string with prefixed objects
*/
std::string fix_identifier_with_newline(const std::string &object_name) {
  return shcore::str_replace(object_name, "\n", "\n-- ");
}

class Object_guard_msg {
 public:
  Object_guard_msg(IFile *file, std::string_view object_type,
                   const std::string &db, const std::string &obj_name)
      : m_file(file),
        m_msg(fix_identifier_with_newline(
            shcore::str_lower(object_type) + " " +
            shcore::quote_identifier(db) +
            (obj_name.empty() ? "" : "." + obj_name))) {
    fputs("-- begin " + m_msg + "\n", m_file);
  }
  ~Object_guard_msg() { fputs("-- end " + m_msg + "\n\n", m_file); }

 private:
  IFile *m_file;
  const std::string m_msg;
};

std::string quote(const std::string &db, const std::string &object) {
  return shcore::quote_identifier(db) + "." + shcore::quote_identifier(object);
}

std::string quote(const std::string &db, const std::string &table,
                  const std::string &object) {
  return quote(db, table) + "." + shcore::quote_identifier(object);
}

void fetch_warnings(
    const std::shared_ptr<mysqlshdk::db::IResult> &result,
    std::vector<std::unique_ptr<mysqlshdk::db::Warning>> *out_warnings) {
  assert(out_warnings);

  if (const auto count = result->get_warning_count()) {
    for (std::size_t i = 0; i < count; ++i) {
      out_warnings->emplace_back(result->fetch_one_warning());
    }
  }
}

}  // namespace

void Schema_dumper::write_header(IFile *sql_file) {
  if (!opt_compact) {
    if (opt_set_charset)
      fprintf(
          sql_file,
          "\n/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;"
          "\n/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS "
          "*/;"
          "\n/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;"
          "\n/*!50503 SET NAMES %s */;\n",
          default_charset);

    if (opt_tz_utc) {
      fputs("/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;\n", sql_file);
      fputs("/*!40103 SET TIME_ZONE='+00:00' */;\n", sql_file);
    }
    if (stats_tables_included) {
      fputs(
          "/*!50606 SET "
          "@OLD_INNODB_STATS_AUTO_RECALC=@@INNODB_STATS_AUTO_RECALC */;\n",
          sql_file);
      fputs("/*!50606 SET GLOBAL INNODB_STATS_AUTO_RECALC=OFF */;\n", sql_file);
    }
    //       if (!path) {
    fputs(
        "/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 "
        "*/;\n",
        sql_file);
    fputs(
        "/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, "
        "FOREIGN_KEY_CHECKS=0 */;\n",
        sql_file);
    //       }
    const char *mode1 =
        "NO_AUTO_VALUE_ON_ZERO";  // path ? "" : "NO_AUTO_VALUE_ON_ZERO";
    const char *mode2 = opt_ansi_mode ? "ANSI" : "";
    const char *comma = *mode1 && *mode2 ? "," : "";
    fprintf(sql_file,
            "/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='%s%s%s' */;\n"
            "/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;\n",
            mode1, comma, mode2);
    check_io(sql_file);
  }
} /* write_header */

void Schema_dumper::write_footer(IFile *sql_file) {
  if (!opt_compact) {
    if (opt_tz_utc)
      fputs("/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;\n", sql_file);
    if (stats_tables_included)
      fputs(
          "/*!50606 SET GLOBAL "
          "INNODB_STATS_AUTO_RECALC=@OLD_INNODB_STATS_AUTO_RECALC */;\n",
          sql_file);

    fputs("\n/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;\n", sql_file);
    //       if (!path) {
    fputs("/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;\n",
          sql_file);
    fputs("/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;\n", sql_file);
    //       }
    if (opt_set_charset)
      fputs(
          "/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;\n"
          "/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;\n"
          "/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;\n",
          sql_file);
    fputs("/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;\n", sql_file);
    fputs("\n", sql_file);

    check_io(sql_file);
  }
} /* write_footer */

void Schema_dumper::write_comment(IFile *sql_file, const std::string &db_name,
                                  const std::string &table_name) {
  if (!opt_compact) {
    print_comment(sql_file, false,
                  "-- MySQLShell dump %s  Distrib %s, for %s (%s)\n--\n",
                  common::k_dumper_version, shcore::get_long_version(),
                  SYSTEM_TYPE, MACHINE_TYPE);

    std::string host;
    const auto &co = m_mysql->get_connection_options();

    if (co.has_host()) {
      host = co.get_host();
    }

    print_comment(sql_file, false, "-- Host: %s",
                  host.empty() ? "localhost" : host.c_str());

    if (!db_name.empty()) {
      print_comment(sql_file, false, "    Database: %s",
                    fix_identifier_with_newline(db_name).c_str());
    }

    if (!table_name.empty()) {
      print_comment(sql_file, false, "    Table: %s",
                    fix_identifier_with_newline(table_name).c_str());
    }

    print_comment(sql_file, false, "\n");

    print_comment(
        sql_file, false,
        "-- ------------------------------------------------------\n");
    print_comment(sql_file, false, "-- Server version\t%s\n",
                  m_mysql->get_server_version().get_full().c_str());
  }
}

int Schema_dumper::execute_no_throw(const std::string &s,
                                    mysqlshdk::db::Error *out_error) {
  try {
    m_mysql->execute(s);
  } catch (const mysqlshdk::db::Error &e) {
    if (out_error) *out_error = e;

    current_console()->print_error("Could not execute '" + s +
                                   "': " + e.format());
    return 1;
  }
  return 0;
}

int Schema_dumper::execute_maybe_throw(const std::string &s,
                                       mysqlshdk::db::Error *out_error) {
  try {
    m_mysql->execute(s);
  } catch (const mysqlshdk::db::Error &e) {
    if (out_error) *out_error = e;

    current_console()->print_error("Could not execute '" + s +
                                   "': " + e.format());
    if (opt_force)
      return 1;
    else
      throw;
  }
  return 0;
}

int Schema_dumper::query_no_throw(
    const std::string &s, std::shared_ptr<mysqlshdk::db::IResult> *out_result,
    mysqlshdk::db::Error *out_error) {
  try {
    *out_result = m_mysql->query(s);
    return 0;
  } catch (const mysqlshdk::db::Error &e) {
    if (out_error) *out_error = e;

    current_console()->print_error("Could not execute '" + s +
                                   "': " + e.format());
    return 1;
  }
}

std::shared_ptr<mysqlshdk::db::IResult> Schema_dumper::query_log_and_throw(
    const std::string &s) {
  try {
    return m_mysql->query(s);
  } catch (const mysqlshdk::db::Error &e) {
    current_console()->print_error("Could not execute '" + s +
                                   "': " + e.format());
    THROW_ERROR(SHERR_DUMP_SD_QUERY_FAILED, s.c_str(), e.format().c_str());
  }
}

std::shared_ptr<mysqlshdk::db::IResult> Schema_dumper::query_log_error(
    const std::string &sql, const std::string &schema,
    const std::string &table) const {
  return m_mysql->queryf(sql, schema, table);
}

int Schema_dumper::query_with_binary_charset(
    const std::string &s, std::shared_ptr<mysqlshdk::db::IResult> *out_result,
    mysqlshdk::db::Error *out_error,
    std::vector<std::unique_ptr<mysqlshdk::db::Warning>> *out_warnings) {
  switch_character_set_results("binary");
  if (query_no_throw(s, out_result, out_error)) return 1;
  (*out_result)->buffer();

  if (out_warnings) {
    fetch_warnings(*out_result, out_warnings);
  }

  switch_character_set_results(opt_character_set_results.c_str());
  return 0;
}

void Schema_dumper::fetch_db_collation(const std::string &db,
                                       std::string *out_db_cl_name,
                                       bool replace_unknown_collation) {
  if (m_cache) {
    *out_db_cl_name = m_cache->schemas.at(db).default_collation;
  } else {
    bool err_status = false;

    use(db);
    auto db_cl_res = query_log_and_throw("select @@collation_database");

    do {
      auto row = db_cl_res->fetch_one();
      if (!row) {
        err_status = true;
        break;
      }

      *out_db_cl_name = row->get_string(0);

      if (db_cl_res->fetch_one() != nullptr) {
        err_status = true;
        break;
      }
    } while (false);

    if (err_status) {
      THROW_ERROR(SHERR_DUMP_SD_COLLATION_DATABASE_ERROR);
    }
  }

  // BUG#38089433 - if database is using an unknown collation we need to replace
  // it with the mapped one in order to keep collations in sync
  if (replace_unknown_collation && !is_supported_collation(*out_db_cl_name)) {
    // don't update the collation if we don't know how to map it,
    // switch_db_collation() will later throw due to unknown collation
    if (const auto &mapped = map_collation(*out_db_cl_name); !mapped.empty()) {
      *out_db_cl_name = mapped;
    }
  }
}

/**
  Switch charset for results to some specified charset.  If the server does
  not support character_set_results variable, nothing can be done here.  As
  for whether something should be done here, future new callers of this
  function should be aware that the server lacking the facility of switching
  charsets is treated as success.

  @note  If the server lacks support, then nothing is changed and no error
         condition is returned.

  @returns  whether there was an error or not
*/
void Schema_dumper::switch_character_set_results(const char *cs_name) {
  try {
    m_mysql->executef("SET SESSION character_set_results = ?", cs_name);
  } catch (const mysqlshdk::db::Error &e) {
    THROW_ERROR(SHERR_DUMP_SD_CHARACTER_SET_RESULTS_ERROR, cs_name);
  }
}

void Schema_dumper::use(const std::string &db) const {
  m_mysql->executef("USE !", db);
}

void Schema_dumper::unescape(IFile *file, std::string_view s) {
  std::string tmp = m_mysql->escape_string(s);

  fputs("'", file);
  fputs(tmp.c_str(), file);
  fputs("'", file);
  check_io(file);
} /* unescape */

/*
  Quote a table name so it can be used in "SHOW TABLES LIKE <tabname>"

  SYNOPSIS
    quote_for_like()
    name     name of the table
    buff     quoted name of the table

  DESCRIPTION
    Quote \, _, ' and % characters

    Note: Because MySQL uses the C escape syntax in strings
    (for example, '\n' to represent newline), you must double
    any '\' that you use in your LIKE  strings. For example, to
    search for '\n', specify it as '\\n'. To search for '\', specify
    it as '\\\\' (the backslashes are stripped once by the parser
    and another time when the pattern match is done, leaving a
    single backslash to be matched).

    Example: "t\1" => "t\\\\1"

*/
std::string Schema_dumper::quote_for_like(const std::string &name_) {
  std::string buff;

  const char *name = &name_[0];
  buff += '\'';
  while (*name) {
    if (*name == '\\')
      buff += "\\\\\\";
    else if (*name == '\'' || *name == '_' || *name == '%')
      buff += '\\';
    buff += *name++;
  }
  buff += '\'';
  return buff;
}

/* A common printing function for xml and non-xml modes. */

void Schema_dumper::print_comment(IFile *sql_file, bool is_error,
                                  const char *format, ...) {
  char comment_buff[COMMENT_LENGTH];
  va_list args;

  /* If its an error message, print it ignoring opt_comments. */
  if (!is_error && !opt_comments) return;

  va_start(args, format);
  vsnprintf(comment_buff, COMMENT_LENGTH, format, args);
  va_end(args);

  fputs(comment_buff, sql_file);
  check_io(sql_file);
}

/*
 create_delimiter
 Generate a new (null-terminated) string that does not exist in  query
 and is therefore suitable for use as a query delimiter.  Store this
 delimiter in  delimiter_buff .

 This is quite simple in that it doesn't even try to parse statements as an
 interpreter would.  It merely returns a string that is not in the query,
 which is much more than adequate for constructing a delimiter.

 RETURN
   ptr to the delimiter  on Success
   NULL                  on Failure
*/
char *Schema_dumper::create_delimiter(const std::string &query,
                                      char *delimiter_buff,
                                      const int delimiter_max_size) {
  int proposed_length;
  const char *presence;

  delimiter_buff[0] = ';'; /* start with one semicolon, and */

  for (proposed_length = 2; proposed_length < delimiter_max_size;
       ++proposed_length) {
    delimiter_buff[proposed_length - 1] = ';'; /* add semicolons, until */
    delimiter_buff[proposed_length] = '\0';

    presence = strstr(&query[0], delimiter_buff);
    if (presence == nullptr) { /* the proposed delimiter is not in the query. */
      return delimiter_buff;
    }
  }
  return nullptr; /* but if we run out of space, return nothing at all. */
}

/*
  dump_events_for_db
  -- retrieves list of events for a given db, and prints out
  the CREATE EVENT statement into the output (the dump).

  RETURN
    List of issues found.
*/
std::vector<Compatibility_issue> Schema_dumper::dump_events_for_db(
    IFile *sql_file, const std::string &db) {
  std::vector<Compatibility_issue> res;
  char query_buff[QUERY_LENGTH];
  char delimiter[QUERY_LENGTH];
  std::string db_name;
  std::string db_cl_name;
  int db_cl_altered = false;

  db_name = m_mysql->escape_string(db);

  /* nice comments */
  std::string text = fix_identifier_with_newline(db);
  print_comment(sql_file, false,
                "\n--\n-- Dumping events for database '%s'\n--\n",
                text.c_str());

  const auto events = get_events(db);

  strcpy(delimiter, ";");
  if (!events.empty()) {
    fputs("/*!50106 SET @save_time_zone= @@TIME_ZONE */ ;\n\n", sql_file);

    /* Get database collation. */

    fetch_db_collation(db, &db_cl_name);

    switch_character_set_results("binary");

    for (const auto &event : events) {
      const auto qualified_name = quote(db, event);
      const auto event_name = shcore::quote_identifier(event);
      log_debug("retrieving CREATE EVENT for %s", event_name.c_str());
      snprintf(query_buff, sizeof(query_buff), "SHOW CREATE EVENT %s",
               qualified_name.c_str());

      auto event_res = query_log_and_throw(query_buff);

      while (auto row = event_res->fetch_one()) {
        /*
          if the user has EXECUTE privilege he can see event names, but not
          the event body!
        */
        std::string field3 = row->get_string(3);
        if (!field3.empty()) {
          Object_guard_msg guard(sql_file, "event", db, event_name);
          if (opt_drop_event)
            fprintf(sql_file, "/*!50106 DROP EVENT IF EXISTS %s */%s\n",
                    event_name.c_str(), delimiter);

          if (create_delimiter(field3.c_str(), delimiter, sizeof(delimiter)) ==
              nullptr) {
            fprintf(stderr, "Warning: Can't create delimiter for event %s\n",
                    qualified_name.c_str());
            THROW_ERROR(SHERR_DUMP_SD_CANNOT_CREATE_DELIMITER,
                        qualified_name.c_str());
          }

          fprintf(sql_file, "DELIMITER %s\n", delimiter);

          if (event_res->get_metadata().size() >= 7) {
            auto event_db_col = row->get_string(6);

            // BUG#38089433 - handle unsupported collations
            handle_collation_update_variable(
                Compatibility_issue::Object_type::EVENT, qualified_name,
                "DATABASE_COLLATION", &event_db_col, &res);

            switch_db_collation(sql_file, db_name, delimiter, db_cl_name,
                                event_db_col, &db_cl_altered);

            const auto client_cs = row->get_string(4);
            auto connection_col = row->get_string(5);

            // BUG#38089433 - handle unsupported collations
            handle_collation_update_variable(
                Compatibility_issue::Object_type::EVENT, qualified_name,
                "COLLATION_CONNECTION", &connection_col, &res);

            switch_cs_variables(
                sql_file, delimiter,
                client_cs.c_str(),       /* character_set_client */
                client_cs.c_str(),       /* character_set_results */
                connection_col.c_str()); /* collation_connection */
          } else {
            /*
              mysqldump is being run against the server, that does not
              provide character set information in SHOW CREATE
              statements.

              NOTE: the dump may be incorrect, since character set
              information is required in order to restore event properly.
            */

            fputs(
                "--\n"
                "-- WARNING: old server version. "
                "The following dump may be incomplete.\n"
                "--\n",
                sql_file);
          }

          switch_sql_mode(sql_file, delimiter, row->get_string(1).c_str());

          switch_time_zone(sql_file, delimiter, row->get_string(2).c_str());

          auto ces = opt_reexecutable ? fixup_event_ddl(row->get_string(3))
                                      : row->get_string(3);

          check_object_for_definer(Compatibility_issue::Object_type::EVENT,
                                   qualified_name, &ces, &res);

          fprintf(sql_file, "/*!50106 %s */ %s\n", ces.c_str(),
                  (const char *)delimiter);

          restore_time_zone(sql_file, delimiter);
          restore_sql_mode(sql_file, delimiter);

          if (event_res->get_metadata().size() >= 7) {
            restore_cs_variables(sql_file, delimiter);

            if (db_cl_altered)
              restore_db_collation(sql_file, db_name.c_str(), delimiter,
                                   db_cl_name.c_str());
          }
        }
      } /* end of event printing */
    }
    /* end of list of events */
    {
      fputs("DELIMITER ;\n", sql_file);
      fputs("/*!50106 SET TIME_ZONE= @save_time_zone */ ;\n", sql_file);
    }

    switch_character_set_results(opt_character_set_results.c_str());
  }

  return res;
}

/*
  dump_routines_for_db
  -- retrieves list of routines for a given db, and prints out
  the CREATE PROCEDURE definition into the output (the dump).

  This function has logic to print the appropriate syntax depending on whether
  this is a procedure or functions

  RETURN
    mysqlaas issues list
*/

std::vector<Compatibility_issue> Schema_dumper::dump_routines_for_db(
    IFile *sql_file, const std::string &db) {
  std::vector<Compatibility_issue> res;
  char query_buff[QUERY_LENGTH];
  const std::array<std::pair<std::string, Compatibility_issue::Object_type>, 2>
      routine_types{
          {{"FUNCTION", Compatibility_issue::Object_type::FUNCTION},
           {"PROCEDURE", Compatibility_issue::Object_type::PROCEDURE}}};
  std::string db_name;

  std::string db_cl_name;
  int db_cl_altered = false;

  db_name = m_mysql->escape_string(db);

  /* nice comments */
  std::string routines_text = fix_identifier_with_newline(db);
  print_comment(sql_file, false,
                "\n--\n-- Dumping routines for database '%s'\n--\n\n",
                routines_text.c_str());

  /* Get database collation. */

  fetch_db_collation(db, &db_cl_name);

  switch_character_set_results("binary");

  /* 0, retrieve and dump functions, 1, procedures */
  for (const auto &routine_type : routine_types) {
    const auto routine_list = get_routines(db, routine_type.first);
    for (const auto &routine : routine_list) {
      const auto qualified_name = quote(db, routine);
      const auto routine_name = shcore::quote_identifier(routine);
      log_debug("retrieving CREATE %s for %s", routine_type.first.c_str(),
                qualified_name.c_str());
      snprintf(query_buff, sizeof(query_buff), "SHOW CREATE %s %s",
               routine_type.first.c_str(), qualified_name.c_str());

      auto routine_res = query_log_and_throw(query_buff);
      while (auto row = routine_res->fetch_one()) {
        /*
          if the user has EXECUTE privilege he see routine names, but NOT
          the routine body of other routines that are not the creator of!
        */
        std::string body = row->is_null(2) ? "" : row->get_string(2);
        log_debug("length of body for %s row[2] '%s' is %zu",
                  qualified_name.c_str(),
                  !row->is_null(2) ? body.c_str() : "(null)", body.length());
        if (row->is_null(2)) {
          print_comment(sql_file, true, "\n-- insufficient privileges to %s\n",
                        query_buff);

          std::string text = fix_identifier_with_newline(
              m_mysql->get_connection_options().get_user());
          print_comment(sql_file, true,
                        "-- does %s have permissions on mysql.proc?\n\n",
                        text.c_str());

          THROW_ERROR(SHERR_DUMP_SD_INSUFFICIENT_PRIVILEGES,
                      m_mysql->get_connection_options().get_user().c_str(),
                      query_buff);
        } else if (body.length() > 0) {
          Object_guard_msg guard(sql_file, routine_type.first, db,
                                 routine_name);
          if (opt_drop_routine || opt_reexecutable)
            fprintf(sql_file, "/*!50003 DROP %s IF EXISTS %s */;\n",
                    routine_type.first.c_str(), routine_name.c_str());

          if (routine_res->get_metadata().size() >= 6) {
            auto routine_db_col = row->get_string(5);

            // BUG#38089433 - handle unsupported collations
            handle_collation_update_variable(
                routine_type.second, qualified_name, "DATABASE_COLLATION",
                &routine_db_col, &res);

            switch_db_collation(sql_file, db_name, ";", db_cl_name,
                                routine_db_col, &db_cl_altered);

            const auto client_cs = row->get_string(3);
            auto connection_col = row->get_string(4);

            // BUG#38089433 - handle unsupported collations
            handle_collation_update_variable(
                routine_type.second, qualified_name, "COLLATION_CONNECTION",
                &connection_col, &res);

            switch_cs_variables(
                sql_file, ";", client_cs.c_str(), /* character_set_client */
                client_cs.c_str(),                /* character_set_results */
                connection_col.c_str());          /* collation_connection */
          } else {
            /*
              mysqldump is being run against the server, that does not
              provide character set information in SHOW CREATE
              statements.

              NOTE: the dump may be incorrect, since character set
              information is required in order to restore stored
              procedure/function properly.
            */

            fputs(
                "--\n"
                "-- WARNING: old server version. "
                "The following dump may be incomplete.\n"
                "--\n",
                sql_file);
          }

          switch_sql_mode(sql_file, ";", row->get_string(1).c_str());

          check_object_for_definer(routine_type.second, qualified_name, &body,
                                   &res);
          check_routine_for_dependencies(db, routine, routine_type.second,
                                         &res);

          if (m_cache) {
            // BUG#38089433 - handle unsupported collations
            const auto &s = m_cache->schemas.at(db);
            const auto &r = (Compatibility_issue::Object_type::FUNCTION ==
                                     routine_type.second
                                 ? s.functions
                                 : s.procedures)
                                .at(routine);
            DBUG_EXECUTE_IF("dumper_unsupported_collation",
                            { use_unsupported_collation(&body); });

            for (const auto &p : r.parameters) {
              handle_collation_update_statement(
                  Compatibility_issue::Object_type::PARAMETER,
                  qualified_name + '.' + shcore::quote_identifier(p.name),
                  "collation", p.collation, &body, &res);
            }

            handle_collation_update_statement(
                Compatibility_issue::Object_type::RETURN_VALUE, qualified_name,
                "collation", r.return_value.collation, &body, &res);
          }

          fprintf(sql_file,
                  "DELIMITER ;;\n"
                  "%s ;;\n"
                  "DELIMITER ;\n",
                  body.c_str());

          restore_sql_mode(sql_file, ";");

          if (routine_res->get_metadata().size() >= 6) {
            restore_cs_variables(sql_file, ";");

            if (db_cl_altered)
              restore_db_collation(sql_file, db_name.c_str(), ";",
                                   db_cl_name.c_str());
          }
        }
      } /* end of routine printing */

    } /* end of list of routines */

  } /* end of for i (0 .. 1)  */

  switch_character_set_results(opt_character_set_results.c_str());

  return res;
}

std::vector<Compatibility_issue> Schema_dumper::dump_libraries_for_db(
    IFile *sql_file, const std::string &db) {
  print_comment(sql_file, false,
                "\n--\n-- Dumping libraries for database '%s'\n--\n\n",
                fix_identifier_with_newline(db).c_str());

  const auto libraries = get_libraries(db);

  if (libraries.empty()) {
    return {};
  }

  const auto db_name = shcore::quote_identifier(db);
  std::string query;
  query.reserve(QUERY_LENGTH);

  switch_character_set_results("binary");

  for (const auto &library : libraries) {
    const auto library_name = shcore::quote_identifier(library);

    log_debug("retrieving CREATE LIBRARY for %s", library_name.c_str());

    query = "SHOW CREATE LIBRARY ";
    query.append(db_name);
    query.append(1, '.');
    query.append(library_name);

    const auto library_res = query_log_and_throw(query);

    while (const auto row = library_res->fetch_one()) {
      const auto body_is_null = row->is_null(2);
      const auto body = body_is_null ? "" : row->get_string(2);

      log_debug("length of body for %s row[2] '%s' is %zu",
                library_name.c_str(), !body_is_null ? body.c_str() : "(null)",
                body.length());

      if (body_is_null) {
        print_comment(sql_file, true, "\n-- insufficient privileges to %s\n",
                      query.c_str());

        const auto &user = m_mysql->get_connection_options().get_user();
        print_comment(
            sql_file, true,
            "-- does %s have permissions on INFORMATION_SCHEMA.LIBRARIES?\n\n",
            fix_identifier_with_newline(user).c_str());

        THROW_ERROR(SHERR_DUMP_SD_INSUFFICIENT_PRIVILEGES, user.c_str(),
                    query.c_str());
      } else if (body.length() > 0) {
        Object_guard_msg guard{sql_file, "library", db, library_name};

        if (opt_drop_library || opt_reexecutable) {
          fprintf(sql_file, "DROP LIBRARY IF EXISTS %s;\n",
                  library_name.c_str());
        }

        switch_sql_mode(sql_file, ";", row->get_string(1).c_str());

        fprintf(sql_file,
                "DELIMITER ;;\n"
                "%s ;;\n"
                "DELIMITER ;\n",
                body.c_str());

        restore_sql_mode(sql_file, ";");
      }
    }
  }

  switch_character_set_results(opt_character_set_results.c_str());

  return {};
}

std::vector<Compatibility_issue> Schema_dumper::check_ct_for_mysqlaas(
    const std::string &db, const std::string &table,
    std::string *create_table) {
  std::vector<Compatibility_issue> res;
  const auto quoted_table = quote(db, table);

  if (opt_mysqlaas) {
    if (compatibility::check_create_table_for_data_index_dir_option(
            *create_table, create_table))
      res.emplace_back(
          Compatibility_issue::fixed::table_data_or_index_dir(quoted_table));

    if (compatibility::check_create_table_for_encryption_option(*create_table,
                                                                create_table))
      res.emplace_back(
          Compatibility_issue::fixed::table_encryption(quoted_table));
  }

  if (opt_mysqlaas || opt_force_innodb) {
    const auto engine = compatibility::check_create_table_for_engine_option(
        *create_table, opt_force_innodb ? create_table : nullptr);
    if (!engine.empty()) {
      res.emplace_back(
          opt_force_innodb
              ? Compatibility_issue::fixed::table_unsupported_engine(
                    quoted_table, engine)
              : Compatibility_issue::error::table_unsupported_engine(
                    quoted_table, engine));
    }

    // if engine is empty, table is already using InnoDB, we want to remove
    // FIXED row format right away
    const auto remove_fixed_row_format = opt_force_innodb || engine.empty();

    if (compatibility::check_create_table_for_fixed_row_format(
            *create_table, remove_fixed_row_format ? create_table : nullptr)) {
      res.emplace_back(
          remove_fixed_row_format
              ? Compatibility_issue::fixed::table_unsupported_row_format(
                    quoted_table)
              : Compatibility_issue::error::table_unsupported_row_format(
                    quoted_table));
    }
  }

  if (opt_mysqlaas || opt_strip_tablespaces) {
    if (compatibility::check_create_table_for_tablespace_option(
            *create_table, opt_strip_tablespaces ? create_table : nullptr)) {
      res.emplace_back(
          opt_strip_tablespaces
              ? Compatibility_issue::fixed::table_tablespace(quoted_table)
              : Compatibility_issue::error::table_tablespace(quoted_table));
    }
  }

  if (opt_mysqlaas) {
    std::size_t count = 0;

    if (m_cache) {
      count = m_cache->schemas.at(db).tables.at(table).all_columns.size();
    } else {
      const auto result = query_log_error(
          "SELECT COUNT(column_name) FROM information_schema.columns WHERE "
          "table_schema=? AND table_name=?",
          db, table);

      if (const auto row = result->fetch_one()) {
        count = row->get_uint(0);
      } else {
        THROW_ERROR(SHERR_DUMP_SD_MISSING_TABLE, quoted_table.c_str());
      }
    }

    if (count > k_max_innodb_columns) {
      res.emplace_back(Compatibility_issue::error::table_too_many_columns(
          quoted_table, count, k_max_innodb_columns));
    }
  }

  return res;
}

void Schema_dumper::check_object_for_definer(
    Compatibility_issue::Object_type object, const std::string &qualified_name,
    std::string *ddl, std::vector<Compatibility_issue> *issues) {
  if (!(opt_mysqlaas || opt_strip_definer)) {
    return;
  }

  const auto rewritten = opt_strip_definer ? ddl : nullptr;
  const auto user =
      compatibility::check_statement_for_definer_clause(*ddl, rewritten);
  const auto set_any_definer = set_any_definer_check();

  if (!user.empty()) {
    if (opt_strip_definer) {
      issues->emplace_back(Compatibility_issue::fixed::object_invalid_definer(
          object, qualified_name));
    } else {
      if (set_any_definer.supported) {
        if (!m_cache) {
          throw std::logic_error(
              "check_object_for_definer() requires cache to be set");
        }

        const auto account = shcore::split_account(user);

        if (compatibility::k_mds_users_with_system_user_priv.count(
                account.user)) {
          issues->emplace_back(
              Compatibility_issue::error::object_restricted_definer(
                  object, qualified_name, user));
        }

        if (m_cache->users.empty()) {
          if (!m_non_existing_definer_reported) {
            issues->emplace_back(Compatibility_issue::warning::
                                     object_invalid_definer_users_not_dumped());
            m_non_existing_definer_reported = true;
          }
        } else {
          if (m_cache->users.end() == std::find(m_cache->users.begin(),
                                                m_cache->users.end(),
                                                account)) {
            issues->emplace_back(Compatibility_issue::warning::
                                     object_invalid_definer_missing_user(
                                         object, qualified_name, user));
          }
        }
      }

      if (set_any_definer.deprecated.report_errors) {
        issues->emplace_back(
            set_any_definer.deprecated.downgrade_errors
                ? Compatibility_issue::warning::object_invalid_definer(
                      object, qualified_name, user)
                : Compatibility_issue::error::object_invalid_definer(
                      object, qualified_name, user));
      }
    }
  }

  if (compatibility::check_statement_for_sqlsecurity_clause(*ddl, rewritten)) {
    if (opt_strip_definer) {
      issues->emplace_back(
          Compatibility_issue::fixed::object_missing_sql_security(
              object, qualified_name));
    } else {
      if (set_any_definer.supported) {
        if (user.empty()) {
          issues->emplace_back(Compatibility_issue::error::
                                   object_missing_sql_security_and_definer(
                                       object, qualified_name));
        }
      }

      if (set_any_definer.deprecated.report_errors) {
        issues->emplace_back(
            set_any_definer.deprecated.downgrade_errors
                ? Compatibility_issue::warning::object_missing_sql_security(
                      object, qualified_name)
                : Compatibility_issue::error::object_missing_sql_security(
                      object, qualified_name));
      }
    }
  }
}

/*
  get_table_structure -- retrieves database structure, prints out
  corresponding CREATE statement.

  ARGS
    table       - table name
    db          - db name
    table_type  - table type, e.g. "MyISAM" or "InnoDB", but also "VIEW"
    ignore_flag - what we must particularly ignore - see IGNORE_ defines above
  RETURN
    vector with compatibility issues with mysqlaas
*/

std::vector<Compatibility_issue> Schema_dumper::get_table_structure(
    IFile *sql_file, const std::string &table, const std::string &db,
    std::string *out_table_type, char *ignore_flag) {
  std::vector<Compatibility_issue> res;
  bool init = false, skip_ddl;
  std::string result_table;
  const char *show_fields_stmt =
      "SELECT `COLUMN_NAME` AS `Field`, "
      "`COLUMN_TYPE` AS `Type`, "
      "`IS_NULLABLE` AS `Null`, "
      "`COLUMN_KEY` AS `Key`, "
      "`COLUMN_DEFAULT` AS `Default`, "
      "`EXTRA` AS `Extra`, "
      "`COLUMN_COMMENT` AS `Comment` "
      "FROM `INFORMATION_SCHEMA`.`COLUMNS` WHERE "
      "TABLE_SCHEMA = ? AND TABLE_NAME = ? "
      "ORDER BY ORDINAL_POSITION";
  bool is_log_table;
  bool is_replication_metadata_table;
  std::shared_ptr<mysqlshdk::db::IResult> result;
  mysqlshdk::db::Error error;
  bool has_pk = false;
  bool has_my_row_id = false;
  bool has_auto_increment = false;
  bool has_partitions = false;
  static constexpr std::string_view k_my_row_id = "my_row_id";
  const std::string auto_increment = "auto_increment";

  *ignore_flag = check_if_ignore_table(db, table, out_table_type);

  /*
    for mysql.innodb_table_stats, mysql.innodb_index_stats tables we
    dont dump DDL
  */
  skip_ddl = innodb_stats_tables(db, table);

  log_debug("-- Retrieving table structure for table %s...", table.c_str());

  result_table = shcore::quote_identifier(table);

  if (!execute_no_throw("SET SQL_QUOTE_SHOW_CREATE=1")) {
    /* using SHOW CREATE statement */
    if (!skip_ddl) {
      /* Make an sql-file, if path was given iow. option -T was given */
      if (query_with_binary_charset("show create table " + result_table,
                                    &result, &error)) {
        THROW_ERROR(SHERR_DUMP_SD_SHOW_CREATE_TABLE_FAILED,
                    result_table.c_str(), error.what());
      }

      auto row = result->fetch_one();
      if (!row) {
        THROW_ERROR(SHERR_DUMP_SD_SHOW_CREATE_TABLE_EMPTY,
                    result_table.c_str());
      }

      std::string create_table = row->get_string(1);

      std::string text = fix_identifier_with_newline(result_table);
      if (*out_table_type == "VIEW") /* view */
        print_comment(sql_file, false,
                      "\n--\n-- Temporary view structure for view %s\n--\n\n",
                      text.c_str());
      else
        print_comment(sql_file, false,
                      "\n--\n-- Table structure for table %s\n--\n\n",
                      text.c_str());

      if ((*out_table_type == "VIEW" && opt_drop_view) ||
          (*out_table_type != "VIEW" && opt_drop_table)) {
        /*
          Even if the "table" is a view, we do a DROP TABLE here.  The
          view-specific code below fills in the DROP VIEW.
          We will skip the DROP TABLE for general_log and slow_log, since
          those stmts will fail, in case we apply dump by enabling logging.
          We will skip this for replication metadata tables as well.
         */
        if (!(general_log_or_slow_log_tables(db, table) ||
              replication_metadata_tables(db, table)))
          fprintf(sql_file, "DROP TABLE IF EXISTS %s;\n", result_table.c_str());
        check_io(sql_file);
      }

      if (result->get_metadata().at(0).get_column_label() == "View") {
        log_debug("-- It's a view, create dummy view");

        /*
          Create a table with the same name as the view and with columns of
          the same name in order to satisfy views that depend on this view.
          The table will be removed when the actual view is created.

          The properties of each column, are not preserved in this temporary
          table, because they are not necessary.

          This will not be necessary once we can determine dependencies
          between views and can simply dump them in the appropriate order.
        */
        std::vector<Instance_cache::Column> columns;

        if (!m_cache) {
          mysqlshdk::db::Error err;

          if (query_with_binary_charset("SHOW FIELDS FROM " + result_table,
                                        &result, &err)) {
            /*
              View references invalid or privileged table/col/fun (err 1356),
              so we cannot create a stand-in table.  Be defensive and dump
              a comment with the view's 'show create' statement. (Bug #17371)
            */

            if (err.code() == ER_VIEW_INVALID)
              fprintf(sql_file, "\n-- failed on view %s: %s\n\n",
                      result_table.c_str(), create_table.c_str());

            THROW_ERROR(SHERR_DUMP_SD_SHOW_FIELDS_FAILED, result_table.c_str());
          }

          while ((row = result->fetch_one())) {
            Instance_cache::Column column;
            column.name = row->get_string(0);
            column.quoted_name = shcore::quote_identifier(column.name);
            columns.emplace_back(std::move(column));
          }
        }

        const auto &all_columns =
            m_cache ? m_cache->schemas.at(db).views.at(table).all_columns
                    : columns;

        if (!all_columns.empty()) {
          if (opt_drop_view) {
            /*
              We have already dropped any table of the same name above, so
              here we just drop the view.
            */

            fprintf(sql_file, "/*!50001 DROP VIEW IF EXISTS %s*/;\n",
                    result_table.c_str());
            check_io(sql_file);
          }

          fprintf(sql_file,
                  "SET @saved_cs_client     = @@character_set_client;\n"
                  "/*!50503 SET character_set_client = utf8mb4 */;\n"
                  "/*!50001 CREATE VIEW %s AS SELECT\n",
                  result_table.c_str());

          /*
            Get first row, following loop will prepend comma - keeps from
            having to know if the row being printed is last to determine if
            there should be a _trailing_ comma.
          */

          /*
            A temporary view is created to resolve the view interdependencies.
            This temporary view is dropped when the actual view is created.
          */
          auto column = all_columns.begin();

          fprintf(sql_file, " 1 AS %s", column->quoted_name.c_str());

          while (++column != all_columns.end()) {
            fprintf(sql_file, ",\n 1 AS %s", column->quoted_name.c_str());
          }

          fprintf(sql_file,
                  " */;\n"
                  "SET character_set_client = @saved_cs_client;\n");

          /*
             The actual formula is based on the column names and how
             the .FRM files are stored and is too volatile to be repeated here.
             Thus we simply warn the user if the columns exceed a limit
             we know works most of the time.
          */
          if (result->get_fetched_row_count() >= 1000)
            fprintf(stderr,
                    "-- Warning: Creating a stand-in table for view %s may"
                    " fail when replaying the dump file produced because "
                    "of the number of columns exceeding 1000. Exercise "
                    "caution when replaying the produced dump file.\n",
                    table.c_str());

          check_io(sql_file);
        }

        seen_views = true;
        return res;
      }

      is_log_table = general_log_or_slow_log_tables(db, table);
      is_replication_metadata_table = replication_metadata_tables(db, table);

      res = check_ct_for_mysqlaas(db, table, &create_table);

      if (m_cache) {
        // BUG#38089433 - handle unsupported collations
        const auto &t = m_cache->schemas.at(db).tables.at(table);
        const auto quoted = quote(db, table);

        DBUG_EXECUTE_IF("dumper_unsupported_collation",
                        { use_unsupported_collation(&create_table); });

        for (const auto &c : t.all_columns) {
          handle_collation_update_statement(
              Compatibility_issue::Object_type::COLUMN,
              quoted + '.' + c.quoted_name, "collation", c.collation,
              &create_table, &res);
        }

        handle_collation_update_statement(
            Compatibility_issue::Object_type::TABLE, quoted,
            "default collation", t.collation, &create_table, &res);
      }

      if (opt_reexecutable || is_log_table || is_replication_metadata_table)
        create_table = shcore::str_replace(create_table, "CREATE TABLE ",
                                           "CREATE TABLE IF NOT EXISTS ");

      fprintf(sql_file,
              "/*!40101 SET @saved_cs_client     = @@character_set_client */;\n"
              "/*!50503 SET character_set_client = utf8mb4 */;\n"
              "%s;\n"
              "/*!40101 SET character_set_client = @saved_cs_client */;\n",
              create_table.c_str());

      check_io(sql_file);
    }

    if (opt_create_invisible_pks) {
      std::vector<Instance_cache::Column> columns;

      if (!m_cache) {
        result = query_log_and_throw("show fields from " + result_table);

        while (auto row = result->fetch_one()) {
          Instance_cache::Column column;
          column.name = row->get_string(SHOW_FIELDNAME);

          if (!row->is_null(SHOW_EXTRA)) {
            column.auto_increment =
                row->get_string(SHOW_EXTRA).find(auto_increment) !=
                std::string::npos;
          }

          columns.emplace_back(std::move(column));
        }
      }

      const auto &all_columns =
          m_cache ? m_cache->schemas.at(db).tables.at(table).all_columns
                  : columns;

      for (const auto &column : all_columns) {
        has_auto_increment |= column.auto_increment;
        has_my_row_id |= shcore::str_caseeq(column.name.c_str(), k_my_row_id);
      }
    }

    if (opt_mysqlaas || opt_create_invisible_pks || opt_ignore_missing_pks) {
      if (m_cache) {
        has_pk = m_cache->schemas.at(db).tables.at(table).primary_key;
      } else {
        result = query_log_and_throw(shcore::sqlformat(
            "SELECT COUNT(*) FROM information_schema.statistics WHERE "
            "INDEX_NAME='PRIMARY' AND TABLE_SCHEMA=? AND TABLE_NAME=?",
            db, table));
        has_pk = result->fetch_one()->get_int(0) > 0;
      }
    }
  } else {
    // the SQL_QUOTE_SHOW_CREATE system variable was added in 3.23.26, we should
    // remove this code at some point
    result =
        query_log_and_throw(shcore::sqlformat(show_fields_stmt, db, table));
    {
      std::string text = fix_identifier_with_newline(result_table);
      print_comment(sql_file, false,
                    "\n--\n-- Table structure for table %s\n--\n\n",
                    text.c_str());

      if (opt_drop_table)
        fprintf(sql_file, "DROP TABLE IF EXISTS %s;\n", result_table.c_str());
      fprintf(sql_file, "CREATE TABLE IF NOT EXISTS %s (\n",
              result_table.c_str());
      check_io(sql_file);
    }

    while (auto row = result->fetch_one()) {
      bool real_column = false;
      if (!row->is_null(SHOW_EXTRA)) {
        std::string extra = row->get_string(SHOW_EXTRA);
        real_column =
            extra != "STORED GENERATED" && extra != "VIRTUAL GENERATED";
      } else {
        real_column = true;
      }

      if (!real_column) continue;

      if (init) {
        fputs(",\n", sql_file);
        check_io(sql_file);
      }
      init = true;
      {
        const auto fieldname = row->get_string(SHOW_FIELDNAME);

        if (opt_create_invisible_pks) {
          has_my_row_id |= shcore::str_caseeq(fieldname, k_my_row_id);
        }

        fprintf(sql_file, "  %s.%s %s", result_table.c_str(),
                shcore::quote_identifier(fieldname).c_str(),
                row->get_string(SHOW_TYPE).c_str());

        if (!row->is_null(SHOW_DEFAULT)) {
          fputs(" DEFAULT ", sql_file);
          std::string def = row->get_string(SHOW_DEFAULT);
          unescape(sql_file, def);
        }
        if (!row->get_string(SHOW_NULL).empty()) fputs(" NOT NULL", sql_file);

        if (!row->is_null(SHOW_EXTRA)) {
          const auto extra = row->get_string(SHOW_EXTRA);

          if (!extra.empty()) {
            fprintf(sql_file, " %s", extra.c_str());

            if (opt_create_invisible_pks) {
              has_auto_increment |=
                  extra.find(auto_increment) != std::string::npos;
            }
          }
        }

        check_io(sql_file);
      }
    }
    {
      uint32_t keynr, primary_key;
      mysqlshdk::db::Error err;
      if (query_no_throw("show keys from " + result_table, &result, &err)) {
        if (err.code() == ER_WRONG_OBJECT) {
          /* it is VIEW */
          goto continue_xml;
        }
        fprintf(stderr, "Can't get keys for table %s (%s)\n",
                result_table.c_str(), err.format().c_str());
        THROW_ERROR(SHERR_DUMP_SD_SHOW_KEYS_FAILED, result_table.c_str(),
                    err.format().c_str());
      }

      /* Find first which key is primary key */
      keynr = 0;
      primary_key = INT_MAX;
      while (auto row = result->fetch_one()) {
        if (row->get_int(3) == 1) {
          keynr++;
          if (row->get_string(2) == "PRIMARY") {
            primary_key = keynr;
            break;
          }
        }
      }

      has_pk = INT_MAX != primary_key;

      result->rewind();
      keynr = 0;
      while (auto row = result->fetch_one()) {
        if (row->get_int(3) == 1) {
          if (keynr++) fputs(")", sql_file);
          if (row->get_int(1)) /* Test if duplicate key */
            /* Duplicate allowed */
            fprintf(sql_file, ",\n  KEY %s (",
                    shcore::quote_identifier(row->get_string(2)).c_str());
          else if (keynr == primary_key)
            fputs(",\n  PRIMARY KEY (", sql_file); /* First UNIQUE is primary */
          else
            fprintf(sql_file, ",\n  UNIQUE %s (",
                    shcore::quote_identifier(row->get_string(2)).c_str());
        } else {
          fputs(",", sql_file);
        }
        fputs(shcore::quote_identifier(row->get_string(4)).c_str(), sql_file);
        if (!row->is_null(7))
          fprintf(sql_file, " (%s)", row->get_string(7).c_str()); /* Sub key */
        check_io(sql_file);
      }
      if (keynr) fputs(")", sql_file);
      fputs("\n)", sql_file);
      check_io(sql_file);

      /* Get MySQL specific create options */
      if (opt_create_options) {
        const auto write_options = [sql_file, this](
                                       const std::string &engine,
                                       const std::string &options,
                                       const std::string &comment) {
          fputs("/*!", sql_file);
          fprintf(sql_file, "engine=%s", engine.c_str());
          fprintf(sql_file, "%s", options.c_str());
          fprintf(sql_file, "comment='%s'",
                  m_mysql->escape_string(comment).c_str());
          fputs(" */", sql_file);
          check_io(sql_file);
        };

        if (m_cache) {
          const auto &t = m_cache->schemas.at(db).tables.at(table);
          write_options(t.engine, t.create_options, t.comment);
        } else {
          mysqlshdk::db::Row_ref_by_name row;
          if (query_no_throw("show table status like " + quote_for_like(table),
                             &result, &err)) {
            if (err.code() != ER_PARSE_ERROR) { /* If old MySQL version */
              log_debug(
                  "-- Warning: Couldn't get status information for "
                  "table %s (%s)",
                  result_table.c_str(), err.format().c_str());
            }
          } else if (!(row = result->fetch_one_named())) {
            fprintf(stderr,
                    "Error: Couldn't read status information for table %s\n",
                    result_table.c_str());
          } else {
            write_options(row.get_string("Engine"),
                          row.get_string("Create_options"),
                          row.get_string("Comment"));
          }
        }
      }
    continue_xml:
      fputs(";\n", sql_file);
      check_io(sql_file);
    }
  }

  if (opt_create_invisible_pks) {
    if (m_cache) {
      has_partitions =
          !m_cache->schemas.at(db).tables.at(table).partitions.empty();
    } else {
      result = query_log_and_throw(shcore::sqlformat(
          "SELECT COUNT(*) FROM information_schema.partitions WHERE "
          "TABLE_SCHEMA=? AND TABLE_NAME=? AND PARTITION_NAME IS NOT NULL",
          db, table));
      has_partitions = result->fetch_one_or_throw()->get_uint(0);
    }
  }

  if (!has_pk) {
    const auto quoted_table = quote(db, table);

    if (opt_create_invisible_pks) {
      if (has_my_row_id || has_auto_increment || has_partitions) {
        if (has_my_row_id) {
          res.emplace_back(
              Compatibility_issue::error::table_missing_pk_manual_fix(
                  quoted_table, "has a column named `my_row_id`"));
        }

        if (has_auto_increment) {
          res.emplace_back(
              Compatibility_issue::error::table_missing_pk_manual_fix(
                  quoted_table,
                  "has a column with 'AUTO_INCREMENT' attribute"));
        }

        if (has_partitions) {
          res.emplace_back(
              Compatibility_issue::error::table_missing_pk_manual_fix(
                  quoted_table, "is partitioned"));
        }
      } else {
        res.emplace_back(
            Compatibility_issue::fixed::table_missing_pk_create(quoted_table));
      }
    } else if (opt_ignore_missing_pks) {
      res.emplace_back(
          Compatibility_issue::fixed::table_missing_pk_ignore(quoted_table));
    } else if (opt_mysqlaas) {
      res.emplace_back(
          Compatibility_issue::error::table_missing_pk(quoted_table));
    }
  }

  return res;
} /* get_table_structure */

std::vector<Compatibility_issue> Schema_dumper::dump_trigger(
    IFile *sql_file,
    const std::shared_ptr<mysqlshdk::db::IResult> &show_create_trigger_rs,
    const std::string &db_name, const std::string &table_name,
    const std::string &trigger_name, const std::string &db_collation_name) {
  int db_cl_altered = false;
  std::vector<Compatibility_issue> res;

  const auto qualified_name = quote(db_name, table_name, trigger_name);

  while (auto row = show_create_trigger_rs->fetch_one()) {
    auto trigger_db_col = row->get_string(5);

    // BUG#38089433 - handle unsupported collations
    handle_collation_update_variable(Compatibility_issue::Object_type::TRIGGER,
                                     qualified_name, "DATABASE_COLLATION",
                                     &trigger_db_col, &res);

    switch_db_collation(sql_file, db_name, ";", db_collation_name,
                        trigger_db_col, &db_cl_altered);

    const auto client_cs = row->get_string(3);
    auto connection_col = row->get_string(4);

    // BUG#38089433 - handle unsupported collations
    handle_collation_update_variable(Compatibility_issue::Object_type::TRIGGER,
                                     qualified_name, "COLLATION_CONNECTION",
                                     &connection_col, &res);

    switch_cs_variables(sql_file, ";",
                        client_cs.c_str(),       /* character_set_client */
                        client_cs.c_str(),       /* character_set_results */
                        connection_col.c_str()); /* collation_connection */

    switch_sql_mode(sql_file, ";", row->get_string(1).c_str());

    if (opt_reexecutable || opt_drop_trigger)
      fprintf(sql_file, "/*!50032 DROP TRIGGER IF EXISTS %s */;\n",
              shcore::quote_identifier(row->get_string(0)).c_str());

    // 5.7 server adds schema name to CREATE TRIGGER statement
    const auto trigger_with_schema =
        " TRIGGER " + shcore::quote_identifier(db_name) + ".";

    std::string body = shcore::str_replace(row->get_string(2),
                                           trigger_with_schema, " TRIGGER ");
    check_object_for_definer(Compatibility_issue::Object_type::TRIGGER,
                             qualified_name, &body, &res);

    fprintf(sql_file,
            "DELIMITER ;;\n"
            "/*!50003 %s */;;\n"
            "DELIMITER ;\n",
            body.c_str());

    restore_sql_mode(sql_file, ";");
    restore_cs_variables(sql_file, ";");

    if (db_cl_altered)
      restore_db_collation(sql_file, db_name, ";", db_collation_name.c_str());
  }

  return res;
}

/**
  Dump the triggers for a given table.

  This should be called after the tables have been dumped in case a trigger
  depends on the existence of a table.

  @param[in] table_name
  @param[in] db_name

  @return mysqlaas issues list
*/

std::vector<Compatibility_issue> Schema_dumper::dump_triggers_for_table(
    IFile *sql_file, const std::string &table, const std::string &db) {
  std::vector<Compatibility_issue> res;
  bool old_ansi_quotes_mode = ansi_quotes_mode;

  std::string db_cl_name;

  /* Do not use ANSI_QUOTES on triggers in dump */
  ansi_quotes_mode = false;

  /* Get database collation. */

  switch_character_set_results("binary");

  fetch_db_collation(db, &db_cl_name);

  /* Get list of triggers. */

  const auto triggers = get_triggers(db, table);

  /* Dump triggers. */
  if (!triggers.empty())
    print_comment(sql_file, false,
                  "\n--\n-- Dumping triggers for table '%s'.'%s'\n--\n\n",
                  db.c_str(), table.c_str());

  for (const auto &trigger : triggers) {
    const auto trigger_name = shcore::quote_identifier(trigger);
    std::shared_ptr<mysqlshdk::db::IResult> show_create_trigger_rs;
    if (query_no_throw("SHOW CREATE TRIGGER " + shcore::quote_identifier(db) +
                           "." + trigger_name,
                       &show_create_trigger_rs) == 0) {
      Object_guard_msg guard(sql_file, "trigger", db, trigger_name);
      const auto out = dump_trigger(sql_file, show_create_trigger_rs, db, table,
                                    trigger, db_cl_name);
      if (!out.empty()) res.insert(res.end(), out.begin(), out.end());
    }
  }

  switch_character_set_results(opt_character_set_results.c_str());

  /*
    make sure to set back ansi_quotes_mode mode to
    original value
  */
  ansi_quotes_mode = old_ansi_quotes_mode;

  return res;
}

std::vector<Instance_cache::Histogram> Schema_dumper::get_histograms(
    const std::string &db_name, const std::string &table_name) {
  std::vector<Instance_cache::Histogram> histograms;

  if (m_cache) {
    histograms = m_cache->schemas.at(db_name).tables.at(table_name).histograms;
  } else if (m_mysql->get_server_version() >=
             mysqlshdk::utils::Version(8, 0, 0)) {
    char query_buff[QUERY_LENGTH * 3 / 2];
    const auto old_ansi_quotes_mode = ansi_quotes_mode;
    const auto escaped_db = m_mysql->escape_string(db_name);
    const auto escaped_table = m_mysql->escape_string(table_name);

    switch_character_set_results("binary");

    /* Get list of columns with statistics. */
    snprintf(query_buff, sizeof(query_buff),
             "SELECT COLUMN_NAME, "
             "JSON_EXTRACT(HISTOGRAM, '$.\"number-of-buckets-specified\"') "
             "FROM information_schema.COLUMN_STATISTICS "
             "WHERE SCHEMA_NAME = '%s' AND TABLE_NAME = '%s';",
             escaped_db.c_str(), escaped_table.c_str());

    const auto column_statistics_rs = query_log_and_throw(query_buff);

    while (auto row = column_statistics_rs->fetch_one()) {
      Instance_cache::Histogram histogram;

      histogram.column = row->get_string(0);
      histogram.buckets = shcore::lexical_cast<std::size_t>(row->get_string(1));

      histograms.emplace_back(std::move(histogram));
    }

    switch_character_set_results(opt_character_set_results.c_str());

    /*
      make sure to set back ansi_quotes_mode mode to
      original value
    */
    ansi_quotes_mode = old_ansi_quotes_mode;
  }

  return histograms;
}

void Schema_dumper::dump_column_statistics_for_table(
    IFile *sql_file, const std::string &table_name,
    const std::string &db_name) {
  const auto quoted_table = shcore::quote_identifier(table_name);

  for (const auto &histogram : get_histograms(db_name, table_name)) {
    fprintf(sql_file,
            "/*!80002 ANALYZE TABLE %s UPDATE HISTOGRAM ON %s "
            "WITH %zu BUCKETS */;\n",
            quoted_table.c_str(),
            shcore::quote_identifier(histogram.column).c_str(),
            histogram.buckets);
  }
}

/*

 SYNOPSIS
  dump_table()

  dump_table saves database contents as a series of INSERT statements.

  ARGS
   table - table name
   db    - db name

   RETURNS
    mysqlaas issues list
*/

std::vector<Compatibility_issue> Schema_dumper::dump_table(
    IFile *file, const std::string &table, const std::string &db) {
  char ignore_flag;
  std::string query_string;
  std::string extended_row;
  std::string table_type;
  std::string result_table, opt_quoted_table;
  std::string order_by;

  return get_table_structure(file, table, db, &table_type, &ignore_flag);
} /* dump_table */

/*
  dump all logfile groups and tablespaces
*/

int Schema_dumper::dump_all_tablespaces(IFile *file) {
  return dump_tablespaces(file, "");
}

int Schema_dumper::dump_tablespaces_for_tables(
    IFile *file, const std::string &db,
    const std::vector<std::string> &table_names) {
  std::string where;
  std::string name = m_mysql->escape_string(db);

  where.assign(
      " AND TABLESPACE_NAME IN ("
      "SELECT DISTINCT TABLESPACE_NAME FROM"
      " INFORMATION_SCHEMA.PARTITIONS"
      " WHERE"
      " TABLE_SCHEMA='");
  where.append(name);
  where.append("' AND TABLE_NAME IN (");

  for (const std::string &table : table_names) {
    name = m_mysql->escape_string(table);

    where.append("'");
    where.append(name);
    where.append("',");
  }
  where.pop_back();
  where.append("))");

  return dump_tablespaces(file, where);
}

int Schema_dumper::dump_tablespaces_for_databases(
    IFile *file, const std::vector<std::string> &databases) {
  std::string where;

  where.assign(
      " AND TABLESPACE_NAME IN ("
      "SELECT DISTINCT TABLESPACE_NAME FROM"
      " INFORMATION_SCHEMA.PARTITIONS"
      " WHERE"
      " TABLE_SCHEMA IN (");

  for (const std::string &db : databases) {
    where.append("'");
    where.append(m_mysql->escape_string(db));
    where.append("',");
  }
  where.pop_back();
  where.append("))");

  return dump_tablespaces(file, where);
}

int Schema_dumper::dump_tablespaces(IFile *file, const std::string &ts_where) {
  std::shared_ptr<mysqlshdk::db::IResult> tableres;
  std::string buf;
  std::string sqlbuf;
  int first = 0;
  /*
    The following are used for parsing the EXTRA field
  */
  char extra_format[] = "UNDO_BUFFER_SIZE=";

  sqlbuf.assign(
      "SELECT LOGFILE_GROUP_NAME,"
      " FILE_NAME,"
      " TOTAL_EXTENTS,"
      " INITIAL_SIZE,"
      " ENGINE,"
      " EXTRA"
      " FROM INFORMATION_SCHEMA.FILES"
      " WHERE FILE_TYPE = 'UNDO LOG'"
      " AND FILE_NAME IS NOT NULL"
      " AND LOGFILE_GROUP_NAME IS NOT NULL");
  if (!ts_where.empty()) {
    sqlbuf.append(
        " AND LOGFILE_GROUP_NAME IN ("
        "SELECT DISTINCT LOGFILE_GROUP_NAME"
        " FROM INFORMATION_SCHEMA.FILES"
        " WHERE FILE_TYPE = 'DATAFILE'");
    sqlbuf.append(ts_where);
    sqlbuf.append(")");
  }
  sqlbuf.append(
      " GROUP BY LOGFILE_GROUP_NAME, FILE_NAME"
      ", ENGINE, TOTAL_EXTENTS, INITIAL_SIZE, EXTRA"
      " ORDER BY LOGFILE_GROUP_NAME");

  mysqlshdk::db::Error err;
  if (query_no_throw(sqlbuf, &tableres, &err)) {
    if (err.code() == ER_BAD_TABLE_ERROR || err.code() == ER_BAD_DB_ERROR ||
        err.code() == ER_UNKNOWN_TABLE) {
      fputs(
          "\n--\n-- Not dumping tablespaces as no INFORMATION_SCHEMA.FILES"
          " table on this server\n--\n",
          file);
      check_io(file);
      return 0;
    }

    log_error("Error: '%s' when trying to dump tablespaces",
              err.format().c_str());
    return 1;
  }

  while (auto row = tableres->fetch_one()) {
    std::string group_name = row->get_string(0);
    std::string file_name = row->get_string(1);

    if (group_name != buf) first = 1;
    if (first) {
      print_comment(file, false, "\n--\n-- Logfile group: %s\n--\n",
                    group_name.c_str());

      fputs("\nCREATE", file);
    } else {
      fputs("\nALTER", file);
    }
    fprintf(file,
            " LOGFILE GROUP %s\n"
            "  ADD UNDOFILE '%s'\n",
            group_name.c_str(), file_name.c_str());
    if (first) {
      if (row->is_null(5)) break;
      std::string extra = row->get_string(5);
      auto ubs = extra.find(extra_format);
      if (ubs == std::string::npos) break;
      ubs += strlen(extra_format);
      extra = extra.substr(ubs);
      auto endsemi = extra.find(";");
      if (endsemi != std::string::npos) extra = extra.substr(0, endsemi);
      fprintf(file, "  UNDO_BUFFER_SIZE %s\n", extra.c_str());
    }
    fprintf(file,
            "  INITIAL_SIZE %s\n"
            "  ENGINE=%s;\n",
            row->get_string(3).c_str(), row->get_string(4).c_str());
    check_io(file);
    if (first) {
      first = 0;
      buf = group_name;
    }
  }
  sqlbuf.assign(
      "SELECT DISTINCT TABLESPACE_NAME,"
      " FILE_NAME,"
      " LOGFILE_GROUP_NAME,"
      " EXTENT_SIZE,"
      " INITIAL_SIZE,"
      " ENGINE"
      " FROM INFORMATION_SCHEMA.FILES"
      " WHERE FILE_TYPE = 'DATAFILE'");

  if (!ts_where.empty()) sqlbuf.append(ts_where);

  sqlbuf.append(" ORDER BY TABLESPACE_NAME, LOGFILE_GROUP_NAME");

  if (query_no_throw(sqlbuf, &tableres)) {
    return 1;
  }

  buf.clear();
  while (auto row = tableres->fetch_one()) {
    std::string ts_name = row->get_string(0);
    if (buf != ts_name) first = 1;
    if (first) {
      print_comment(file, false, "\n--\n-- Tablespace: %s\n--\n",
                    ts_name.c_str());
      fputs("\nCREATE", file);
    } else {
      fputs("\nALTER", file);
    }
    fprintf(file,
            " TABLESPACE %s\n"
            "  ADD DATAFILE '%s'\n",
            ts_name.c_str(), row->get_string(1).c_str());
    if (first) {
      fprintf(file,
              "  USE LOGFILE GROUP %s\n"
              "  EXTENT_SIZE %s\n",
              row->get_string(2).c_str(), row->get_string(3).c_str());
    }
    fprintf(file,
            "  INITIAL_SIZE %s\n"
            "  ENGINE=%s;\n",
            row->get_string(4).c_str(), row->get_string(5).c_str());
    check_io(file);
    if (first) {
      first = 0;
      buf = ts_name;
    }
  }

  return 0;
}

int Schema_dumper::is_ndbinfo(const std::string &dbname) {
  if (!checked_ndbinfo) {
    checked_ndbinfo = 1;

    if (m_cache) {
      have_ndbinfo = m_cache->has_ndbinfo;
    } else {
      std::shared_ptr<mysqlshdk::db::IResult> res;
      if (query_no_throw(
              "SHOW VARIABLES LIKE " + quote_for_like("ndbinfo_version"), &res))
        return 0;

      if (!res->fetch_one()) return 0;

      have_ndbinfo = 1;
    }
  }

  if (!have_ndbinfo) return 0;

  if (dbname == "ndbinfo") return 1;

  return 0;
}

/*
View Specific database initialization.

SYNOPSIS
  init_dumping_views
  qdatabase      quoted name of the database

RETURN VALUES
  0        Success.
  1        Failure.
*/
int Schema_dumper::init_dumping_views(IFile *, const char * /*qdatabase*/) {
  return 0;
} /* init_dumping_views */

int Schema_dumper::init_dumping(
    IFile *file, const std::string &database,
    const std::function<int(IFile *, const char *)> &init_func) {
  if (is_ndbinfo(database)) {
    log_debug("-- Skipping dump of ndbinfo database");
    return 0;
  }

  use(database);

  if (opt_databases || opt_alldbs) {
    std::string qdatabase = shcore::quote_identifier(database);

    std::string text = fix_identifier_with_newline(qdatabase);
    print_comment(file, false, "\n--\n-- Current Database: %s\n--\n",
                  text.c_str());

    /* Call the view or table specific function */
    if (init_func) init_func(file, qdatabase.c_str());

    fprintf(file, "\nUSE %s;\n", qdatabase.c_str());
    check_io(file);
  }

  return 0;
} /* init_dumping */

std::vector<std::string> Schema_dumper::get_table_names() {
  std::vector<std::string> names;
  std::shared_ptr<mysqlshdk::db::IResult> result;

  if (query_no_throw("SHOW TABLES", &result) == 0) {
    while (auto row = result->fetch_one()) {
      names.push_back(row->get_string(0));
    }
  }

  return names;
}

/*
  get_actual_table_name -- executes a SHOW TABLES LIKE '%s' to get the actual
  table name from the server for the table name given on the command line.
  we do this because the table name given on the command line may be a
  different case (e.g.  T1 vs t1)

  RETURN
    pointer to the table name
    0 if error
*/

std::string Schema_dumper::get_actual_table_name(
    const std::string &old_table_name) {
  std::shared_ptr<mysqlshdk::db::IResult> table_res;
  std::string query;

  query = "SHOW TABLES LIKE " + quote_for_like(old_table_name);

  if (query_no_throw(query, &table_res)) return {};

  if (auto row = table_res->fetch_one()) return row->get_string(0);

  return {};
}

/*
  SYNOPSIS

  Check if the table is one of the table types that should be ignored:
  MRG_ISAM, MRG_MYISAM.

  If the table should be altogether ignored, it returns a true, false if it
  should not be ignored.

  ARGS

    check_if_ignore_table()
    table_name                  Table name to check
    table_type                  Type of table

  GLOBAL VARIABLES
    mysql                       MySQL connection
    verbose                     Write warning messages

  RETURN
    char (bit value)            See IGNORE_ values at top
*/

char Schema_dumper::check_if_ignore_table(const std::string &db,
                                          const std::string &table_name,
                                          std::string *out_table_type) {
  char result = IGNORE_NONE;

  std::shared_ptr<mysqlshdk::db::IResult> res;

  if (m_cache) {
    const auto &schema = m_cache->schemas.at(db);
    const auto it = schema.tables.find(table_name);

    if (it == schema.tables.end()) {
      *out_table_type = "VIEW";
    } else {
      *out_table_type = it->second.engine;
    }
  } else {
    try {
      res = m_mysql->query("show table status like " +
                           quote_for_like(table_name));
    } catch (const mysqlshdk::db::Error &e) {
      if (e.code() != ER_PARSE_ERROR) { /* If old MySQL version */
        log_debug(
            "-- Warning: Couldn't get status information for "
            "table %s (%s)",
            table_name.c_str(), e.format().c_str());
        return result; /* assume table is ok */
      }
      throw;
    }
    auto row = res->fetch_one();
    if (!row) {
      log_error("Error: Couldn't read status information for table %s\n",
                table_name.c_str());
      return result; /* assume table is ok */
    }
    if (row->is_null(1)) {
      *out_table_type = "VIEW";
    } else {
      *out_table_type = row->get_string(1);
    }
  }

  /*  If these two types, we want to skip dumping the table. */
  if ((shcore::str_caseeq(out_table_type->c_str(), "MRG_MyISAM") ||
       *out_table_type == "MRG_ISAM" || *out_table_type == "FEDERATED")) {
    result = IGNORE_DATA;
  }

  return result;
}

/**
  This function sets the session binlog in the dump file.
  When --set-gtid-purged is used, this function is called to
  disable the session binlog and at the end of the dump, to restore
  the session binlog.

  @note: md_result_file should have been opened, before
         this function is called.

  @param[in]      flag          If false, disable binlog.
                                If true and binlog disabled previously,
                                restore the session binlog.
*/

void Schema_dumper::set_session_binlog(IFile *file, bool flag) {
  if (!flag && !is_binlog_disabled) {
    fputs("SET @MYSQLDUMP_TEMP_LOG_BIN = @@SESSION.SQL_LOG_BIN;\n", file);
    fputs("SET @@SESSION.SQL_LOG_BIN= 0;\n", file);
    is_binlog_disabled = true;
  } else if (flag && is_binlog_disabled) {
    fputs("SET @@SESSION.SQL_LOG_BIN = @MYSQLDUMP_TEMP_LOG_BIN;\n", file);
    is_binlog_disabled = false;
  }
}

/**
  This function gets the GTID_EXECUTED sets from the
  server and assigns those sets to GTID_PURGED in the
  dump file.

  @param[in]  mysql_con     connection to the server

  @retval     false         successfully printed GTID_PURGED sets
                             in the dump file.
  @retval     true          failed.

*/

bool Schema_dumper::add_set_gtid_purged(IFile *file) {
  std::shared_ptr<mysqlshdk::db::IResult> gtid_purged_res;

  /* query to get the GTID_EXECUTED */
  if (query_no_throw("SELECT @@GLOBAL.GTID_EXECUTED", &gtid_purged_res))
    return true;

  /* Proceed only if gtid_purged_res is non empty */
  if (auto gtid_set = gtid_purged_res->fetch_one()) {
    if (opt_comments)
      fputs("\n--\n-- GTID state at the beginning of the backup \n--\n\n",
            file);

    const char *comment_suffix = "";
    if (opt_set_gtid_purged_mode == SET_GTID_PURGED_COMMENTED) {
      comment_suffix = "*/";
      fputs("/* SET @@GLOBAL.GTID_PURGED='+", file);
    } else {
      fputs("SET @@GLOBAL.GTID_PURGED=/*!80000 '+'*/ '", file);
    }

    /* close the SET expression */
    fprintf(file, "%s';%s\n", gtid_set->get_string(0).c_str(), comment_suffix);
  }
  // NOTE: original code in mysqldump assumed there can be multiple rows
  // returned by gtid_executed, but that seems wrong???

  return false; /*success */
}

/**
  This function processes the opt_set_gtid_purged option.
  This function also calls set_session_binlog() function before
  setting the SET @@GLOBAL.GTID_PURGED in the output.

  @param[in]          mysql_con     the connection to the server

  @retval             false         successful according to the value
                                    of opt_set_gtid_purged.
  @retval             true          fail.
*/

bool Schema_dumper::process_set_gtid_purged(IFile *file) {
  std::shared_ptr<mysqlshdk::db::IResult> gtid_mode_res;
  std::string gtid_mode_val;

  if (opt_set_gtid_purged_mode == SET_GTID_PURGED_OFF)
    return false; /* nothing to be done */

  /*
    Check if the server has the knowledge of GTIDs(pre mysql-5.6)
    or if the gtid_mode is ON or OFF.
  */

  if (query_no_throw("SHOW VARIABLES LIKE " + quote_for_like("gtid_mode"),
                     &gtid_mode_res))
    return true;

  auto gtid_mode_row = gtid_mode_res->fetch_one();

  /*
     gtid_mode_row is NULL for pre 5.6 versions. For versions >= 5.6,
     get the gtid_mode value from the second column.
  */
  gtid_mode_val = gtid_mode_row ? gtid_mode_row->get_string(1) : "";

  if (!gtid_mode_val.empty() && gtid_mode_val != "OFF") {
    /*
       For any gtid_mode !=OFF and irrespective of --set-gtid-purged
       being AUTO or ON,  add GTID_PURGED in the output.
    */
    if (opt_databases || !opt_alldbs) {
      fprintf(stderr,
              "Warning: A partial dump from a server that has GTIDs will "
              "by default include the GTIDs of all transactions, even "
              "those that changed suppressed parts of the database. If "
              "you don't want to restore GTIDs, pass "
              "--set-gtid-purged=OFF. To make a complete dump, pass "
              "--all-databases --triggers --routines --events. \n");
    }

    set_session_binlog(file, false);
    if (add_set_gtid_purged(file)) {
      return true;
    }
  } else /* gtid_mode is off */
  {
    if (opt_set_gtid_purged_mode == SET_GTID_PURGED_ON ||
        opt_set_gtid_purged_mode == SET_GTID_PURGED_COMMENTED) {
      fprintf(stderr, "Error: Server has GTIDs disabled.\n");
      return true;
    }
  }

  return false;
}

/*
  Getting VIEW structure

  SYNOPSIS
    get_view_structure()
    table   view name
    db      db name

  RETURN
    mysqlaas issues list
*/

std::vector<Compatibility_issue> Schema_dumper::get_view_structure(
    IFile *sql_file, const std::string &table, const std::string &db) {
  std::vector<Compatibility_issue> res;
  std::shared_ptr<mysqlshdk::db::IResult> table_res, infoschema_res;

  log_debug("-- Retrieving view structure for table %s...", table.c_str());

  const auto result_table = shcore::quote_identifier(table);
  std::vector<std::unique_ptr<mysqlshdk::db::Warning>> warnings;
  bool invalid_definition = false;

  if (query_with_binary_charset("SHOW CREATE TABLE " + result_table, &table_res,
                                nullptr, &warnings)) {
    THROW_ERROR(SHERR_DUMP_SD_SHOW_CREATE_VIEW_FAILED, result_table.c_str());
  }

  /* Check if this is a view */
  if (table_res->get_metadata()[0].get_column_label() != "View") {
    log_debug("-- It's base table, skipped");
    return res;
  }

  // BUG#35415976 - detect invalid views
  for (const auto &warning : warnings) {
    if (ER_VIEW_INVALID == warning->code) {
      invalid_definition = true;
      break;
    }
  }

  std::string text = fix_identifier_with_newline(result_table);
  print_comment(sql_file, false,
                "\n--\n-- Final view structure for view %s\n--\n\n",
                text.c_str());

  log_debug("-- Dropping the temporary view structure created");
  fprintf(sql_file, "/*!50001 DROP VIEW IF EXISTS %s*/;\n",
          result_table.c_str());

  if (!m_cache &&
      query_with_binary_charset(
          shcore::sqlformat("SELECT CHECK_OPTION, DEFINER, SECURITY_TYPE, "
                            "CHARACTER_SET_CLIENT, COLLATION_CONNECTION "
                            "FROM information_schema.views "
                            "WHERE table_name=? AND table_schema=?",
                            table, db),
          &infoschema_res)) {
    /*
      Use the raw output from SHOW CREATE TABLE if
       information_schema query fails.
     */
    auto row = table_res->fetch_one();
    fprintf(sql_file, "/*!50001 %s */;\n", row->get_string(1).c_str());
    check_io(sql_file);
  } else {
    std::string ds_view;

    {
      /* Save the result of SHOW CREATE TABLE in ds_view */
      const auto row = table_res->fetch_one();
      ds_view = row->get_string(1);
    }

    std::string client_cs;
    std::string connection_col;

    if (m_cache) {
      const auto &view = m_cache->schemas.at(db).views.at(table);
      client_cs = view.character_set_client;
      connection_col = view.collation_connection;
    } else {
      const auto row = infoschema_res->fetch_one();

      /* Get the result from "select ... information_schema" */
      if (!row) {
        THROW_ERROR(SHERR_DUMP_SD_SHOW_CREATE_VIEW_EMPTY, result_table.c_str());
      }

      client_cs = row->get_string(3);
      connection_col = row->get_string(4);
    }

    const auto qualified_name = quote(db, table);

    check_object_for_definer(Compatibility_issue::Object_type::VIEW,
                             qualified_name, &ds_view, &res);
    check_view_for_table_references(db, table, invalid_definition, &res);

    // BUG#38089433 - handle unsupported collations
    handle_collation_update_variable(Compatibility_issue::Object_type::VIEW,
                                     qualified_name, "COLLATION_CONNECTION",
                                     &connection_col, &res);

    /* Dump view structure to file */
    fprintf(
        sql_file,
        "/*!50001 SET @saved_cs_client          = @@character_set_client */;\n"
        "/*!50001 SET @saved_cs_results         = @@character_set_results */;\n"
        "/*!50001 SET @saved_col_connection     = @@collation_connection */;\n"
        "/*!50001 SET character_set_client      = %s */;\n"
        "/*!50001 SET character_set_results     = %s */;\n"
        "/*!50001 SET collation_connection      = %s */;\n"
        "/*!50001 %s */;\n"
        "/*!50001 SET character_set_client      = @saved_cs_client */;\n"
        "/*!50001 SET character_set_results     = @saved_cs_results */;\n"
        "/*!50001 SET collation_connection      = @saved_col_connection */;\n",
        client_cs.c_str(), client_cs.c_str(), connection_col.c_str(),
        ds_view.c_str());

    check_io(sql_file);
  }

  return res;
}

Schema_dumper::Schema_dumper(
    const std::shared_ptr<mysqlshdk::db::ISession> &mysql)
    : m_mysql(mysql), default_charset(MYSQL_UNIVERSAL_CLIENT_CHARSET) {}

void Schema_dumper::dump_all_tablespaces_ddl(IFile *file) {
  dump_all_tablespaces(file);
}

void Schema_dumper::dump_tablespaces_ddl_for_dbs(
    IFile *file, const std::vector<std::string> &dbs) {
  dump_tablespaces_for_databases(file, dbs);
}

void Schema_dumper::dump_tablespaces_ddl_for_tables(
    IFile *file, const std::string &db,
    const std::vector<std::string> &tables) {
  dump_tablespaces_for_tables(file, db, tables);
}

std::vector<Compatibility_issue> Schema_dumper::dump_schema_ddl(
    IFile *file, const std::string &db) {
  log_debug("Dumping database %s", db.c_str());

  try {
    std::vector<Compatibility_issue> res;

    if (!opt_create_db) {
      const auto qdatabase = shcore::quote_identifier(db);
      const auto result =
          m_mysql->query("SHOW CREATE DATABASE IF NOT EXISTS " + qdatabase);

      if (const auto row = result->fetch_one(); row && !row->is_null(1)) {
        auto createdb = row->get_string(1);

        if (opt_mysqlaas) {
          if (compatibility::check_create_table_for_encryption_option(
                  createdb, &createdb))
            res.emplace_back(
                Compatibility_issue::fixed::schema_encryption(qdatabase));
        }

        {
          // BUG#38089433 - handle unsupported collations
          std::string collation;

          fetch_db_collation(db, &collation, false);

          DBUG_EXECUTE_IF("dumper_unsupported_collation",
                          { use_unsupported_collation(&createdb); });

          handle_collation_update_statement(
              Compatibility_issue::Object_type::SCHEMA, qdatabase,
              "default collation", collation, &createdb, &res);
        }

        print_comment(file, false, "\n--\n-- Dumping database '%s'\n--\n\n",
                      fix_identifier_with_newline(db).c_str());

        Object_guard_msg guard(file, "database", db, {});

        if (opt_drop_database)
          fprintf(file, "/*!40000 DROP DATABASE IF EXISTS %s*/;\n",
                  qdatabase.c_str());

        fprintf(file, "%s;\n", createdb.c_str());
      } else {
        throw std::runtime_error("No create database statement");
      }
    }

    return res;
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_SCHEMA_DDL_ERROR, db.c_str(), e.what());
  }
}

std::vector<Compatibility_issue> Schema_dumper::dump_table_ddl(
    IFile *file, const std::string &db, const std::string &table) {
  try {
    log_debug("Dumping table %s from database %s", table.c_str(), db.c_str());
    init_dumping(file, db, nullptr);
    auto res = dump_table(file, table, db);

    if (opt_column_statistics)
      dump_column_statistics_for_table(file, table, db);
    return res;
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_TABLE_DDL_ERROR, db.c_str(), table.c_str(),
                e.what());
  }
}

void Schema_dumper::dump_temporary_view_ddl(IFile *file, const std::string &db,
                                            const std::string &view) {
  try {
    char ignore_flag;
    std::string table_type;
    log_debug("Dumping view %s (temporary DDL) from database %s", view.c_str(),
              db.c_str());
    init_dumping(file, db, nullptr);
    get_table_structure(file, view, db, &table_type, &ignore_flag);
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_VIEW_TEMPORARY_DDL_ERROR, db.c_str(),
                view.c_str(), e.what());
  }
}

std::vector<Compatibility_issue> Schema_dumper::dump_view_ddl(
    IFile *file, const std::string &db, const std::string &view) {
  try {
    log_debug("Dumping view %s from database %s", view.c_str(), db.c_str());
    init_dumping(file, db, nullptr);
    return get_view_structure(file, view, db);
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_VIEW_DDL_ERROR, db.c_str(), view.c_str(),
                e.what());
  }
}

int Schema_dumper::count_triggers_for_table(const std::string &db,
                                            const std::string &table) {
  if (m_cache) {
    return m_cache->schemas.at(db).tables.at(table).triggers.size();
  } else {
    const auto res = query_log_error(
        "select count(TRIGGER_NAME) from information_schema.triggers where "
        "TRIGGER_SCHEMA = ? and EVENT_OBJECT_TABLE = ?;",
        db.c_str(), table.c_str());
    if (auto row = res->fetch_one()) return row->get_int(0);
    THROW_ERROR(SHERR_DUMP_SD_TRIGGER_COUNT_ERROR, db.c_str(), table.c_str());
  }
}

std::vector<Compatibility_issue> Schema_dumper::dump_triggers_for_table_ddl(
    IFile *file, const std::string &db, const std::string &table) {
  try {
    log_debug("Dumping triggers for table %s from database %s", table.c_str(),
              db.c_str());
    init_dumping(file, db, nullptr);
    return dump_triggers_for_table(file, table, db);
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_TRIGGER_DDL_ERROR, db.c_str(), table.c_str(),
                e.what());
  }
}

std::vector<std::string> Schema_dumper::get_triggers(const std::string &db,
                                                     const std::string &table) {
  std::vector<std::string> triggers;

  if (m_cache) {
    for (const auto &trigger :
         m_cache->schemas.at(db).tables.at(table).triggers) {
      triggers.emplace_back(trigger);
    }
  } else {
    use(db);
    auto show_triggers_rs =
        query_log_and_throw("SHOW TRIGGERS LIKE " + quote_for_like(table));

    while (auto row = show_triggers_rs->fetch_one()) {
      triggers.emplace_back(row->get_string(0));
    }
  }
  return triggers;
}

std::vector<Compatibility_issue> Schema_dumper::dump_events_ddl(
    IFile *file, const std::string &db) {
  try {
    log_debug("Dumping events for database %s", db.c_str());
    init_dumping(file, db, nullptr);
    return dump_events_for_db(file, db);
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_EVENT_DDL_ERROR, db.c_str(), e.what());
  }
}

std::vector<std::string> Schema_dumper::get_events(const std::string &db) {
  std::vector<std::string> events;

  if (m_cache) {
    for (const auto &event : m_cache->schemas.at(db).events) {
      events.emplace_back(event);
    }
  } else {
    use(db);
    auto event_res = query_log_and_throw("show events");

    while (auto row = event_res->fetch_one()) {
      events.emplace_back(row->get_string(1));
    }
  }
  return events;
}

std::vector<Compatibility_issue> Schema_dumper::dump_routines_ddl(
    IFile *file, const std::string &db) {
  try {
    log_debug("Dumping routines for database %s", db.c_str());
    init_dumping(file, db, nullptr);
    return dump_routines_for_db(file, db);
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_ROUTINE_DDL_ERROR, db.c_str(), e.what());
  }
}

std::vector<std::string> Schema_dumper::get_routines(const std::string &db,
                                                     const std::string &type) {
  std::vector<std::string> routine_list;

  if (m_cache) {
    const auto &schema = m_cache->schemas.at(db);
    const auto &routines =
        "PROCEDURE" == type ? schema.procedures : schema.functions;

    for (const auto &routine : routines) {
      routine_list.emplace_back(routine.first);
    }
  } else {
    use(db);

    shcore::sqlstring query(
        shcore::str_format("SHOW %s STATUS WHERE Db = ?", type.c_str()), 0);
    query << db;

    auto routine_list_res = query_log_and_throw(query.str());

    while (auto routine_list_row = routine_list_res->fetch_one()) {
      routine_list.emplace_back(routine_list_row->get_string(1));
    }
  }

  return routine_list;
}

std::vector<Instance_cache::Routine::Library_reference>
Schema_dumper::get_routine_dependencies(const std::string &db,
                                        const std::string &routine,
                                        const std::string_view type) {
  std::vector<Instance_cache::Routine::Library_reference> dependency_list;

  if (m_cache) {
    const auto &schema = m_cache->schemas.at(db);
    const auto &routines =
        "PROCEDURE" == type ? schema.procedures : schema.functions;

    dependency_list = routines.at(routine).library_references;
  } else if (compatibility::supports_library_ddl(
                 m_mysql->get_server_version())) {
    const auto dependency_list_res = query_log_and_throw(shcore::sqlformat(
        "SELECT"
        " IFNULL(l.library_schema, rl.library_schema),"
        " IFNULL(l.library_name, rl.library_name),"
        " l.library_schema IS NOT NULL "
        "FROM information_schema.routine_libraries AS rl"
        " LEFT JOIN information_schema.libraries AS l"
        " ON rl.library_schema=l.library_schema"
        " AND rl.library_name=l.library_name "
        "WHERE ROUTINE_SCHEMA=? AND ROUTINE_NAME=? AND ROUTINE_TYPE=?",
        db, routine, type));

    while (const auto dependency_list_row = dependency_list_res->fetch_one()) {
      dependency_list.emplace_back(Instance_cache::Routine::Library_reference{
          dependency_list_row->get_string(0),
          dependency_list_row->get_string(1),
          1 == dependency_list_row->get_int(2)});
    }
  }

  return dependency_list;
}

std::vector<Compatibility_issue> Schema_dumper::dump_libraries_ddl(
    IFile *file, const std::string &db) {
  try {
    log_debug("Dumping libraries for database %s", db.c_str());
    init_dumping(file, db, nullptr);
    return dump_libraries_for_db(file, db);
  } catch (const std::exception &e) {
    THROW_ERROR(SHERR_DUMP_SD_LIBRARY_DDL_ERROR, db.c_str(), e.what());
  }
}

std::vector<std::string> Schema_dumper::get_libraries(const std::string &db) {
  std::vector<std::string> library_list;

  if (m_cache) {
    const auto &schema = m_cache->schemas.at(db);

    for (const auto &library : schema.libraries) {
      library_list.emplace_back(library);
    }
  } else if (compatibility::supports_library_ddl(
                 m_mysql->get_server_version())) {
    const auto library_list_res = query_log_and_throw(shcore::sqlformat(
        "SELECT LIBRARY_NAME FROM INFORMATION_SCHEMA.LIBRARIES WHERE "
        "LIBRARY_SCHEMA=? ORDER BY LIBRARY_NAME",
        db));

    while (const auto library_list_row = library_list_res->fetch_one()) {
      library_list.emplace_back(library_list_row->get_string(0));
    }
  }

  return library_list;
}

namespace {
enum class Priv_level_type { GLOBAL, SCHEMA, TABLE };

Priv_level_type check_priv_level(const std::string &s, std::string *out_schema,
                                 std::string *out_table) {
  size_t rest;
  shcore::split_priv_level(s, out_schema, out_table, &rest);

  if (*out_schema == "*" && *out_table == "*")
    return Priv_level_type::GLOBAL;
  else if (*out_schema != "*" && *out_table == "*")
    return Priv_level_type::SCHEMA;
  else
    return Priv_level_type::TABLE;
}
}  // namespace

std::string Schema_dumper::expand_all_privileges(const std::string &stmt,
                                                 const std::string &grantee,
                                                 std::string *out_schema) {
  const std::string k_grant_all = "GRANT ALL PRIVILEGES ON ";

  *out_schema = "";

  if (shcore::str_beginswith(stmt, k_grant_all)) {
    std::string target = stmt.substr(k_grant_all.size());
    std::string table;
    std::string query;

    switch (check_priv_level(target, out_schema, &table)) {
      case Priv_level_type::GLOBAL: {
        query =
            "SELECT group_concat(privilege_type) FROM "
            "information_schema.user_privileges WHERE grantee = " +
            shcore::quote_sql_string(grantee);
        break;
      }
      case Priv_level_type::SCHEMA: {
        query =
            "SELECT group_concat(privilege_type) FROM "
            "information_schema.schema_privileges WHERE table_schema = " +
            shcore::quote_sql_string(*out_schema) +
            " AND grantee = " + shcore::quote_sql_string(grantee);
        break;
      }
      case Priv_level_type::TABLE: {
        query =
            "SELECT group_concat(privilege_type) FROM "
            "information_schema.table_privileges WHERE table_schema = " +
            shcore::quote_sql_string(*out_schema) +
            " AND table_name = " + shcore::quote_sql_string(table) +
            " AND grantee = " + shcore::quote_sql_string(grantee);
        break;
      }
    }
    if (!query.empty()) {
      auto r = query_log_and_throw(query);
      auto all_grants = r->fetch_one_or_throw()->get_string(0);

      return "GRANT " + all_grants + " ON " + stmt.substr(k_grant_all.length());
    }
  }

  return stmt;
}

std::vector<Compatibility_issue> Schema_dumper::dump_grants(IFile *file) {
  const auto &filters = m_filters ? *m_filters : Filtering_options{};

  std::vector<Compatibility_issue> problems;
  std::map<std::string, std::string> default_roles;

  log_debug("Dumping grants for server");

  fputs("--\n-- Dumping user accounts\n--\n\n", file);

  const auto roles = get_roles();
  const auto is_role = [&roles](const shcore::Account &a) {
    return roles.end() != std::find(roles.begin(), roles.end(), a);
  };

  using get_grants_t =
      std::function<std::vector<std::string>(const std::string &)>;

  const get_grants_t get_grants_all = [this](const std::string &u) {
    const auto result = query_log_and_throw("SHOW GRANTS FOR " + u);
    std::vector<std::string> grants;

    while (const auto row = result->fetch_one()) {
      grants.emplace_back(row->get_string(0));
    }

    return grants;
  };

  std::unordered_map<std::string, std::vector<std::string>> all_grants_5_6;

  const get_grants_t get_grants_5_6 = [&all_grants_5_6](const std::string &u) {
    return all_grants_5_6.at(u);
  };

  const auto is_5_6 = m_cache
                          ? m_cache->server.version.is_5_6
                          : m_mysql->get_server_version() < Version(5, 7, 0);
  const auto &get_grants = is_5_6 ? get_grants_5_6 : get_grants_all;

  using get_create_user_t = std::function<std::string(const std::string &)>;

  const get_create_user_t get_create_user_5_7_or_8_0 =
      [this](const std::string &u) -> std::string {
    const auto result = query_log_and_throw("SHOW CREATE USER " + u);

    if (const auto row = result->fetch_one()) {
      return row->get_string(0);
    }

    return "";
  };

  const get_create_user_t get_create_user_5_6 = [this, &get_grants_all,
                                                 &all_grants_5_6](
                                                    const std::string &u) {
    auto grants = get_grants_all(u);
    std::string create_user;

    if (!grants.empty()) {
      // CREATE USER needs to have information about the authentication plugin,
      // so that it can be processed in the subsequent steps, GRANT statement
      // may lack this information, fetch it from the DB
      std::string user;
      std::string host;
      shcore::split_account(u, &user, &host);

      const auto result = query_log_and_throw(shcore::sqlformat(
          "SELECT plugin FROM mysql.user WHERE User=? AND Host=?", user, host));
      const auto plugin = result->fetch_one_or_throw()->get_string(0);

      // first grant contains the information required to recreate the account
      create_user = compatibility::convert_grant_to_create_user(
          grants[0], plugin, &grants[0]);

      all_grants_5_6.emplace(u, std::move(grants));
    }

    return create_user;
  };

  const auto &get_create_user =
      is_5_6 ? get_create_user_5_6 : get_create_user_5_7_or_8_0;

  std::vector<std::string> users;

  for (const auto &u : get_users()) {
    const auto user = shcore::make_account(u);

    if (u.user.find('\'') != std::string::npos) {
      // we don't allow accounts with 's in them because they're incorrectly
      // escaped in the output of SHOW GRANTS, which would generate invalid
      // or dangerous SQL.
      THROW_ERROR(SHERR_DUMP_ACCOUNT_WITH_APOSTROPHE, user.c_str());
    }

    auto create_user = get_create_user(user);

    if (create_user.empty()) {
      current_console()->print_error("No create user statement for user " +
                                     user);
      return problems;
    }

    create_user = shcore::str_replace(create_user, "CREATE USER",
                                      "CREATE USER IF NOT EXISTS");

    bool add_user = true;

    if (opt_mysqlaas || opt_skip_invalid_accounts ||
        opt_lock_invalid_accounts) {
      bool account_migrated = false;
      using handler =
          const std::function<Compatibility_issue(const std::string &)> &;

      const auto handle_invalid_account = [&](handler error, handler remove,
                                              handler lock) {
        // we're removing the user from the list unless lock_invalid_accounts
        // is set, account is invalid in MDS, so other checks can be skipped
        add_user = opt_lock_invalid_accounts;

        // notify about migrated account only once
        if (!account_migrated) {
          problems.emplace_back(
              opt_skip_invalid_accounts
                  ? remove(user)
                  : (opt_lock_invalid_accounts ? lock(user) : error(user)));

          account_migrated = opt_lock_invalid_accounts;
        }
      };

      const auto handle_unsupported_plugin = [&](const std::string &plugin) {
        if (opt_lock_invalid_accounts) {
          create_user =
              compatibility::replace_authentication_plugin(create_user, plugin);
        }

        const auto wrap = [&plugin](Compatibility_issue (*f)(
                              const std::string &, const std::string &)) {
          return [&plugin, f](const std::string &us) { return f(us, plugin); };
        };

        handle_invalid_account(
            wrap(&Compatibility_issue::error::user_unsupported_auth_plugin),
            wrap(&Compatibility_issue::fixed::
                     user_unsupported_auth_plugin_removed),
            wrap(&Compatibility_issue::fixed::
                     user_unsupported_auth_plugin_locked));
      };

      for (const auto &plugin :
           compatibility::check_create_user_for_authentication_plugins(
               create_user)) {
        if (!k_mhs_allowed_authentication_plugins.contains(plugin)) {
          handle_unsupported_plugin(plugin);
        } else if (const auto it =
                       k_mhs_deprecated_authentication_plugins.find(plugin);
                   k_mhs_deprecated_authentication_plugins.end() != it) {
          const auto &version_info = it->second;

          if (version_info.removed.has_value() &&
              opt_target_version >= *version_info.removed) {
            handle_unsupported_plugin(plugin);
          } else if (opt_target_version >= version_info.deprecated) {
            auto issue =
                Compatibility_issue::warning::user_deprecated_auth_plugin(
                    user, plugin);

            if ("mysql_native_password" == plugin &&
                opt_target_version >= Version{8, 4, 0}) {
              issue.description += " which is disabled by default";
            }

            problems.emplace_back(std::move(issue));
          }
        }
      }

      if (add_user &&
          compatibility::check_create_user_for_empty_password(create_user)) {
        if (is_role(u)) {
          // the account with an empty password is a role, convert it to
          // CREATE ROLE statement, so that it is loaded without problems when
          // validate_password is active
          create_user =
              compatibility::convert_create_user_to_create_role(create_user);
        } else {
          if (opt_lock_invalid_accounts) {
            create_user = compatibility::replace_empty_passwords(create_user);
          }

          handle_invalid_account(
              &Compatibility_issue::error::user_no_password,
              &Compatibility_issue::fixed::user_no_password_removed,
              &Compatibility_issue::fixed::user_no_password_locked);
        }
      }

      if (account_migrated) {
        // account has been migrated, lock it
        create_user = compatibility::lock_account(create_user);
      }
    }

    if (add_user) {
      fputs("-- begin user " + user + "\n", file);
      auto default_role =
          compatibility::strip_default_role(create_user, &create_user);
      if (!default_role.empty())
        default_roles.emplace(user, std::move(default_role));
      fputs(create_user + ";\n", file);
      fputs("-- end user " + user + "\n\n", file);

      users.emplace_back(std::move(user));
    }
  }

  mysqlshdk::db::Filtering_options::User_filters mhs_roles;
  // some of these accounts are not roles, but that's OK, we're using this
  // filter to downgrade warnings on grant statements on MHS roles (which are
  // automatically excluded) to notes, and if account is not a role, it's not
  // going to be reported as a warning
  mhs_roles.include(common::k_mhs_excluded_users);

  for (const auto &user : users) {
    std::set<std::string> restricted;
    auto grants = get_grants(user);

    for (auto &grant : grants) {
      if (opt_mysqlaas || opt_strip_restricted_grants) {
        if (const auto mysql_table_grant =
                compatibility::is_grant_on_object_from_mysql_schema(grant);
            !mysql_table_grant.empty()) {
          if (opt_strip_restricted_grants) {
            grant.clear();
            problems.emplace_back(
                Compatibility_issue::fixed::user_restricted_grants_on_mysql(
                    user, mysql_table_grant));
            continue;
          } else {
            problems.emplace_back(
                Compatibility_issue::error::user_restricted_grants_on_mysql(
                    user, mysql_table_grant));
          }
        }

        // In MySQL <= 5.7, if a user has all privs, the SHOW GRANTS will say
        // ALL PRIVILEGES, which isn't helpful for filtering out grants.
        // Also, ALL PRIVILEGES can appear even in 8.0 for DB grants
        std::string schema;
        grant = expand_all_privileges(grant, user, &schema);

        // grants on specific user schemas don't need to be filtered
        if (schema.empty() || is_system_schema_or_ndb(schema) ||
            schema == "*") {
          if (auto privs = strip_restricted_grants(user, &grant, &problems);
              !privs.empty()) {
            restricted.insert(std::make_move_iterator(privs.begin()),
                              std::make_move_iterator(privs.end()));
          }
        }
      }

      if (!grant.empty()) {
        compatibility::Privilege_level_info priv;
        bool comment_out = false;

        if (compatibility::parse_grant_statement(grant, &priv)) {
          // check if grant is on an existing object
          using Level = compatibility::Privilege_level_info::Level;

          const auto check_if_missing =
              [this, &priv, &problems, &user, &grant,
               &comment_out](const std::string &query) {
                const auto result =
                    query_log_error(query, priv.schema, priv.object);

                if (result->fetch_one()) {
                  // object exists
                  return;
                }

                const char *object_type;

                switch (priv.level) {
                  case Level::TABLE:
                    object_type = "table";
                    break;

                  case Level::ROUTINE:
                    object_type = "routine";
                    break;

                  default:
                    throw std::logic_error("Unexpected object type");
                }

                if (opt_strip_invalid_grants) {
                  comment_out = true;
                  problems.emplace_back(
                      Compatibility_issue::fixed::user_invalid_grants(
                          user, object_type, grant));
                } else {
                  problems.emplace_back(
                      Compatibility_issue::error::user_invalid_grants(
                          user, object_type, grant));
                }
              };

          bool included_object = true;

          switch (priv.level) {
            case Level::GLOBAL:
              // nothing to do
              break;

            case Level::SCHEMA:
              // NOTE: grants on a missing schema are valid
              // handle wildcard grants at schema level
              if ((opt_mysqlaas || opt_ignore_wildcard_grants ||
                   opt_unescape_wildcard_grants) &&
                  shcore::has_sql_wildcard(priv.schema)) {
                // we report this one always, even if ignore_wildcard_grants is
                // used, as this grant will not work as expected
                if (shcore::has_escaped_sql_wildcard(priv.schema)) {
                  if (opt_unescape_wildcard_grants) {
                    const auto old_schema = priv.schema;
                    priv.schema = shcore::unescape_sql_wildcards(priv.schema);
                    grant = compatibility::to_grant_statement(priv);
                    problems.emplace_back(
                        Compatibility_issue::fixed::user_escaped_wildcard_grant(
                            user, old_schema, priv.schema));
                  } else if (!partial_revokes()) {
                    // only report error if partial_revokes is disabled, if it's
                    // enabled we assume that grants are valid
                    problems.emplace_back(
                        Compatibility_issue::warning::
                            user_escaped_wildcard_grant(user, priv.schema));
                  }
                }

                // we report this one if only unescape_wildcard_grants is used
                if (opt_mysqlaas || opt_ignore_wildcard_grants) {
                  if (opt_ignore_wildcard_grants) {
                    problems.emplace_back(
                        Compatibility_issue::fixed::user_wildcard_grant(user,
                                                                        grant));
                  } else if (!partial_revokes()) {
                    // only report error if partial_revokes is disabled, if it's
                    // enabled we assume that grants are valid
                    problems.emplace_back(
                        Compatibility_issue::error::user_wildcard_grant(user,
                                                                        grant));
                  }
                }
              }

              // check if schema is included in the dump
              if (partial_revokes()) {
                // if partial revokes are enabled, server treats wildcard
                // characters as any other character, we trust that grants are
                // referring to existing schemas and use exact match here
                included_object = filters.schemas().is_included(priv.schema);
              } else {
                if (shcore::has_sql_wildcard(priv.schema)) {
                  if (shcore::has_unescaped_sql_wildcard(priv.schema)) {
                    // if there's an unescaped wildcard character we're unable
                    // to verify if such schema is included or not, we don't
                    // warn about these
                    included_object = true;
                  } else {
                    // schema name has only escaped wildcard characters, we're
                    // able to check if this schema is included, but we need to
                    // use wildcard match
                    included_object =
                        filters.schemas().matches_included(priv.schema);
                  }
                } else {
                  // there are no wildcards, use exact match
                  included_object = filters.schemas().is_included(priv.schema);
                }
              }
              break;

            case Level::TABLE:
              // grant on a missing table is invalid if it does not contain a
              // CREATE privilege
              if (0 == priv.privileges.count("CREATE")) {
                check_if_missing(
                    "SELECT TABLE_NAME FROM information_schema.tables WHERE "
                    "TABLE_SCHEMA=? AND TABLE_NAME=?");
              }

              // check if table is included in the dump
              included_object =
                  filters.tables().is_included(priv.schema, priv.object);
              break;

            case Level::ROUTINE:
              // all grants on a missing routine are invalid
              check_if_missing(
                  "SELECT ROUTINE_NAME FROM information_schema.routines WHERE "
                  "ROUTINE_SCHEMA=? AND ROUTINE_NAME=?");

              // check if routine is included in the dump
              // BUG#34764157 routine names are case insensitive, MySQL 5.7 has
              // lower-case names of routines in grant statements
              included_object =
                  filters.routines().is_included_ci(priv.schema, priv.object);
              break;

            case Level::LIBRARY:
              // NOTE: there's no check for missing libraries, as once the
              // library is dropped, the grants are dropped as well, and it's
              // not possible to grant a privilege on non-existent library
              included_object = is_library_included(priv.schema, priv.object);
              break;

            case Level::ROLE:
              for (const auto &role : priv.privileges) {
                if (!filters.users().is_included(role)) {
                  // BUG#38264847 - grants on MHS roles which are automatically
                  // excluded are downgraded to notes
                  problems.emplace_back(
                      mhs_roles.is_included(role)
                          ? Compatibility_issue::note::
                                user_grant_on_missing_role(user, role, grant)
                          : Compatibility_issue::warning::
                                user_grant_on_missing_role(user, role, grant));
                }
              }
              break;
          }

          if (!included_object && !is_system_schema(priv.schema)) {
            problems.emplace_back(
                Compatibility_issue::warning::user_grant_on_missing_object(
                    user, grant));
          }
        }

        if (comment_out) {
          grant = "/* " + grant + " */";
        }
      }
    }

    if (!restricted.empty()) {
      problems.emplace_back(
          opt_strip_restricted_grants
              ? Compatibility_issue::fixed::user_restricted_grants(user,
                                                                   restricted)
              : Compatibility_issue::error::user_restricted_grants(user,
                                                                   restricted));
    }

    grants.erase(std::remove_if(grants.begin(), grants.end(),
                                [](const auto &g) { return g.empty(); }),
                 grants.end());

    if (!grants.empty()) {
      fputs("-- begin grants " + user + "\n", file);
      for (const auto &grant : grants) fputs(grant + ";\n", file);
      fputs("-- end grants " + user + "\n\n", file);
    }
  }

  for (const auto &df : default_roles) {
    fputs("-- begin default role " + df.first + "\n", file);
    fprintf(file, "SET %s TO %s;\n", df.second.c_str(), df.first.c_str());
    fputs("-- end default role " + df.first + "\n\n", file);
  }

  fputs("-- End of dumping user accounts\n\n", file);
  return problems;
}

std::vector<shcore::Account> Schema_dumper::get_users() {
  if (m_cache) {
    return m_cache->users;
  }

  try {
    // mysql.user holds the account information data, but some users do not have
    // access to this table, in such case fall back to
    // information_schema.user_privileges, though on servers < 8.0.17, the
    // grantee column is too short and the host name can be truncated
    return fetch_users("SELECT DISTINCT user, host from mysql.user", "", false);
  } catch (const mysqlshdk::db::Error &e) {
    if (m_mysql->get_server_version() < Version(8, 0, 17)) {
      log_warning(
          "Failed to get account information from the mysql.user table: %s. "
          "Falling back to information_schema.user_privileges, data might be "
          "inaccurate if the host name is too long.",
          e.format().c_str());
    }

    constexpr auto users_query =
        "SELECT * FROM (SELECT DISTINCT "
        "SUBSTR(grantee, 2, LENGTH(grantee)-LOCATE('@', REVERSE(grantee))-2)"
        " AS user, "
        "SUBSTR(grantee, LENGTH(grantee)-LOCATE('@', REVERSE(grantee))+3,"
        " LOCATE('@', REVERSE(grantee))-3) AS host "
        "FROM information_schema.user_privileges) AS user";

    return fetch_users(users_query, "");
  }
}

std::vector<shcore::Account> Schema_dumper::get_roles() {
  if (m_cache) {
    return m_cache->roles;
  }

  if (m_mysql->get_server_version() < Version(8, 0, 0)) {
    return {};
  }

  try {
    // check if server supports roles
    m_mysql->query("SELECT @@GLOBAL.activate_all_roles_on_login");
  } catch (const mysqlshdk::db::Error &e) {
    if (ER_UNKNOWN_SYSTEM_VARIABLE == e.code()) {
      // roles are not supported
      current_console()->print_warning("Failed to fetch role information.");
      return {};
    } else {
      // report any other error
      throw;
    }
  }

  return fetch_users("SELECT DISTINCT user, host FROM mysql.user",
                     "authentication_string='' AND account_locked='Y' AND "
                     "password_expired='Y'");
}

std::vector<Schema_dumper::User_statements>
Schema_dumper::preprocess_users_script(
    const std::string &script,
    const std::function<bool(const std::string &)> &include_user_cb,
    const std::function<bool(const std::string &, const std::string &)>
        &strip_privilege_cb) {
  static constexpr const char *k_begin_cmt = "-- begin ";
  static constexpr const char *k_create_user_cmt = "-- begin user ";
  static constexpr const char *k_grant_cmt = "-- begin grants ";
  static constexpr auto k_default_role_cmt = "-- begin default role ";
  static constexpr const char *k_end_cmt = "-- end ";

  const auto console = current_console();
  std::vector<User_statements> result;
  User_statements *current = &result.emplace_back();
  const auto start_new_statements = [&result, &current]() {
    // remove entries with no statements
    if (current->statements.empty()) {
      result.pop_back();
    }

    current = &result.emplace_back();
  };
  bool skip = false;

  for (auto line : shcore::str_split(script, "\n")) {
    line = shcore::str_strip(line);

    if (line.empty()) {
      continue;
    }

    if (shcore::str_beginswith(line, k_begin_cmt)) {
      using Type = User_statements::Type;

      start_new_statements();

      auto account_pos = std::string::npos;

      if (shcore::str_beginswith(line, k_create_user_cmt)) {
        current->type = Type::CREATE_USER;
        account_pos = strlen(k_create_user_cmt);
      } else if (shcore::str_beginswith(line, k_default_role_cmt)) {
        current->type = Type::DEFAULT_ROLE;
        account_pos = strlen(k_default_role_cmt);
      } else if (shcore::str_beginswith(line, k_grant_cmt)) {
        current->type = Type::GRANT;
        account_pos = strlen(k_grant_cmt);
      }

      if (std::string::npos == account_pos) {
        account_pos = line.find_first_of("'`\"", strlen(k_begin_cmt));
      }

      if (std::string::npos != account_pos) {
        current->account = line.substr(account_pos);

        // Skip create and grant statements for the excluded users
        if (!include_user_cb(current->account)) {
          skip = true;

          std::string what;

          switch (current->type) {
            case Type::CREATE_USER:
              what = "CREATE/ALTER USER";
              break;

            case Type::DEFAULT_ROLE:
              what = "DEFAULT ROLE";
              break;

            case Type::GRANT:
              what = "GRANT/REVOKE";
              break;

            case Type::UNKNOWN:
              break;
          }

          if (!what.empty()) {
            what += ' ';
          }

          console->print_note("Skipping " + what + "statements for user " +
                              current->account);
        }
      }
    }

    if (skip) {
      log_info("Skipping %s", line.c_str());
    } else {
      if (shcore::str_ibeginswith(line, "REVOKE") ||
          shcore::str_ibeginswith(line, "GRANT")) {
        if (strip_privilege_cb) {
          // filter privileges
          auto fixed = compatibility::filter_grant_or_revoke(
              line, [&](bool /*is_revoke*/, const std::string &priv_type,
                        const std::string & /*column_list*/,
                        const std::string &object_type,
                        const std::string &priv_level) {
                if (!object_type.empty() &&
                    !shcore::str_caseeq(object_type, "TABLE"))
                  return true;
                return !strip_privilege_cb(priv_type, priv_level);
              });

          if (fixed.empty()) {
            console->print_note("Filtered statement with restricted grants: " +
                                line + " -> (skipped)");
            line.clear();
          } else if (fixed != line) {
            console->print_note("Filtered statement with restricted grants: " +
                                line + " -> " + fixed);
            line = std::move(fixed);
          }
        }
      }

      if (!line.empty()) {
        current->statements.emplace_back(std::move(line));
      }
    }

    if (shcore::str_beginswith(line, k_end_cmt)) {
      skip = false;
      start_new_statements();
    }
  }

  return result;
}

std::vector<shcore::Account> Schema_dumper::fetch_users(
    const std::string &select, const std::string &where, bool log_error) {
  std::string where_filter = where;

  if (!where_filter.empty()) {
    where_filter += " ";
  }

  {
    const auto filter = [](const std::vector<shcore::Account> &accounts) {
      std::vector<std::string> result;

      for (const auto &account : accounts) {
        if (account.host.empty()) {
          result.emplace_back(shcore::sqlstring("(user.user=?)", 0)
                              << account.user);
        } else {
          result.emplace_back(
              shcore::sqlstring("(user.user=? AND user.host=?)", 0)
              << account.user << account.host);
        }
      }

      return result;
    };
    const auto &filters =
        m_filters ? m_filters->users() : Filtering_options::User_filters{};
    const auto include = filter(filters.included());
    const auto exclude = filter(filters.excluded());

    if (!include.empty()) {
      if (!where_filter.empty()) {
        where_filter += "AND";
      }

      where_filter += "(" + shcore::str_join(include, "OR") + ")";
    }

    if (!exclude.empty()) {
      constexpr auto condition = "AND NOT";

      if (where_filter.empty()) {
        where_filter += "NOT";
      } else {
        where_filter += condition;
      }

      where_filter += shcore::str_join(exclude, condition);
    }

    if (!where_filter.empty()) {
      where_filter = " WHERE " + where_filter;
    }
  }

  auto users_res = log_error ? query_log_and_throw(select + where_filter)
                             : m_mysql->query(select + where_filter);

  std::set<shcore::Account> users;

  while (const auto u = users_res->fetch_one()) {
    shcore::Account account;
    account.user = u->get_string(0);
    account.host = u->get_string(1);
    users.emplace(std::move(account));
  }

  return {std::make_move_iterator(users.begin()),
          std::make_move_iterator(users.end())};
}

bool Schema_dumper::partial_revokes() const {
  if (m_cache) {
    return m_cache->server.sysvars.partial_revokes.value_or(false);
  }

  if (!m_partial_revokes.has_value()) {
    m_partial_revokes = mysqlshdk::mysql::Instance(m_mysql)
                            .get_sysvar_bool("partial_revokes")
                            .value_or(false);
  }

  return *m_partial_revokes;
}

Schema_dumper::Version_dependent_check Schema_dumper::set_any_definer_check()
    const {
  Version_dependent_check check;

  check.supported =
      compatibility::supports_set_any_definer_privilege(opt_target_version);
  check.deprecated.report_errors =
      !check.supported || opt_report_deprecated_errors_as_warnings;
  check.deprecated.downgrade_errors = check.supported;

  return check;
}

std::vector<std::string> Schema_dumper::strip_restricted_grants(
    const std::string &user, std::string *ddl,
    std::vector<Compatibility_issue> *issues) const {
  if (!(opt_mysqlaas || opt_strip_restricted_grants)) {
    return {};
  }

  const auto rewritten = opt_strip_restricted_grants ? ddl : nullptr;

  const auto set_any_definer = set_any_definer_check();
  auto allowed_privs = compatibility::k_mysqlaas_allowed_privileges;

  if (set_any_definer.supported) {
    static constexpr std::string_view k_set_user_id = "SET_USER_ID";
    static constexpr std::string_view k_set_any_definer = "SET_ANY_DEFINER";

    if (rewritten && compatibility::replace_keyword(
                         *ddl, k_set_user_id, k_set_any_definer, rewritten)) {
      issues->emplace_back(
          Compatibility_issue::fixed::user_restricted_grants_replaced(
              user, k_set_user_id, k_set_any_definer));
    }

    allowed_privs.emplace(k_set_any_definer);
  }

  return compatibility::check_privileges(*ddl, rewritten, allowed_privs);
}

void Schema_dumper::check_view_for_table_references(
    const std::string &db, const std::string &name, bool has_invalid_definition,
    std::vector<Compatibility_issue> *issues) const {
  if (!m_cache) {
    return;
  }

  assert(m_cache->server.sysvars.lower_case_table_names >= 0 &&
         m_cache->server.sysvars.lower_case_table_names <= 2);

  const auto case_insensitive_search =
      2 == m_cache->server.sysvars.lower_case_table_names;
  const auto qualified_name = quote(db, name);

  // NOTE: Invalid views (i.e. referencing non-existing tables/columns) are
  // detected earlier (when fetching CREATE VIEW statement), we report the error
  // here.

  if (has_invalid_definition) {
    issues->emplace_back(
        Compatibility_issue::error::view_invalid_definition(qualified_name));
    return;
  }

  for (const auto &ref :
       m_cache->schemas.at(db).views.at(name).table_references) {
    bool included = false;

    if (const auto schema = m_cache->schemas.find(ref.schema);
        m_cache->schemas.end() != schema) {
      if (schema->second.tables.contains(ref.table) ||
          schema->second.views.contains(ref.table)) {
        included = true;
      }
    }

    if (!included && case_insensitive_search) {
      bool found = false;

      if (const auto schema =
              m_cache->schemas_lowercase.find(shcore::utf8_lower(ref.schema));
          m_cache->schemas_lowercase.end() != schema) {
        const auto table_lowercase = shcore::utf8_lower(ref.table);

        if (schema->second->tables_lowercase.contains(table_lowercase) ||
            schema->second->views_lowercase.contains(table_lowercase)) {
          found = true;
        }
      }

      if (found) {
        issues->emplace_back(
            opt_mysqlaas
                ? Compatibility_issue::error::view_mismatched_reference(
                      qualified_name, quote(ref.schema, ref.table))
                : Compatibility_issue::warning::view_mismatched_reference(
                      qualified_name, quote(ref.schema, ref.table)));
        // don't report the other error
        included = true;
      }
    }

    if (!included) {
      issues->emplace_back(Compatibility_issue::warning::view_invalid_reference(
          qualified_name, quote(ref.schema, ref.table)));
    }
  }
}

void Schema_dumper::check_routine_for_dependencies(
    const std::string &db, const std::string &name,
    Compatibility_issue::Object_type type,
    std::vector<Compatibility_issue> *issues) const {
  if (!m_cache || !m_filters) {
    return;
  }

  const auto qualified_name = quote(db, name);
  const auto &schema = m_cache->schemas.at(db);
  const auto &routine =
      (Compatibility_issue::Object_type::PROCEDURE == type ? schema.procedures
                                                           : schema.functions)
          .at(name);
  const auto quote_library =
      [](const Instance_cache::Routine::Library_reference &dep) {
        return quote(dep.schema, dep.library);
      };

  for (const auto &dependency : routine.library_references) {
    if (!dependency.exists) {
      issues->emplace_back(
          Compatibility_issue::warning::routine_missing_dependency(
              type, qualified_name, quote_library(dependency)));
    } else if (!is_library_included(dependency.schema, dependency.library)) {
      issues->emplace_back(
          Compatibility_issue::note::routine_missing_dependency(
              type, qualified_name, quote_library(dependency)));
    }
  }
}

bool Schema_dumper::is_library_included(const std::string &schema,
                                        const std::string &library) const {
  // if there are no filters, we assume that library is included
  if (m_filters && !m_filters->libraries().is_included_ci(schema, library)) {
    return false;
  }

  // if there's no cache, we assume that library is being dumped
  if (m_cache) {
    if (const auto s = m_cache->schemas.find(schema);
        m_cache->schemas.end() != s) {
      const auto &libraries = s->second.libraries;

      if (!libraries.contains(library)) {
        // case-sensitive match failed, try case-insensitive one
        if (libraries.end() ==
            std::find_if(libraries.begin(), libraries.end(),
                         [library_ci = shcore::utf8_lower(library)](
                             const std::string &l) {
                           return library_ci == shcore::utf8_lower(l);
                         })) {
          return false;
        }
      }
    } else {
      // schema is not included
      return false;
    }
  }

  return true;
}

}  // namespace dump
}  // namespace mysqlsh
