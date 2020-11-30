/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {

namespace {

const char *k_excluded_users[] = {"mysql.infoschema", "mysql.session",
                                  "mysql.sys"};

const char *k_excluded_schemas[] = {"information_schema", "mysql", "ndbinfo",
                                    "performance_schema", "sys"};

}  // namespace

Dump_instance_options::Dump_instance_options(const std::string &output_url)
    : Dump_schemas_options(output_url) {}

void Dump_instance_options::unpack_options(shcore::Option_unpacker *unpacker) {
  Dump_schemas_options::unpack_options(unpacker);

  std::unordered_set<std::string> excluded_users;
  std::unordered_set<std::string> included_users;

  unpacker->optional("excludeSchemas", &m_excluded_schemas)
      .optional("users", &m_dump_users)
      .optional("excludeUsers", &excluded_users)
      .optional("includeUsers", &included_users);

  // some users are always excluded
  excluded_users.insert(std::begin(k_excluded_users),
                        std::end(k_excluded_users));

  set_excluded_users(excluded_users);
  set_included_users(included_users);

  // some schemas are always excluded
  m_excluded_schemas.insert(std::begin(k_excluded_schemas),
                            std::end(k_excluded_schemas));
}

void Dump_instance_options::on_set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (m_dump_users &&
      session->get_server_version() < mysqlshdk::utils::Version(5, 7, 0)) {
    throw std::invalid_argument(
        "Dumping user accounts is currently not supported in MySQL versions "
        "before 5.7. Set the 'users' option to false to continue.");
  }
}

void Dump_instance_options::validate_options() const {
  // call method up the chain, Dump_schemas_options has an empty list of schemas
  // and would throw
  Ddl_dumper_options::validate_options();

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
}

}  // namespace dump
}  // namespace mysqlsh
