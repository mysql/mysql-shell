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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_UTIL_UPGRADE_CHECKER_FEATURE_LIFE_CYCLE_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECKER_FEATURE_LIFE_CYCLE_CHECK_H_

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check.h"
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

struct Feature_entry {
  Feature_definition feature;
  bool enabled;
};

enum class Grouping {
  NONE = 0,     // One Upgrade Issue per item
  FEATURE = 1,  // One Upgrade Issue per feature
  ALL = 2       // One Upgrade Issue per check
};

/**
 * Base check to for the usage of features that have been deprecated and/or
 * removed.
 *
 * The level of the issues generated with this check depend on the feature state
 * on the target server:
 *
 * - NOTICE: When the feature is not yet deprecated in the target server.
 * - WARNING: When the Feature is deprecated in the target server.
 * - ERROR: When the Feature is removed in the target server.
 *
 * This check supports handling of several features with different
 * start/deprecated/removal versions, allowing to group the generated issues:
 *
 * - Single issue enclosing all.
 * - A separate issue per registered feature.
 * - Using a separate issue for each found problem.
 */

class Feature_life_cycle_check : public Upgrade_check {
 public:
  explicit Feature_life_cycle_check(std::string_view name,
                                    Grouping grouping = Grouping::NONE);
  ~Feature_life_cycle_check() override = default;

  virtual std::string build_query(const Upgrade_info &server_info) = 0;

  bool enabled() const override;

  const std::string &get_description() const override;

  void add_feature(const Feature_definition &feature,
                   const Upgrade_info &server_info);

  bool has_feature(const std::string &feature_id) const;
  const Feature_definition &get_feature(const std::string &feature_id) const;

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) override;

  std::vector<const Feature_definition *> get_features(
      bool only_enabled = false) const;

 protected:
  void add_issue(const std::string &feature, const std::string &item,
                 Upgrade_issue::Object_type object_type);

  std::map<std::string, Feature_entry> m_features;
  std::map<std::string,
           std::vector<std::pair<std::string, Upgrade_issue::Object_type>>>
      m_feature_issues;

 private:
  virtual void process_row(const mysqlshdk::db::IRow *row) = 0;

  virtual std::string resolve_check_description() const;
  virtual std::string resolve_feature_description(
      const Feature_definition &feature, Upgrade_issue::Level level,
      const std::string &item = "") const;

  Grouping m_grouping;

  // Feature checks contains a complex condition to restrictive about when to
  // executed the check as well as accurate about what the output should be, for
  // the purpose of listing the checks, what really matters is when the check is
  // activated, so we store that here
  Version_condition m_list_condition;
};

class Auth_method_usage_check : public Feature_life_cycle_check {
 public:
  Auth_method_usage_check(const Upgrade_info &server_info);

  std::string build_query(const Upgrade_info &server_info) override;

 private:
  void process_row(const mysqlshdk::db::IRow *row) override;
};

class Plugin_usage_check : public Feature_life_cycle_check {
 public:
  Plugin_usage_check(const Upgrade_info &server_info);

  std::string build_query(const Upgrade_info &server_info) override;

 private:
  void process_row(const mysqlshdk::db::IRow *row) override;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_FEATURE_LIFE_CYCLE_CHECK_H_