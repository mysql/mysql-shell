/*
   Copyright (c) 2000, 2020, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

// Dump a table's contents and format to a text file.
// Adapted from mysqldump.cc

#define DUMP_VERSION "1.0.0"

#include "include/mysh_config.h"

#include "modules/util/dump/schema_dumper.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <array>
#include <functional>
#include <regex>
#include <unordered_set>

#include "m_ctype.h"
#include "my_sys.h"
#include "mysqld_error.h"

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

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
    std::string quoted_db_name = shcore::quote_identifier_if_needed(db_name);

    CHARSET_INFO *db_cl =
        get_charset_by_name(required_db_cl_name.c_str(), MYF(0));

    if (!db_cl)
      throw std::runtime_error("Can't get charset: " + required_db_cl_name);

    fprintf(sql_file, "ALTER DATABASE %s CHARACTER SET %s COLLATE %s %s\n",
            quoted_db_name.c_str(), (const char *)db_cl->csname,
            (const char *)db_cl->name, (const char *)delimiter);

    *db_cl_altered = 1;
    return;
  }
  *db_cl_altered = 0;
}

void restore_db_collation(IFile *sql_file, const std::string &db_name,
                          const char *delimiter, const char *db_cl_name) {
  std::string quoted_db_name = shcore::quote_identifier_if_needed(db_name);

  CHARSET_INFO *db_cl = get_charset_by_name(db_cl_name, MYF(0));

  if (!db_cl)
    throw std::runtime_error(std::string("Unable to find charset: ") +
                             db_cl_name);

  fprintf(sql_file, "ALTER DATABASE %s CHARACTER SET %s COLLATE %s %s\n",
          quoted_db_name.c_str(), (const char *)db_cl->csname,
          (const char *)db_cl->name, (const char *)delimiter);
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
  if (file->error() || errno == 5)
    throw std::runtime_error(
        shcore::str_format("Got errno %d on write", errno));
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
  Object_guard_msg(IFile *file, const std::string &object_type,
                   const std::string &db, const std::string &obj_name)
      : m_file(file),
        m_msg(fix_identifier_with_newline(
            shcore::str_lower(object_type) + " " +
            shcore::quote_identifier_if_needed(db) + "." + obj_name)) {
    fputs("-- begin " + m_msg + "\n", m_file);
  }
  ~Object_guard_msg() { fputs("-- end " + m_msg + "\n\n", m_file); }

 private:
  IFile *m_file;
  const std::string m_msg;
};
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
    print_comment(
        sql_file, false, "-- MySQLShell dump %s  Distrib %s, for %s (%s)\n--\n",
        DUMP_VERSION, shcore::get_long_version(), SYSTEM_TYPE, MACHINE_TYPE);

    std::string host = m_mysql->get_connection_options().get_host();
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
    throw std::runtime_error("Could not execute '" + s + "': " + e.format());
  }
}

int Schema_dumper::query_with_binary_charset(
    const std::string &s, std::shared_ptr<mysqlshdk::db::IResult> *out_result,
    mysqlshdk::db::Error *out_error) {
  switch_character_set_results("binary");
  if (query_no_throw(s, out_result, out_error)) return 1;
  (*out_result)->buffer();
  switch_character_set_results(opt_character_set_results.c_str());
  return 0;
}

void Schema_dumper::fetch_db_collation(const std::string &db,
                                       std::string *out_db_cl_name) {
  bool err_status = false;

  m_mysql->executef("use !", db);
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

  if (err_status)
    throw std::runtime_error(
        "Error processing select @@collation_database; results");
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
    throw std::runtime_error(
        std::string("Unable to set character_set_results to") + cs_name);
  }
}

void Schema_dumper::unescape(IFile *file, const char *pos, size_t length) {
  std::string tmp = m_mysql->escape_string(pos, length);

  fputs("'", file);
  fputs(tmp.c_str(), file);
  fputs("'", file);
  check_io(file);
} /* unescape */

