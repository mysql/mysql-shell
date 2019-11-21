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

#include "mysqlshdk/include/scripting/type_info/custom.h"

#include "mysqlshdk/include/scripting/types_cpp.h"

namespace shcore {
namespace detail {

namespace {

struct Nullable_validator : public Parameter_validator {
 public:
  bool valid_type(const Parameter &param, Value_type type) const override {
    return (Value_type::Null == type) ||
           Parameter_validator::valid_type(param, type);
  }
};

}  // namespace

std::unique_ptr<Parameter_validator> get_nullable_validator() {
  return std::make_unique<Nullable_validator>();
}

}  // namespace detail
}  // namespace shcore
