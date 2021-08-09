/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/capability.h"

#include <stdexcept>

namespace mysqlsh {
namespace dump {
namespace capability {

namespace {

using mysqlshdk::utils::Version;

const std::string k_partition_awareness_capability = "partition_awareness";

}  // namespace

std::string id(Capability capability) {
  switch (capability) {
    case Capability::PARTITION_AWARENESS:
      return k_partition_awareness_capability;
  }

  throw std::logic_error("Should not happen");
}

std::string description(Capability capability) {
  switch (capability) {
    case Capability::PARTITION_AWARENESS:
      return "Partition awareness - dumper treats each partition as a separate "
             "table, improving both dump and load times.";
  }

  throw std::logic_error("Should not happen");
}

Version version_required(Capability capability) {
  switch (capability) {
    case Capability::PARTITION_AWARENESS:
      return Version(8, 0, 27);
  }

  throw std::logic_error("Should not happen");
}

bool is_supported(const std::string &id) {
  if (k_partition_awareness_capability == id) {
    return true;
  } else {
    return false;
  }
}

}  // namespace capability
}  // namespace dump
}  // namespace mysqlsh
