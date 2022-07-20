/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/parser/base/symbol-info.h"
#include "mysqlshdk/libs/parser/server/sql_modes.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

namespace shcore {
namespace completer {

using mysqlshdk::db::Error;
using mysqlshdk::db::IRow;
using mysqlshdk::utils::Version;

using Columns = mysqlshdk::Sql_completion_result::Columns;
using Names = mysqlshdk::Sql_completion_result::Names;

namespace {

const Version k_current_version{MYSH_VERSION};

struct Instance {
  class Object {
   public:
    Object() = default;

    explicit Object(std::wstring n) : m_wide_name(std::move(n)) {}

    explicit Object(std::string n) : m_utf8_name(std::move(n)) {
      m_wide_name = shcore::utf8_to_wide(m_utf8_name.value());
    }

    const std::wstring &wide_name() const { return m_wide_name; }

    const std::string &name() const {
      if (!m_utf8_name) {
        m_utf8_name = shcore::wide_to_utf8(m_wide_name);
      }

      return m_utf8_name.value();
    }

   private:
    std::wstring m_wide_name;
    mutable std::optional<std::string> m_utf8_name;
  };
  using Objects = std::vector<Object>;

  struct Table : Object {
    using Object::Object;
    Objects columns;
  };
  using Tables = std::vector<Table>;

  struct Schema : Object {
    using Object::Object;
    Objects events;
    Objects functions;
    Objects procedures;
    Tables tables;
    Objects triggers;
    Tables views;
  };
  using Schemas = std::vector<Schema>;

  Objects charsets;
  Objects collations;
  Objects engines;
  Objects logfile_groups;
  Objects plugins;
  Schemas schemas;
  const Objects *system_functions = nullptr;
  Objects system_variables;
  Objects tablespaces;
  Objects udfs;
  Objects users;
  Objects user_variables;

  void clear() {
    charsets.clear();
    collations.clear();
    engines.clear();
    logfile_groups.clear();
    plugins.clear();
    schemas.clear();
    system_functions = nullptr;
    system_variables.clear();
    tablespaces.clear();
    udfs.clear();
    users.clear();
    user_variables.clear();
  }
};

}  // namespace

class Provider_sql::Cache final {
 public:
  Cache() { set_system_functions(k_current_version); }

  Cache(const Cache &) = default;
  Cache(Cache &&) = default;

  Cache &operator=(const Cache &) = default;
  Cache &operator=(Cache &&) = default;

  ~Cache() = default;

  void cancel() { m_cancelled = true; }

  void refresh_schemas(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    m_cancelled = false;

    fetch_schemas(session);
  }

  void refresh_schema(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                      const std::string &schema, bool force) {
    m_cancelled = false;

    // cache schema names if not done yet
    if (m_instance.schemas.empty() || force) {
      refresh_schemas(session);
    }

    if (m_instance.engines.empty() || force) {
      refresh_static_symbols(session);
    }

    if (m_instance.users.empty() || force) {
      refresh_database_symbols(session);
    }

    if (schema.empty()) {
      return;
    }

    if (auto s = find(&m_instance.schemas, schema)) {
      if (s->tables.empty() || force) {
        fetch_tables(session, s);
      }

      if (s->views.empty() || force) {
        fetch_views(session, s);
      }

      if (s->functions.empty() || force) {
        fetch_functions(session, s);
      }

      if (s->procedures.empty() || force) {
        fetch_procedures(session, s);
      }

      if (s->events.empty() || force) {
        fetch_events(session, s);
      }

      if (s->triggers.empty() || force) {
        fetch_triggers(session, s);
      }
    }
  }

  Completion_list complete_schema(const std::string &prefix) const {
    Completion_list result;

    for (const auto &schema : find_prefix_ci(m_instance.schemas, prefix)) {
      result.emplace_back(schema->name());
    }

    return result;
  }

