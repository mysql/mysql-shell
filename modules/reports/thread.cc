/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#include "modules/reports/thread.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "modules/reports/native_report.h"
#include "modules/reports/utils.h"
#include "modules/reports/utils_options.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysqlshdk::utils::nullable;

namespace mysqlsh {
namespace reports {

namespace {

#define OPTIONS                                                                \
  XX(tid, Integer, "Lists information for the specified thread ID.", "t", {},  \
     false, false, nullable<uint64_t>)                                         \
  XX(cid, Integer, "Lists information for the specified connection ID.", "c",  \
     {}, false, false, nullable<uint64_t>)                                     \
  XX(brief, Bool,                                                              \
     "Causes the report to list just a brief information on the given "        \
     "thread.",                                                                \
     "B")                                                                      \
  XX(client, Bool,                                                             \
     "Shows additional information about the client connection and the "       \
     "client session.",                                                        \
     "C")                                                                      \
  XX(general, Bool, "Shows the basic information about the specified thread.", \
     "G",                                                                      \
     {"If no other option is specified, report is executed as if the "         \
      "<b>general</b> option was given by the user."})                         \
  XX(innodb, Bool,                                                             \
     "Shows additional information about the current InnoDB transaction.",     \
     "I")                                                                      \
  XX(locks, Bool,                                                              \
     "Shows additional information about locks blocking the thread and locks " \
     "blocked by the thread.",                                                 \
     "L")                                                                      \
  XX(prep_stmts, Bool,                                                         \
     "Lists the prepared statements allocated for the thread.", "P")           \
  XX(status, String, "Lists the session status variables for the thread.",     \
     "S", {}, false, true, nullable<std::string>)                              \
  XX(vars, String, "Lists the session system variables for the thread.", "V",  \
     {}, false, true, nullable<std::string>)                                   \
  XX(user_vars, String, "Lists the user-defined variables for the thread.",    \
     "U", {}, false, true, nullable<std::string>)                              \
  XX(all, Bool, "Provides all available information for the thread.", "A")

class Thread_report : public Native_report {
 public:
  class Config {
   public:
    static const char *name() { return "thread"; }
    static Report::Type type() { return Report::Type::CUSTOM; }
    static const char *brief() {
      return "Provides various information regarding the specified thread.";
    }

    static std::vector<std::string> details() {
      return {
          "By default lists information on the thread used by the current "
          "connection.",
          "The options which list variables may receive an argument in the "
          "following form: <b>prefix[,prefix]*</b>. If it is provided, "
          "they will list variables which match the given prefixes. "
          "Otherwise, they will list all variables."};
    }
#define X X_GET_OPTIONS
    GET_OPTIONS
#undef X
    static Report::Argc argc() { return {0, 0}; }
    static Report::Formatter formatter() { return thread_formatter; }
    static Report::Examples examples() {
      return {{"Show general, locks, InnoDB and client information for the "
               "thread used by the current connection.",
               {{"G", "true"}, {"L", "true"}, {"I", "true"}, {"C", "true"}},
               {}},
              {"Show general information for the thread with thread ID 55.",
               {{"tid", "55"}},
               {}},
              {"Show status variables which begin with 'Bytes' and 'Ssl' for "
               "the thread with connection ID 72.",
               {{"cid", "72"}, {"status", "Bytes,Ssl"}},
               {}}};
    }
  };

 private:
  struct Section {
   public:
#define SECTION_TYPE \
  X(BRIEF, "brief")  \
  X(LIST, "list")    \
  X(TABLE, "table")  \
  X(GROUP, "group")

    enum class Type {
#define X(value, name) value,
      SECTION_TYPE
#undef X
    };

    static std::string format(const shcore::Array_t &sections,
                              uint32_t level = 0) {
      constexpr auto k_section_not_available = "N/A\n";

      std::string result;

      for (const auto &row : *sections) {
        const auto section = row.as_map();
        const auto &name = section->at(k_section_title).get_string();

        if (!name.empty()) {
          result.append(header(name, level)).append("\n");
        }

        const auto type = from_string(section->at(k_section_type).get_string());
        const auto data = section->at(k_section_data).as_array();
        std::string formatted;

        switch (type) {
          case Type::BRIEF:
            formatted = brief_formatter(data);
            break;

          case Type::LIST:
            formatted = status_formatter(data);
            break;

          case Type::TABLE:
            formatted = table_formatter(data);
            break;

          case Type::GROUP:
            formatted = format(data, level + 1);

            if (!formatted.empty()) {
              // remove extra new line
              formatted.pop_back();
            }
            break;
        }

        if (formatted.empty()) {
          result.append(k_section_not_available);
        } else {
          result.append(formatted);
        }

        result.append("\n");
      }

      if (!result.empty()) {
        result.pop_back();
      }

      return result;
    }

