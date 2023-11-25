/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECK_H_

#include <forward_list>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk {
namespace db {
class ISession;
class IRow;
}  // namespace db
}  // namespace mysqlshdk

namespace mysqlsh {

class Upgrade_check_output_formatter;

struct Upgrade_issue {
  enum Level { ERROR = 0, WARNING, NOTICE };
  static const char *level_to_string(const Upgrade_issue::Level level);

  std::string schema;
  std::string table;
  std::string column;
  std::string description;
  Level level = ERROR;

  bool empty() const {
    return schema.empty() && table.empty() && column.empty() &&
           description.empty();
  }
  std::string get_db_object() const;
};

struct Upgrade_check_options {
  static const shcore::Option_pack_def<Upgrade_check_options> &options();

  std::optional<mysqlshdk::utils::Version> target_version;
  std::string config_path;
  std::string output_format;
  std::optional<std::string> password;

  mysqlshdk::utils::Version get_target_version() const;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Upgrade_check_options, set_target_version);
#endif
  void set_target_version(const std::string &value);
};

std::string upgrade_issue_to_string(const Upgrade_issue &problem);

class Upgrade_check {
 public:
  struct Upgrade_info {
    mysqlshdk::utils::Version server_version;
    std::string server_version_long;
    mysqlshdk::utils::Version target_version;
    std::string server_os;
    std::string config_path;
    bool explicit_target_version;
  };

  enum class Target {
    INNODB_INTERNALS,
    OBJECT_DEFINITIONS,
    ENGINES,
    TABLESPACES,
    SYSTEM_VARIABLES,
    AUTHENTICATION_PLUGINS,
    MDS_SPECIFIC,
    LAST,
  };

  using Target_flags = mysqlshdk::utils::Enum_set<Target, Target::LAST>;

  using Creator =
      std::function<std::unique_ptr<Upgrade_check>(const Upgrade_info &)>;

  struct Creator_info {
    std::forward_list<mysqlshdk::utils::Version> versions;
    Creator creator;
    Target target;
  };

  using Collection = std::vector<Creator_info>;

  class Check_configuration_error : public std::runtime_error {
   public:
    explicit Check_configuration_error(const char *what)
        : std::runtime_error(what) {}
  };

  class Check_not_needed : public std::exception {};

  template <class... Ts>
  static bool register_check(Creator creator, Target target, Ts... params) {
    std::forward_list<std::string> vs{params...};
    std::forward_list<mysqlshdk::utils::Version> vers;
    for (const auto &v : vs) vers.emplace_front(mysqlshdk::utils::Version(v));
    s_available_checks.emplace_back(
        Creator_info{std::move(vers), creator, target});
    return true;
  }

  static bool register_check(Creator creator, Target target,
                             const mysqlshdk::utils::Version &ver) {
    s_available_checks.emplace_back(Creator_info{
        std::forward_list<mysqlshdk::utils::Version>{ver}, creator, target});
    return true;
  }

  static void register_manual_check(const char *ver, const char *name,
                                    Upgrade_issue::Level level, Target target);

  static void prepare_translation_file(const char *filename);

  static std::vector<std::unique_ptr<Upgrade_check>> create_checklist(
      const Upgrade_info &info,
      Target_flags flags = Target_flags::all().unset(Target::MDS_SPECIFIC));

  explicit Upgrade_check(const char *name) : m_name(name) {}
  virtual ~Upgrade_check() {}

  virtual const char *get_name() const { return m_name; }
  virtual const char *get_title() const;
  virtual const char *get_description() const;
  virtual const char *get_doc_link() const;
  virtual Upgrade_issue::Level get_level() const = 0;
  virtual bool is_runnable() const { return true; }
  virtual bool is_multi_lvl_check() const { return false; }

  virtual std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) = 0;

 protected:
  virtual const char *get_description_internal() const { return nullptr; }
  virtual const char *get_doc_link_internal() const { return nullptr; }
  virtual const char *get_title_internal() const { return nullptr; }

  const char *m_name;

 private:
  static const char *get_text(const char *name, const char *field);
  static Collection s_available_checks;
};