  Completion_list complete(mysqlshdk::Sql_completion_result &&result) const {
    Completion_list list;
    const auto add_from_set = [&list](Names *s) {
      while (!s->empty()) {
        list.emplace_back(std::move(s->extract(s->begin()).value()));
      }
    };
    const auto quote = [&prefix = std::as_const(result.context.prefix)](
                           const std::string &s, bool as_string) {
      if (prefix.quoted_as_identifier) {
        return shcore::quote_identifier(s, prefix.quote);
      } else if (as_string && prefix.quoted_as_string) {
        return shcore::quote_string(s, prefix.quote);
      } else {
        // we quote here with '`' characters, they are still valid even if
        // ANSI_QUOTES is enabled
        return shcore::quote_identifier_if_needed(s);
      }
    };
    const auto quote_identifier = [&quote](const std::string &s) {
      return quote(s, false);
    };
    const auto quote_string_or_identifier = [&quote](const std::string &s) {
      return quote(s, true);
    };
    const auto add_identifiers_and_quote_if_needed =
        [&list, &quote_identifier](const auto &container) {
          for (const auto &item : container) {
            list.emplace_back(quote_identifier(item));
          }
        };
    const auto add_identifiers =
        [&list, &quote_identifier,
         &prefix = std::as_const(result.context.prefix.as_identifier)](
            const auto &container, bool as_function_call = false) {
          for (const auto &item : find_prefix_ci(container, prefix)) {
            auto name = quote_identifier(item->name());

            if (as_function_call) {
              name += '(';
              name += ')';
            }

            list.emplace_back(std::move(name));
          }
        };
    const auto add_strings_or_identifiers =
        [&list, &quote_string_or_identifier,
         &prefix =
             std::as_const(result.context.prefix.as_string_or_identifier)](
            const auto &container) {
          for (const auto &item : find_prefix_ci(container, prefix)) {
            list.emplace_back(quote_string_or_identifier(item->name()));
          }
        };
    const auto add_identifiers_from = [&add_identifiers, this](
                                          const Names &schemas, auto source,
                                          bool as_function_call = false) {
      for (const auto &schema : schemas) {
        if (const auto &s = find(m_instance.schemas, schema)) {
          add_identifiers(s->*source, as_function_call);
        }
      }
    };
    const auto add_columns = [&add_identifiers, this](const Columns &columns) {
      for (const auto &schema : columns) {
        if (const auto &s = find(m_instance.schemas, schema.first)) {
          for (const auto &table : schema.second) {
            if (const auto &t = find(s->tables, table)) {
              add_identifiers(t->columns);
            }

            if (const auto &v = find(s->views, table)) {
              add_identifiers(v->columns);
            }
          }
        }
      }
    };

    // these are already filtered
    add_from_set(&result.keywords);
    add_from_set(&result.system_functions);
    add_identifiers_and_quote_if_needed(result.tables);

    for (const auto &candidate : result.candidates) {
      using Candidate = mysqlshdk::Sql_completion_result::Candidate;

      switch (candidate) {
        case Candidate::SCHEMA:
          add_identifiers(m_instance.schemas);
          break;

        case Candidate::TABLE:
          add_identifiers_from(result.tables_from, &Instance::Schema::tables);
          break;

        case Candidate::VIEW:
          add_identifiers_from(result.views_from, &Instance::Schema::views);
          break;

        case Candidate::COLUMN:
          add_columns(result.columns);
          break;

        case Candidate::INTERNAL_COLUMN:
          add_columns(result.internal_columns);
          break;

        case Candidate::PROCEDURE:
          add_identifiers_from(result.procedures_from,
                               &Instance::Schema::procedures, true);
          break;

        case Candidate::FUNCTION:
          add_identifiers_from(result.functions_from,
                               &Instance::Schema::functions, true);
          break;

        case Candidate::TRIGGER:
          add_identifiers_from(result.triggers_from,
                               &Instance::Schema::triggers);
          break;

        case Candidate::EVENT:
          add_identifiers_from(result.events_from, &Instance::Schema::events);
          break;

        case Candidate::ENGINE:
          add_identifiers(m_instance.engines);
          break;

        case Candidate::UDF:
          add_identifiers(m_instance.udfs, true);
          break;

        case Candidate::RUNTIME_FUNCTION:
          // if prefix was quoted then we cannot match any functions
          if (!result.context.prefix.quoted) {
            for (const auto &item :
                 find_prefix_ci(*m_instance.system_functions,
                                result.context.prefix.full_wide)) {
              list.emplace_back(item->name());
            }
          }
          break;

        case Candidate::LOGFILE_GROUP:
          add_identifiers(m_instance.logfile_groups);
          break;

        case Candidate::USER_VAR:
          // user variables can be quoted as strings or identifiers
          add_strings_or_identifiers(m_instance.user_variables);
          break;

        case Candidate::SYSTEM_VAR:
          // system variables can be quoted as identifiers, but if ANSI_QUOTES
          // is active, only "sys_var" (and not i.e. @@"sys_var") form is
          // recognized; since we don't know the context, we simply disallow
          // double quotes
          if (!result.context.prefix.quoted ||
              '`' == result.context.prefix.quote) {
            for (const auto &item :
                 find_prefix_ci(m_instance.system_variables,
                                result.context.prefix.as_identifier)) {
              auto name = item->name();

              if (result.context.prefix.quote) {
                name = shcore::quote_identifier(name);
              }

              list.emplace_back(std::move(name));
            }
          }
          break;

        case Candidate::TABLESPACE:
          add_identifiers(m_instance.tablespaces);
          break;

        case Candidate::USER: {
          // construct the prefix which matches the format of user accounts
          // stored in cache: 'user'@'host'
          std::string prefix;
          const auto &as_account = result.context.as_account;

          using Account_part = parsers::Sql_completion_result::Account_part;

          if (!as_account.user.full.empty()) {
            const auto add_part = [&prefix](const Account_part &part) {
              prefix += shcore::quote_string(part.unquoted, '\'');
            };

            add_part(as_account.user);

            if (as_account.has_at_sign) {
              prefix += '@';

              if (!as_account.host.full.empty()) {
                add_part(as_account.host);
              }
            }

            // remove the last quote to allow for partial matches
            if (prefix.back() == '\'') {
              prefix.pop_back();
            }
          }

          // recreate candidate based on quotes used by the user
          const auto quote_user = [&as_account](const std::string &name) {
            const auto quote_like_part = [](const std::string &s,
                                            const Account_part &part) {
              if (part.full.empty()) {
                return shcore::quote_string(s, '\'');
              } else if (!part.quoted) {
                return s;
              } else {
                if (part.quote == '`') {
                  return shcore::quote_identifier(s);
                } else {
                  return shcore::quote_string(s, part.quote);
                }
              }
            };

            const auto account = shcore::split_account(name);
            std::string quoted = quote_like_part(account.user, as_account.user);
            quoted += '@';
            // if host part was not provided, quote it like the user, to keep
            // the same style
            quoted += quote_like_part(account.host, as_account.host.full.empty()
                                                        ? as_account.user
                                                        : as_account.host);

            return quoted;
          };

          for (const auto &item : find_prefix_ci(m_instance.users, prefix)) {
            list.emplace_back(quote_user(item->name()));
          }
        } break;

        case Candidate::CHARSET:
          // character sets can be quoted as strings or identifiers
          add_strings_or_identifiers(m_instance.charsets);
          break;

        case Candidate::COLLATION:
          // collations can be quoted as strings or identifiers
          add_strings_or_identifiers(m_instance.collations);
          break;

        case Candidate::PLUGIN:
          add_identifiers(m_instance.plugins);
          break;

        case Candidate::LABEL:
          // already filtered
          add_identifiers_and_quote_if_needed(result.context.labels);
          break;
      }
    }

    return list;
  }

