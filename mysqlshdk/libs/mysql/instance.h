/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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
  virtual utils::nullable<bool> get_sysvar_bool(
      const std::string& name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual utils::nullable<std::string> get_sysvar_string(
      const std::string& name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;
  virtual utils::nullable<int64_t> get_sysvar_int(
      const std::string& name,
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
  virtual void set_sysvar_default(const std::string &name,
      const Var_qualifier scope = Var_qualifier::SESSION) const = 0;

  virtual bool is_read_only(bool super) const = 0;
  virtual utils::Version get_version() const = 0;

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
  virtual void drop_users_with_regexp(const std::string &regexp) const = 0;
  virtual std::unique_ptr<User_privileges> get_user_privileges(
      const std::string &user, const std::string &host) const = 0;
};

/**
 * Implementation of the low level instance operations.
 */
class Instance : public IInstance {
 public:
  explicit Instance(std::shared_ptr<db::ISession> session);

  utils::nullable<bool> get_sysvar_bool(
      const std::string& name,
      const Var_qualifier scope = Var_qualifier::SESSION) const override;
  utils::nullable<std::string> get_sysvar_string(
      const std::string& name,
      const Var_qualifier scope = Var_qualifier::SESSION) const override;
  utils::nullable<int64_t> get_sysvar_int(
      const std::string& name,
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
  void set_sysvar_default(const std::string &name,
      const Var_qualifier qualifier = Var_qualifier::SESSION) const override;
  std::shared_ptr<db::ISession> get_session() const override {
    return _session;
  }
  std::map<std::string, utils::nullable<std::string> > get_system_variables(
      const std::vector<std::string>& names,
      const Var_qualifier scope = Var_qualifier::SESSION) const;
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
  void drop_users_with_regexp(const std::string &regexp) const override;
  std::unique_ptr<User_privileges> get_user_privileges(const std::string &user,
      const std::string &host) const override;

  bool is_read_only(bool super) const override;
  utils::Version get_version() const override;

 private:
  std::shared_ptr<db::ISession> _session;
  mutable mysqlshdk::utils::Version _version;
};

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
