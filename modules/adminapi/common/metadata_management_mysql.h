/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_METADATA_MANAGEMENT_MYSQL_H_
#define MODULES_ADMINAPI_COMMON_METADATA_MANAGEMENT_MYSQL_H_

#include <memory>
#include <string>
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dba {
namespace metadata {

// Metadata schema version supported by "this" version of the shell.
mysqlshdk::utils::Version current_version();

// Metadata schema interface version supported by "this" version of the shell.
mysqlshdk::utils::Version current_public_version();

// Metadata schema version currently installed in target
mysqlshdk::utils::Version installed_version(
    std::shared_ptr<mysqlshdk::db::ISession> session);

void install(std::shared_ptr<mysqlshdk::db::ISession> session);
void upgrade(std::shared_ptr<mysqlshdk::db::ISession> session);
void uninstall(std::shared_ptr<mysqlshdk::db::ISession> session);

}  // namespace metadata
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_METADATA_MANAGEMENT_MYSQL_H_
