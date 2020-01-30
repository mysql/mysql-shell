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

#ifndef MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_
#define MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_

#include <string>

#include "modules/util/dump/compatibility_option.h"
#include "modules/util/dump/dump_options.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dump {

class Ddl_dumper_options : public Dump_options {
 public:
  Ddl_dumper_options() = delete;

  Ddl_dumper_options(const Ddl_dumper_options &) = default;
  Ddl_dumper_options(Ddl_dumper_options &&) = default;

  Ddl_dumper_options &operator=(const Ddl_dumper_options &) = default;
  Ddl_dumper_options &operator=(Ddl_dumper_options &&) = default;

  virtual ~Ddl_dumper_options() = default;

  bool dump_events() const { return m_dump_events; }

  bool dump_routines() const { return m_dump_routines; }

  bool dump_triggers() const { return m_dump_triggers; }

  bool timezone_utc() const { return m_timezone_utc; }

  bool dump_ddl_only() const { return m_ddl_only; }

  bool dump_data_only() const { return m_data_only; }

  bool dry_run() const { return m_dry_run; }

  bool consistent_dump() const { return m_consistent_dump; }

  bool dump_users() const { return m_dump_users; }

  // MySQL Database Service
  const mysqlshdk::utils::nullable<mysqlshdk::utils::Version>
      &mds_compatibility() const {
    return m_mds;
  }

  const Compatibility_options &compatibility_options() const {
    return m_compatibility_options;
  }

 protected:
  Ddl_dumper_options(const std::string &output_dir, bool users);

  void unpack_options(shcore::Option_unpacker *unpacker) override;

  void validate_options() const override;

 private:
  bool m_dump_events = true;
  bool m_dump_routines = true;
  bool m_dump_triggers = true;
  bool m_timezone_utc = true;
  bool m_ddl_only = false;
  bool m_data_only = false;
  bool m_dry_run = false;
  bool m_consistent_dump = true;
  bool m_dump_users;
  mysqlshdk::utils::nullable<mysqlshdk::utils::Version> m_mds;
  Compatibility_options m_compatibility_options;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_
