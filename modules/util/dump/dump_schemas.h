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

#ifndef MODULES_UTIL_DUMP_DUMP_SCHEMAS_H_
#define MODULES_UTIL_DUMP_DUMP_SCHEMAS_H_

#include "modules/util/dump/ddl_dumper.h"
#include "modules/util/dump/dump_schemas_options.h"

namespace mysqlsh {
namespace dump {

class Dump_schemas : public Ddl_dumper {
 public:
  Dump_schemas() = delete;
  explicit Dump_schemas(const Dump_schemas_options &options)
      : Ddl_dumper(options) {}

  Dump_schemas(const Dump_schemas &) = delete;
  Dump_schemas(Dump_schemas &&) = delete;

  Dump_schemas &operator=(const Dump_schemas &) = delete;
  Dump_schemas &operator=(Dump_schemas &&) = delete;

  ~Dump_schemas() override = default;

 protected:
  std::vector<std::string> object_stats(
      const Instance_cache::Stats &filtered,
      const Instance_cache::Stats &total) const override {
    std::vector<std::string> stats;

    stats.emplace_back(std::to_string(filtered.schemas) + " schemas");

#define FORMAT_OBJECT_STATS(type) \
  stats.emplace_back(format_object_stats(filtered.type, total.type, #type))

    FORMAT_OBJECT_STATS(tables);
    FORMAT_OBJECT_STATS(views);

#undef FORMAT_OBJECT_STATS

    return stats;
  }

 private:
  const char *name() const override { return "dumpSchemas"; }

  void summary() const override {}

  void on_create_table_task(const std::string &, const std::string &,
                            const Instance_cache::Table *) override {}
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_SCHEMAS_H_
