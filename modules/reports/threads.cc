/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#include "modules/reports/threads.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "modules/mod_shell_reports.h"
#include "modules/reports/native_report.h"
#include "modules/reports/utils.h"
#include "modules/reports/utils_options.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysqlshdk::utils::nullable;

namespace mysqlsh {
namespace reports {

namespace {

#define DEFAULT_SORT_ORDER "tid"

#define COLUMN_DESCRIPTIONS                                                    \
  S("Thread identifiers")                                                      \
  X("tid", "t.THREAD_ID", "thread ID")                                         \
  X("cid", "t.PROCESSLIST_ID",                                                 \
    "connection ID in case of threads associated with a user connection, or "  \
    "NULL for a background thread")                                            \
  S("Information about a thread")                                              \
  X("command", "t.PROCESSLIST_COMMAND",                                        \
    "the type of command the thread is executing")                             \
  X("db", "t.PROCESSLIST_DB",                                                  \
    "the default database, if one is selected; otherwise NULL")                \
  X("group", "NULLIF(CONCAT(''/*!80000, t.RESOURCE_GROUP*/), '')",             \
    "the resource group label; this value is NULL if resource groups are not " \
    "supported on the current platform or server configuration")               \
  X("history", "t.HISTORY",                                                    \
    "whether event history logging is enabled for the thread")                 \
  X("host", "t.PROCESSLIST_HOST",                                              \
    "the host name of the client who issued the statement, or NULL for a "     \
    "background thread")                                                       \
  X("instr", "t.INSTRUMENTED",                                                 \
    "whether events executed by the thread are instrumented")                  \
  X("lastwait", "p.last_wait",                                                 \
    "the name of the most recent wait event for the thread")                   \
  X("lastwaitl", "p.last_wait_latency",                                        \
    "the wait time of the most recent wait event for the thread")              \
  X("memory", "p.current_memory",                                              \
    "the number of bytes allocated by the thread")                             \
  X("name", "replace(t.NAME,'thread/','')",                                    \
    "the name associated with the current thread instrumentation in the "      \
    "server")                                                                  \
  X("osid", "t.THREAD_OS_ID",                                                  \
    "the thread or task identifier as defined by the underlying operating "    \
    "system, if there is one")                                                 \
  X("protocol", "t.CONNECTION_TYPE",                                           \
    "the protocol used to establish the connection, or NULL for background "   \
    "threads")                                                                 \
  X("ptid", "t.PARENT_THREAD_ID",                                              \
    "If this thread is a subthread, this is the thread ID value of the "       \
    "spawning thread. ")                                                       \
  X("started",                                                                 \
    "cast(date_sub(now(), interval t.PROCESSLIST_TIME second) as char)",       \
    "time when thread started to be in its current state")                     \
  X("state", "t.PROCESSLIST_STATE",                                            \
    "an action, event, or state that indicates what the thread is doing")      \
  X("tidle",                                                                   \
    "if(esc.END_EVENT_ID,(SELECT concat(if(x-(esc.TIMER_END DIV "              \
    "1e+12)<36000,'0',''),(x-(esc.TIMER_END DIV 1e+12)) DIV "                  \
    "3600,':',lpad(((x-(esc.TIMER_END DIV 1e+12)) DIV "                        \
    "60)%60,2,'0'),':',lpad((x-(esc.TIMER_END DIV 1e+12))%60,2,'0')) FROM "    \
    "(SELECT gs.VARIABLE_VALUE AS x FROM performance_schema.global_status AS " \
    "gs WHERE gs.VARIABLE_NAME = 'Uptime') AS x),NULL)",                       \
    "the time the thread has been idle")                                       \
  X("time",                                                                    \
    "concat(if(t.PROCESSLIST_TIME<36000,'0',''),t.PROCESSLIST_TIME DIV "       \
    "3600,':',lpad((t.PROCESSLIST_TIME DIV "                                   \
    "60)%60,2,'0'),':',lpad(t.PROCESSLIST_TIME%60,2,'0'))",                    \
    "the time that the thread has been in its current state,")                 \
  X("type", "t.TYPE", "the thread type, either FOREGROUND or BACKGROUND")      \
  X("user", "t.PROCESSLIST_USER",                                              \
    "the user who issued the statement, or NULL for a background thread")      \
  S("Information about statements")                                            \
  X("digest", "if((esc.END_EVENT_ID is NULL),esc.DIGEST,NULL)",                \
    "the hash value of the statement digest the thread is executing")          \
  X("digesttxt", "if((esc.END_EVENT_ID is NULL),esc.DIGEST_TEXT,NULL)",        \
    "the normalized digest of the statement the thread is executing")          \
  X("fullscan", "p.full_scan",                                                 \
    "whether a full table scan was performed by the current statement")        \
  X("info", "sys.format_statement(replace(t.PROCESSLIST_INFO,'\n',' '))",      \
    "the statement the thread is executing, truncated to be shorter")          \
  X("infofull", "replace(t.PROCESSLIST_INFO,'\n',' ')",                        \
    "the statement the thread is executing, or NULL if it is not executing "   \
    "any statement")                                                           \
  X("latency", "p.statement_latency",                                          \
    "how long the statement has been executing")                               \
  X("llatency", "p.lock_latency",                                              \
    "the time spent waiting for locks by the current statement")               \
  X("nraffected", "p.rows_affected",                                           \
    "the number of rows affected by the current statement")                    \
  X("nrexamined", "p.rows_examined",                                           \
    "the number of rows read from storage engines by the current statement")   \
  X("nrsent", "p.rows_sent",                                                   \
    "the number of rows returned by the current statement")                    \
  X("ntmpdtbls", "p.tmp_disk_tables",                                          \
    "the number of internal on-disk temporary tables created by the current "  \
    "statement")                                                               \
  X("ntmptbls", "p.tmp_tables",                                                \
    "the number of internal in-memory temporary tables created by the "        \
    "current statement")                                                       \
  X("pdigest", "if((esc.END_EVENT_ID is NOT NULL),esc.DIGEST,NULL)",           \
    "the hash value of the last statement digest executed by the thread")      \
  X("pdigesttxt", "if((esc.END_EVENT_ID is NOT NULL),esc.DIGEST_TEXT,NULL)",   \
    "the normalized digest of the last statement executed by the thread")      \
  X("pinfo", "p.last_statement",                                               \
    "the last statement executed by the thread, if there is no currently "     \
    "executing statement or wait")                                             \
  X("platency", "p.last_statement_latency",                                    \
    "how long the last statement was executed")                                \
  X("progress", "p.progress",                                                  \
    "the percentage of work completed for stages that support progress "       \
    "reporting")                                                               \
  X("stmtid", "concat(esc.THREAD_ID,':',esc.EVENT_ID)",                        \
    "ID of the statement that is currently being executed by the thread")      \
  S("Counters")                                                                \
  X("nblocked",                                                                \
    "(SELECT count(*) FROM sys.innodb_lock_waits AS ilw WHERE "                \
    "t.PROCESSLIST_ID = ilw.blocking_pid) + (SELECT count(*) FROM "            \
    "sys.schema_table_lock_waits AS stlw WHERE t.THREAD_ID = "                 \
    "stlw.blocking_thread_id)",                                                \
    "the number of other threads blocked by the thread")                       \
  X("nblocking",                                                               \
    "(SELECT count(*) FROM sys.innodb_lock_waits AS ilw WHERE "                \
    "t.PROCESSLIST_ID = ilw.waiting_pid) + (SELECT count(*) FROM "             \
    "sys.schema_table_lock_waits AS stlw WHERE t.THREAD_ID = "                 \
    "stlw.waiting_thread_id)",                                                 \
    "the number of other threads blocking the thread")                         \
  X("npstmts",                                                                 \
    "(SELECT count(*) FROM performance_schema.prepared_statements_instances "  \
    "AS psi WHERE psi.OWNER_THREAD_ID = t.THREAD_ID)",                         \
    "the number of prepared statements allocated by the thread")               \
  X("nvars",                                                                   \
    "(SELECT count(*) FROM performance_schema.user_variables_by_thread AS "    \
    "uvbt WHERE t.THREAD_ID = uvbt.THREAD_ID)",                                \
    "the number of user variables defined for the thread")                     \
  S("Information about transactions")                                          \
  X("ntxrlckd", "trx.TRX_ROWS_LOCKED",                                         \
    "the approximate number or rows locked by the current InnoDB transaction") \
  X("ntxrmod", "trx.TRX_ROWS_MODIFIED",                                        \
    "the number of modified and inserted rows in the current InnoDB "          \
    "transaction")                                                             \
  X("ntxtlckd", "trx.TRX_TABLES_LOCKED",                                       \
    "the number of InnoDB tables that the current transaction has row locks "  \
    "on")                                                                      \
  X("txaccess",                                                                \
    "if(trx.TRX_IS_READ_ONLY IS NULL,NULL,if(trx.TRX_IS_READ_ONLY=1,'READ "    \
    "ONLY','READ WRITE'))",                                                    \
    "the access mode of the current InnoDB transaction")                       \
  X("txid", "trx.TRX_ID",                                                      \
    "ID of the InnoDB transaction that is currently being executed by the "    \
    "thread")                                                                  \
  X("txisolvl", "trx.TRX_ISOLATION_LEVEL",                                     \
    "the isolation level of the current InnoDB transaction")                   \
  X("txstart", "trx.TRX_STARTED",                                              \
    "time when thread started executing the current InnoDB transaction")       \
  X("txstate", "trx.TRX_STATE", "the current InnoDB transaction state")        \
  X("txtime", "cast(timediff(now(), trx.TRX_STARTED) as char)",                \
    "the time that the thread is executing the current InnoDB transaction")    \
  S("Information about I/O events")                                            \
  X("ioavgltncy", "io.avg_latency",                                            \
    "the average wait time per timed I/O event for the thread")                \
  X("ioltncy", "io.total_latency",                                             \
    "the total wait time of timed I/O events for the thread")                  \
  X("iomaxltncy", "io.max_latency",                                            \
    "the maximum single wait time of timed I/O events for the thread")         \
  X("iominltncy", "io.min_latency",                                            \
    "the minimum single wait time of timed I/O events for the thread")         \
  X("nio", "io.total", "the total number of I/O events for the thread")        \
  S("Information about client")                                                \
  X("pid", "p.pid", "the client process ID")                                   \
  X("progname", "p.program_name", "the client program name")                   \
  X("ssl",                                                                     \
    "CASE t.NAME WHEN 'thread/mysqlx/worker' THEN '?' WHEN "                   \
    "'thread/sql/one_connection' THEN (SELECT sbt.VARIABLE_VALUE FROM "        \
    "performance_schema.status_by_thread AS sbt WHERE sbt.THREAD_ID = "        \
    "t.THREAD_ID AND sbt.VARIABLE_NAME = 'Ssl_cipher') ELSE '' END",           \
    "SSL cipher in use by the client")                                         \
  S("Diagnostic information")                                                  \
  X("diagerrno", "esc.MYSQL_ERRNO", "the statement error number")              \
  X("diagerror", "if(ifnull(esc.ERRORS,0)=0,'NO','YES')",                      \
    "whether an error occurred for the statement")                             \
  X("diagstate", "esc.RETURNED_SQLSTATE", "the statement SQLSTATE value")      \
  X("diagtext", "esc.RETURNED_SQLSTATE", "the statement error message")        \
  X("diagwarn", "esc.WARNINGS", "the statement number of warnings")            \
  S("Statistics")                                                              \
  X("nseljoin", "esc.SELECT_FULL_JOIN",                                        \
    "the number of joins that perform table scans because they do not use "    \
    "indexes")                                                                 \
  X("nselrjoin", "esc.SELECT_FULL_RANGE_JOIN",                                 \
    "the number of joins that used a range search on a reference table")       \
  X("nselrange", "esc.SELECT_RANGE",                                           \
    "the number of joins that used ranges on the first table")                 \
  X("nselrangec", "esc.SELECT_RANGE_CHECK",                                    \
    "the number of joins without keys that check for key usage after each "    \
    "row")                                                                     \
  X("nselscan", "esc.SELECT_SCAN",                                             \
    "the number of joins that did a full scan of the first table")             \
  X("nsortmp", "esc.SORT_MERGE_PASSES",                                        \
    "the number of merge passes that the sort algorithm has had to do")        \
  X("nsortrange", "esc.SORT_RANGE",                                            \
    "the number of sorts that were done using ranges")                         \
  X("nsortrows", "esc.SORT_ROWS", "the number of sorted rows")                 \
  X("nsortscan", "esc.SORT_SCAN",                                              \
    "the number of sorts that were done by scanning the table")                \
  S("Special columns")                                                         \
  X("status",                                                                  \
    "(SELECT sbt.VARIABLE_VALUE FROM performance_schema.status_by_thread AS "  \
    "sbt WHERE sbt.THREAD_ID = t.THREAD_ID AND sbt.VARIABLE_NAME = ?)",        \
    "used as <b>status.NAME</b>, provides value of session status variable "   \
    "'NAME'")                                                                  \
  X("system",                                                                  \
    "(SELECT vbt.VARIABLE_VALUE FROM performance_schema.variables_by_thread "  \
    "AS vbt WHERE vbt.THREAD_ID = t.THREAD_ID AND vbt.VARIABLE_NAME = ?)",     \
    "used as <b>system.NAME</b>, provides value of session system variable "   \
    "'NAME'")

#define X(name, query, _) {name, {name, nullptr, query}},
#define S(section)

const std::map<std::string, Column_definition,
               shcore::Case_insensitive_comparator>
    allowed_columns{{COLUMN_DESCRIPTIONS}};

#undef S
#undef X

constexpr auto query = R"(AS th
FROM performance_schema.threads AS t
LEFT JOIN information_schema.innodb_trx AS trx ON t.PROCESSLIST_ID = trx.TRX_MYSQL_THREAD_ID
LEFT JOIN sys.processlist AS p ON t.THREAD_ID = p.thd_id
LEFT JOIN performance_schema.events_statements_current AS esc ON t.THREAD_ID = esc.THREAD_ID
LEFT JOIN sys.io_by_thread_by_latency AS io ON t.THREAD_ID = io.thread_id
)";

std::vector<std::string> get_details() {
  std::vector<std::string> details{
      "This report may contain the following columns:"};

#define X(name, _, description) \
  details.emplace_back(std::string{"@entry{"} + name + "} " + description);
#define S(section) details.emplace_back(section);

  COLUMN_DESCRIPTIONS

#undef S
#undef X

  return details;
}

#undef COLUMN_DESCRIPTIONS

#define OPTIONS                                                                \
  XX(foreground, Bool, "Causes the report to list all foreground threads.",    \
     "f")                                                                      \
  XX(background, Bool, "Causes the report to list all background threads.",    \
     "b")                                                                      \
  XX(all, Bool,                                                                \
     "Causes the report to list all threads, both foreground and background.", \
     "A")                                                                      \
  XX(format, String,                                                           \
     "Allows to specify order and number of columns returned by the report, "  \
     "optionally defining new column names.",                                  \
     "o",                                                                      \
     {"The <b>format</b> option expects a string in the following form: "      \
      "<b>column[=alias][,column[=alias]]*</b>. If column with the given "     \
      "name does not exist, all columns that match the given prefix are "      \
      "selected and alias is ignored."},                                       \
     false, false, nullable<std::string>)                                      \
  XX(where, String, "Allows to filter the result.", "",                        \
     std::vector<std::string>(                                                 \
         {"The <b>where</b> option expects a string in the following format: " \
          "<b>column OP value [LOGIC column OP value]*</b>, where:",           \
          "@li column - name of the column,", "@li value - an SQL value,",     \
          "@li OP - relational operator: =, !=, <>, >, >=, <, <=, LIKE,",      \
          "@li LOGIC - logical operator: AND, OR, NOT (case insensitive),",    \
          "@li the logical expressions can be grouped using parenthesis "      \
          "characters: ( )."}))                                                \
  XX(order_by, String,                                                         \
     "A comma separated list which specifies the columns to be used to sort "  \
     "the output.",                                                            \
     "", {"By default, output is ordered by: <b>" DEFAULT_SORT_ORDER "</b>."}) \
  XX(desc, Bool, "Causes the output to be sorted in descending order.", "",    \
     {"If the <b>desc</b> option is not provided, output is sorted in "        \
      "ascending order."})                                                     \
  XX(limit, Integer, "Limits the number of returned threads.", "l", {}, false, \
     false, nullable<uint64_t>)

class Threads_report : public Native_report {
 public:
  class Config {
   public:
    static const char *name() { return "threads"; }
    static Report::Type type() { return Report::Type::LIST; }
    static const char *brief() {
      return "Lists threads that belong to the user who owns the current "
             "session.";
    }

