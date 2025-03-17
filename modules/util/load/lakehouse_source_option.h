/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_LOAD_LAKEHOUSE_SOURCE_OPTION_H_
#define MODULES_UTIL_LOAD_LAKEHOUSE_SOURCE_OPTION_H_

#include <string>

#include "mysqlshdk/include/scripting/types/option_pack.h"

#include "modules/util/common/dump/resource_principals_info.h"

namespace mysqlsh {
namespace load {

class Lakehouse_source_option final {
 public:
  using Resource_principals = dump::common::Resource_principals_info;

  Lakehouse_source_option() = default;

  Lakehouse_source_option(const Lakehouse_source_option &) = default;
  Lakehouse_source_option(Lakehouse_source_option &&) = default;

  Lakehouse_source_option &operator=(const Lakehouse_source_option &) = default;
  Lakehouse_source_option &operator=(Lakehouse_source_option &&) = default;

  ~Lakehouse_source_option() = default;

  static const shcore::Option_pack_def<Lakehouse_source_option> &options();

  static constexpr auto option_name() noexcept { return "lakehouseSource"; }

  constexpr inline const std::string &par() const noexcept { return m_par; }

  constexpr inline const Resource_principals &resource_principals()
      const noexcept {
    return m_resource_principals;
  }

 private:
  static constexpr auto par_option() noexcept { return "par"; }

  static constexpr auto bucket_option() noexcept { return "bucket"; }

  static constexpr auto namespace_option() noexcept { return "namespace"; }

  static constexpr auto region_option() noexcept { return "region"; }

  static constexpr auto prefix_option() noexcept { return "prefix"; }

  void on_unpacked_options();

  std::string m_par;
  Resource_principals m_resource_principals;
};

}  // namespace load
}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_LAKEHOUSE_SOURCE_OPTION_H_
