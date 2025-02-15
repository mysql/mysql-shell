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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CONDITION_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CONDITION_H_

#include <forward_list>
#include <memory>
#include <string>
#include <utility>

#include "modules/util/upgrade_checker/common.h"

#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

class Condition {
 public:
  virtual bool evaluate(const Upgrade_info &info) = 0;
  virtual ~Condition() = default;

  virtual std::string description() const = 0;
};

/**
 * This condition will be met if the source and target upgrade versions in the
 * Upgrade_info enclose any of the versions registered on this condition.
 *
 * This condition is meant to be used in checks with the following
 * characteristics:
 *
 * - Different items to be checked continue getting added in different versions
 * - The check is only meant to be executed if the src -> target cross any of
 *   the versions that introduced items to verify.
 */
class Version_condition : public Condition {
 public:
  Version_condition() = default;
  explicit Version_condition(std::forward_list<Version> versions);
  explicit Version_condition(Version version);
  ~Version_condition() override = default;

  bool evaluate(const Upgrade_info &info) override;
  void add_version(const Version &version);

  std::string description() const override;

 private:
  std::forward_list<Version> m_versions;
};

// Custom condition for one-off specific conditions
class Custom_condition : public Condition {
 public:
  using Callback = std::function<bool(const Upgrade_info &)>;

  explicit Custom_condition(Callback cb, std::string description)
      : m_condition{std::move(cb)}, m_description(std::move(description)) {}
  bool evaluate(const Upgrade_info &info) override { return m_condition(info); }

  std::string description() const override { return m_description; }

 private:
  Callback m_condition;
  std::string m_description;
};
/**
 * This condition can be used to track the different states of a feature and
 * activate or not a check depending on that.
 */
class Life_cycle_condition : public Condition {
 public:
  Life_cycle_condition(std::optional<Version> start,
                       std::optional<Version> deprecation,
                       std::optional<Version> removal);

  bool evaluate(const Upgrade_info &info) override;

  std::string description() const override;

  const std::optional<Version> &start_version() const {
    return m_start_version;
  }

  const std::optional<Version> &deprecation_version() const {
    return m_deprecation_version;
  }

  const std::optional<Version> &removal_version() const {
    return m_removal_version;
  }

 protected:
  Life_cycle_condition() = default;
  std::optional<Version> m_start_version;
  std::optional<Version> m_deprecation_version;
  std::optional<Version> m_removal_version;
};

class Aggregated_life_cycle_condition : public Life_cycle_condition {
 public:
  Aggregated_life_cycle_condition() = default;

  void add_condition(const Life_cycle_condition &condition);

 private:
  bool m_handle_start_version = true;
  bool m_handle_removal_version = true;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh
#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_CONDITION_H_