    shcore::Dictionary_t to_json() && {
      auto section = shcore::make_dict();

      section->emplace(k_section_type, to_string(type));
      section->emplace(k_section_title, title);
      section->emplace(k_section_data, std::move(data));

      return section;
    }

    Type type;
    const char *title;
    shcore::Array_t data;

   private:
    static std::string to_string(Type t) {
#define X(value, name) \
  case Type::value:    \
    return name;

      switch (t) { SECTION_TYPE }

#undef X

      throw std::logic_error("Shouldn't happen, but compiler complains");
    }

    static Type from_string(const std::string &t) {
#define X(value, name) \
  if (t == name) return Type::value;

      SECTION_TYPE

#undef X

      throw std::invalid_argument("Unknown section type: " + t);
    }

#undef SECTION_TYPE

    static std::string header(const std::string &h, uint32_t level) {
      using mysqlshdk::textui::bold;
      using mysqlshdk::textui::cyan;

      switch (level) {
        case 0:
          return bold(cyan(shcore::str_upper(h)));

        default:
          return bold(h);
      }
    }

    static constexpr auto k_section_type = "type";
    static constexpr auto k_section_title = "title";
    static constexpr auto k_section_data = "data";
  };

  static std::string thread_formatter(const shcore::Array_t &data,
                                      const shcore::Dictionary_t &) {
    return Section::format(data);
  }

  void parse(const shcore::Array_t & /* argv */,
             const shcore::Dictionary_t &options) override {
#define X X_OPTIONS_STRUCT
    OPTIONS_STRUCT
#undef X

#define X X_PARSE_OPTIONS
    PARSE_OPTIONS
#undef X

    set_id(o.cid, o.tid);

    // if 'all' was set, turn on all sections of the report except for 'brief'
    if (o.all) {
      o.client = o.innodb = o.locks = o.prep_stmts = o.general = true;
      o.status = o.vars = o.user_vars = "";
    }

    m_skip_empty = o.all;

    // if 'general' was not set explicitly, turn it on implicitly if none of the
    // remaining sections was requested
    if (!o.general) {
      o.general = !(o.brief | o.client | o.innodb | o.locks | o.prep_stmts |
                    static_cast<bool>(o.status) | static_cast<bool>(o.vars) |
                    static_cast<bool>(o.user_vars));
    }

    if (o.brief) {
      m_sections.emplace_back(&Thread_report::brief_section);
    }

    if (o.general) {
      m_sections.emplace_back(&Thread_report::thread_section);
    }

    if (o.client) {
      m_sections.emplace_back(&Thread_report::client_section);
    }

    if (o.innodb) {
      m_sections.emplace_back(&Thread_report::innodb_section);
    }

    if (o.locks) {
      m_sections.emplace_back(&Thread_report::locks_section);
    }

    if (o.prep_stmts) {
      m_sections.emplace_back(&Thread_report::prep_stmts_section);
    }

    if (o.status) {
      m_sections.emplace_back(&Thread_report::status_section);
      m_status = get_vars_condition(*o.status);
    }

    if (o.vars) {
      m_sections.emplace_back(&Thread_report::vars_section);
      m_vars = get_vars_condition(*o.vars);
    }

    if (o.user_vars) {
      m_sections.emplace_back(&Thread_report::user_vars_section);
      m_user_vars = get_vars_condition(*o.user_vars);
    }
  }

  shcore::Array_t execute() const override {
    const auto report = shcore::make_array();

    for (const auto &section : m_sections) {
      add_section(report, (this->*section)());
    }

    return report;
  }

  void set_id(const nullable<uint64_t> &cid, const nullable<uint64_t> &tid) {
    if (cid && tid) {
      throw shcore::Exception::argument_error(
          "Both 'cid' and 'tid' cannot be used at the same time.");
    }

    if (!cid && !tid) {
      m_where = "t.PROCESSLIST_ID = CONNECTION_ID()";
    } else {
      auto query = shcore::sqlstring("t.! = ?", 0)
                   << (tid ? "THREAD_ID" : "PROCESSLIST_ID")
                   << (tid ? *tid : *cid);

      m_where = query.str();
    }

    m_where = " WHERE " + m_where;

    validate_id();
  }

