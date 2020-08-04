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

#ifndef MODULES_UTIL_DUMP_DUMP_SCHEMAS_OPTIONS_H_
#define MODULES_UTIL_DUMP_DUMP_SCHEMAS_OPTIONS_H_

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "modules/util/dump/compatibility_option.h"
#include "modules/util/dump/ddl_dumper_options.h"

namespace mysqlsh {
namespace dump {

class Dump_schemas_options : public Ddl_dumper_options {
 public:
  Dump_schemas_options() = delete;
  Dump_schemas_options(const std::vector<std::string> &schemas,
                       const std::string &output_url);

  Dump_schemas_options(const Dump_schemas_options &) = default;
  Dump_schemas_options(Dump_schemas_options &&) = default;

  Dump_schemas_options &operator=(const Dump_schemas_options &) = default;
  Dump_schemas_options &operator=(Dump_schemas_options &&) = default;

  virtual ~Dump_schemas_options() = default;

  bool dump_events() const override { return m_dump_events; }

  bool dump_routines() const override { return m_dump_routines; }

  bool dump_users() const override { return false; }

  const Compatibility_options &compatibility_options() const {
    return m_compatibility_options;
  }

 protected:
  explicit Dump_schemas_options(const std::string &output_url);

  void unpack_options(shcore::Option_unpacker *unpacker) override;

  void validate_options() const override;

 private:
  mysqlshdk::oci::Oci_options::Unpack_target oci_target() const override {
    return mysqlshdk::oci::Oci_options::Unpack_target::OBJECT_STORAGE;
  }

  bool m_dump_events = true;
  bool m_dump_routines = true;
  Compatibility_options m_compatibility_options;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_SCHEMAS_OPTIONS_H_
