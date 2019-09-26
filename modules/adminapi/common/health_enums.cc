/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/health_enums.h"
#include <stdexcept>

namespace mysqlsh {
namespace dba {
#if 0
std::string to_string(Instance_status status) {
  switch (status) {
    case Instance_status::UNREACHABLE:
      return "UNREACHABLE";
    case Instance_status::MISSING:
      return "MISSING";
    case Instance_status::INVALID:
      return "INVALID";
    case Instance_status::OUT_OF_QUORUM:
      return "OUT_OF_QUORUM";
    case Instance_status::RECOVERING:
      return "RECOVERING";
    case Instance_status::OK:
      return "OK";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Group_status status) {
  switch (status) {
    case Group_status::UNREACHABLE:
      return "UNREACHABLE";
    case Group_status::INVALIDATED:
      return "INVALIDATED";
    case Group_status::UNAVAILABLE:
      return "UNAVAILABLE";
    case Group_status::NO_QUORUM:
      return "NO_QUORUM";
    case Group_status::SPLIT_BRAIN:
      return "SPLIT_BRAIN";
    case Group_status::OK:
      return "OK";
  }
  throw std::logic_error("internal error");
}

std::string to_string(Group_health status) {
  switch (status) {
    case Group_health::UNREACHABLE:
      return "UNREACHABLE";
    case Group_health::INVALIDATED:
      return "INVALIDATED";
    case Group_health::UNAVAILABLE:
      return "UNAVAILABLE";
    case Group_health::NO_QUORUM:
      return "NO_QUORUM";
    case Group_health::SPLIT_BRAIN:
      return "SPLIT_BRAIN";
    case Group_health::OK_PARTIAL:
      return "OK_PARTIAL";
    case Group_health::OK_NO_TOLERANCE:
      return "OK_NO_TOLERANCE";
    case Group_health::OK:
      return "OK";
  }
  throw std::logic_error("internal error");
}
#endif
std::string to_string(Global_availability_status status) {
  switch (status) {
    case Global_availability_status::CATASTROPHIC:
      return "CATASTROPHIC";
    case Global_availability_status::UNAVAILABLE:
      return "UNAVAILABLE";
    case Global_availability_status::AVAILABLE_PARTIAL:
      return "AVAILABLE_PARTIAL";
    case Global_availability_status::AVAILABLE:
      return "AVAILABLE";
  }
  throw std::logic_error("internal error");
}

}  // namespace dba
}  // namespace mysqlsh