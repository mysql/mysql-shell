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

#ifndef MODULES_UTIL_DUMP_DDL_DUMPER_H_
#define MODULES_UTIL_DUMP_DDL_DUMPER_H_

#include <memory>

#include "modules/util/dump/ddl_dumper_options.h"
#include "modules/util/dump/dumper.h"

namespace mysqlsh {
namespace dump {

class Ddl_dumper : public Dumper {
 public:
  Ddl_dumper() = delete;
  explicit Ddl_dumper(const Ddl_dumper_options &options);

  Ddl_dumper(const Ddl_dumper &) = delete;
  Ddl_dumper(Ddl_dumper &&) = delete;

  Ddl_dumper &operator=(const Ddl_dumper &) = delete;
  Ddl_dumper &operator=(Ddl_dumper &&) = delete;

  virtual ~Ddl_dumper() = default;

 protected:
  std::unique_ptr<Schema_dumper> schema_dumper(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) const override;

 private:
  bool is_export_only() const override { return false; }

  bool should_dump_ddl() const override;

  bool should_dump_data() const override;

  bool is_dry_run() const override;

  bool consistent_dump() const override;

  bool dump_events() const override;

  bool dump_routines() const override;

  bool dump_triggers() const override;

  bool dump_users() const override;

  const mysqlshdk::utils::nullable<mysqlshdk::utils::Version>
      &mds_compatibility() const override;

  bool use_timezone_utc() const override;

  const Ddl_dumper_options &m_options;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DDL_DUMPER_H_