void Schema_dumper::unescape(IFile *file, const std::string &s) {
  std::string tmp = m_mysql->escape_string(s.data(), s.length());

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
                                      int delimiter_max_size) {
  int proposed_length;
  const char *presence;

  delimiter_buff[0] = ';'; /* start with one semicolon, and */

  for (proposed_length = 2; proposed_length < delimiter_max_size;
       delimiter_max_size++) {
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
std::vector<Schema_dumper::Issue> Schema_dumper::dump_events_for_db(
    IFile *sql_file, const std::string &db) {
  std::vector<Issue> res;
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

    for (const auto &event_name : events) {
      log_debug("retrieving CREATE EVENT for %s", event_name.c_str());
      snprintf(query_buff, sizeof(query_buff), "SHOW CREATE EVENT %s",
               event_name.c_str());

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
            fprintf(stderr, "Warning: Can't create delimiter for event '%s'\n",
                    event_name.c_str());
            throw std::runtime_error("Can't create delimiter for event " +
                                     event_name);
          }

          fprintf(sql_file, "DELIMITER %s\n", delimiter);

          if (event_res->get_metadata().size() >= 7) {
            switch_db_collation(sql_file, db_name, delimiter, db_cl_name,
                                row->get_string(6), &db_cl_altered);

            auto client_cs = row->get_string(4);
            auto connection_col = row->get_string(5);
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

          check_object_for_definer(db, "Event", event_name, &ces, &res);

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

std::vector<Schema_dumper::Issue> Schema_dumper::dump_routines_for_db(
    IFile *sql_file, const std::string &db) {
  std::vector<Issue> res;
  char query_buff[QUERY_LENGTH];
  const std::array<std::string, 2> routine_types = {"FUNCTION", "PROCEDURE"};
  std::string db_name;

  std::string db_cl_name;
  int db_cl_altered = false;

  db_name = m_mysql->escape_string(db);

  /* nice comments */
  std::string routines_text = fix_identifier_with_newline(db);
  print_comment(sql_file, false,
                "\n--\n-- Dumping routines for database '%s'\n--\n\n",
                routines_text.c_str());

  /*
    not using "mysql_query_with_error_report" because we may have not
    enough privileges to lock mysql.proc.
  */
  if (lock_tables) {
    try {
      m_mysql->execute("LOCK TABLES mysql.proc READ");
    } catch (const mysqlshdk::db::Error &e) {
      log_debug("LOCK TABLES mysql.proc READ failed: %s", e.format().c_str());
    }
  }

  /* Get database collation. */

  fetch_db_collation(db, &db_cl_name);

  switch_character_set_results("binary");

  /* 0, retrieve and dump functions, 1, procedures */
  for (const auto &routine_type : routine_types) {
    const auto routine_list = get_routines(db, routine_type);
    for (const auto &routine_name : routine_list) {
      log_debug("retrieving CREATE %s for %s", routine_type.c_str(),
                routine_name.c_str());
      snprintf(query_buff, sizeof(query_buff), "SHOW CREATE %s %s",
               routine_type.c_str(), routine_name.c_str());

      auto routine_res = query_log_and_throw(query_buff);
      while (auto row = routine_res->fetch_one()) {
        /*
          if the user has EXECUTE privilege he see routine names, but NOT
          the routine body of other routines that are not the creator of!
        */
        std::string body = row->is_null(2) ? "" : row->get_string(2);
        log_debug("length of body for %s row[2] '%s' is %zu",
                  routine_name.c_str(),
                  !row->is_null(2) ? body.c_str() : "(null)", body.length());
        if (row->is_null(2)) {
          print_comment(sql_file, true, "\n-- insufficient privileges to %s\n",
                        query_buff);

          std::string text = fix_identifier_with_newline(
              m_mysql->get_connection_options().get_user());
          print_comment(sql_file, true,
                        "-- does %s have permissions on mysql.proc?\n\n",
                        text.c_str());

          throw std::runtime_error(shcore::str_format(
              "%s has insufficient privileges to %s!",
              m_mysql->get_connection_options().get_user().c_str(),
              query_buff));
        } else if (body.length() > 0) {
          Object_guard_msg guard(sql_file, routine_type, db, routine_name);
          if (opt_drop_routine || opt_reexecutable)
            fprintf(sql_file, "/*!50003 DROP %s IF EXISTS %s */;\n",
                    routine_type.c_str(), routine_name.c_str());

          if (routine_res->get_metadata().size() >= 6) {
            switch_db_collation(sql_file, db_name, ";", db_cl_name,
                                row->get_string(5), &db_cl_altered);

            auto client_cs = row->get_string(3);
            auto connection_col = row->get_string(4);
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

          check_object_for_definer(db, routine_type, routine_name, &body, &res);

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

  if (lock_tables) {
    try {
      m_mysql->execute("UNLOCK TABLES");
    } catch (const mysqlshdk::db::Error &e) {
      log_debug("UNLOCK TABLES failed: %s", e.format().c_str());
    }
  }

  return res;
}

bool Schema_dumper::is_charset_supported(const std::string &charset) {
  bool supported = false;
  for (const auto &m : m_mysqlaas_supported_charsets)
    // beginswith to support checking collations
    if (shcore::str_ibeginswith(charset, m)) supported = true;
  return supported;
}

std::vector<Schema_dumper::Issue> Schema_dumper::check_ct_for_mysqlaas(
    const std::string &db, const std::string &table,
    std::string *create_table) {
  std::vector<Schema_dumper::Issue> res;
  const auto prefix = "Table '" + db + "'.'" + table + "' ";

  if (opt_pk_mandatory_check) {
    try {
      // check if table has primary key
      auto result = m_mysql->queryf(
          "SELECT count(column_name) FROM information_schema.columns where "
          "table_schema = ? and table_name = ? and column_key = 'PRI';",
          db.c_str(), table.c_str());
      if (auto row = result->fetch_one()) {
        if (row->get_int(0) == 0)
          res.emplace_back(
              prefix + "does not have primary or unique non null key defined",
              false);
      } else {
        throw std::runtime_error("Table not present in information_schema");
      }
    } catch (const std::exception &e) {
      log_error("Exception caught when checking private keys for t%s: %s",
                prefix.c_str() + 1, e.what());
    }
  }

  if (opt_mysqlaas) {
    if (compatibility::check_create_table_for_data_index_dir_option(
            *create_table, create_table))
      res.emplace_back(
          prefix + "had {DATA|INDEX} DIRECTORY table option commented out",
          true);

    if (compatibility::check_create_table_for_encryption_option(*create_table,
                                                                create_table))
      res.emplace_back(prefix + "had ENCRYPTION table option commented out",
                       true);
  }

  if (opt_mysqlaas || opt_force_innodb) {
    const auto engine = compatibility::check_create_table_for_engine_option(
        *create_table, opt_force_innodb ? create_table : nullptr);
    if (!engine.empty()) {
      if (opt_force_innodb)
        res.emplace_back(
            prefix + "had unsupported engine " + engine + " changed to InnoDB",
            opt_force_innodb);
      else
        res.emplace_back(prefix + "uses unsupported storage engine " + engine,
                         opt_force_innodb);
    }
  }

  if (opt_mysqlaas || opt_strip_tablespaces) {
    if (compatibility::check_create_table_for_tablespace_option(
            *create_table, opt_strip_tablespaces ? create_table : nullptr)) {
      if (opt_strip_tablespaces)
        res.emplace_back(prefix + "had unsupported tablespace option removed",
                         opt_strip_tablespaces);
      else
        res.emplace_back(prefix + "uses unsupported tablespace option",
                         opt_strip_tablespaces);
    }
  }
  return res;
}

namespace {
std::string get_object_err_prefix(const std::string &db,
                                  const std::string &object,
                                  const std::string &name) {
  return static_cast<char>(std::toupper(object[0])) +
         shcore::str_lower(object.substr(1)) + " " +
         shcore::quote_identifier_if_needed(db) + "." +
         shcore::quote_identifier_if_needed(name) + " ";
}
}  // namespace

void Schema_dumper::check_object_for_definer(const std::string &db,
                                             const std::string &object,
                                             const std::string &name,
                                             std::string *ddl,
                                             std::vector<Issue> *issues) {
  if (opt_mysqlaas || opt_strip_definer) {
    const auto user = compatibility::check_statement_for_definer_clause(
        *ddl, opt_strip_definer ? ddl : nullptr);
    if (!user.empty()) {
      const auto prefix = get_object_err_prefix(db, object, name);
      if (opt_strip_definer) {
        issues->emplace_back(prefix + "had definer clause removed", true);
        if (compatibility::check_statement_for_sqlsecurity_clause(*ddl, ddl))
          issues->back().description +=
              " and SQL SECURITY characteristic set to INVOKER";
      } else {
        issues->emplace_back(
            prefix + "- definition uses DEFINER clause set to user " + user +
                " which can only be executed by this user or a user with "
                "SET_USER_ID or SUPER privileges",
            false);
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
    real_columns- Contains one byte per column, 0 means unused, 1 is used
                  Generated columns are marked as unused
  RETURN
    vector with compatibility issues with mysqlaas
*/

std::vector<Schema_dumper::Issue> Schema_dumper::get_table_structure(
    IFile *sql_file, const std::string &table, const std::string &db,
    std::string *out_table_type, char *ignore_flag, bool real_columns[]) {
  std::vector<Issue> res;
  bool init = false, skip_ddl;
  std::string result_table, opt_quoted_table;
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
  unsigned int colno;
  std::shared_ptr<mysqlshdk::db::IResult> result;
  mysqlshdk::db::Error error;

  *ignore_flag = check_if_ignore_table(table, out_table_type);

  /*
    for mysql.innodb_table_stats, mysql.innodb_index_stats tables we
    dont dump DDL
  */
  skip_ddl = innodb_stats_tables(db, table);

  log_debug("-- Retrieving table structure for table %s...", table.c_str());

  result_table = shcore::quote_identifier(table);
  opt_quoted_table = shcore::quote_identifier_if_needed(table);

  if (!execute_no_throw("SET SQL_QUOTE_SHOW_CREATE=1")) {
    /* using SHOW CREATE statement */
    if (!skip_ddl) {
      /* Make an sql-file, if path was given iow. option -T was given */
      if (query_with_binary_charset("show create table " + result_table,
                                    &result, &error))
        throw std::runtime_error("Failed running: show create table " +
                                 result_table + " with error: " + error.what());

      auto row = result->fetch_one();
      if (!row)
        throw std::runtime_error("Empty create table for table: " + table);
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
          fprintf(sql_file, "DROP TABLE IF EXISTS %s;\n",
                  opt_quoted_table.c_str());
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

          throw std::runtime_error("SHOW FIELDS FROM failed on view: " +
                                   result_table);
        }

        row = result->fetch_one();
        if (row != nullptr) {
          if (opt_drop_view) {
            /*
              We have already dropped any table of the same name above, so
              here we just drop the view.
            */

            fprintf(sql_file, "/*!50001 DROP VIEW IF EXISTS %s*/;\n",
                    opt_quoted_table.c_str());
            check_io(sql_file);
          }

          fprintf(sql_file,
                  "SET @saved_cs_client     = @@character_set_client;\n"
                  "/*!50503 SET character_set_client = utf8mb4 */;\n"
                  "/*!50001 CREATE VIEW %s AS SELECT \n",
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

          fprintf(
              sql_file, " 1 AS %s",
              shcore::quote_identifier_if_needed(row->get_string(0)).c_str());

          while ((row = result->fetch_one())) {
            fprintf(
                sql_file, ",\n 1 AS %s",
                shcore::quote_identifier_if_needed(row->get_string(0)).c_str());
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
    result = query_log_and_throw("show fields from " + result_table);
    colno = 0;
    while (auto row = result->fetch_one()) {
      if (!row->is_null(SHOW_EXTRA)) {
        std::string extra = row->get_string(SHOW_EXTRA);
        real_columns[colno] =
            extra != "STORED GENERATED" && extra != "VIRTUAL GENERATED";
      } else {
        real_columns[colno] = true;
      }
    }
  } else {
    result =
        query_log_and_throw(shcore::sqlformat(show_fields_stmt, db, table));
    {
      std::string text = fix_identifier_with_newline(result_table.c_str());
      print_comment(sql_file, false,
                    "\n--\n-- Table structure for table %s\n--\n\n",
                    text.c_str());

      if (opt_drop_table)
        fprintf(sql_file, "DROP TABLE IF EXISTS %s;\n", result_table.c_str());
      fprintf(sql_file, "CREATE TABLE IF NOT EXISTS %s (\n",
              result_table.c_str());
      check_io(sql_file);
    }

    colno = 0;
    while (auto row = result->fetch_one()) {
      if (!row->is_null(SHOW_EXTRA)) {
        std::string extra = row->get_string(SHOW_EXTRA);
        real_columns[colno] =
            extra != "STORED GENERATED" && extra != "VIRTUAL GENERATED";
      } else {
        real_columns[colno] = true;
      }

      if (!real_columns[colno++]) continue;

      if (init) {
        fputs(",\n", sql_file);
        check_io(sql_file);
      }
      init = true;
      {
        fprintf(
            sql_file, "  %s.%s %s", result_table.c_str(),
            shcore::quote_identifier_if_needed(row->get_string(SHOW_FIELDNAME))
                .c_str(),
            row->get_string(SHOW_TYPE).c_str());

        if (!row->is_null(SHOW_DEFAULT)) {
          fputs(" DEFAULT ", sql_file);
          std::string def = row->get_string(SHOW_DEFAULT);
          unescape(sql_file, def);
        }
        if (!row->get_string(SHOW_NULL).empty()) fputs(" NOT NULL", sql_file);
        if (!row->is_null(SHOW_EXTRA) && !row->get_string(SHOW_EXTRA).empty())
          fprintf(sql_file, " %s", row->get_string(SHOW_EXTRA).c_str());
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
        throw std::runtime_error("Can't get keys for table " + result_table +
                                 ": " + err.format());
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
      result->rewind();
      keynr = 0;
      while (auto row = result->fetch_one()) {
        if (row->get_int(3) == 1) {
          if (keynr++) fputs(")", sql_file);
          if (row->get_int(1)) /* Test if duplicate key */
            /* Duplicate allowed */
            fprintf(
                sql_file, ",\n  KEY %s (",
                shcore::quote_identifier_if_needed(row->get_string(2)).c_str());
          else if (keynr == primary_key)
            fputs(",\n  PRIMARY KEY (", sql_file); /* First UNIQUE is primary */
          else
            fprintf(
                sql_file, ",\n  UNIQUE %s (",
                shcore::quote_identifier_if_needed(row->get_string(2)).c_str());
        } else {
          fputs(",", sql_file);
        }
        fputs(shcore::quote_identifier_if_needed(row->get_string(4)).c_str(),
              sql_file);
        if (!row->is_null(7))
          fprintf(sql_file, " (%s)", row->get_string(7).c_str()); /* Sub key */
        check_io(sql_file);
      }
      if (keynr) fputs(")", sql_file);
      fputs("\n)", sql_file);
      check_io(sql_file);

      /* Get MySQL specific create options */
      if (opt_create_options) {
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
          fputs("/*!", sql_file);
          fprintf(sql_file, "engine=%s", row.get_string("Engine").c_str());
          fprintf(sql_file, "%s", row.get_string("Create_options").c_str());
          fprintf(sql_file, "comment='%s'",
                  m_mysql->escape_string(row.get_string("Comment")).c_str());
          fputs(" */", sql_file);
          check_io(sql_file);
        }
      }
    continue_xml:
      fputs(";\n", sql_file);
      check_io(sql_file);
    }
  }

  return res;
} /* get_table_structure */

std::vector<Schema_dumper::Issue> Schema_dumper::dump_trigger(
    IFile *sql_file,
    const std::shared_ptr<mysqlshdk::db::IResult> &show_create_trigger_rs,
    const std::string &db_name, const std::string &db_cl_name,
    const std::string &trigger_name) {
  int db_cl_altered = false;
  std::vector<Issue> res;

  while (auto row = show_create_trigger_rs->fetch_one()) {
    switch_db_collation(sql_file, db_name, ";", db_cl_name, row->get_string(5),
                        &db_cl_altered);

    auto client_cs = row->get_string(3);
    auto connection_col = row->get_string(4);
    switch_cs_variables(sql_file, ";",
                        client_cs.c_str(),       /* character_set_client */
                        client_cs.c_str(),       /* character_set_results */
                        connection_col.c_str()); /* collation_connection */

    switch_sql_mode(sql_file, ";", row->get_string(1).c_str());

    if (opt_reexecutable || opt_drop_trigger)
      fprintf(sql_file, "/*!50032 DROP TRIGGER IF EXISTS %s */;\n",
              row->get_string(0).c_str());

    // 5.7 server adds schema name to CREATE TRIGGER statement
    const auto trigger_with_schema =
        " TRIGGER " + shcore::quote_identifier(db_name) + ".";

    std::string body = shcore::str_replace(row->get_string(2),
                                           trigger_with_schema, " TRIGGER ");
    check_object_for_definer(db_name, "Trigger", trigger_name, &body, &res);

    fprintf(sql_file,
            "DELIMITER ;;\n"
            "/*!50003 %s */;;\n"
            "DELIMITER ;\n",
            body.c_str());

    restore_sql_mode(sql_file, ";");
    restore_cs_variables(sql_file, ";");

    if (db_cl_altered)
      restore_db_collation(sql_file, db_name, ";", db_cl_name.c_str());
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

std::vector<Schema_dumper::Issue> Schema_dumper::dump_triggers_for_table(
    IFile *sql_file, const std::string &table, const std::string &db) {
  std::vector<Issue> res;
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
    std::shared_ptr<mysqlshdk::db::IResult> show_create_trigger_rs;
    if (query_no_throw("SHOW CREATE TRIGGER " + trigger,
                       &show_create_trigger_rs) == 0) {
      Object_guard_msg guard(sql_file, "trigger", db, trigger);
      const auto out = dump_trigger(sql_file, show_create_trigger_rs, db,
                                    db_cl_name, trigger);
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

std::vector<Schema_dumper::Histogram> Schema_dumper::get_histograms(
    const std::string &db_name, const std::string &table_name) {
  std::vector<Histogram> histograms;

  if (m_mysql->get_server_version() >= mysqlshdk::utils::Version(8, 0, 0)) {
    char query_buff[QUERY_LENGTH * 3 / 2];
    const auto old_ansi_quotes_mode = ansi_quotes_mode;
    const auto escaped_db = m_mysql->escape_string(db_name);
    const auto escaped_table = m_mysql->escape_string(table_name);

    switch_character_set_results("binary");

    /* Get list of columns with statistics. */
    snprintf(query_buff, sizeof(query_buff),
             "SELECT COLUMN_NAME, \
                JSON_EXTRACT(HISTOGRAM, '$.\"number-of-buckets-specified\"') \
              FROM information_schema.COLUMN_STATISTICS \
              WHERE SCHEMA_NAME = '%s' AND TABLE_NAME = '%s';",
             escaped_db.c_str(), escaped_table.c_str());

    const auto column_statistics_rs = query_log_and_throw(query_buff);

    while (auto row = column_statistics_rs->fetch_one()) {
      Histogram histogram;

      histogram.column = shcore::quote_identifier_if_needed(row->get_string(0));
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
  const auto quoted_table = shcore::quote_identifier_if_needed(table_name);

  for (const auto &histogram : get_histograms(db_name, table_name)) {
    fprintf(sql_file,
            "/*!80002 ANALYZE TABLE %s UPDATE HISTOGRAM ON %s "
            "WITH %zu BUCKETS */;\n",
            quoted_table.c_str(), histogram.column.c_str(), histogram.buckets);
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

std::vector<Schema_dumper::Issue> Schema_dumper::dump_table(
    IFile *file, const std::string &table, const std::string &db) {
  char ignore_flag;
  std::string query_string;
  std::string extended_row;
  std::string table_type;
  std::string result_table, opt_quoted_table;
  //  uint32_t num_fields;
  bool real_columns[MAX_FIELDS];
  std::string order_by;

  return get_table_structure(file, table, db, &table_type, &ignore_flag,
                             real_columns);
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

    std::shared_ptr<mysqlshdk::db::IResult> res;
    if (query_no_throw(
            "SHOW VARIABLES LIKE " + quote_for_like("ndbinfo_version"), &res))
      return 0;

    if (!res->fetch_one()) return 0;

    have_ndbinfo = 1;
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

  m_mysql->executef("USE !", database);

  if (opt_databases || opt_alldbs) {
    std::string qdatabase = opt_quoted
                                ? shcore::quote_identifier_if_needed(database)
                                : shcore::quote_identifier_if_needed(database);

    std::string text = fix_identifier_with_newline(qdatabase.c_str());
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

  if (query_no_throw(query, &table_res)) return nullptr;

  if (auto row = table_res->fetch_one()) {
    return row->get_string(0);
  }

  return "";
}

int Schema_dumper::do_flush_tables_read_lock() {
  /*
    We do first a FLUSH TABLES. If a long update is running, the FLUSH TABLES
    will wait but will not stall the whole mysqld, and when the long update is
    done the FLUSH TABLES WITH READ LOCK will start and succeed quickly. So,
    FLUSH TABLES is to lower the probability of a stage where both mysqldump
    and most client connections are stalled. Of course, if a second long
    update starts between the two FLUSHes, we have that bad stall.
  */
  return (execute_no_throw("FLUSH TABLES") ||
          execute_no_throw("FLUSH TABLES WITH READ LOCK"));
}

int Schema_dumper::do_unlock_tables() {
  return execute_no_throw("UNLOCK TABLES");
}

int Schema_dumper::start_transaction() {
  log_debug("-- Starting transaction...");
  /*
    We use BEGIN for old servers. --single-transaction --master-data will fail
    on old servers, but that's ok as it was already silently broken (it didn't
    do a consistent read, so better tell people frankly, with the error).

    We want the first consistent read to be used for all tables to dump so we
    need the REPEATABLE READ level (not anything lower, for example READ
    COMMITTED would give one new consistent read per dumped table).
  */
  return (execute_no_throw(
              "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ") ||
          execute_no_throw("START TRANSACTION WITH CONSISTENT SNAPSHOT"));
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

char Schema_dumper::check_if_ignore_table(const std::string &table_name,
                                          std::string *out_table_type) {
  char result = IGNORE_NONE;

  std::shared_ptr<mysqlshdk::db::IResult> res;

  try {
    res =
        m_mysql->query("show table status like " + quote_for_like(table_name));
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
  if (row->is_null(1))
    *out_table_type = "VIEW";
  else {
    *out_table_type = row->get_string(1);

    /*  If these two types, we want to skip dumping the table. */
    if ((shcore::str_caseeq(out_table_type->c_str(), "MRG_MyISAM") ||
         *out_table_type == "MRG_ISAM" || *out_table_type == "FEDERATED"))
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

std::vector<Schema_dumper::Issue> Schema_dumper::get_view_structure(
    IFile *sql_file, const std::string &table, const std::string &db) {
  std::vector<Issue> res;
  std::shared_ptr<mysqlshdk::db::IResult> table_res, infoschema_res;
  std::string result_table, opt_quoted_table;

  log_debug("-- Retrieving view structure for table %s...", table.c_str());

  result_table = shcore::quote_identifier(table);
  opt_quoted_table = shcore::quote_identifier_if_needed(table);

  if (query_with_binary_charset("SHOW CREATE TABLE " + result_table,
                                &table_res))
    throw std::runtime_error("Failed: SHOW CREATE TABLE " + result_table);

  /* Check if this is a view */
  if (table_res->get_metadata()[0].get_column_label() != "View") {
    log_debug("-- It's base table, skipped");
    return res;
  }

  std::string text = fix_identifier_with_newline(result_table);
  print_comment(sql_file, false,
                "\n--\n-- Final view structure for view %s\n--\n\n",
                text.c_str());

  log_debug("-- Dropping the temporary view structure created");
  fprintf(sql_file, "/*!50001 DROP VIEW IF EXISTS %s*/;\n",
          opt_quoted_table.c_str());

  if (query_with_binary_charset(
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

    /* Save the result of SHOW CREATE TABLE in ds_view */
    auto row = table_res->fetch_one();
    ds_view = row->get_string(1);

    /* Get the result from "select ... information_schema" */
    if (!(row = infoschema_res->fetch_one()))
      throw std::runtime_error("No information about view: " + table);

    auto client_cs = row->get_string(3);
    auto connection_col = row->get_string(4);
    check_object_for_definer(db, "View", table, &ds_view, &res);

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
    const std::shared_ptr<mysqlshdk::db::ISession> &mysql,
    const std::vector<std::string> &mysqlaas_supported_charsets,
    const std::set<std::string> &mysqlaas_allowed_privileges)
    : m_mysql(mysql),
      m_mysqlaas_supported_charsets(mysqlaas_supported_charsets),
      m_mysqlaas_allowed_priveleges(mysqlaas_allowed_privileges),
      default_charset(MYSQL_UNIVERSAL_CLIENT_CHARSET) {
  std::time_t t = std::time(nullptr);
  m_dump_info = ", server " +
                m_mysql->get_connection_options().as_uri(
                    mysqlshdk::db::uri::formats::only_transport()) +
                ", MySQL " + m_mysql->get_server_version().get_full() + " at " +
                shcore::lexical_cast<std::string>(
                    std::put_time(std::localtime(&t), "%c %Z")) +
                "\n";
}

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

std::vector<Schema_dumper::Issue> Schema_dumper::dump_schema_ddl(
    IFile *file, const std::string &db) {
  try {
    std::vector<Issue> res;
    auto qdatabase = shcore::quote_identifier_if_needed(db);

    print_comment(file, false, "\n--\n-- Current Database: %s\n--\n",
                  fix_identifier_with_newline(db).c_str());

    if (!opt_create_db) {
      char qbuf[256];

      int qlen =
          snprintf(qbuf, sizeof(qbuf), "SHOW CREATE DATABASE IF NOT EXISTS %s",
                   qdatabase.c_str());

      auto result = m_mysql->querys(qbuf, qlen);

      if (opt_drop_database)
        fprintf(file, "\n/*!40000 DROP DATABASE IF EXISTS %s*/;\n",
                qdatabase.c_str());

      auto row = result->fetch_one();
      if (row && !row->is_null(1)) {
        auto createdb = row->get_string(1);

        if (opt_mysqlaas) {
          if (compatibility::check_create_table_for_encryption_option(
                  createdb, &createdb))
            res.emplace_back(
                "Database " + qdatabase +
                    " had unsupported ENCRYPTION option commented out",
                true);
        }

        fprintf(file, "\n%s;\n", createdb.c_str());
      } else {
        throw std::runtime_error("No create database statement");
      }
    }
    fprintf(file, "\nUSE %s;\n", qdatabase.c_str());
    return res;
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping DDL for schema '" + db +
                             "': " + e.what());
  }
}

std::vector<Schema_dumper::Issue> Schema_dumper::dump_table_ddl(
    IFile *file, const std::string &db, const std::string &table) {
  try {
    log_debug("Dumping table %s from database %s", table.c_str(), db.c_str());
    init_dumping(file, db, nullptr);
    auto res = dump_table(file, table, db);

    if (opt_column_statistics)
      dump_column_statistics_for_table(file, table, db);
    return res;
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping DDL for table '" + table +
                             "': " + e.what());
  }
}

void Schema_dumper::dump_temporary_view_ddl(IFile *file, const std::string &db,
                                            const std::string &view) {
  try {
    char ignore_flag;
    std::string table_type;
    //  uint32_t num_fields;
    bool real_columns[MAX_FIELDS];
    log_debug("Dumping view %s temporary ddll from database %s", view.c_str(),
              db.c_str());
    init_dumping(file, db, nullptr);
    get_table_structure(file, view, db, &table_type, &ignore_flag,
                        real_columns);
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping temporary DDL for view '" +
                             view + "': " + e.what());
  }
}

std::vector<Schema_dumper::Issue> Schema_dumper::dump_view_ddl(
    IFile *file, const std::string &db, const std::string &view) {
  try {
    log_debug("Dumping view %s from database %s", view.c_str(), db.c_str());
    init_dumping(file, db, nullptr);
    return get_view_structure(file, view, db);
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping DDL for view '" + view +
                             "': " + e.what());
  }
}

int Schema_dumper::count_triggers_for_table(const std::string &db,
                                            const std::string &table) {
  auto res = m_mysql->queryf(
      "select count(TRIGGER_NAME) from information_schema.triggers where "
      "TRIGGER_SCHEMA = ? and EVENT_OBJECT_TABLE = ?;",
      db.c_str(), table.c_str());
  if (auto row = res->fetch_one()) return row->get_int(0);
  throw std::runtime_error("Unable to check trigger count for table: " + db +
                           "." + table);
}

std::vector<Schema_dumper::Issue> Schema_dumper::dump_triggers_for_table_ddl(
    IFile *file, const std::string &db, const std::string &table) {
  try {
    return dump_triggers_for_table(file, table, db);
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping triggers for table '" +
                             table + "': " + e.what());
  }
}

std::vector<std::string> Schema_dumper::get_triggers(const std::string &db,
                                                     const std::string &table) {
  m_mysql->executef("USE !", db);
  auto show_triggers_rs =
      query_log_and_throw("SHOW TRIGGERS LIKE " + quote_for_like(table));
  std::vector<std::string> triggers;
  while (auto row = show_triggers_rs->fetch_one())
    triggers.emplace_back(
        shcore::quote_identifier_if_needed(row->get_string(0)));
  return triggers;
}

std::vector<Schema_dumper::Issue> Schema_dumper::dump_events_ddl(
    IFile *file, const std::string &db) {
  try {
    log_debug("Dumping events for database %s", db.c_str());
    return dump_events_for_db(file, db);
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping events for schema '" + db +
                             "': " + e.what());
  }
}

std::vector<std::string> Schema_dumper::get_events(const std::string &db) {
  m_mysql->executef("USE !", db);
  auto event_res = query_log_and_throw("show events");
  std::vector<std::string> events;
  while (auto row = event_res->fetch_one())
    events.emplace_back(shcore::quote_identifier_if_needed(row->get_string(1)));
  return events;
}

std::vector<Schema_dumper::Issue> Schema_dumper::dump_routines_ddl(
    IFile *file, const std::string &db) {
  try {
    log_debug("Dumping routines for database %s", db.c_str());
    return dump_routines_for_db(file, db);
  } catch (const std::exception &e) {
    throw std::runtime_error("Error while dumping routines for schema '" + db +
                             "': " + e.what());
  }
}

std::vector<std::string> Schema_dumper::get_routines(const std::string &db,
                                                     const std::string &type) {
  m_mysql->executef("USE !", db);

  shcore::sqlstring query(
      shcore::str_format("SHOW %s STATUS WHERE Db = ?", type.c_str()), 0);
  query << db;

  auto routine_list_res = query_log_and_throw(query.str());
  std::vector<std::string> routine_list;
  while (auto routine_list_row = routine_list_res->fetch_one())
    routine_list.emplace_back(
        shcore::quote_identifier_if_needed(routine_list_row->get_string(1)));
  return routine_list;
}

std::string Schema_dumper::normalize_user(const std::string &user,
                                          const std::string &host) {
  return shcore::escape_sql_string(user) + "@" +
         shcore::escape_sql_string(host);
}

std::vector<Schema_dumper::Issue> Schema_dumper::dump_grants(
    IFile *file, const std::vector<shcore::Account> &included,
    const std::vector<shcore::Account> &excluded) {
  std::vector<Issue> problems;
  std::set<std::string> allowed_privs =
      opt_mysqlaas || opt_strip_restricted_grants
          ? m_mysqlaas_allowed_priveleges
          : std::set<std::string>();
  bool compatibility = opt_strip_restricted_grants;
  log_debug("Dumping grants for server");

  fputs("--\n-- Dumping user accounts\n--\n\n", file);
  const auto users = get_users(included, excluded);
  for (const auto &u : users) {
    auto res = query_log_and_throw("SHOW CREATE USER " + u.second);
    auto row = res->fetch_one();
    if (!row) {
      current_console()->print_error("No create user statement for user " +
                                     u.first);
      return problems;
    }
    auto create_user = row->get_string(0);
    assert(!create_user.empty());
    fputs("-- begin user " + u.first + "\n", file);
    fputs(shcore::str_replace(create_user, "CREATE USER",
                              "CREATE USER IF NOT EXISTS") +
              ";\n",
          file);
    fputs(shcore::str_replace(create_user, "CREATE USER", "/* ALTER USER") +
              "; */\n",
          file);
    fputs("-- end user " + u.first + "\n\n", file);
  }

  for (const auto &u : users) {
    auto res = query_log_and_throw("SHOW GRANTS FOR " + u.second);
    std::vector<std::string> restricted;
    std::vector<std::string> grants;
    for (auto row = res->fetch_one(); row; row = res->fetch_one()) {
      auto grant = row->get_string(0);
      if (opt_mysqlaas || compatibility) {
        std::string rewritten;
        const auto privs = compatibility::check_privileges(
            grant, compatibility ? &rewritten : nullptr, allowed_privs);
        if (!privs.empty()) {
          restricted.insert(restricted.end(), privs.begin(), privs.end());
          if (compatibility) grant = rewritten;
        }
      }
      if (!grant.empty()) grants.emplace_back(grant);
    }

    if (!restricted.empty()) {
      if (opt_strip_restricted_grants) {
        problems.emplace_back(
            "User " + u.first + " had restricted " +
                (restricted.size() > 1 ? "privileges (" : "privilege (") +
                shcore::str_join(restricted, ", ") + ") removed",
            true);
      } else {
        problems.emplace_back(
            "User " + u.first + " is granted restricted " +
                (restricted.size() > 1 ? "privileges: " : "privilege: ") +
                shcore::str_join(restricted, ", "),
            false);
      }
    }
    if (!grants.empty()) {
      fputs("-- begin grants " + u.first + "\n", file);
      for (const auto &grant : grants) fputs(grant + ";\n", file);
      fputs("-- end grants " + u.first + "\n\n", file);
    }
  }
  fputs("-- End of dumping user accounts\n\n", file);
  return problems;
}

std::vector<std::pair<std::string, std::string>> Schema_dumper::get_users(
    const std::vector<shcore::Account> &included,
    const std::vector<shcore::Account> &excluded) {
  std::string where_filter;

  {
    const auto filter = [](const std::vector<shcore::Account> &accounts) {
      std::vector<std::string> result;

      for (const auto &account : accounts) {
        if (account.host.empty()) {
          result.emplace_back(shcore::sqlstring("(users.user=?)", 0)
                              << account.user);
        } else {
          result.emplace_back(
              shcore::sqlstring("(users.user=? AND users.host=?)", 0)
              << account.user << account.host);
        }
      }

      return result;
    };
    const auto include = filter(included);
    const auto exclude = filter(excluded);

    if (!include.empty()) {
      where_filter += "(" + shcore::str_join(include, "OR") + ")";
    }

    if (!exclude.empty()) {
      constexpr auto condition = "AND NOT";

      if (!include.empty()) {
        where_filter += condition;
      } else {
        where_filter += "NOT";
      }

      where_filter += shcore::str_join(exclude, condition);
    }

    if (!where_filter.empty()) {
      where_filter = " WHERE " + where_filter;
    }
  }

  constexpr auto users_query =
      "SELECT * FROM (SELECT DISTINCT "
      "grantee, "
      "SUBSTR(grantee, 2, LENGTH(grantee)-LOCATE('@', REVERSE(grantee))-2)"
      "  AS user, "
      "SUBSTR(grantee, LENGTH(grantee)-LOCATE('@', REVERSE(grantee))+3,"
      "  LOCATE('@', REVERSE(grantee))-3) AS host "
      "FROM information_schema.user_privileges ORDER BY user, host) AS users";

  auto users_res = query_log_and_throw(users_query + where_filter);
  std::vector<std::pair<std::string, std::string>> users;
  for (auto u = users_res->fetch_one(); u; u = users_res->fetch_one()) {
    const auto user = u->get_string(1);
    const auto host = u->get_string(2);
    users.emplace_back(normalize_user(user, host), u->get_string(0));
  }
  return users;
}

const char *Schema_dumper::version() { return DUMP_VERSION; }

std::string Schema_dumper::preprocess_users_script(
    const std::string &script,
    const std::function<bool(const std::string &)> &include_user) {
  std::string out;
  bool skip = false;

  static constexpr const char *k_create_user_cmt = "-- begin user ";
  static constexpr const char *k_grant_cmt = "-- begin grants ";
  static constexpr const char *k_end_cmt = "-- end ";

  // Skip create and grant statements for the excluded users
  for (const auto &line : shcore::str_split(script, "\n")) {
    std::string user;
    bool grant_statement = false;

    if (shcore::str_beginswith(line, k_create_user_cmt)) {
      user = line.substr(strlen(k_create_user_cmt));
    } else if (shcore::str_beginswith(line, k_grant_cmt)) {
      user = line.substr(strlen(k_grant_cmt));
      grant_statement = true;
    }

    if (!user.empty()) {
      if (!include_user(user)) {
        skip = true;
        if (grant_statement) {
          current_console()->print_note("Skipping GRANT statements for user " +
                                        user);
        } else {
          current_console()->print_note(
              "Skipping CREATE/ALTER USER statements for user " + user);
        }
      }
    }

    if (skip) {
      log_info("Skipping %s", line.c_str());
    } else {
      out.append(line + "\n");
    }

    if (shcore::str_beginswith(line, k_end_cmt)) skip = false;
  }

  return out;
}

}  // namespace mysqlsh
