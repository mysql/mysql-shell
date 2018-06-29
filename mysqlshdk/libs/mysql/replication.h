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

/**
 * Generate a random server ID.
 *
 * Generate a pseudo-random server_id, with a value between 1 and 4294967295.
 * Two generated values have a low probability of being the same (independently
 * of the time they are generated). Minimum value generated is 1 and maximum
 * 4294967295.
 *
 * For the random generation (assuming random generation is uniformly) the
 * probability of the same value being generated is given by:
 *      P(n, t) = 1 - t!/(t^n * (t-n)!)
 * where t is the total number of different values that can be generated, and
 * n is the number of values that are generated.
 *
 * In this case, t = 4294967295 (max number of values that can be generated),
 * and for example the probability of generating the same id for 15, 100, and
 * 1000 servers (n=15, n=100, and n=1000) is approximately:
 *      P(15, 4294967295)   = 2.44 * 10^-8    (0.00000244 %)
 *      P(100, 4294967295)  = 1.15 * 10^-6    (0.000115 %)
 *      P(1000, 4294967295) = 1.16 * 10^-4    (0.0116 %)
 *
 *  Note: Zero is not a valid sever_id.
 *
 * @return an integer with a random between 1 and 4294967295.
 */
int64_t generate_server_id();

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_REPLICATION_H_