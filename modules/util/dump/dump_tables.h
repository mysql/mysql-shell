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

#ifndef MODULES_UTIL_DUMP_DUMP_TABLES_H_
#define MODULES_UTIL_DUMP_DUMP_TABLES_H_

#include "modules/util/dump/ddl_dumper.h"
#include "modules/util/dump/dump_tables_options.h"

namespace mysqlsh {
namespace dump {

class Dump_tables : public Ddl_dumper {
 public:
  Dump_tables() = delete;
  explicit Dump_tables(const Dump_tables_options &options)
      : Ddl_dumper(options) {}

  Dump_tables(const Dump_tables &) = delete;
  Dump_tables(Dump_tables &&) = delete;

  Dump_tables &operator=(const Dump_tables &) = delete;
  Dump_tables &operator=(Dump_tables &&) = delete;

  ~Dump_tables() override = default;

 private:
  const char *name() const override { return "dumpTables"; }

  void summary() const override {}

  void on_create_table_task(const std::string &, const std::string &,
                            const Instance_cache::Table *) override {}

  std::vector<std::string> object_stats(
      const Instance_cache::Stats &filtered,
      const Instance_cache::Stats &) const override {
    std::vector<std::string> stats;

    stats.emplace_back(std::to_string(filtered.tables) + " tables and " +
                       std::to_string(filtered.views) + " views");

    return stats;
  }
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_TABLES_H_
