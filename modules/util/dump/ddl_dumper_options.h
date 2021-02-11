/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

namespace mysqlsh {
namespace dump {

class Ddl_dumper_options : public Dump_options {
 public:
  Ddl_dumper_options(const Ddl_dumper_options &) = default;
  Ddl_dumper_options(Ddl_dumper_options &&) = default;

  Ddl_dumper_options &operator=(const Ddl_dumper_options &) = default;
  Ddl_dumper_options &operator=(Ddl_dumper_options &&) = default;

  virtual ~Ddl_dumper_options() = default;

  static const shcore::Option_pack_def<Ddl_dumper_options> &options();

  bool split() const override { return m_split; }

  uint64_t bytes_per_chunk() const override { return m_bytes_per_chunk; }

  std::size_t threads() const override { return m_threads; }

  bool is_export_only() const override { return false; }

  bool use_single_file() const override { return false; }

  bool dump_ddl() const override { return !m_data_only; }

  bool dump_data() const override { return !m_ddl_only; }

  bool is_dry_run() const override { return m_dry_run; }

  bool consistent_dump() const override { return m_consistent_dump; }

  bool dump_triggers() const override { return m_dump_triggers; }

  bool use_timezone_utc() const override { return m_timezone_utc; }

  const Compatibility_options &compatibility_options() const {
    return m_compatibility_options;
  }

  bool dump_binlog_info() const override { return true; }

 protected:
  Ddl_dumper_options();

  void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &) override {}

  void on_unpacked_options();

 private:
  void set_bytes_per_chunk(const std::string &value);
  void set_ocimds(bool value);
  void set_compatibility_options(const std::vector<std::string> &options);
  mysqlshdk::oci::Oci_option_unpacker<
      mysqlshdk::oci::Oci_options::Unpack_target::OBJECT_STORAGE>
      m_oci_option_unpacker;

  bool m_split = true;
  uint64_t m_bytes_per_chunk;
  uint64_t m_threads = 4;

  bool m_dump_triggers = true;
  bool m_timezone_utc = true;
  bool m_ddl_only = false;
  bool m_data_only = false;
  bool m_dry_run = false;
  bool m_consistent_dump = true;

  Compatibility_options m_compatibility_options;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DDL_DUMPER_OPTIONS_H_