class Sql_upgrade_check : public Upgrade_check {
 public:
  static std::unique_ptr<Sql_upgrade_check> get_reserved_keywords_check(
      const Upgrade_info &info);
  static std::unique_ptr<Upgrade_check> get_routine_syntax_check();
  static std::unique_ptr<Sql_upgrade_check> get_utf8mb3_check();
  static std::unique_ptr<Sql_upgrade_check> get_innodb_rowformat_check();
  static std::unique_ptr<Sql_upgrade_check> get_zerofill_check();
  static std::unique_ptr<Sql_upgrade_check> get_nonnative_partitioning_check();
  static std::unique_ptr<Sql_upgrade_check> get_mysql_schema_check();
  static std::unique_ptr<Sql_upgrade_check> get_old_temporal_check();
  static std::unique_ptr<Sql_upgrade_check> get_foreign_key_length_check();
  static std::unique_ptr<Sql_upgrade_check> get_maxdb_sql_mode_flags_check();
  static std::unique_ptr<Sql_upgrade_check> get_obsolete_sql_mode_flags_check();
  static std::unique_ptr<Sql_upgrade_check> get_enum_set_element_length_check();
  static std::unique_ptr<Sql_upgrade_check>
  get_partitioned_tables_in_shared_tablespaces_check(const Upgrade_info &info);
  static std::unique_ptr<Sql_upgrade_check> get_circular_directory_check();
  static std::unique_ptr<Sql_upgrade_check> get_removed_functions_check();
  static std::unique_ptr<Sql_upgrade_check> get_groupby_asc_syntax_check();
  static std::unique_ptr<Upgrade_check> get_removed_sys_log_vars_check(
      const Upgrade_info &info);
  static std::unique_ptr<Upgrade_check> get_removed_sys_vars_check(
      const Upgrade_info &info);

  static std::unique_ptr<Upgrade_check> get_sys_vars_new_defaults_check();
  static std::unique_ptr<Sql_upgrade_check> get_zero_dates_check();
  static std::unique_ptr<Sql_upgrade_check> get_schema_inconsistency_check();
  static std::unique_ptr<Sql_upgrade_check> get_fts_in_tablename_check(
      const Upgrade_info &info);
  static std::unique_ptr<Sql_upgrade_check> get_engine_mixup_check();
  static std::unique_ptr<Sql_upgrade_check> get_old_geometry_types_check(
      const Upgrade_info &info);
  static std::unique_ptr<Sql_upgrade_check>
  get_changed_functions_generated_columns_check(const Upgrade_info &info);
  static std::unique_ptr<Sql_upgrade_check>
  get_columns_which_cannot_have_defaults_check();
  static std::unique_ptr<Sql_upgrade_check> get_invalid_57_names_check();
  static std::unique_ptr<Sql_upgrade_check> get_orphaned_routines_check();
  static std::unique_ptr<Sql_upgrade_check> get_dollar_sign_name_check();
  static std::unique_ptr<Sql_upgrade_check> get_index_too_large_check();
  static std::unique_ptr<Sql_upgrade_check> get_empty_dot_table_syntax_check();
  static std::unique_ptr<Sql_upgrade_check>
  get_invalid_engine_foreign_key_check();
  static std::unique_ptr<Sql_upgrade_check> get_deprecated_auth_method_check(
      const Upgrade_check::Upgrade_info &info);
  static std::unique_ptr<Sql_upgrade_check>
  get_deprecated_router_auth_method_check(
      const Upgrade_check::Upgrade_info &info);
  static std::unique_ptr<Sql_upgrade_check> get_deprecated_default_auth_check(
      const Upgrade_check::Upgrade_info &info);

  Sql_upgrade_check(const char *name, const char *title,
                    std::vector<std::string> &&queries,
                    Upgrade_issue::Level level = Upgrade_issue::WARNING,
                    const char *advice = "",
                    const char *minimal_version = nullptr,
                    std::forward_list<std::string> &&set_up =
                        std::forward_list<std::string>(),
                    std::forward_list<std::string> &&clean_up =
                        std::forward_list<std::string>());

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override;

  const std::vector<std::string> &get_queries() const { return m_queries; }

 protected:
  virtual Upgrade_issue parse_row(const mysqlshdk::db::IRow *row);
  virtual void add_issue(const mysqlshdk::db::IRow *row,
                         std::vector<Upgrade_issue> *issues);
  const char *get_description_internal() const override;
  const char *get_title_internal() const override;
  Upgrade_issue::Level get_level() const override { return m_level; }

  std::vector<std::string> m_queries;
  std::forward_list<std::string> m_set_up;
  std::forward_list<std::string> m_clean_up;
  const Upgrade_issue::Level m_level;
  std::string m_title;
  std::string m_advice;
  std::string m_doc_link;
  const char *m_minimal_version;
};

