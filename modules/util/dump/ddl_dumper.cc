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

#include "modules/util/dump/ddl_dumper.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

Ddl_dumper::Ddl_dumper(const Ddl_dumper_options &options)
    : Dumper(options), m_options(options) {}

bool Ddl_dumper::should_dump_ddl() const { return !m_options.dump_data_only(); }

bool Ddl_dumper::should_dump_data() const { return !m_options.dump_ddl_only(); }

bool Ddl_dumper::is_dry_run() const { return m_options.dry_run(); }

bool Ddl_dumper::consistent_dump() const { return m_options.consistent_dump(); }

bool Ddl_dumper::dump_events() const { return m_options.dump_events(); }

bool Ddl_dumper::dump_routines() const { return m_options.dump_routines(); }

bool Ddl_dumper::dump_triggers() const { return m_options.dump_triggers(); }

bool Ddl_dumper::dump_users() const { return m_options.dump_users(); }

const mysqlshdk::utils::nullable<mysqlshdk::utils::Version>
    &Ddl_dumper::mds_compatibility() const {
  return m_options.mds_compatibility();
}

bool Ddl_dumper::use_timezone_utc() const { return m_options.timezone_utc(); }

std::unique_ptr<Schema_dumper> Ddl_dumper::schema_dumper(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) const {
  auto dumper = Dumper::schema_dumper(session);

  dumper->opt_column_statistics = false;

  const auto &options = m_options.compatibility_options();

  dumper->opt_force_innodb = options.is_set(Compatibility_option::FORCE_INNODB);
  dumper->opt_strip_definer =
      options.is_set(Compatibility_option::STRIP_DEFINERS);
  dumper->opt_strip_restricted_grants =
      options.is_set(Compatibility_option::STRIP_RESTRICTED_GRANTS);
  dumper->opt_strip_role_admin =
      options.is_set(Compatibility_option::STRIP_ROLE_ADMIN);
  dumper->opt_strip_tablespaces =
      options.is_set(Compatibility_option::STRIP_TABLESPACES);

  return dumper;
}

}  // namespace dump
}  // namespace mysqlsh
