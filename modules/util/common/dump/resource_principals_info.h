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

#ifndef MODULES_UTIL_COMMON_DUMP_RESOURCE_PRINCIPALS_INFO_H_
#define MODULES_UTIL_COMMON_DUMP_RESOURCE_PRINCIPALS_INFO_H_

#include <string>

#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlsh {
namespace dump {
namespace common {

struct Resource_principals_info {
  std::string bucket;
  std::string namespace_;
  std::string region;

  bool empty() const noexcept;

  bool valid() const noexcept;

  void set_prefix(std::string prefix);

  inline const std::string &prefix() const noexcept { return m_prefix; }

 private:
  std::string m_prefix;
};

void serialize(const Resource_principals_info &info,
               shcore::JSON_dumper *dumper);

Resource_principals_info resource_principals_info(
    const shcore::json::Object &info);

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_RESOURCE_PRINCIPALS_INFO_H_
