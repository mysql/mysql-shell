/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MODULES_ADMINAPI_MOD_DBA_SQL_
#define _MODULES_ADMINAPI_MOD_DBA_SQL_

#include "modules/mysql_connection.h"

namespace mysqlsh {
namespace dba {

  enum class GRInstanceType {
    Standalone,
    GroupReplication,
    InnoDBCluster
  };

  enum class SlaveReplicationState {
    New,
    Recoverable,
    Diverged,
    Irrecoverable
  };


  GRInstanceType get_gr_instance_type(mysqlsh::mysql::Connection* connection);
  void get_port_and_datadir(mysqlsh::mysql::Connection* connection, int &port, std::string& datadir);
  void get_gtid_state_variables(mysqlsh::mysql::Connection* connection, std::string &executed, std::string &purged);
  SlaveReplicationState get_slave_replication_state(mysqlsh::mysql::Connection* connection, std::string &slave_executed);

}
}

#endif