/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

class Condition;

enum class Category {
  ACCOUNTS,
  CONFIG,
  PARSING,
  SCHEMA,
};

class Upgrade_check {
 public:
  explicit Upgrade_check(const std::string_view name, Category category)
      : m_name(name), m_category(category) {}
  virtual ~Upgrade_check() {}

  const std::string &get_name() const { return m_name; }
  const std::string &get_category() const { return to_string(m_category); }
  virtual const std::string &get_title() const;
  virtual std::string get_description(
      const std::string &group = "",
      const Token_definitions &tokens = {}) const;
  virtual const std::string &get_doc_link(const std::string &group = "") const;
  virtual bool is_runnable() const { return true; }
  virtual bool is_multi_lvl_check() const { return false; }
  std::vector<std::string> get_solutions(const std::string &group = "") const;

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

  void set_condition(Condition *condition) { m_condition = condition; }
  Condition *get_condition() const noexcept { return m_condition; }

  void set_groups(std::vector<std::string> groups) {
    m_groups = std::move(groups);
  }
  const std::vector<std::string> &groups() const { return m_groups; }
  Upgrade_issue create_issue() const;

  virtual bool is_custom_session_required() const { return false; }

 protected:
  virtual Token_definitions base_tokens() const { return {}; }

 private:
  const std::string &to_string(Category category) const;

  std::string m_name;
  Category m_category;
  Condition *m_condition = nullptr;
  std::vector<std::string> m_groups;
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