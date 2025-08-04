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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_MANUAL_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECKER_MANUAL_CHECK_H_

#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlsh {
namespace upgrade_checker {
class Manual_check : public Upgrade_check {
 public:
  Manual_check(const std::string_view name, Category category,
               Upgrade_issue::Level level)
      : Upgrade_check(name, category), m_level(level) {}

  Upgrade_issue::Level get_level() const { return m_level; }
  bool is_runnable() const override { return false; }

  std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> & /*session*/,
      const Upgrade_info & /*server_info*/) override {
    throw std::runtime_error("Manual check not meant to be executed");
  }

 protected:
  Upgrade_issue::Level m_level;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_MANUAL_CHECK_H_