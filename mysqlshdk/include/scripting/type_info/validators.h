/*
 * Copyright (c) 2019, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_VALIDATORS_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_VALIDATORS_H_

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/include/scripting/type_info.h"
#include "mysqlshdk/include/scripting/types/validator.h"

namespace shcore {
namespace detail {

template <typename T>
struct Validator_for<std::vector<T>> {
  static std::unique_ptr<List_validator> get() {
    auto validator = std::make_unique<List_validator>();
    validator->set_element_type(Type_info<T>::vtype);
    return validator;
  }
};

template <typename T>
struct Validator_for<std::unordered_set<T>> {
  static std::unique_ptr<List_validator> get() {
    auto validator = std::make_unique<List_validator>();
    validator->set_element_type(Type_info<T>::vtype);
    return validator;
  }
};

template <typename T>
struct Validator_for<std::optional<T>> {
  static std::unique_ptr<Nullable_validator> get() {
    return std::make_unique<Nullable_validator>();
  }
};

}  // namespace detail
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPE_INFO_VALIDATORS_H_
