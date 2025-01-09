/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_UTIL_DUMP_DUMP_SCHEMAS_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_SCHEMAS_OPTIONS_H_

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "modules/util/dump/ddl_dumper_options.h"

namespace mysqlsh {
namespace dump {

class Dump_schemas_options : public Ddl_dumper_options {
 public:
  Dump_schemas_options();

  Dump_schemas_options(const Dump_schemas_options &) = default;
  Dump_schemas_options(Dump_schemas_options &&) = default;

  Dump_schemas_options &operator=(const Dump_schemas_options &) = default;
  Dump_schemas_options &operator=(Dump_schemas_options &&) = default;

  ~Dump_schemas_options() override = default;

  static const shcore::Option_pack_def<Dump_schemas_options> &options();

  bool dump_events() const override { return m_dump_events; }

  bool dump_routines() const override { return m_dump_routines; }

  bool dump_libraries() const override { return m_dump_libraries; }

  bool dump_users() const override { return false; }

  void set_schemas(const std::vector<std::string> &schemas);

 protected:
  explicit Dump_schemas_options(const char *name);

  void on_validate() const override;

 private:
  void on_unpacked_options();

  bool m_dump_events = true;
  bool m_dump_routines = true;
  bool m_dump_libraries = true;

  std::unordered_set<std::string> m_schemas;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_SCHEMAS_OPTIONS_H_
