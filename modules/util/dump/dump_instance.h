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

#ifndef MODULES_UTIL_DUMP_DUMP_INSTANCE_H_
#define MODULES_UTIL_DUMP_DUMP_INSTANCE_H_

#include <string>
#include <unordered_set>

#include "modules/util/dump/dump_instance_options.h"
#include "modules/util/dump/dump_schemas.h"

namespace mysqlsh {
namespace dump {

class Dump_instance : public Dump_schemas {
 public:
  Dump_instance() = delete;
  explicit Dump_instance(const Dump_instance_options &options);

  Dump_instance(const Dump_instance &) = delete;
  Dump_instance(Dump_instance &&) = delete;

  Dump_instance &operator=(const Dump_instance &) = delete;
  Dump_instance &operator=(Dump_instance &&) = delete;

  virtual ~Dump_instance() = default;

 protected:
  bool dump_all_schemas() const override { return true; }

  const std::unordered_set<std::string> &excluded_schemas() const override;

 private:
  const char *name() const override { return "dumpInstance"; }

  bool dump_users() const override;

  const Dump_instance_options &m_options;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_INSTANCE_H_
