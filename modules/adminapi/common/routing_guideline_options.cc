/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/routing_guideline_options.h"
#include "modules/adminapi/common/common_cmd_options.h"

namespace {
inline constexpr shcore::Option_data<bool> kOptionEnabled{"enabled"};
inline constexpr shcore::Option_data<uint64_t> kOptionConnectionSharingAllowed{
    "connectionSharingAllowed"};
inline constexpr shcore::Option_data<uint64_t> kOptionOrder{"order"};
inline constexpr shcore::Option_data<std::string> kOptionRouter{"router"};
}  // namespace

namespace mysqlsh::dba {

const shcore::Option_pack_def<Add_destination_options>
    &Add_destination_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Add_destination_options> b;

    b.optional(kOptionDryRun, &Add_destination_options::dry_run);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Add_route_options> &Add_route_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Add_route_options> b;

    b.optional(kOptionDryRun, &Add_route_options::dry_run);
    b.optional(kOptionEnabled, &Add_route_options::enabled);
    b.optional(kOptionConnectionSharingAllowed,
               &Add_route_options::connection_sharing_allowed);
    b.optional(kOptionOrder, &Add_route_options::order);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Show_options> &Show_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Show_options> b;

    b.optional(kOptionRouter, &Show_options::router);

    return b.build();
  });

  return opts;
}

}  // namespace mysqlsh::dba
