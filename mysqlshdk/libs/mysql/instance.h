/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/version.h"

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

/**
 * This interface defines the low level instance operations to be available.
 */
class IInstance {
 public:
  virtual ~IInstance() {}

  virtual std::string descr() const = 0;
  virtual std::string get_canonical_hostname() const = 0;
  virtual int get_canonical_port() const = 0;
  virtual std::string get_canonical_address() const = 0;

  virtual void cache_global_sysvars(bool force_refresh = false) = 0;
  virtual utils::nullable<std::string> get_cached_global_sysvar(
      const std::string &name) const = 0;
  virtual utils::nullable<bool> get_cached_global_sysvar_as_bool(
      const std::string &name) const = 0;

  virtual utils::nullable<bool> get_sysvar_bool(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual utils::nullable<std::string> get_sysvar_string(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual utils::nullable<int64_t> get_sysvar_int(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual void set_sysvar(
      const std::string &name, const std::string &value,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual void set_sysvar(
      const std::string &name, const int64_t value,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual void set_sysvar(
      const std::string &name, const bool value,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual void set_sysvar_default(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual bool has_variable_compiled_value(const std::string &name) const = 0;
  virtual bool is_performance_schema_enabled() const = 0;

  virtual bool is_read_only(bool super) const = 0;
  virtual utils::Version get_version() const = 0;

  virtual std::map<std::string, utils::nullable<std::string>>
  get_system_variables_like(
      const std::string &pattern,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;

  virtual std::shared_ptr<db::ISession> get_session() const = 0;
  virtual void close_session() const = 0;
  virtual void install_plugin(const std::string &plugin_name) const = 0;
  virtual void uninstall_plugin(const std::string &plugin_name) const = 0;
  virtual utils::nullable<std::string> get_plugin_status(
      const std::string &plugin_name) const = 0;
  virtual void create_user(
      const std::string &user, const std::string &host, const std::string &pwd,
      const std::vector<std::tuple<std::string, std::string, bool>> &privileges)
      const = 0;
  virtual void drop_user(const std::string &user, const std::string &host,
                         bool if_exists = false) const = 0;
  virtual void drop_users_with_regexp(const std::string &regexp) const = 0;
  virtual mysqlshdk::db::Connection_options get_connection_options() const = 0;
  virtual void get_current_user(std::string *current_user,
                                std::string *current_host) const = 0;
  virtual bool user_exists(const std::string &username,
                           const std::string &hostname) const = 0;
  virtual std::unique_ptr<User_privileges> get_user_privileges(
      const std::string &user, const std::string &host) const = 0;
  virtual utils::nullable<bool> is_set_persist_supported() const = 0;

  virtual void suppress_binary_log(bool) = 0;
};

struct Suppress_binary_log {
  explicit Suppress_binary_log(IInstance *inst) : m_instance(inst) {
    m_instance->suppress_binary_log(true);
  }

  ~Suppress_binary_log() { m_instance->suppress_binary_log(false); }

  IInstance *m_instance;
};

/**
 * Implementation of the low level instance operations.
 */
class Instance : public IInstance {
 public:
  Instance() {}
  explicit Instance(std::shared_ptr<db::ISession> session);

  std::string descr() const override;
  std::string get_canonical_hostname() const override;
  std::string get_canonical_address() const override;
  int get_canonical_port() const override;

  void cache_global_sysvars(bool force_refresh = false) override;
  utils::nullable<std::string> get_cached_global_sysvar(
      const std::string &name) const override;
  utils::nullable<bool> get_cached_global_sysvar_as_bool(
      const std::string &name) const override;

  utils::nullable<bool> get_sysvar_bool(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const override;
  utils::nullable<std::string> get_sysvar_string(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const override;
  utils::nullable<int64_t> get_sysvar_int(
      const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const override;

  void set_sysvar(
      const std::string &name, const std::string &value,
      const Var_qualifier qualifier = Var_qualifier::SESSION) const override;
  void set_sysvar(
      const std::string &name, const int64_t value,
      const Var_qualifier qualifier = Var_qualifier::SESSION) const override;
  void set_sysvar(
      const std::string &name, const bool value,
      const Var_qualifier qualifier = Var_qualifier::SESSION) const override;
  void set_sysvar_default(
      const std::string &name,
      const Var_qualifier qualifier = Var_qualifier::SESSION) const override;
  bool has_variable_compiled_value(const std::string &name) const override;
  bool is_performance_schema_enabled() const override;

  std::shared_ptr<db::ISession> get_session() const override {
    return _session;
  }

  void close_session() const override { _session->close(); }
  std::map<std::string, utils::nullable<std::string>> get_system_variables(
      const std::vector<std::string> &names,
      const Var_qualifier scope = Var_qualifier::SESSION) const;
  std::map<std::string, utils::nullable<std::string>> get_system_variables_like(
      const std::string &pattern,
      const Var_qualifier scope = Var_qualifier::SESSION) const override;
  void install_plugin(const std::string &plugin_name) const override;
  void uninstall_plugin(const std::string &plugin_name) const override;
  utils::nullable<std::string> get_plugin_status(
      const std::string &plugin_name) const override;
  void create_user(const std::string &user, const std::string &host,
                   const std::string &pwd,
                   const std::vector<std::tuple<std::string, std::string, bool>>
                       &grants) const override;
  void drop_user(const std::string &user, const std::string &host,
                 bool if_exists = false) const override;
  void drop_users_with_regexp(const std::string &regexp) const override;
  mysqlshdk::db::Connection_options get_connection_options() const override {
    return _session->get_connection_options();
  }
  std::unique_ptr<User_privileges> get_user_privileges(
      const std::string &user, const std::string &host) const override;

  bool is_read_only(bool super) const override;
  utils::Version get_version() const override;

  void get_current_user(std::string *current_user,
                        std::string *current_host) const override;
  bool user_exists(const std::string &username,
                   const std::string &hostname) const override;
  /**
   * Determine if SET PERSIST is supported by the instance.
   *
   * Supported for server versions >= 8.0.11 as long as 'persisted_globals_load'
   * is enabled.
   *
   * @return mysqlshdk::utils::nullable<bool> object with the value true if SET
   *         PERSIST is support, false if it is supported but
   *         'persisted_globals_load' is disabled, and null if not supported.
   */
  utils::nullable<bool> is_set_persist_supported() const override;

  void suppress_binary_log(bool flag) override;

 private:
  std::shared_ptr<db::ISession> _session;
  mutable mysqlshdk::utils::Version _version;
  mutable std::string m_version_compile_os;
  std::map<std::string, utils::nullable<std::string>> _global_sysvars;
  int m_sql_binlog_suppress_count = 0;

  const std::string &get_version_compile_os() const;
  std::string get_plugin_library_extension() const;
};

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