  void clear_cache() {
    m_instance.clear();
    set_system_functions(k_current_version);
  }

 private:
  struct Compare_ci : public Case_insensitive_comparator {
    using Case_insensitive_comparator::operator();

    bool operator()(const Instance::Object &a,
                    const Instance::Object &b) const {
      return (*this)(a.wide_name(), b.wide_name());
    }

    bool operator()(const Instance::Object &o, const std::wstring &n) const {
      return (*this)(o.wide_name(), n);
    }

    bool operator()(const std::wstring &n, const Instance::Object &o) const {
      return (*this)(n, o.wide_name());
    }
  };

  template <class T>
  using is_instance_object =
      std::enable_if_t<std::is_base_of_v<Instance::Object, T>, int>;

  template <class T, is_instance_object<T> = 0>
  static void sort(std::vector<T> *container) {
    std::sort(container->begin(), container->end(), Compare_ci{});
  }

  template <class T, is_instance_object<T> = 0>
  static T *find(std::vector<T> *container, const std::string &name) {
    return const_cast<T *>(find(*container, name));
  }

  template <class T, is_instance_object<T> = 0>
  static const T *find(const std::vector<T> &container,
                       const std::string &name) {
    const auto wname = shcore::utf8_to_wide(name);
    const auto range = std::equal_range(container.begin(), container.end(),
                                        wname, Compare_ci{});

    for (auto it = range.first; it != range.second; ++it) {
      if (wname == it->wide_name()) {
        return &(*it);
      }
    }

    return nullptr;
  }