    static std::vector<std::string> details() { return get_details(); }
#define X X_GET_OPTIONS
    GET_OPTIONS
#undef X
    static Report::Argc argc() { return {0, 0}; }
    static Report::Examples examples() {
      return {{"Show foreground threads, display 'tid' and 'cid' columns and "
               "value of 'autocommit' system variable.",
               {{"o", "tid,cid,system.autocommit"}, {"foreground", "true"}},
               {}},
              {"Show background threads with 'tid' greater than 10 and name "
               "containing 'innodb' string.",
               {{"where", "tid > 10 AND name LIKE '%innodb%'"},
                {"background", "true"}},
               {}}};
    }
  };

 private:
  void parse(const shcore::Array_t & /* argv */,
             const shcore::Dictionary_t &options) override {
#define X X_OPTIONS_STRUCT
    OPTIONS_STRUCT
#undef X

    o.where = "TRUE";
    o.order_by = DEFAULT_SORT_ORDER;

#define X X_PARSE_OPTIONS
    PARSE_OPTIONS
#undef X

    // WHERE part of the query, based on 'all', 'foreground' and 'background'
    // options
    std::string where;

    if (o.all || (o.foreground && o.background)) {
      where = "TRUE";
    } else if (o.foreground) {
      where = "t.TYPE = 'FOREGROUND'";
    } else if (o.background) {
      where = "t.TYPE = 'BACKGROUND'";
    } else {
      const auto co = m_session->get_connection_options();
      where = shcore::sqlstring(
                  "t.PROCESSLIST_USER = ? AND t.PROCESSLIST_HOST = ?", 0)
              << co.get_user() << co.get_host();
    }

    // format
    if (!o.format) {
      if (o.background && !o.all && !o.foreground) {
        o.format = "tid,name,nio,ioltncy,iominltncy,ioavgltncy,iomaxltncy";
      } else {
        o.format =
            "tid,cid,user,host,db,command,time,state,txstate,info,nblocking";
      }
    }

    if (o.format->empty()) {
      throw shcore::Exception::argument_error(
          "The 'format' parameter cannot be empty.");
    }

    for (const auto &column : shcore::str_split(*o.format, ",")) {
      auto names = shcore::str_split(column, "=");
      auto name = std::move(names[0]);
      std::string alias;

      switch (names.size()) {
        case 1:
          alias = name;
          break;

        case 2:
          if (names[1].empty()) {
            throw shcore::Exception::argument_error(
                "Name of the alias in the 'format' parameter cannot be empty.");
          }

          alias = std::move(names[1]);
          break;

        default:
          throw shcore::Exception::argument_error(
              "Invalid format of the 'format' parameter.");
      }

      try {
        m_format_columns.emplace_back(add_column(name));
        m_format_names.emplace_back(std::move(alias));
      } catch (const std::exception &ex) {
        // no exact match found, try to find a partial match
        std::vector<std::string> matches;

        for (const auto &col : allowed_columns) {
          if (shcore::str_ibeginswith(col.first, name) &&
              !is_special_column(col.first)) {
            matches.emplace_back(col.first);
          }
        }

        if (matches.empty()) {
          throw shcore::Exception::argument_error(
              "Cannot use '" + name + "' in 'format' parameter: " + ex.what());
        } else {
          for (auto &m : matches) {
            m_format_columns.emplace_back(add_column(m));
            m_format_names.emplace_back(std::move(m));
          }
        }
      }
    }

    // HAVING part of the query, based on 'where' option
    if (o.where.empty()) {
      throw shcore::Exception::argument_error(
          "The 'where' parameter cannot be empty.");
    }

    std::string having;

    try {
      having = Sql_condition{[this](const std::string &ident) {
                 return "th->>'$.\"" + add_column(ident) + "\"'";
               }}.parse(o.where);
    } catch (const std::exception &e) {
      throw shcore::Exception::argument_error(
          "Failed to parse 'where' parameter: " + std::string(e.what()));
    }

    // ORDER BY part of the query
    std::string order_by;

    if (o.order_by.empty()) {
      throw shcore::Exception::argument_error(
          "The 'order-by' parameter cannot be empty.");
    }

    for (const auto &column : shcore::str_split(o.order_by, ",")) {
      try {
        order_by += "th->'$.\"" + add_column(column) + "\"',";
      } catch (const std::exception &ex) {
        throw shcore::Exception::argument_error(
            "Cannot use '" + column +
            "' as a column to order by: " + ex.what());
      }
    }

    // remove extra comma
    order_by.pop_back();

    m_conditions = shcore::str_format(
        "WHERE %s HAVING %s ORDER BY %s %s", where.c_str(), having.c_str(),
        order_by.c_str(), o.desc ? "DESC" : "ASC");

    if (o.limit) {
      m_conditions.append(shcore::sqlstring(" LIMIT ?", 0) << *o.limit);
    }
  }

