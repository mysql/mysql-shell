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

#ifndef MODULES_UTIL_DUMP_DUMP_TABLES_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_TABLES_OPTIONS_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "modules/util/dump/ddl_dumper_options.h"

namespace mysqlsh {
namespace dump {

class Dump_tables_options : public Ddl_dumper_options {
 public:
  Dump_tables_options() = delete;
  Dump_tables_options(const std::string &schema,
                      const std::vector<std::string> &tables,
                      const std::string &output_url);

  Dump_tables_options(const Dump_tables_options &) = default;
  Dump_tables_options(Dump_tables_options &&) = default;

  Dump_tables_options &operator=(const Dump_tables_options &) = default;
  Dump_tables_options &operator=(Dump_tables_options &&) = default;

  virtual ~Dump_tables_options() = default;

  const std::string &schema() const { return m_schema; }

  const std::unordered_set<std::string> &tables() const { return m_tables; }

  bool dump_all() const { return m_dump_all; }

  bool table_only() const override { return true; }

  bool dump_events() const override { return false; }

  bool dump_routines() const override { return false; }

  bool dump_users() const override { return false; }

 private:
  void unpack_options(shcore::Option_unpacker *unpacker) override;

  void validate_options() const override;

  std::string m_schema;
  std::unordered_set<std::string> m_tables;
  bool m_dump_all = false;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_TABLES_OPTIONS_H_
