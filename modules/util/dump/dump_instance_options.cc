/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_instance_options.h"

#include <utility>
#include <vector>

#include "modules/util/dump/dump_utils.h"

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {

namespace {

const char *k_excluded_users[] = {"mysql.infoschema", "mysql.session",
                                  "mysql.sys"};

const char *k_excluded_schemas[] = {"information_schema", "mysql", "ndbinfo",
                                    "performance_schema", "sys"};

}  // namespace

Dump_instance_options::Dump_instance_options() {
  // some users are always excluded
  std::vector<std::string> data{std::begin(k_excluded_users),
                                std::end(k_excluded_users)};
  add_excluded_users(data);

  // some schemas are always excluded
  m_excluded_schemas.insert(std::begin(k_excluded_schemas),
                            std::end(k_excluded_schemas));
}

const shcore::Option_pack_def<Dump_instance_options>
    &Dump_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_instance_options>()
          .include<Dump_schemas_options>()
          .optional("excludeSchemas",
                    &Dump_instance_options::set_string_list_option)
          .optional("includeSchemas",
                    &Dump_instance_options::set_string_list_option)
          .optional("users", &Dump_instance_options::m_dump_users)
          .optional("excludeUsers",
                    &Dump_instance_options::set_string_list_option)
          .optional("includeUsers",
                    &Dump_instance_options::set_string_list_option)
          .on_done(&Dump_instance_options::on_unpacked_options)
          .on_log(&Dump_instance_options::on_log_options);

  return opts;
}

void Dump_instance_options::set_string_list_option(
    const std::string &option, const std::unordered_set<std::string> &data) {
  if (option == "excludeUsers") {
    add_excluded_users(data);
  } else if (option == "includeUsers") {
    set_included_users(data);
  } else if (option == "excludeSchemas") {
    m_excluded_schemas.insert(data.begin(), data.end());
  } else if (option == "includeSchemas") {
    m_included_schemas.insert(data.begin(), data.end());
  } else {
    // This function should only be called with the options above.
    assert(false);
  }
}

void Dump_instance_options::on_unpacked_options() {
  if (!m_dump_users) {
    if (excluded_users().size() > shcore::array_size(k_excluded_users)) {
      throw std::invalid_argument(
          "The 'excludeUsers' option cannot be used if the 'users' option is "
          "set to false.");
    }

    if (!included_users().empty()) {
      throw std::invalid_argument(
          "The 'includeUsers' option cannot be used if the 'users' option is "
          "set to false.");
    }
  }

  error_on_schema_filters_conflicts();
  error_on_user_filters_conflicts();
}

void Dump_instance_options::validate_options() const {
  // call method up the chain, Dump_schemas_options has an empty list of schemas
  // and would throw
  Ddl_dumper_options::validate_options();
}

void Dump_instance_options::error_on_user_filters_conflicts() {
  m_filter_conflicts |= ::mysqlsh::dump::error_on_user_filters_conflicts(
      included_users(), excluded_users());
}

}  // namespace dump
}  // namespace mysqlsh
