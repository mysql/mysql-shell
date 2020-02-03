/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_DUMP_SCHEMA_DUMPER_H_
#define MODULES_UTIL_DUMP_SCHEMA_DUMPER_H_

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlsh {

using mysqlshdk::storage::IFile;

class Schema_dumper {
 public:
  struct Issue {
    Issue(const std::string &d, bool f) : description(d), fixed(f) {}
    std::string description;
    bool fixed;
  };

  explicit Schema_dumper(
      const std::shared_ptr<mysqlshdk::db::ISession> &mysql,
      const std::vector<std::string> &mysqlaas_supported_charsets = {"utf8mb4"},
      const std::set<std::string> &mysqlaas_restricted_priveleges = {
          "SUPER", "FILE", "RELOAD", "BINLOG_ADMIN", "SET_USER_ID"});

  static std::string normalize_user(const std::string &user,
                                    const std::string &host);

  void dump_all_tablespaces_ddl(IFile *file);
  void dump_tablespaces_ddl_for_dbs(IFile *file,
                                    const std::vector<std::string> &dbs);
  void dump_tablespaces_ddl_for_tables(IFile *file, const std::string &db,
                                       const std::vector<std::string> &tables);

  std::vector<Issue> dump_schema_ddl(IFile *file, const std::string &db);
  std::vector<Issue> dump_table_ddl(IFile *file, const std::string &db,
                                    const std::string &table);
  void dump_temporary_view_ddl(IFile *file, const std::string &db,
                               const std::string &view);
  std::vector<Issue> dump_view_ddl(IFile *file, const std::string &db,
                                   const std::string &view);
  int count_triggers_for_table(const std::string &db, const std::string &table);
  std::vector<Issue> dump_triggers_for_table_ddl(IFile *file,
                                                 const std::string &db,
                                                 const std::string &table);
  std::vector<std::string> get_triggers(const std::string &db,
                                        const std::string &table);

  std::vector<Issue> dump_events_ddl(IFile *file, const std::string &db);
  std::vector<std::string> get_events(const std::string &db);

  std::vector<Issue> dump_routines_ddl(IFile *file, const std::string &db);
  std::vector<std::string> get_routines(const std::string &db,
                                        const std::string &type);

  std::vector<Issue> dump_grants(IFile *file,
                                 const std::vector<std::string> &filter);
  std::vector<std::pair<std::string, std::string>> get_users(
      const std::vector<std::string> &filter);

  // Must be called for each file
  void write_header(IFile *sql_file, const std::string &db_name);
  void write_footer(IFile *sql_file);

 public:
  // Config options
  bool opt_force = false;
  bool opt_flush_privileges = false;
  bool opt_drop = true;
  bool opt_reexecutable = true;
  bool opt_create_options = true;
  bool opt_quoted = false;
  bool opt_databases = false;
  bool opt_alldbs = false;
  bool opt_create_db = false;
  bool opt_set_charset = true;
  bool opt_single_transaction = false;
  bool opt_comments = true;
  bool opt_compact = false;
  bool opt_drop_database = false;
  bool opt_tz_utc = true;
  bool opt_drop_trigger = true;
  bool opt_ansi_mode = false;  ///< Force the "ANSI" SQL_MODE.
  bool opt_column_statistics = false;
  bool opt_mysqlaas = false;
  bool opt_pk_mandatory_check = false;
  bool opt_force_innodb = false;
  bool opt_strip_directory = false;
  bool opt_strip_restricted_grants = false;
  bool opt_strip_tablespaces = false;
  bool opt_strip_definer = false;

  enum enum_set_gtid_purged_mode {
    SET_GTID_PURGED_OFF = 0,
    SET_GTID_PURGED_AUTO = 1,
    SET_GTID_PURGED_ON = 2,
    SET_GTID_PURGED_COMMENTED = 3
  } opt_set_gtid_purged_mode = SET_GTID_PURGED_AUTO;

 private:
  std::shared_ptr<mysqlshdk::db::ISession> m_mysql;
  const std::vector<std::string> m_mysqlaas_supported_charsets;
  const std::set<std::string> m_mysqlaas_restricted_priveleges;

  bool stats_tables_included = false;

  bool lock_tables = true;

  /**
   Use double quotes ("") like in the standard  to quote identifiers if true,
    otherwise backticks (``, non-standard MySQL feature).
  */
  bool ansi_quotes_mode = false;

  /*
    Constant for detection of default value of default_charset.
    If default_charset is equal to mysql_universal_client_charset, then
    it is the default value which assigned at the very beginning of main().
  */
  const char *default_charset;

  /* have we seen any VIEWs during table scanning? */
  bool seen_views = false;

  std::string m_dump_info;

 private:
  int execute_no_throw(const std::string &s,
                       mysqlshdk::db::Error *out_error = nullptr);

  int execute_maybe_throw(const std::string &s,
                          mysqlshdk::db::Error *out_error = nullptr);

  int query_no_throw(const std::string &s,
                     std::shared_ptr<mysqlshdk::db::IResult> *out_result,
                     mysqlshdk::db::Error *out_error = nullptr);

  std::shared_ptr<mysqlshdk::db::IResult> query_log_and_throw(
      const std::string &s);

  int query_with_binary_charset(
      const std::string &s, std::shared_ptr<mysqlshdk::db::IResult> *out_result,
      mysqlshdk::db::Error *out_error = nullptr);

  void fetch_db_collation(const std::string &db_name,
                          std::string *out_db_cl_name);

  void switch_character_set_results(const char *cs_name);

  void unescape(IFile *file, const char *pos, size_t length);

  void unescape(IFile *file, const std::string &s);

  std::string quote_for_like(const std::string &name_);

  /* A common printing function for xml and non-xml modes. */

  void print_comment(IFile *sql_file, bool is_error, const char *format, ...);

  char *create_delimiter(const std::string &query, char *delimiter_buff,
                         int delimiter_max_size);

  std::vector<Issue> dump_events_for_db(IFile *sql_file, const std::string &db);

  std::vector<Issue> dump_routines_for_db(IFile *sql_file,
                                          const std::string &db);

  bool is_charset_supported(const std::string &cs);

  std::vector<Issue> check_ct_for_mysqlaas(const std::string &db,
                                           const std::string &table,
                                           std::string *create_table);

  void check_object_for_definer(const std::string &db,
                                const std::string &object,
                                const std::string &name, std::string *ddl,
                                std::vector<Issue> *issues);
  void check_objects_charsets(const std::string &db, const std::string &object,
                              const std::string &name,
                              const std::string &client_charset,
                              const std::string &conn_collation,
                              std::vector<Issue> *issues,
                              const std::string &ddl = "");

  std::vector<Issue> get_table_structure(
      IFile *sql_file, const std::string &table, const std::string &db,
      std::string *out_table_type, char *ignore_flag, bool real_columns[]);

  std::vector<Issue> dump_trigger(
      IFile *sql_file,
      const std::shared_ptr<mysqlshdk::db::IResult> &show_create_trigger_rs,
      const std::string &db_name, const std::string &db_cl_name,
      const std::string &trigger_name);

  std::vector<Issue> dump_triggers_for_table(IFile *sql_file,
                                             const std::string &table_name,
                                             const std::string &db_name);
  void dump_column_statistics_for_table(IFile *sql_file,
                                        const std::string &table_name,
                                        const std::string &db_name);

  std::vector<Issue> dump_table(IFile *sql_file, const std::string &table,
                                const std::string &db);

  int dump_all_tablespaces(IFile *file);
  int dump_tablespaces_for_databases(IFile *sql_file,
                                     const std::vector<std::string> &databases);
  int dump_tablespaces_for_tables(IFile *file, const std::string &db,
                                  const std::vector<std::string> &table_names);

  int dump_tablespaces(IFile *sql_file, const std::string &ts_where);
  int checked_ndbinfo = 0;
  int have_ndbinfo = 0;

  int is_ndbinfo(const std::string &dbname);

  int init_dumping_views(IFile *sql_file, const char * /*qdatabase*/);

  int init_dumping(IFile *sql_file, const std::string &database,
                   const std::function<int(IFile *, const char *)> &init_func);

  std::vector<std::string> get_table_names();

  std::string get_actual_table_name(const std::string &old_table_name);

  int do_flush_tables_read_lock();

  int do_unlock_tables();

  int start_transaction();

  char check_if_ignore_table(const std::string &table_name,
                             std::string *out_table_type);

  bool is_binlog_disabled = false;

  void set_session_binlog(IFile *sql_file, bool flag);

  bool add_set_gtid_purged(IFile *sql_file);

  bool process_set_gtid_purged(IFile *sql_file);
  std::vector<Schema_dumper::Issue> get_view_structure(IFile *sql_file,
                                                       const std::string &table,
                                                       const std::string &db);
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_SCHEMA_DUMPER_H_
