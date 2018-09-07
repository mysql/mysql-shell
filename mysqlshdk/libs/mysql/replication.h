/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_
#define MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_

#include <memory>
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

/**
 * Wait until the given GTID set is applied on the target instance.
 *
 * @param instance target instance to wait for GTIDs to be applied.
 * @param gtid_set string with the GTID set to wait to be applied.
 * @param timeout positive integer with the maximum time in seconds to wait for
 *                all GTIDs to be applied on the instance.
 * @return Return true if the operation succeeded and false if the timeout was
 *         reached.
 * @throws an error if some issue occured when waiting for transaction to be
 *         applied.
 */
bool wait_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &gtid_set, int timeout);

/**
 * Wait until GTID_EXECUTED from the source instance is applied at the target.
 *
 * @param target target instance to wait for GTIDs to be applied.
 * @param source source instance to take GTID_EXECUTED from.
 * @param timeout positive integer with the maximum time in seconds to wait for
 *                all GTIDs to be applied on the instance.
 * @return Return true if the operation succeeded and false if the timeout was
 *         reached.
 * @throws an error if some issue occured when waiting for transaction to be
 *         applied.
 *
 * Returns true immediately if GTID_EXECUTED from the source instance is
 * empty or NULL.
 */
bool wait_for_gtid_set_from(const mysqlshdk::mysql::IInstance &target,
                            const mysqlshdk::mysql::IInstance &source,
                            int timeout);

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_