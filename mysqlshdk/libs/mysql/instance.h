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

#ifndef MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
#define MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/utils/version.h"

using Warnings_callback =
    std::function<void(const std::string &sql, int code,
                       const std::string &level, const std::string &msg)>;

namespace mysqlshdk {
namespace mysql {

/**
 * Supported system variables qualifiers: SESSION (default), GLOBAL, PERSIST,
 * and PERSIST_ONLY.
 */
enum class Var_qualifier {
  SESSION,
  GLOBAL,
  PERSIST,
  PERSIST_ONLY,
};

struct Auth_options {
  std::string user;
  std::optional<std::string> password;
  db::Ssl_options ssl_options;

  Auth_options() = default;
  Auth_options(const std::string &user_, const std::string &password_)
      : user(user_), password(password_) {}
  Auth_options(const mysqlshdk::db::Connection_options &copts) { get(copts); }
  void get(const mysqlshdk::db::Connection_options &copts);
  void set(mysqlshdk::db::Connection_options *copts) const;
};

inline bool operator==(const Auth_options &a, const Auth_options &b) {
  return a.user == b.user && a.password == b.password &&
         a.ssl_options == b.ssl_options;
}

class User_privileges;

/**
 * This interface defines the low level instance operations to be available.
 */
class IInstance {
 public:
  virtual ~IInstance() = default;

  virtual void register_warnings_callback(
      const Warnings_callback &callback) = 0;

  virtual std::string descr() const = 0;
  virtual std::string get_canonical_hostname() const = 0;
  virtual int get_canonical_port() const = 0;
  virtual std::optional<int> get_xport() const = 0;
  /**
   * @brief returns the canonical address that should be used to reach this
   * instance in the format: canonical_hostname + ':' + canonical_port
   *
   * @return std::string canonical address
   */
  virtual std::string get_canonical_address() const = 0;

  virtual const std::string &get_uuid() const = 0;
  virtual uint32_t get_id() const = 0;
  virtual const std::string &get_group_name() const = 0;
  virtual const std::string &get_version_compile_os() const = 0;
  virtual const std::string &get_version_compile_machine() const = 0;
  virtual uint32_t get_server_id() const = 0;

  virtual void refresh() = 0;

  virtual std::optional<bool> get_sysvar_bool(
      std::string_view name, const Var_qualifier scope) const = 0;
  virtual std::optional<std::string> get_sysvar_string(
      std::string_view name, const Var_qualifier scope) const = 0;
  virtual std::optional<int64_t> get_sysvar_int(
      std::string_view name, const Var_qualifier scope) const = 0;

  // get_sysvar_ with a default value
  bool get_sysvar_bool(std::string_view name, const Var_qualifier scope,
                       bool default_value) const {
    return get_sysvar_bool(name, scope).value_or(default_value);
  }
  std::string get_sysvar_string(std::string_view name,
                                const Var_qualifier scope,
                                const std::string &default_value) const {
    return get_sysvar_string(name, scope).value_or(default_value);
  }
  int64_t get_sysvar_int(std::string_view name, const Var_qualifier scope,
                         int64_t default_value) const {
    return get_sysvar_int(name, scope).value_or(default_value);
  }

  // get_sysvar_ without scope (defaults to GLOBAL), with and without a
  // default value
  std::optional<bool> get_sysvar_bool(std::string_view name) const {
    return get_sysvar_bool(name, Var_qualifier::GLOBAL);
  }
  std::optional<std::string> get_sysvar_string(std::string_view name) const {
    return get_sysvar_string(name, Var_qualifier::GLOBAL);
  }
  std::optional<int64_t> get_sysvar_int(std::string_view name) const {
    return get_sysvar_int(name, Var_qualifier::GLOBAL);
  }
  bool get_sysvar_bool(std::string_view name, bool default_value) const {
    return get_sysvar_bool(name, Var_qualifier::GLOBAL).value_or(default_value);
  }
  std::string get_sysvar_string(std::string_view name,
                                const std::string &default_value) const {
    return get_sysvar_string(name, Var_qualifier::GLOBAL)
        .value_or(default_value);
  }
  int64_t get_sysvar_int(std::string_view name, int64_t default_value) const {
    return get_sysvar_int(name, Var_qualifier::GLOBAL).value_or(default_value);
  }