  template <class T, is_instance_object<T> = 0>
  static std::vector<const T *> find_prefix_ci(const std::vector<T> &container,
                                               const std::wstring &prefix) {
    auto it = std::lower_bound(container.begin(), container.end(), prefix,
                               Compare_ci{});
    std::vector<const T *> result;

    while (it != container.end()) {
      if (str_ibeginswith(it->wide_name(), prefix)) {
        result.emplace_back(&(*it));
        ++it;
      } else {
        break;
      }
    }

    return result;
  }

  template <class T, is_instance_object<T> = 0>
  static std::vector<const T *> find_prefix_ci(const std::vector<T> &container,
                                               const std::string &prefix) {
    return find_prefix_ci(container, shcore::utf8_to_wide(prefix));
  }

  Instance::Objects init_system_functions(base::MySQLVersion version) {
    Instance::Objects result;

    for (const auto &func :
         base::MySQLSymbolInfo::systemFunctionsForVersion(version)) {
      result.emplace_back(func + "()");
    }

    sort(&result);

    return result;
  }

  void refresh_static_symbols(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch_engines(session);
    fetch_character_sets(session);
    fetch_collations(session);
    fetch_system_variables(session);
    set_system_functions(session->get_server_version());
  }

  void refresh_database_symbols(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch_udfs(session);
    fetch_logfile_groups(session);
    fetch_user_variables(session);
    fetch_tablespaces(session);
    fetch_users(session);
    fetch_plugins(session);
  }

  template <class T, is_instance_object<T> = 0>
  void fetch(const std::shared_ptr<mysqlshdk::db::ISession> &session,
             std::string_view query, std::vector<T> *container) const {
    if (m_cancelled) {
      return;
    }

    container->clear();

    if (const auto result = session->query(query)) {
      while (!m_cancelled) {
        const auto row = result->fetch_one();

        if (!row) {
          break;
        }

        container->emplace_back(row->get_wstring(0));
      }
    }

    if (m_cancelled) {
      return;
    }

    // we're sorting on our own (instead of via query) to be consistent with
    // other methods which depend on the order
    sort(container);
  }

  void fetch_schemas(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch(session, "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA",
          &m_instance.schemas);
  }

  void fetch_tables(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                    Instance::Schema *schema) {
    fetch_tables(session, schema, &schema->tables);
  }

  void fetch_views(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                   Instance::Schema *schema) {
    fetch_tables(session, schema, &schema->views);
  }

  void fetch_tables(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                    const Instance::Schema *schema, Instance::Tables *target) {
    fetch(
        session,
        "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE" +
            std::string{target == &schema->tables ? "=" : "<>"} +
            "'BASE TABLE' AND TABLE_SCHEMA=" + quote_sql_string(schema->name()),
        target);

    for (auto &table : *target) {
      fetch_columns(session, schema, &table);
    }
  }

  void fetch_columns(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                     const Instance::Schema *schema, Instance::Table *table) {
    fetch(session,
          "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE "
          "TABLE_SCHEMA=" +
              quote_sql_string(schema->name()) +
              " AND TABLE_NAME=" + quote_sql_string(table->name()),
          &table->columns);
  }

  void fetch_functions(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                       Instance::Schema *schema) {
    fetch_routines(session, schema, &schema->functions);
  }

  void fetch_procedures(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                        Instance::Schema *schema) {
    fetch_routines(session, schema, &schema->procedures);
  }

  void fetch_routines(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                      const Instance::Schema *schema,
                      Instance::Objects *target) {
    fetch(session,
          "SELECT ROUTINE_NAME FROM INFORMATION_SCHEMA.ROUTINES WHERE "
          "ROUTINE_TYPE='" +
              std::string{target == &schema->functions ? "FUNCTION"
                                                       : "PROCEDURE"} +
              "' AND ROUTINE_SCHEMA=" + quote_sql_string(schema->name()),
          target);
  }

