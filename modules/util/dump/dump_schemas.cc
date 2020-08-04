/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/dump/dump_schemas.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

Dump_schemas::Dump_schemas(const Dump_schemas_options &options)
    : Dumper(options), m_options(options) {}

std::unique_ptr<Schema_dumper> Dump_schemas::schema_dumper(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  auto dumper = Dumper::schema_dumper(session);

  const auto &options = m_options.compatibility_options();

  dumper->opt_force_innodb = options.is_set(Compatibility_option::FORCE_INNODB);
  dumper->opt_strip_definer =
      options.is_set(Compatibility_option::STRIP_DEFINERS);
  dumper->opt_strip_restricted_grants =
      options.is_set(Compatibility_option::STRIP_RESTRICTED_GRANTS);
  dumper->opt_strip_tablespaces =
      options.is_set(Compatibility_option::STRIP_TABLESPACES);

  return dumper;
}

}  // namespace dump
}  // namespace mysqlsh
