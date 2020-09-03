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

#include "modules/util/dump/compatibility_option.h"

#include <stdexcept>

namespace mysqlsh {
namespace dump {

namespace {
constexpr auto k_force_innodb = "force_innodb";
constexpr auto k_strip_definers = "strip_definers";
constexpr auto k_strip_restricted_grants = "strip_restricted_grants";
constexpr auto k_strip_tablespaces = "strip_tablespaces";
}  // namespace

Compatibility_option to_compatibility_option(const std::string &c) {
  if (c == k_force_innodb) return Compatibility_option::FORCE_INNODB;
  if (c == k_strip_definers) return Compatibility_option::STRIP_DEFINERS;
  if (c == k_strip_restricted_grants)
    return Compatibility_option::STRIP_RESTRICTED_GRANTS;
  if (c == k_strip_tablespaces) return Compatibility_option::STRIP_TABLESPACES;

  throw std::invalid_argument("Unknown compatibility option: " + c);
}

Compatibility_option to_compatibility_option(Schema_dumper::Issue::Status s) {
  if (s == Schema_dumper::Issue::Status::USE_FORCE_INNODB)
    return Compatibility_option::FORCE_INNODB;
  else if (s == Schema_dumper::Issue::Status::USE_STRIP_DEFINERS)
    return Compatibility_option::STRIP_DEFINERS;
  else if (s == Schema_dumper::Issue::Status::USE_STRIP_RESTRICTED_GRANTS)
    return Compatibility_option::STRIP_RESTRICTED_GRANTS;
  else if (s == Schema_dumper::Issue::Status::USE_STRIP_TABLESPACES)
    return Compatibility_option::STRIP_TABLESPACES;

  throw std::logic_error(
      "This status cannot be converted to Compatibility_option");
}

std::string to_string(Compatibility_option c) {
  switch (c) {
    case Compatibility_option::FORCE_INNODB:
      return k_force_innodb;

    case Compatibility_option::STRIP_DEFINERS:
      return k_strip_definers;

    case Compatibility_option::STRIP_RESTRICTED_GRANTS:
      return k_strip_restricted_grants;

    case Compatibility_option::STRIP_TABLESPACES:
      return k_strip_tablespaces;
  }

  throw std::logic_error("Shouldn't happen, but compiler complains");
}

}  // namespace dump
}  // namespace mysqlsh