  void fetch_events(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                    Instance::Schema *schema) {
    fetch(
        session,
        "SELECT EVENT_NAME FROM INFORMATION_SCHEMA.EVENTS WHERE EVENT_SCHEMA=" +
            quote_sql_string(schema->name()),
        &schema->events);
  }

  void fetch_triggers(const std::shared_ptr<mysqlshdk::db::ISession> &session,
                      Instance::Schema *schema) {
    fetch(session,
          "SELECT TRIGGER_NAME FROM INFORMATION_SCHEMA.TRIGGERS WHERE "
          "TRIGGER_SCHEMA=" +
              quote_sql_string(schema->name()),
          &schema->triggers);
  }

  void fetch_engines(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    // SUPPORT can be YES, DEFAULT, NO, DISABLED
    fetch(session,
          "SELECT ENGINE FROM INFORMATION_SCHEMA.ENGINES WHERE SUPPORT IN "
          "('YES', 'DEFAULT')",
          &m_instance.engines);
  }

  void fetch_character_sets(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch(session,
          "SELECT CHARACTER_SET_NAME FROM INFORMATION_SCHEMA.CHARACTER_SETS",
          &m_instance.charsets);
  }

  void fetch_collations(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch(session, "SELECT COLLATION_NAME FROM INFORMATION_SCHEMA.COLLATIONS",
          &m_instance.collations);
  }

  void fetch_system_variables(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    try {
      fetch(session,
            "SELECT VARIABLE_NAME FROM performance_schema.session_variables",
            &m_instance.system_variables);
    } catch (const mysqlshdk::db::Error &e) {
      // this table does not exist in 5.6
      log_warning(
          "Failed to fetch system variables for SQL auto-completion: %s",
          e.format().c_str());
    }
  }

  void set_system_functions(const Version &version) {
    static const auto k_system_functions_57 =
        init_system_functions(base::MySQLVersion::MySQL57);
    static const auto k_system_functions_80 =
        init_system_functions(base::MySQLVersion::MySQL80);

    m_instance.system_functions = version < Version(8, 0, 0)
                                      ? &k_system_functions_57
                                      : &k_system_functions_80;
  }

  void fetch_udfs(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    try {
      fetch(session,
            "SELECT UDF_NAME FROM performance_schema.user_defined_functions",
            &m_instance.udfs);
    } catch (const mysqlshdk::db::Error &) {
      try {
        // this table does not exist in 5.7, mysql.func does not provide info on
        // functions registered by a component or plugin
        fetch(session, "SELECT name FROM mysql.func", &m_instance.udfs);
      } catch (const mysqlshdk::db::Error &e) {
        // some users don't have access to mysql tables
        log_warning("Failed to fetch UDFs for SQL auto-completion: %s",
                    e.format().c_str());
      }
    }
  }

  void fetch_logfile_groups(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch(session,
          "SELECT LOGFILE_GROUP_NAME FROM INFORMATION_SCHEMA.FILES WHERE "
          "LOGFILE_GROUP_NAME IS NOT NULL",
          &m_instance.logfile_groups);

    DBUG_EXECUTE_IF("sql_auto_completion_logfile_group", {
      m_instance.logfile_groups.emplace_back("log_file_group");
    });
  }

  void fetch_user_variables(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    try {
      std::string query =
          "SELECT VARIABLE_NAME FROM "
          "performance_schema.user_variables_by_thread WHERE THREAD_ID=";

      if (session->get_server_version() >= Version(8, 0, 16)) {
        query += "PS_CURRENT_THREAD_ID()";
      } else {
        query += "sys.ps_thread_id(NULL)";
      }

      fetch(session, query, &m_instance.user_variables);
    } catch (const mysqlshdk::db::Error &e) {
      // this table does not exist in 5.6
      log_warning("Failed to fetch user variables for SQL auto-completion: %s",
                  e.format().c_str());
    }
  }