  void validate_id() const {
    const auto result = m_session->query(
        "SELECT t.THREAD_ID FROM performance_schema.threads AS t" + m_where);

    if (nullptr == result->fetch_one()) {
      throw shcore::Exception::argument_error(
          "The specified thread does not exist.");
    }
  }

  Section brief_section() const {
    static constexpr auto query = R"(AS brief
FROM performance_schema.threads AS t)";

    return section(Section::Type::BRIEF, "", query,
                   {{"tid", "Thread ID", "t.THREAD_ID"},
                    {"id", "Connection ID", "t.PROCESSLIST_ID"},
                    {"user", "User",
                     "concat(ifnull(t.PROCESSLIST_USER, ''), "
                     "ifnull(concat('@', t.PROCESSLIST_HOST), ''))"}});
  }

  Section client_section() const {
    static constexpr auto query = R"(AS client
FROM performance_schema.threads AS t
LEFT JOIN performance_schema.socket_instances AS si ON si.THREAD_ID = t.THREAD_ID
LEFT JOIN performance_schema.setup_instruments AS setup ON si.EVENT_NAME = setup.NAME
LEFT JOIN performance_schema.socket_summary_by_instance AS ssbi ON si.OBJECT_INSTANCE_BEGIN = ssbi.OBJECT_INSTANCE_BEGIN)";

    return section(
        Section::Type::LIST, "Client", query,
        {
            {"connect_attrs", "Connection attributes",
             "(SELECT json_objectagg(sca.ATTR_NAME, sca.ATTR_VALUE) FROM "
             "performance_schema.session_connect_attrs AS sca WHERE "
             "t.PROCESSLIST_ID = sca.PROCESSLIST_ID)"},
            {"protocol", "Protocol",
             "CASE t.NAME WHEN 'thread/mysqlx/worker' THEN 'X' WHEN "
             "'thread/sql/one_connection' THEN 'classic' ELSE t.NAME END"},
            {"socket_ip", "Socket IP", "si.ip"},
            {"socket_port", "Socket port", "si.port"},
            {"socket_state", "Socket state", "si.state"},
            {"socket_stats",
             "Socket stats",
             R"(
IF(setup.enabled = 'YES',
  json_object(
    'count_read', ssbi.COUNT_READ,
    'total_read_bytes', sys.format_bytes(ssbi.SUM_NUMBER_OF_BYTES_READ),
    'total_read_wait', sys.format_time(ssbi.SUM_TIMER_READ),
    'min_read_wait', sys.format_time(ssbi.MIN_TIMER_READ),
    'avg_read_wait', sys.format_time(ssbi.AVG_TIMER_READ),
    'max_read_wait', sys.format_time(ssbi.MAX_TIMER_READ),
    'count_write', ssbi.COUNT_WRITE,
    'total_write_bytes', sys.format_bytes(ssbi.SUM_NUMBER_OF_BYTES_WRITE),
    'total_write_wait', sys.format_time(ssbi.SUM_TIMER_WRITE),
    'min_write_wait', sys.format_time(ssbi.MIN_TIMER_WRITE),
    'avg_write_wait', sys.format_time(ssbi.AVG_TIMER_WRITE),
    'max_write_wait', sys.format_time(ssbi.MAX_TIMER_WRITE)),
  NULL))",
             {
                 {"count_read", "Socket reads"},
                 {"total_read_bytes", "Read bytes"},
                 {"total_read_wait", "Total read wait"},
                 {"min_read_wait", "Min. read wait"},
                 {"avg_read_wait", "Avg. read wait"},
                 {"max_read_wait", "Max. read wait"},
                 {"count_write", "Socket writes"},
                 {"total_write_bytes", "Wrote bytes"},
                 {"total_write_wait", "Total write wait"},
                 {"min_write_wait", "Min. write wait"},
                 {"avg_write_wait", "Avg. write wait"},
                 {"max_write_wait", "Max. write wait"},
             }},
            {"status",
             "Encryption status",
             // status variables for X are not properly reported (BUG28920582)
             "(SELECT json_objectagg(lower(sbt.VARIABLE_NAME), CASE t.NAME "
             "WHEN 'thread/mysqlx/worker' THEN '?' ELSE sbt.VARIABLE_VALUE "
             "END) FROM performance_schema.status_by_thread AS sbt WHERE "
             "t.THREAD_ID = sbt.THREAD_ID AND sbt.VARIABLE_NAME in "
             "('Ssl_cipher', 'Ssl_version', 'Compression'))",
             {{"compression", "Compression"},
              {"ssl_cipher", "SSL cipher"},
              {"ssl_version", "SSL version"}}},
        },
        "t.TYPE = 'FOREGROUND'");
  }

