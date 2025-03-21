/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_FUNCTION_METADATA_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_FUNCTION_METADATA_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types/parameter.h"

namespace shcore {

using Raw_signature = std::vector<std::shared_ptr<Parameter>>;

struct Function_metadata {
  Function_metadata() = default;
  Function_metadata(const Function_metadata &) = delete;

  std::string name[2];
  Raw_signature signature;
  std::optional<bool> cli_enabled;

  std::vector<std::pair<std::string, Value_type>> param_types;
  std::string param_codes;
  Value_type return_type{shcore::Undefined};

  void set_name(const std::string &name);
  void set(const std::string &name, Value_type rtype,
           const std::vector<std::pair<std::string, Value_type>> &ptypes,
           const std::string &pcodes);
  void set(const std::string &name, Value_type rtype,
           const Raw_signature &params,
           const std::vector<std::pair<std::string, Value_type>> &ptypes,
           const std::string &pcodes);
  void cli(bool enable = true);
};

Raw_signature generate_signature(
    const std::vector<std::pair<std::string, Value_type>> &param_types);

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_TYPES_FUNCTION_METADATA_H_
