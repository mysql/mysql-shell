/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
#define MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/nullable.h"


namespace mysqlshdk {
namespace mysql {

/**
 * Supported system variables scopes: SESSION (default), GLOBAL.
 */
enum class VarScope {
  SESSION,
  GLOBAL
};

/**
 * This interface defines the low level instance operations to be available.
 */
class IInstance {
 public:
  virtual ~IInstance() {}
  virtual utils::nullable<bool> get_sysvar_bool(
      const std::string& name,
      const VarScope &scope = VarScope::SESSION) const = 0;
  virtual utils::nullable<std::string> get_sysvar_string(
      const std::string& name,
      const VarScope &scope = VarScope::SESSION) const = 0;
  virtual utils::nullable<int64_t> get_sysvar_int(
      const std::string& name,
      const VarScope &scope = VarScope::SESSION) const = 0;
  virtual void set_sysvar(const std::string &name,
                          const std::string &value,
                          const VarScope &scope = VarScope::SESSION) const = 0;
  virtual void set_sysvar(const std::string &name, const int64_t value,
                          const VarScope &scope = VarScope::SESSION) const = 0;
  virtual void set_sysvar(const std::string &name, const bool value,
                          const VarScope &scope = VarScope::SESSION) const = 0;
  virtual std::shared_ptr<db::ISession> get_session() const = 0;
  virtual void install_plugin(const std::string &plugin_name) const = 0;
  virtual void uninstall_plugin(const std::string &plugin_name) const = 0;
  virtual utils::nullable<std::string> get_plugin_status(
      const std::string &plugin_name) const = 0;
  virtual void create_user(
      const std::string &user, const std::string &host, const std::string &pwd,
      const std::vector<std::tuple<std::string, std::string, bool>> &privileges)
      const = 0;
  virtual void drop_user(const std::string &user,
                         const std::string &host) const = 0;
  virtual std::tuple<bool, std::string, bool> check_user(
      const std::string &user, const std::string &host,
      const std::vector<std::string> &privileges,
      const std::string &on_db, const std::string &on_obj) const = 0;
};

/**
 * Implementation of the low level instance operations.
 */
class Instance : public IInstance {
 public:
  explicit Instance(std::shared_ptr<db::ISession> session);

  utils::nullable<bool> get_sysvar_bool(
      const std::string& name,
      const VarScope &scope = VarScope::SESSION) const override;
  utils::nullable<std::string> get_sysvar_string(
      const std::string& name,
      const VarScope &scope = VarScope::SESSION) const override;
  utils::nullable<int64_t> get_sysvar_int(
      const std::string& name,
      const VarScope &scope = VarScope::SESSION) const override;
  void set_sysvar(const std::string &name,
                  const std::string &value,
                  const VarScope &scope = VarScope::SESSION) const override;
  void set_sysvar(const std::string &name, const int64_t value,
                  const VarScope &scope = VarScope::SESSION) const override;
  void set_sysvar(const std::string &name, const bool value,
                  const VarScope &scope = VarScope::SESSION) const override;
  std::shared_ptr<db::ISession> get_session() const override {
    return _session;
  }
  std::map<std::string, utils::nullable<std::string> > get_system_variables(
      const std::vector<std::string>& names,
      const VarScope &scope = VarScope::SESSION) const;
  void install_plugin(const std::string &plugin_name) const override;
  void uninstall_plugin(const std::string &plugin_name) const override;
  utils::nullable<std::string> get_plugin_status(
      const std::string &plugin_name) const override;
  void create_user(
      const std::string &user, const std::string &host, const std::string &pwd,
      const std::vector<std::tuple<std::string, std::string, bool>> &grants)
      const override;
  void drop_user(const std::string &user,
                 const std::string &host) const override;
  std::tuple<bool, std::string, bool> check_user(
      const std::string &user, const std::string &host,
      const std::vector<std::string> &privileges,
      const std::string &on_db, const std::string &on_obj) const override;

 private:
  std::shared_ptr<db::ISession> _session;
  // List of privileges equivalent to "ALL [PRIVILEGES]".
  const std::vector<std::string> kAllPrivileges = {
      "ALTER", "ALTER ROUTINE", "CREATE", "CREATE ROUTINE",
      "CREATE TABLESPACE", "CREATE TEMPORARY TABLES", "CREATE USER",
      "CREATE VIEW", "DELETE", "DROP", "EVENT", "EXECUTE", "FILE", "INDEX",
      "INSERT", "LOCK TABLES", "PROCESS", "REFERENCES", "RELOAD",
      "REPLICATION CLIENT", "REPLICATION SLAVE", "SELECT", "SHOW DATABASES",
      "SHOW VIEW", "SHUTDOWN", "SUPER", "TRIGGER", "UPDATE"};
};

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