  shcore::Array_t execute() const override {
    std::vector<Column_definition> columns;

    for (size_t idx = 0, size = m_format_columns.size(); idx < size; ++idx) {
      columns.emplace_back(m_used_columns.at(m_format_columns[idx]));
      columns.back().name = m_format_names[idx].c_str();
    }

    std::vector<Column_definition> used_columns;

    std::transform(std::begin(m_used_columns), std::end(m_used_columns),
                   std::back_inserter(used_columns),
                   [](const decltype(m_used_columns)::value_type &it) {
                     return it.second;
                   });

    return create_report_from_json_object(m_session, query + m_conditions,
                                          used_columns, columns);
  }

  std::string add_column(const std::string &name) {
    std::string new_name;
    Column_definition column;

    const auto pos = name.find(".");

    if (std::string::npos == pos) {
      const auto col = allowed_columns.find(name);

      if (allowed_columns.end() != col) {
        if (is_special_column(col->first)) {
          throw shcore::Exception::argument_error(
              "Column '" + name + "' requires a variable name, i.e.: " + name +
              ".NAME");
        }

        new_name = col->first;
        column = col->second;
      }
    } else {
      const auto col = allowed_columns.find(name.substr(0, pos));

      if (allowed_columns.end() != col && is_special_column(col->first)) {
        const std::string variable = name.substr(pos + 1);

        column.id = add_to_cache(col->first + "." + variable);
        column.query =
            add_to_cache(shcore::sqlstring(col->second.query, 0) << variable);
        new_name = column.id;
      }
    }

    if (new_name.empty()) {
      throw shcore::Exception::argument_error("Unknown column name: " + name);
    }

    m_used_columns.emplace(new_name, std::move(column));

    return new_name;
  }

  const char *add_to_cache(std::string &&v) {
    m_string_cache.emplace_back(std::make_unique<std::string>(move(v)));
    return m_string_cache.back()->c_str();
  }

  bool is_special_column(const std::string &c) const {
    static constexpr const char *special[] = {"status", "system"};
    return std::end(special) !=
           std::find(std::begin(special), std::end(special), c);
  }

  std::map<std::string, Column_definition> m_used_columns;
  std::vector<std::string> m_format_columns;
  std::vector<std::string> m_format_names;
  std::string m_conditions;
  std::vector<std::unique_ptr<std::string>> m_string_cache;
};

}  // namespace

void register_threads_report(Shell_reports *reports) {
  Native_report::register_report<Threads_report>(reports);
}

}  // namespace reports
}  // namespace mysqlsh
