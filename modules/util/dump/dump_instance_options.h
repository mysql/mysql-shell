/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DUMP_INSTANCE_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_INSTANCE_OPTIONS_H_

#include <memory>
#include <string>

#include "modules/util/dump/dump_schemas_options.h"

namespace mysqlsh {
namespace dump {

class Dump_instance_options : public Dump_schemas_options {
 public:
  Dump_instance_options();

  Dump_instance_options(const Dump_instance_options &) = default;
  Dump_instance_options(Dump_instance_options &&) = default;

  Dump_instance_options &operator=(const Dump_instance_options &) = default;
  Dump_instance_options &operator=(Dump_instance_options &&) = default;

  virtual ~Dump_instance_options() = default;

  static const shcore::Option_pack_def<Dump_instance_options> &options();

  bool dump_users() const override { return m_dump_users; }

 private:
  void on_unpacked_options();

  void validate_options() const override;

  bool m_dump_users = true;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_INSTANCE_OPTIONS_H_
