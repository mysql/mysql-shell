/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

class Upgrade_check {
 public:
  explicit Upgrade_check(const std::string_view name) : m_name(name) {}
  virtual ~Upgrade_check() {}

  virtual const std::string &get_name() const { return m_name; }
  virtual const std::string &get_title() const;
  virtual const std::string &get_description() const;
  virtual const std::string &get_doc_link() const;
  virtual bool is_runnable() const { return true; }
  virtual bool is_multi_lvl_check() const { return false; }

  virtual std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) {
    (void)session;
    (void)server_info;
    throw std::logic_error("not implemented");
  }

  virtual std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) {
    // override this method if cache is used by the check
    (void)cache;
    return run(session, server_info);
  }

  const std::string &get_text(const char *field) const;
  virtual bool enabled() const { return true; }

 private:
  std::string m_name;
};

class Removed_sys_var_check : public Upgrade_check {
 public:
  Removed_sys_var_check(const std::string_view name,
                        const Upgrade_info &server_info);

  void add_sys_var(const Version &version,
                   std::map<std::string, const char *> removed_vars);
  bool has_sys_var(const std::string &name) const;
  bool enabled() const override;

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) override;

 protected:
  std::map<std::string, const char *> m_vars;
  const Upgrade_info &m_server_info;
};

class Sys_var_allowed_values_check : public Upgrade_check {
 public:
  explicit Sys_var_allowed_values_check(const Upgrade_info &server_info);

  void add_sys_var(const Version &version, const std::string &name,
                   const std::vector<std::string> &defaults);
  bool has_sys_var(const std::string &name) const;
  bool enabled() const override;

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) override;

 private:
  std::map<std::string, std::vector<std::string>> m_sys_vars;
  const Upgrade_info &m_server_info;
};

class Sysvar_new_defaults : public Upgrade_check {
 public:
  using Sysvar_defaults = std::map<std::string, const char *>;
  explicit Sysvar_new_defaults(const Upgrade_info &server_info);

  void add_sys_var(Version version, const Sysvar_defaults &defaults);
  bool has_sys_var(const std::string &name) const;
  bool enabled() const override;

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) override;

 protected:
  const Upgrade_info &m_server_info;
  std::map<Version, Sysvar_defaults> m_version_defaults;
};

class Invalid_privileges_check : public Upgrade_check {
 public:
  explicit Invalid_privileges_check(const Upgrade_info &server_info);

  void add_privileges(Version version, const std::set<std::string> &privileges);

  bool has_privilege(const std::string &privilege);

  bool enabled() const override;

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override;

 private:
  const Upgrade_info &m_upgrade_info;
  std::map<Version, std::set<std::string>> m_privileges;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_H_