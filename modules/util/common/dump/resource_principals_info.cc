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

#include "modules/util/common/dump/resource_principals_info.h"

#include <utility>

namespace mysqlsh {
namespace dump {
namespace common {

namespace {

std::string required_string(const shcore::json::Object &o, const char *n) {
  return shcore::json::required(o, n, false);
}

}  // namespace

bool Resource_principals_info::empty() const noexcept {
  return bucket.empty() && namespace_.empty() && region.empty();
}

bool Resource_principals_info::valid() const noexcept {
  return !bucket.empty() && !namespace_.empty() && !region.empty();
}

void Resource_principals_info::set_prefix(std::string prefix) {
  m_prefix = std::move(prefix);

  if (!m_prefix.empty() && '/' != m_prefix.back()) {
    m_prefix += '/';
  }
}

void serialize(const Resource_principals_info &info,
               shcore::JSON_dumper *dumper) {
  dumper->start_object();

  dumper->append("bucket", info.bucket);
  dumper->append("namespace", info.namespace_);
  dumper->append("region", info.region);
  dumper->append("prefix", info.prefix());

  dumper->end_object();
}

Resource_principals_info resource_principals_info(
    const shcore::json::Object &info) {
  Resource_principals_info result;

  result.bucket = required_string(info, "bucket");
  result.namespace_ = required_string(info, "namespace");
  result.region = required_string(info, "region");
  result.set_prefix(
      shcore::json::optional(info, "prefix", true).value_or(std::string{}));

  return result;
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