  void fetch_tablespaces(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    // tablespaces created by users cannot contain a '/' character and cannot
    // begin with 'innodb_'
    fetch(session,
          "SELECT TABLESPACE_NAME FROM INFORMATION_SCHEMA.FILES WHERE "
          "TABLESPACE_NAME NOT LIKE '%/%' AND TABLESPACE_NAME NOT LIKE "
          "'innodb\\_%' AND TABLESPACE_NAME<>'mysql'",
          &m_instance.tablespaces);
  }

  void fetch_users(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    try {
      fetch(session,
            "SELECT CONCAT(QUOTE(User),'@',QUOTE(Host)) FROM mysql.user",
            &m_instance.users);
    } catch (const mysqlshdk::db::Error &) {
      // some users don't have access to the mysql tables
      // in some versions of MySQL server the GRANTEE column is truncated,
      // append a quote in this case
      fetch(
          session,
          R"(SELECT DISTINCT IF(RIGHT(GRANTEE, 1)="'",GRANTEE,CONCAT(GRANTEE,"'")) FROM INFORMATION_SCHEMA.USER_PRIVILEGES)",
          &m_instance.users);
    }
  }

  void fetch_plugins(const std::shared_ptr<mysqlshdk::db::ISession> &session) {
    fetch(session, "SELECT PLUGIN_NAME FROM INFORMATION_SCHEMA.PLUGINS",
          &m_instance.plugins);
  }

  Instance m_instance;
  volatile bool m_cancelled = false;
};

Provider_sql::Provider_sql()
    : m_completion_context{k_current_version},
      m_cache(std::make_unique<Cache>()) {
  m_completion_context.set_filtered(true);
  m_completion_context.set_uppercase_keywords(true);
  m_completion_context.set_sql_mode(std::string{k_default_sql_mode_80});
}

Provider_sql::~Provider_sql() = default;

Completion_list Provider_sql::complete_schema(const std::string &prefix) const {
  return m_cache->complete_schema(prefix);
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
 */
Completion_list Provider_sql::complete(const std::string &buffer,
                                       const std::string &line,
                                       size_t *compl_offset) {
  // completer needs full query
  const auto sql = buffer + line;

  // Line contains the line buffer up to the offset where TAB was pressed,
  // compl_offset is the offset in that line where linenoise thinks there's a
  // break character. Since it does not consider '.' as a break character, we
  // just send the whole line up to the TAB, completer will find the prefix and
  // we can adjust compl_offset accordingly.
  auto result = m_completion_context.complete(sql, sql.length());

  if (result.context.prefix.full.length() <= line.length()) {
    *compl_offset = line.length() - result.context.prefix.full.length();
  } else {
    // this should not happen
    assert(false);
    *compl_offset = 0;
  }

  return m_cache->complete(std::move(result));
}

void Provider_sql::interrupt_rehash() { m_cache->cancel(); }

void Provider_sql::refresh_schema_cache(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  update_completion_context(session);
  m_cache->refresh_schemas(session);
}

void Provider_sql::refresh_name_cache(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &current_schema, bool force) {
  update_completion_context(session);
  m_completion_context.set_active_schema(current_schema);
  m_cache->refresh_schema(session, current_schema, force);
}

void Provider_sql::update_completion_context(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  auto server_version = session->get_server_version();

  DBUG_EXECUTE_IF("sql_auto_completion_unsupported_version_lower",
                  { server_version = Version(1, 2, 3); });

  DBUG_EXECUTE_IF("sql_auto_completion_unsupported_version_higher",
                  { server_version = Version(10, 9, 8); });

  auto version = std::max(server_version, Version(5, 7, 0));
  version = std::min(version, k_current_version);

  if (version != server_version) {
    log_warning(
        "Connected to an unsupported server version %s, SQL auto-completion is "
        "falling back to: %s",
        server_version.get_full().c_str(), version.get_full().c_str());
  }

  m_completion_context.set_server_version(version);
  m_completion_context.set_sql_mode(session->get_sql_mode());
}

void Provider_sql::clear_name_cache() {
  reset_completion_context();
  m_cache->clear_cache();
}

void Provider_sql::reset_completion_context() {
  m_completion_context.set_server_version(k_current_version);
  m_completion_context.set_sql_mode(std::string{k_default_sql_mode_80});
  m_completion_context.set_active_schema({});
}

}  // namespace completer
}  // namespace shcore
