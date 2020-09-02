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

#ifndef MODULES_UTIL_DUMP_EXPORT_TABLE_H_
#define MODULES_UTIL_DUMP_EXPORT_TABLE_H_

#include "modules/util/dump/dumper.h"
#include "modules/util/dump/export_table_options.h"

namespace mysqlsh {
namespace dump {

class Export_table : public Dumper {
 public:
  Export_table() = delete;
  explicit Export_table(const Export_table_options &options);

  Export_table(const Export_table &) = delete;
  Export_table(Export_table &&) = delete;

  Export_table &operator=(const Export_table &) = delete;
  Export_table &operator=(Export_table &&) = delete;

  virtual ~Export_table() = default;

 private:
  void create_schema_tasks() override;

  const char *name() const override { return "exportTable"; }

  void summary() const override;

  void on_create_table_task(const Table_task &task) override;

  const Export_table_options &m_options;

  Table_task m_task;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_EXPORT_TABLE_H_