// This class enables checking server configuration file for defined/undefined
// system variables that have been removed, deprecated etc.
class Config_check : public Upgrade_check {
 public:
  enum class Mode { FLAG_DEFINED, FLAG_UNDEFINED };

  Config_check(const char *name, std::map<std::string, const char *> &&vars,
               Mode mode = Mode::FLAG_DEFINED,
               Upgrade_issue::Level level = Upgrade_issue::ERROR,
               const char *problem_description = "is set and will be removed",
               const char *title = "", const char *advice = "");

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override;

 protected:
  const char *get_description_internal() const override {
    return m_advice.empty() ? nullptr : m_advice.c_str();
  }
  const char *get_title_internal() const override {
    return m_title.empty() ? nullptr : m_title.c_str();
  }
  Upgrade_issue::Level get_level() const override { return m_level; }

  std::map<std::string, const char *> m_vars;
  Mode m_mode;
  const Upgrade_issue::Level m_level;
  std::string m_problem_description;
  std::string m_title;
  std::string m_advice;
};

class Check_table_command : public Upgrade_check {
 public:
  Check_table_command();

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override;

  Upgrade_issue::Level get_level() const override {
    throw std::runtime_error("Unimplemented");
  }

 protected:
  const char *get_description_internal() const override { return nullptr; }

  const char *get_title_internal() const override {
    return "Issues reported by 'check table x for upgrade' command";
  }
};

class Manual_check : public Upgrade_check {
 public:
  Manual_check(const char *name, Upgrade_issue::Level level)
      : Upgrade_check(name), m_level(level) {}

  Upgrade_issue::Level get_level() const override { return m_level; }
  bool is_runnable() const override { return false; }

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> & /*session*/,
      const Upgrade_info & /*server_info*/) override {
    throw std::runtime_error("Manual check not meant to be executed");
  }

 protected:
  const char *get_description_internal() const override {
    return Upgrade_issue::level_to_string(m_level);
  }

  Upgrade_issue::Level m_level;
};

class Deprecated_default_auth_check : public Sql_upgrade_check {
 public:
  Deprecated_default_auth_check(mysqlshdk::utils::Version target_ver);
  ~Deprecated_default_auth_check();

  void parse_var(const std::string &item, const std::string &auth,
                 std::vector<Upgrade_issue> *issues);

  bool is_multi_lvl_check() const override { return true; }

 protected:
  void add_issue(const mysqlshdk::db::IRow *row,
                 std::vector<Upgrade_issue> *issues) override;

 private:
  const mysqlshdk::utils::Version m_target_version;
};

class Upgrade_check_config final {
 public:
  using Include_issue = std::function<bool(const Upgrade_issue &)>;

  explicit Upgrade_check_config(const Upgrade_check_options &options);

  const Upgrade_check::Upgrade_info &upgrade_info() const {
    return m_upgrade_info;
  }

  void set_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  const std::shared_ptr<mysqlshdk::db::ISession> &session() const {
    return m_session;
  }

  std::unique_ptr<Upgrade_check_output_formatter> formatter() const;

  void set_user_privileges(
      const mysqlshdk::mysql::User_privileges *privileges) {
    m_privileges = privileges;
  }

  const mysqlshdk::mysql::User_privileges *user_privileges() const {
    return m_privileges;
  }

  void set_issue_filter(const Include_issue &filter) { m_filter = filter; }

  std::vector<Upgrade_issue> filter_issues(
      std::vector<Upgrade_issue> &&issues) const;

  void set_targets(Upgrade_check::Target_flags flags) {
    m_target_flags = flags;
  }

  Upgrade_check::Target_flags targets() const { return m_target_flags; }

 private:
  Upgrade_check::Upgrade_info m_upgrade_info;
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  std::string m_output_format;
  const mysqlshdk::mysql::User_privileges *m_privileges;
  Include_issue m_filter;
  Upgrade_check::Target_flags m_target_flags =
      Upgrade_check::Target_flags::all().unset(
          Upgrade_check::Target::MDS_SPECIFIC);
};

/**
 * Checks if server is ready for upgrade.
 *
 * @param config upgrade configuration
 *
 * @returns true if server is eligible for upgrade (there were no errors)
 */
bool check_for_upgrade(const Upgrade_check_config &config);

} /* namespace mysqlsh */

#endif  // MODULES_UTIL_UPGRADE_CHECK_H_