  virtual void set_sysvar(
      const std::string &name, const std::string &value,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const = 0;
  virtual void set_sysvar(
      const std::string &name, const int64_t value,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const = 0;
  virtual void set_sysvar(
      const std::string &name, const bool value,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const = 0;
  virtual void set_sysvar_default(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const = 0;

  virtual bool has_variable_compiled_value(std::string_view name) const = 0;
  virtual bool is_performance_schema_enabled() const = 0;

  virtual bool is_ssl_enabled() const = 0;

  virtual bool is_read_only(bool super) const = 0;
  virtual utils::Version get_version() const = 0;

  virtual std::optional<std::string> get_system_variable(
      std::string_view name,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const = 0;
  virtual std::map<std::string, std::optional<std::string>>
  get_system_variables_like(
      const std::string &pattern,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const = 0;

  virtual std::shared_ptr<db::ISession> get_session() const = 0;
  virtual void close_session() const = 0;
  virtual void install_plugin(const std::string &plugin_name) const = 0;
  virtual void uninstall_plugin(const std::string &plugin_name) const = 0;
  virtual std::optional<std::string> get_plugin_status(
      const std::string &plugin_name) const = 0;

  struct Create_user_options {
    std::optional<std::string> password;
    std::string cert_issuer;
    std::string cert_subject;
    bool disable_pwd_expire{false};

    struct Grant {
      std::string privileges;
      std::string target;
      bool grant_option{false};
    };
    std::vector<Grant> grants;
  };
  virtual void create_user(std::string_view user, std::string_view host,
                           const Create_user_options &options) const = 0;
  virtual void drop_user(const std::string &user, const std::string &host,
                         bool if_exists = false) const = 0;

  virtual mysqlshdk::db::Connection_options get_connection_options() const = 0;
  virtual void get_current_user(std::string *current_user,
                                std::string *current_host) const = 0;
  virtual bool user_exists(const std::string &username,
                           const std::string &hostname) const = 0;
  virtual void set_user_password(const std::string &username,
                                 const std::string &hostname,
                                 const std::string &password) const = 0;
  virtual std::unique_ptr<User_privileges> get_user_privileges(
      const std::string &user, const std::string &host,
      bool allow_skip_grants_user = false) const = 0;
  virtual std::unique_ptr<User_privileges> get_current_user_privileges(
      bool allow_skip_grants_user = false) const = 0;
  virtual std::optional<bool> is_set_persist_supported() const = 0;
  virtual std::optional<std::string> get_persisted_value(
      const std::string &variable_name) const = 0;

  virtual std::vector<std::string> get_fence_sysvars() const = 0;

  virtual void suppress_binary_log(bool) = 0;

  virtual std::string get_plugin_library_extension() const = 0;

  virtual std::string generate_uuid() const = 0;

 public:
  // Convenience interface for session
  virtual std::shared_ptr<mysqlshdk::db::IResult> query(
      const std::string &sql, bool buffered = false) const = 0;

  virtual std::shared_ptr<mysqlshdk::db::IResult> query_udf(
      const std::string &sql, bool buffered = false) const = 0;

  virtual void execute(const std::string &sql) const = 0;

  template <typename... Args>
  inline std::shared_ptr<mysqlshdk::db::IResult> queryf(
      const std::string &sql, const Args &... args) const {
    return query(shcore::sqlformat(sql, args...));
  }

  template <typename... Args>
  inline void executef(const std::string &sql, const Args &... args) const {
    execute(shcore::sqlformat(sql, args...));
  }

  template <typename... Args>
  inline std::string queryf_one_string(int32_t column_index,
                                       const std::string &default_if_null,
                                       const std::string &sql,
                                       const Args &... args) const {
    auto result = query(shcore::sqlformat(sql, args...));
    if (auto row = result->fetch_one())
      return row->get_as_string(column_index, default_if_null);
    return default_if_null;
  }

  template <typename... Args>
  inline int64_t queryf_one_int(int32_t column_index, int64_t default_if_null,
                                const std::string &sql,
                                const Args &... args) const {
    auto result = query(shcore::sqlformat(sql, args...));
    if (auto row = result->fetch_one())
      return row->get_int(column_index, default_if_null);
    return default_if_null;
  }
};

struct Suppress_binary_log {
  explicit Suppress_binary_log(IInstance *inst) : m_instance(inst) {
    m_instance->suppress_binary_log(true);
  }

  ~Suppress_binary_log() {
    try {
      m_instance->suppress_binary_log(false);
    } catch (...) {
    }
  }

  IInstance *m_instance;
};

class Set_variable final {
 public:
  Set_variable(const Set_variable &) = delete;
  Set_variable(Set_variable &&) = delete;
  Set_variable &operator=(const Set_variable &) = delete;

  Set_variable(const IInstance &inst, const std::string &variable,
               const std::string &value, bool global)
      : m_instance(inst) {
    std::string fmt;
    if (global)
      fmt = "SET GLOBAL " + variable + "=?";
    else
      fmt = "SET SESSION " + variable + "=?";

    {
      auto res = inst.queryf("SELECT " + (global ? "@@global." + variable
                                                 : "@@session." + variable));
      auto row = res->fetch_one_or_throw();

      if (row->is_null(0)) {
        m_restore_sql = shcore::sqlformat(fmt, nullptr);
      } else {
        if (auto oval = row->get_string(0); oval != value)
          m_restore_sql = shcore::sqlformat(fmt, oval);
      }
    }
    if (!m_restore_sql.empty()) inst.executef(fmt, value);
  }

  Set_variable(const IInstance &inst, const std::string &variable, int value,
               bool global)
      : m_instance(inst) {
    std::string fmt;
    if (global)
      fmt = "SET GLOBAL " + variable + "=?";
    else
      fmt = "SET SESSION " + variable + "=?";
    {
      auto res = inst.queryf("SELECT " + (global ? "@@global." + variable
                                                 : "@@session." + variable));
      auto row = res->fetch_one_or_throw();

      if (row->is_null(0)) {
        m_restore_sql = shcore::sqlformat(fmt, nullptr);
      } else {
        if (auto oval = row->get_int(0); oval != value)
          m_restore_sql = shcore::sqlformat(fmt, oval);
      }
    }
    if (!m_restore_sql.empty()) inst.executef(fmt, value);
  }

  void restore() {
    if (!m_restore_sql.empty()) {
      m_instance.execute(std::exchange(m_restore_sql, {}));
    }
  }

  void restore_nothrow() noexcept {
    try {
      restore();
    } catch (...) {
    }
  }

  ~Set_variable() noexcept { restore_nothrow(); }

 private:
  const IInstance &m_instance;
  std::string m_restore_sql;
};

/**
 * Implementation of the low level instance operations.
 */
class Instance : public IInstance {
 public:
  Instance() = default;
  explicit Instance(const std::shared_ptr<db::ISession> &session);

  void register_warnings_callback(const Warnings_callback &callback) override;

  std::string descr() const override;
  std::string get_canonical_hostname() const override;
  std::string get_canonical_address() const override;
  int get_canonical_port() const override;
  std::optional<int> get_xport() const override;

  const std::string &get_uuid() const override;
  uint32_t get_id() const override;
  const std::string &get_group_name() const override;
  const std::string &get_version_compile_os() const override;
  const std::string &get_version_compile_machine() const override;
  uint32_t get_server_id() const override;

  // Clears cached values, forcing all methods (except for those that have
  // values that cannot change) to query the DB again, if they use a cache.
  void refresh() override;

  std::optional<bool> get_sysvar_bool(std::string_view name,
                                      const Var_qualifier scope) const override;
  std::optional<std::string> get_sysvar_string(
      std::string_view name, const Var_qualifier scope) const override;
  std::optional<int64_t> get_sysvar_int(
      std::string_view name, const Var_qualifier scope) const override;

  // expose the overloaded methods
  using IInstance::get_sysvar_bool;
  using IInstance::get_sysvar_int;
  using IInstance::get_sysvar_string;

  void set_sysvar(
      const std::string &name, const std::string &value,
      const Var_qualifier qualifier = Var_qualifier::GLOBAL) const override;
  void set_sysvar(
      const std::string &name, const int64_t value,
      const Var_qualifier qualifier = Var_qualifier::GLOBAL) const override;
  void set_sysvar(
      const std::string &name, const bool value,
      const Var_qualifier qualifier = Var_qualifier::GLOBAL) const override;
  void set_sysvar_default(
      const std::string &name,
      const Var_qualifier qualifier = Var_qualifier::GLOBAL) const override;

  bool has_variable_compiled_value(std::string_view name) const override;
  bool is_performance_schema_enabled() const override;
  bool is_ssl_enabled() const override;

  std::shared_ptr<db::ISession> get_session() const override {
    return _session;
  }

  void close_session() const override { _session->close(); }

  std::optional<std::string> get_system_variable(
      std::string_view name,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const override;
  std::map<std::string, std::optional<std::string>> get_system_variables_like(
      const std::string &pattern,
      const Var_qualifier scope = Var_qualifier::GLOBAL) const override;

  void install_plugin(const std::string &plugin_name) const override;
  void uninstall_plugin(const std::string &plugin_name) const override;
  std::optional<std::string> get_plugin_status(
      const std::string &plugin_name) const override;
  void create_user(std::string_view user, std::string_view host,
                   const Create_user_options &options) const override;
  void drop_user(const std::string &user, const std::string &host,
                 bool if_exists = false) const override;
  mysqlshdk::db::Connection_options get_connection_options() const override {
    auto co = _session->get_connection_options();

    // we don't want to be using cached value of connect-timeout option
    co.clear_connect_timeout();

    return co;
  }
  std::unique_ptr<User_privileges> get_user_privileges(
      const std::string &user, const std::string &host,
      bool allow_skip_grants_user = false) const override;
  std::unique_ptr<User_privileges> get_current_user_privileges(
      bool allow_skip_grants_user = false) const override;

  bool is_read_only(bool super) const override;
  utils::Version get_version() const override;

  void get_current_user(std::string *current_user,
                        std::string *current_host) const override;
  bool user_exists(const std::string &username,
                   const std::string &hostname) const override;
  void set_user_password(const std::string &username,
                         const std::string &hostname,
                         const std::string &password) const override;
  /**
   * Determine if SET PERSIST is supported by the instance.
   *
   * Supported for server versions >= 8.0.11 as long as 'persisted_globals_load'
   * is enabled.
   *
   * @return optional bool object with the value true if SET
   *         PERSIST is support, false if it is supported but
   *         'persisted_globals_load' is disabled, and null if not supported.
   */
  std::optional<bool> is_set_persist_supported() const override;

  /**
   * Return the persisted value for the given variable.
   *
   * The persisted value of a variable may not match its current system variable
   * value, in particular if PERSIST_ONLY was used to set it. This function
   * returns the persisted value of a specified variable from the
   * performance_schema.persisted_variables table.
   *
   * @param variable_name string with the name of the variable to obtain the
   *                      persisted value.
   * @return nullable string with the persisted value for the specified
   *         variable, or null if no persisted value was found.
   */
  std::optional<std::string> get_persisted_value(
      const std::string &variable_name) const override;

  std::vector<std::string> get_fence_sysvars() const override;

  void suppress_binary_log(bool flag) override;

  std::string get_plugin_library_extension() const override;

  /**
   * Generate a UUID to use for the group name / view change uuid
   *
   * The UUID is generated from the target instance (MySQL server) using the
   * UUID() SQL function.
   *
   * @return A string with a new UUID to be used,
   */
  std::string generate_uuid() const override;

 public:
  std::shared_ptr<mysqlshdk::db::IResult> query(
      const std::string &sql, bool buffered = false) const override;

  std::shared_ptr<mysqlshdk::db::IResult> query_udf(
      const std::string &sql, bool buffered = false) const override;

  void execute(const std::string &sql) const override;

 private:
  void process_result_warnings(const std::string &sql,
                               mysqlshdk::db::IResult &result) const;

 private:
  std::shared_ptr<db::ISession> _session;
  mutable mysqlshdk::utils::Version _version;
  mutable std::string m_version_compile_os;
  mutable std::string m_version_compile_machine;
  mutable std::string m_uuid;
  mutable std::optional<uint32_t> m_id;
  mutable std::string m_group_name;
  mutable std::string m_hostname;
  mutable int m_port = 0;
  mutable std::optional<int> m_xport;
  mutable uint32_t m_server_id = 0;
  int m_sql_binlog_suppress_count = 0;
  Warnings_callback m_warnings_callback = nullptr;
};

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
