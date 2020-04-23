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

#include "modules/util/dump/ddl_dumper_options.h"

#include <vector>

namespace mysqlsh {
namespace dump {

Ddl_dumper_options::Ddl_dumper_options(const std::string &output_dir,
                                       bool users)
    : Dump_options(output_dir), m_dump_users(users) {}

void Ddl_dumper_options::unpack_options(shcore::Option_unpacker *unpacker) {
  std::vector<std::string> compatibility_options;
  bool mds = false;

  unpacker->optional("events", &m_dump_events)
      .optional("routines", &m_dump_routines)
      .optional("triggers", &m_dump_triggers)
      .optional("tzUtc", &m_timezone_utc)
      .optional("ddlOnly", &m_ddl_only)
      .optional("dataOnly", &m_data_only)
      .optional("dryRun", &m_dry_run)
      .optional("consistent", &m_consistent_dump)
      .optional("users", &m_dump_users)
      .optional("ocimds", &mds)
      .optional("compatibility", &compatibility_options);

  if (mds) {
    m_mds = mysqlshdk::utils::Version(MYSH_VERSION);
  }

  for (const auto &option : compatibility_options) {
    m_compatibility_options |= to_compatibility_option(option);
  }
}

void Ddl_dumper_options::validate_options() const {
  if (dump_ddl_only() && dump_data_only()) {
    throw std::invalid_argument(
        "The 'ddlOnly' and 'dataOnly' options cannot be both set to true.");
  }
}

}  // namespace dump
}  // namespace mysqlsh
