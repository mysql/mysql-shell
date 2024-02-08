/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_REPLICA_SET_STATUS_H_
#define MODULES_ADMINAPI_REPLICA_SET_STATUS_H_

#include "scripting/types.h"

namespace mysqlsh::dba {
class Replica_set_impl;
}

namespace mysqlsh::dba::replicaset {

class Status {
 protected:
  Status(Replica_set_impl &rset) noexcept : m_rset(rset) {}

  shcore::Value do_run(int extended);
  static constexpr bool supports_undo() noexcept { return false; }

 private:
  void scan_async_channel_ssl_info(shcore::Dictionary_t status);

 private:
  Replica_set_impl &m_rset;
};

}  // namespace mysqlsh::dba::replicaset

#endif  // MODULES_ADMINAPI_REPLICA_SET_STATUS_H_
