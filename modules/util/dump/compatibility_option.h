/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_COMPATIBILITY_OPTION_H_
#define MODULES_UTIL_DUMP_COMPATIBILITY_OPTION_H_

#include <string>

#include "mysqlshdk/libs/utils/enumset.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

enum class Compatibility_option {
  CREATE_INVISIBLE_PKS,
  FORCE_INNODB,
  IGNORE_MISSING_PKS,
  STRIP_DEFINERS,
  STRIP_RESTRICTED_GRANTS,
  STRIP_TABLESPACES,
  SKIP_INVALID_ACCOUNTS,
  STRIP_INVALID_GRANTS,
  IGNORE_WILDCARD_GRANTS,
};

Compatibility_option to_compatibility_option(const std::string &c);

Compatibility_option to_compatibility_option(Schema_dumper::Issue::Status c);

std::string to_string(Compatibility_option c);

using Compatibility_options =
    mysqlshdk::utils::Enum_set<Compatibility_option,
                               Compatibility_option::IGNORE_WILDCARD_GRANTS>;

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_COMPATIBILITY_OPTION_H_
