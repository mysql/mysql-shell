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

#include <random>
#include <string>

#include "mysqlshdk/libs/mysql/replication.h"

#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

bool wait_for_gtid_set(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &gtid_set, int timeout) {
  // NOTE: According to the GR team the max supported timeout value for
  //       WAIT_FOR_EXECUTED_GTID_SET() is 18446744073.7096 seconds
  //       (2^64/1000000000). Therefore, a type with a max value that include
  //       that range should be used.
  auto result = instance.get_session()->queryf(
      "SELECT WAIT_FOR_EXECUTED_GTID_SET(?, ?)", gtid_set, timeout);
  auto row = result->fetch_one();
  // WAIT_FOR_EXECUTED_GTID_SET() returns 0 for success, 1 for timeout,
  // otherwise an error is generated.
  return row->get_int(0) == 0;
}

bool wait_for_gtid_set_from(const mysqlshdk::mysql::IInstance &target,
                            const mysqlshdk::mysql::IInstance &source,
                            int timeout) {
  auto result = source.get_session()->query("SELECT @@GTID_EXECUTED");
  auto row = result->fetch_one();
  if (!row || row->is_null(0) || row->get_string(0).empty()) return true;

  return wait_for_gtid_set(target, row->get_string(0), timeout);
}

int64_t generate_server_id() {
  // Setup uniform random generation of integers between [1, 4294967295] using
  // Mersenne Twister algorithm and a non-determinist seed.
  std::random_device rd_seed;
  std::mt19937 rnd_gen(rd_seed());
  std::uniform_int_distribution<int64_t> distribution(1, 4294967295);
  return distribution(rnd_gen);
}

}  // namespace mysql
}  // namespace mysqlshdk
