/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <string>
#include <vector>

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

Instance::Instance(std::shared_ptr<db::ISession> session) : _session(session) {}

utils::nullable<bool> Instance::get_sysvar_bool(const std::string& name) const {
  utils::nullable<bool> ret_val;

  auto variables = get_system_variables({name});

  if (variables[name]) {
    std::string str_value = variables[name];
    const char* value = str_value.c_str();

    if (shcore::str_caseeq(value, "YES") || shcore::str_caseeq(value, "TRUE") ||
        shcore::str_caseeq(value, "1") || shcore::str_caseeq(value, "ON"))
      ret_val = true;
    else if (shcore::str_caseeq(value, "NO") ||
             shcore::str_caseeq(value, "FALSE") ||
             shcore::str_caseeq(value, "0") || shcore::str_caseeq(value, "OFF"))
      ret_val = false;
    else
      throw std::runtime_error("The variable " + name + "is not boolean.");
  }

  return ret_val;
}

utils::nullable<std::string> Instance::get_sysvar_string(
    const std::string& name) const {
  return get_system_variables({name})[name];
}

utils::nullable<int64_t> Instance::get_sysvar_int(
    const std::string& name) const {
  utils::nullable<int64_t> ret_val;

  auto variables = get_system_variables({name});

  if (variables[name]) {
    std::string value = variables[name];

    if (!value.empty()) {
      size_t end_pos;
      int64_t int_val = std::stoi(value, &end_pos);

      if (end_pos == value.size())
        ret_val = int_val;
      else
        throw std::runtime_error("The variable " + name +
                                 " is not an integer.");
    }
  }

  return ret_val;
}

std::map<std::string, utils::nullable<std::string> >
Instance::get_system_variables(const std::vector<std::string>& names) const {
  std::map<std::string, utils::nullable<std::string> > ret_val;

  if (!names.empty()) {
    std::string query_format =
        "show variables where ! in (?";

    ret_val[names[0]] = utils::nullable<std::string>();

    for (size_t index = 1; index < names.size(); index++) {
      query_format.append(", ?");
      ret_val[names[index]] = utils::nullable<std::string>();
    }

    query_format.append(")");

    shcore::sqlstring query(query_format.c_str(), 0);

    query << "variable_name";

    for (auto name : names)
      query << name;

    query.done();

    auto result = _session->query(query);
    auto row = result->fetch_one();

    while (row) {
      ret_val[row->get_string(0)] = row->get_string(1);
      row = result->fetch_one();
    }
  }

  return ret_val;
}

}  // namespace mysql
}  // namespace mysqlshdk