  Section innodb_section() const {
    static constexpr auto query = R"(AS innodb
FROM performance_schema.threads AS t
INNER JOIN information_schema.innodb_trx AS trx ON t.PROCESSLIST_ID = trx.TRX_MYSQL_THREAD_ID)";

    return section(
        Section::Type::LIST, "InnoDB status", query,
        {
            {"txstate", "State", "trx.TRX_STATE"},
            {"txid", "ID", "trx.TRX_ID"},
            {"txtime", "Elapsed",
             "cast(timediff(now(), trx.TRX_STARTED) as char)"},
            {"txstarted", "Started", "trx.TRX_STARTED"},
            {"txisolation", "Isolation level", "trx.TRX_ISOLATION_LEVEL"},
            {"txaccess", "Access",
             "if(trx.TRX_IS_READ_ONLY IS "
             "NULL,NULL,if(trx.TRX_IS_READ_ONLY=1,'READ ONLY','READ WRITE'))"},
            {"txtableslocked", "Locked tables", "trx.TRX_TABLES_LOCKED"},
            {"txrowslocked", "Locked rows", "trx.TRX_ROWS_LOCKED"},
            {"txrowsmodified", "Modified rows", "trx.TRX_ROWS_MODIFIED"},
        });
  }

  Section locks_section() const {
    auto group = shcore::make_array();

    {
      static constexpr auto query = R"(AS innodb_blocked_by
FROM performance_schema.threads AS t
JOIN sys.innodb_lock_waits AS ilw ON ilw.waiting_pid = t.PROCESSLIST_ID)";

      add_section(
          group,
          section(Section::Type::TABLE, "Waiting for InnoDB locks", query,
                  {
                      {"wait_started", "Wait started",
                       "cast(ilw.wait_started as char)"},
                      {"wait_elapsed", "Elapsed", "cast(ilw.wait_age as char)"},
                      {"object", "Locked table", "ilw.locked_table"},
                      {"waiting_lock_type", "Type", "ilw.locked_type"},
                      {"blocking_pid", "CID", "ilw.blocking_pid"},
                      {"blocking_query", "Query", "ilw.blocking_query"},
                      {"blocking_account", "Account",
                       "concat(t.processlist_user, '@', t.processlist_host)"},
                      {"blocking_trx_started", "Transaction started",
                       "cast(ilw.blocking_trx_started as char)"},
                      {"blocking_trx_elapsed", "Elapsed",
                       "cast(ilw.blocking_trx_age as char)"},
                  }));
    }

    {
      static constexpr auto query = R"(AS metadata_blocked_by
FROM performance_schema.threads AS t
JOIN sys.schema_table_lock_waits AS stlw ON stlw.waiting_thread_id = t.THREAD_ID)";

      add_section(
          group,
          section(Section::Type::TABLE, "Waiting for metadata locks", query,
                  {
                      {"wait_started", "Wait started",
                       "cast(date_sub(now(), interval stlw.waiting_query_secs "
                       "second) as char)"},
                      {"wait_elapsed", "Elapsed",
                       "cast(sec_to_time(stlw.waiting_query_secs) as char)"},
                      {"object", "Locked table",
                       "concat(sys.quote_identifier(stlw.object_schema), '.', "
                       "sys.quote_identifier(stlw.object_name))"},
                      {"waiting_lock_type", "Type",
                       "concat(stlw.waiting_lock_type, ' (', "
                       "stlw.waiting_lock_duration, ')')"},
                      {"blocking_pid", "CID", "stlw.blocking_pid"},
                      {"blocking_query", "Query", "''"},
                      {"blocking_account", "Account", "stlw.blocking_account"},
                      {"blocking_trx_started", "Transaction started", "''"},
                      {"blocking_trx_elapsed", "Elapsed", "''"},
                  }));
    }

    {
      static constexpr auto query = R"(AS innodb_blocking
FROM performance_schema.threads AS t
JOIN sys.innodb_lock_waits AS ilw ON ilw.blocking_pid = t.PROCESSLIST_ID)";

      add_section(
          group,
          section(Section::Type::TABLE, "Blocking InnoDB locks", query,
                  {
                      {"wait_started", "Wait started",
                       "cast(ilw.wait_started as char)"},
                      {"wait_elapsed", "Elapsed", "cast(ilw.wait_age as char)"},
                      {"object", "Locked table", "ilw.locked_table"},
                      {"waiting_lock_type", "Type", "ilw.locked_type"},
                      {"waiting_pid", "CID", "ilw.waiting_pid"},
                      {"waiting_query", "Query", "ilw.waiting_query"},
                  }));
    }

    {
      static constexpr auto query = R"(AS metadata_blocking
FROM performance_schema.threads AS t
JOIN sys.schema_table_lock_waits AS stlw ON stlw.blocking_thread_id = t.THREAD_ID)";

      add_section(
          group,
          section(Section::Type::TABLE, "Blocking metadata locks", query,
                  {
                      {"wait_started", "Wait started",
                       "cast(date_sub(now(), interval "
                       "stlw.waiting_query_secs second) as char)"},
                      {"wait_elapsed", "Elapsed",
                       "cast(sec_to_time(stlw.waiting_query_secs) as char)"},
                      {"object", "Locked table",
                       "concat(sys.quote_identifier(stlw.object_schema),"
                       " '.', sys.quote_identifier(stlw.object_name))"},
                      {"waiting_lock_type", "Type",
                       "concat(stlw.waiting_lock_type, ' (', "
                       "stlw.waiting_lock_duration, ')')"},
                      {"waiting_pid", "CID", "stlw.waiting_pid"},
                      {"waiting_query", "Query", "stlw.waiting_query"},
                  }));
    }

    return section(Section::Type::GROUP, "Locks", std::move(group));
  }

  Section prep_stmts_section() const {
    static constexpr auto query = R"(AS prep_stmts
FROM performance_schema.threads AS t
INNER JOIN performance_schema.prepared_statements_instances AS psi ON psi.OWNER_THREAD_ID = t.THREAD_ID)";

    return section(
        Section::Type::TABLE, "Prepared statements", query,
        {
            {"id", "ID", "psi.STATEMENT_ID"},
            {"event_id", "Event ID",
             "concat(psi.OWNER_THREAD_ID, ':', psi.OWNER_EVENT_ID)"},
            {"name", "Name", "psi.STATEMENT_NAME"},
            {"owner", "Owner",
             "concat(ifnull(psi.OWNER_OBJECT_SCHEMA, ''), ifnull(concat('.', "
             "psi.OWNER_OBJECT_NAME), ''), ifnull(concat(' (', "
             "psi.OWNER_OBJECT_TYPE, ')'), ''))"},
            {"prepare_time", "Prepared in",
             "sys.format_time(psi.TIMER_PREPARE)"},
            {"executed", "Executed", "psi.COUNT_EXECUTE"},
            {"sum_execute_time", "Total time",
             "sys.format_time(psi.SUM_TIMER_EXECUTE)"},
            {"min_execute_time", "Min. time",
             "sys.format_time(psi.MIN_TIMER_EXECUTE)"},
            {"avg_execute_time", "Avg. time",
             "sys.format_time(psi.AVG_TIMER_EXECUTE)"},
            {"max_execute_time", "Max. time",
             "sys.format_time(psi.MAX_TIMER_EXECUTE)"},
        });
  }

  Section status_section() const {
    static constexpr auto query = R"(
SELECT json_objectagg(sbt.VARIABLE_NAME, sbt.VARIABLE_VALUE) AS status
FROM performance_schema.threads AS t
JOIN performance_schema.status_by_thread AS sbt ON t.THREAD_ID = sbt.THREAD_ID)";

    return section(Section::Type::LIST, "Status variables", query, {},
                   m_status);
  }

  Section thread_section() const {
    static constexpr auto query = R"(AS general
FROM performance_schema.threads AS t
LEFT JOIN information_schema.innodb_trx AS trx ON t.PROCESSLIST_ID = trx.TRX_MYSQL_THREAD_ID
LEFT JOIN sys.processlist AS p ON t.THREAD_ID = p.thd_id)";

    return section(
        Section::Type::LIST, "General", query,
        {{"tid", "Thread ID", "t.THREAD_ID"},
         {"id", "Connection ID", "t.PROCESSLIST_ID"},
         {"type", "Thread type", "t.TYPE"},
         {"program_name", "Program name", "p.program_name"},
         {"user", "User", "t.PROCESSLIST_USER"},
         {"host", "Host", "t.PROCESSLIST_HOST"},
         {"db", "Database", "t.PROCESSLIST_DB"},
         {"command", "Command", "t.PROCESSLIST_COMMAND"},
         {"time", "Time",
          "concat(if(t.PROCESSLIST_TIME<36000,'0',''),t.PROCESSLIST_TIME DIV "
          "3600,':',lpad((t.PROCESSLIST_TIME DIV "
          "60)%60,2,'0'),':',lpad(t.PROCESSLIST_TIME%60,2,'0'))"},
         {"state", "State", "t.PROCESSLIST_STATE"},
         {"txstate", "Transaction state", "trx.TRX_STATE"},
         {"num_prepared_statements", "Prepared statements",
          "(SELECT count(*) FROM "
          "performance_schema.prepared_statements_instances AS psi WHERE "
          "psi.owner_thread_id = t.thread_id)"},
         {"socket",
          "Socket",
          // status variables for X are not properly reported (BUG28920582)
          "(SELECT json_objectagg(lower(sbt.VARIABLE_NAME), CASE t.NAME WHEN "
          "'thread/mysqlx/worker' THEN '?' ELSE sbt.VARIABLE_VALUE END) FROM "
          "performance_schema.status_by_thread AS sbt WHERE t.THREAD_ID = "
          "sbt.THREAD_ID AND sbt.VARIABLE_NAME IN ('Bytes_received', "
          "'Bytes_sent'))",
          {{"bytes_received", "Bytes received"}, {"bytes_sent", "Bytes sent"}}},
         {
             "info",
             "Info",
             "sys.format_statement(replace(t.PROCESSLIST_INFO,'\n',' '))",
         },
         {"prevstmt", "Previous statement", "p.last_statement"}});
  }

  Section vars_section() const {
    static constexpr auto query = R"(
SELECT json_objectagg(vbt.VARIABLE_NAME, vbt.VARIABLE_VALUE) AS vars
FROM performance_schema.threads AS t
JOIN performance_schema.variables_by_thread AS vbt ON t.THREAD_ID = vbt.THREAD_ID)";

    return section(Section::Type::LIST, "System variables", query, {}, m_vars);
  }

  Section user_vars_section() const {
    static constexpr auto query = R"(
SELECT json_objectagg(uvbt.VARIABLE_NAME, CAST(uvbt.VARIABLE_VALUE AS CHAR)) AS user_vars
FROM performance_schema.threads AS t
JOIN performance_schema.user_variables_by_thread AS uvbt ON t.THREAD_ID = uvbt.THREAD_ID)";

    return section(Section::Type::LIST, "User variables", query, {},
                   m_user_vars);
  }

  Section section(Section::Type type, const char *title,
                  const std::string &query,
                  const std::vector<Column_definition> &columns,
                  const std::string &where = "") const {
    const auto full =
        query + m_where + (where.empty() ? "" : " AND (" + where + ")");
    return section(type, title,
                   create_report_from_json_object(m_session, full, columns));
  }

  Section section(Section::Type type, const char *title,
                  shcore::Array_t &&report) const {
    return {type, title, std::move(report)};
  }

  void add_section(const shcore::Array_t &report, Section &&section) const {
    const size_t expected_size = (Section::Type::GROUP == section.type) ? 0 : 1;

    if (!m_skip_empty || section.data->size() > expected_size) {
      report->emplace_back(std::move(section).to_json());
    }
  }

  static std::string get_vars_condition(const std::string &s) {
    std::string cond;

    if (!s.empty()) {
      for (const auto &var : shcore::str_split(s, ",")) {
        if (var.empty()) {
          throw shcore::Exception::argument_error(
              "Variable name to be used as a filter cannot be empty.");
        }

        if (!cond.empty()) {
          cond.append(" OR ");
        }

        cond.append(shcore::sqlstring("VARIABLE_NAME LIKE ?", 0) << var + "%");
      }
    }

    return cond;
  }

  using Create_section = Section (Thread_report::*)() const;

  std::string m_where;
  std::vector<Create_section> m_sections;
  bool m_skip_empty = false;
  std::string m_status;
  std::string m_vars;
  std::string m_user_vars;
};

}  // namespace

void register_thread_report(Shell_reports *reports) {
  Native_report::register_report<Thread_report>(reports);
}

}  // namespace reports
}  // namespace mysqlsh
