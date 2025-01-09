/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_CAPABILITY_H_
#define MODULES_UTIL_DUMP_CAPABILITY_H_

#include <optional>
#include <string>

#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dump {

enum class Capability {
  PARTITION_AWARENESS,
  MULTIFILE_SCHEMA_DDL,
  LAST_VALUE = MULTIFILE_SCHEMA_DDL,
};

using Capability_set =
    mysqlshdk::utils::Enum_set<Capability, Capability::LAST_VALUE>;

namespace capability {

std::string id(Capability capability);

std::string description(Capability capability);

mysqlshdk::utils::Version version_required(Capability capability);

std::optional<Capability> to_capability(const std::string &id);

}  // namespace capability
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_CAPABILITY_H_
