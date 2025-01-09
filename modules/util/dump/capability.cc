/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/capability.h"

#include <stdexcept>
#include <string>

namespace mysqlsh {
namespace dump {
namespace capability {

namespace {

using mysqlshdk::utils::Version;

constexpr auto k_partition_awareness_capability = "partition_awareness";
constexpr auto k_multifile_schema_ddl_capability = "multifile_schema_ddl";

}  // namespace

std::string id(Capability capability) {
  switch (capability) {
    case Capability::PARTITION_AWARENESS:
      return k_partition_awareness_capability;

    case Capability::MULTIFILE_SCHEMA_DDL:
      return k_multifile_schema_ddl_capability;
  }

  throw std::logic_error("Should not happen");
}

std::string description(Capability capability) {
  switch (capability) {
    case Capability::PARTITION_AWARENESS:
      return "Partition awareness - dumper treats each partition as a separate "
             "table, improving both dump and load times.";

    case Capability::MULTIFILE_SCHEMA_DDL:
      return "Multifile schema DDL - schema DDL is split into multiple files, "
             "containing DDL for schema itself, events, routines and "
             "libraries.";
  }

  throw std::logic_error("Should not happen");
}

Version version_required(Capability capability) {
  // this is Shell's version

  switch (capability) {
    case Capability::PARTITION_AWARENESS:
      return Version(8, 0, 27);

    case Capability::MULTIFILE_SCHEMA_DDL:
      return Version(9, 3, 0);
  }

  throw std::logic_error("Should not happen");
}

std::optional<Capability> to_capability(const std::string &id) {
  if (k_partition_awareness_capability == id) {
    return Capability::PARTITION_AWARENESS;
  } else if (k_multifile_schema_ddl_capability == id) {
    return Capability::MULTIFILE_SCHEMA_DDL;
  } else {
    return std::nullopt;
  }
}

}  // namespace capability
}  // namespace dump
}  // namespace mysqlsh
