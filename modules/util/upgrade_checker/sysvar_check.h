/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_UTIL_UPGRADE_CHECKER_SYSVAR_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECKER_SYSVAR_CHECK_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check.h"

namespace mysqlsh {
namespace upgrade_checker {

class Sysvar_platform_value {
 public:
  std::optional<std::string> get_default(size_t bits) const;
  void set_default(std::string value, size_t bits = 0);

  std::optional<std::vector<std::string>> get_allowed(size_t bits) const;
  void set_allowed(std::vector<std::string> allowed, size_t bits = 0);

  std::optional<std::vector<std::string>> get_forbidden(size_t bits) const;
  void set_forbidden(std::vector<std::string> forbidden, size_t bits = 0);

 private:
  std::map<size_t, std::string> m_defaults;
  std::map<size_t, std::vector<std::string>> m_allowed;
  std::map<size_t, std::vector<std::string>> m_forbidden;
};
class Sysvar_configuration {
 public:
  void set_default(std::string value, size_t bits = 0, char platform = 'A');
  std::optional<std::string> get_default(size_t bits, char platform) const;

  void set_allowed(std::vector<std::string> allowed, size_t bits = 0,
                   char platform = 'A');
  std::optional<std::vector<std::string>> get_allowed(size_t bits,
                                                      char platform) const;

  void set_forbidden(std::vector<std::string> forbidden, size_t bits = 0,
                     char platform = 'A');
  std::optional<std::vector<std::string>> get_forbidden(size_t bits,
                                                        char platform) const;

 private:
  std::map<char, Sysvar_platform_value> m_platform_values;
};

class Sysvar_version_configuration : public Sysvar_configuration {
 public:
  Sysvar_version_configuration(Version version) {
    m_version = std::move(version);
  }

  const Version &get_version() const { return m_version; }

 private:
  Version m_version;
};

struct Sysvar_version_check;

struct Sysvar_definition {
  std::string name;
  std::optional<Version> introduction_version;
  std::optional<Version> deprecation_version;
  std::optional<Version> removal_version;
  Sysvar_configuration initials;
  std::map<Version, Sysvar_configuration> changes;
  std::optional<std::string> replacement;

  void set_default(std::string value,
                   const std::optional<Version> &version = {}, size_t bits = 0,
                   char platform = 'A');
  void set_allowed(std::vector<std::string> allowed,
                   const std::optional<Version> &version = {}, size_t bits = 0,
                   char platform = 'A');

  void set_forbidden(std::vector<std::string> set_forbidden,
                     const std::optional<Version> &version = {},
                     size_t bits = 0, char platform = 'A');

  std::optional<Sysvar_version_check> get_check(
      const Upgrade_info &upgrade_info) const;
};

/**
 * This struct holds all the data needed to perform the different
 * checks given a speficic source and target version.
 */
struct Sysvar_version_check {
  std::string name;
  std::string initial_defaults;
  std::string updated_defaults;
  std::vector<std::string> allowed_values;
  std::vector<std::string> forbidden_values;
  std::optional<std::string> replacement;
  std::optional<Version> removal_version;
  std::optional<Version> deprecation_version;
};

class Sysvar_registry {
 public:
  void register_sysvar(Sysvar_definition &&def);

  std::vector<Sysvar_version_check> get_checks(
      const Upgrade_info &upgrade_info);

  void load_configuration();

  const Sysvar_definition &get_sysvar(const std::string &name) const;

 private:
  std::map<std::string, Sysvar_definition> m_registry;
};

class Sysvar_check : public Upgrade_check {
 public:
  Sysvar_check(const Upgrade_info &info);

  bool enabled() const override { return !m_checks.empty(); }

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info, Checker_cache *cache) override;

  const std::vector<Sysvar_version_check> &get_checks() const {
    return m_checks;
  }

 private:
  bool has_invalid_allowed_values(
      const Checker_cache::Sysvar_info *sysvar,
      const Sysvar_version_check &sysvar_check) const;
  bool has_invalid_forbidden_values(
      const Checker_cache::Sysvar_info *sysvar,
      const Sysvar_version_check &sysvar_check) const;

  Upgrade_issue get_issue(const std::string &group,
                          const Checker_cache::Sysvar_info *sysvar,
                          const Sysvar_version_check &sysvar_check);

  static Sysvar_registry s_registry;

  std::vector<Sysvar_version_check> m_checks;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // !MODULES_UTIL_UPGRADE_CHECKER_SYSVAR_CHECK_H_
