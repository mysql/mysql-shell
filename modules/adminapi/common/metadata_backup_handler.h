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

#ifndef MODULES_ADMINAPI_COMMON_METADATA_BACKUP_HANDLER_H_
#define MODULES_ADMINAPI_COMMON_METADATA_BACKUP_HANDLER_H_

#include <functional>
#include <memory>
#include <string>

#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dba {
namespace metadata {
using mysqlshdk::utils::Version;
/**
 * Executes a backup of the metadata schema using the schema for the specified
 * version.
 *
 * @param version The version of the metadata schema to be used on the creation
 * of the backup schema.
 * @param src_schema The schema from which the data will be copied.
 * @param tgt_schema The schema to which the data will be copied.
 * @param instance The instance in which the backup will be executed.
 * @param use_updating_version Flag to update the schema_version to be equal to
 * the upgrading version (0.0.0).
 *
 * This function will use a factory function to generate the apropiate backup
 * handled for the indicated schema version, this is required because some
 * versions of the metadata schema may require special handling while doing the
 * backup.
 */
void backup(const Version &version, const std::string &src_schema,
            const std::string &tgt_schema,
            const std::shared_ptr<Instance> &instance,
            const std::string &context, bool keep_upgrading_version = false);
}  // namespace metadata
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_METADATA_BACKUP_HANDLER_H_
