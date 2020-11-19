/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_EXPORT_TABLE_OPTIONS_H_
#define MODULES_UTIL_DUMP_EXPORT_TABLE_OPTIONS_H_

#include <memory>
#include <string>

#include "modules/util/dump/dump_options.h"

namespace mysqlsh {
namespace dump {

class Export_table_options : public Dump_options {
 public:
  Export_table_options() = delete;
  Export_table_options(const std::string &schema_table,
                       const std::string &output_url);

  Export_table_options(const Export_table_options &) = default;
  Export_table_options(Export_table_options &&) = default;

  Export_table_options &operator=(const Export_table_options &) = default;
  Export_table_options &operator=(Export_table_options &&) = default;

  virtual ~Export_table_options() = default;

  const std::string &schema() const { return m_schema; }

  const std::string &table() const { return m_table; }

  bool is_export_only() const override { return true; }

  bool use_single_file() const override { return true; }

  bool split() const override { return false; }

  uint64_t bytes_per_chunk() const override { return 0; }

  std::size_t threads() const override { return 1; }

  bool dump_ddl() const override { return false; }

  bool dump_data() const override { return true; }

  bool is_dry_run() const override { return false; }

  bool consistent_dump() const override { return false; }

  bool dump_events() const override { return false; }

  bool dump_routines() const override { return false; }

  bool dump_triggers() const override { return false; }

  bool dump_users() const override { return false; }

  bool use_timezone_utc() const override { return false; }

 private:
  void unpack_options(shcore::Option_unpacker *unpacker) override;

  void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session) override;

  void validate_options() const override;

  mysqlshdk::oci::Oci_options::Unpack_target oci_target() const override {
    return mysqlshdk::oci::Oci_options::Unpack_target::
        OBJECT_STORAGE_NO_PAR_SUPPORT;
  }

  void set_includes();

  std::string m_schema;
  std::string m_table;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_EXPORT_TABLE_OPTIONS_H_
