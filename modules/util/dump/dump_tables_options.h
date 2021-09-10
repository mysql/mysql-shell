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

#ifndef MODULES_UTIL_DUMP_DUMP_TABLES_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_TABLES_OPTIONS_H_

#include <string>
#include <vector>

#include "modules/util/dump/ddl_dumper_options.h"

namespace mysqlsh {
namespace dump {

class Dump_tables_options : public Ddl_dumper_options {
 public:
  Dump_tables_options() = default;

  Dump_tables_options(const Dump_tables_options &) = default;
  Dump_tables_options(Dump_tables_options &&) = default;

  Dump_tables_options &operator=(const Dump_tables_options &) = default;
  Dump_tables_options &operator=(Dump_tables_options &&) = default;

  virtual ~Dump_tables_options() = default;

  static const shcore::Option_pack_def<Dump_tables_options> &options();

  void set_schema(const std::string &schema);

  void set_tables(const std::vector<std::string> &tables);

  bool dump_events() const override { return false; }

  bool dump_routines() const override { return false; }

  bool dump_users() const override { return false; }

 private:
  void validate_options() const override;

  bool m_dump_all = false;
  bool m_has_tables = false;

  std::string m_schema;

  Instance_cache_builder::Filter m_tables;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_TABLES_OPTIONS_H_
