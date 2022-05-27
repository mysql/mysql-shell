/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECK_FORMATTER_H_
#define MODULES_UTIL_UPGRADE_CHECK_FORMATTER_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/util/upgrade_check.h"

namespace mysqlsh {

class Upgrade_check_output_formatter {
 public:
  static std::unique_ptr<Upgrade_check_output_formatter> get_formatter(
      const std::string &format);

  virtual ~Upgrade_check_output_formatter() = default;

  virtual void check_info(const std::string &server_addres,
                          const std::string &server_version,
                          const std::string &target_version) = 0;
  virtual void check_results(const Upgrade_check &check,
                             const std::vector<Upgrade_issue> &results) = 0;
  virtual void check_error(const Upgrade_check &check, const char *description,
                           bool runtime_error = true) = 0;
  virtual void manual_check(const Upgrade_check &check) = 0;
  virtual void summarize(int error, int warning, int notice,
                         const std::string &text) = 0;
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECK_FORMATTER_H_
