/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_OPTION_TRACKER_H_
#define MYSQLSHDK_LIBS_UTILS_OPTION_TRACKER_H_

#include <string>

namespace shcore {
namespace option_tracker {

enum class Shell_feature {
  NONE = 0,
  SHELL_USE = 1,
  UTIL_DUMP = 2,
  UTIL_LOAD_DUMP = 3,
  UTIL_COPY = 4,
  UTIL_UPGRADE_CHECK = 5,
};

std::string get_option_tracker_feature_id(
    Shell_feature option = Shell_feature::SHELL_USE);
std::string get_option_tracker_feature_id(const std::string &custom_option);

}  // namespace option_tracker
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_OPTION_TRACKER_H_