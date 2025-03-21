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

#include "mysqlshdk/include/scripting/types/function_metadata.h"

#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_general.h"

#include "mysqlshdk/include/scripting/naming_style.h"

namespace shcore {

void Function_metadata::set_name(const std::string &name_) {
  auto index = name_.find("|");
  if (index == std::string::npos) {
    name[LowerCamelCase] = get_member_name(name_, LowerCamelCase);
    name[LowerCaseUnderscores] = get_member_name(name_, LowerCaseUnderscores);
  } else {
    name[LowerCamelCase] = name_.substr(0, index);
    name[LowerCaseUnderscores] = name_.substr(index + 1);
  }
}

void Function_metadata::set(
    const std::string &name_, Value_type rtype,
    const std::vector<std::pair<std::string, Value_type>> &ptypes,
    const std::string &pcodes) {
  set_name(name_);

  signature = generate_signature(ptypes);

  param_types = ptypes;
  param_codes = pcodes;
  return_type = rtype;
}

void Function_metadata::set(
    const std::string &name_, Value_type rtype, const Raw_signature &params,
    const std::vector<std::pair<std::string, Value_type>> &ptypes,
    const std::string &pcodes) {
  set_name(name_);

  signature = params;

  for (const auto &param : signature) {
    std::string param_name = param->name;

    if (param->flag == Param_flag::Optional) param_name = "?" + param_name;

    param_types.push_back(std::make_pair(param_name, param->type()));
  }

  param_types = ptypes;
  param_codes = pcodes;
  return_type = rtype;
}

void Function_metadata::cli(bool enable) { cli_enabled = enable; }

Raw_signature generate_signature(
    const std::vector<std::pair<std::string, Value_type>> &args) {
  Raw_signature sig;
  for (auto &i : args) {
    Param_flag flag = Param_flag::Mandatory;
    std::string name = i.first;
    if (i.first[0] == '?') {
      name = i.first.substr(1);
      flag = Param_flag::Optional;
    }

    sig.push_back(std::make_shared<Parameter>(name, i.second, flag));
  }
  return sig;
}

}  // namespace shcore
