/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "modules/util/common/dump/server_info.h"

#include <mysqld_error.h>

#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/dump_errors.h"

namespace mysqlsh {
namespace dump {
namespace common {

namespace {

void initialize_sysvars(Server_variables *sysvars) {
  if (const auto hostname = sysvars->all.find("hostname");
      sysvars->all.end() != hostname) {
    sysvars->hostname = hostname->second;
  }

  if (const auto lower_case_table_names =
          sysvars->all.find("lower_case_table_names");
      sysvars->all.end() != lower_case_table_names) {
    sysvars->lower_case_table_names =
        shcore::lexical_cast<int8_t>(lower_case_table_names->second);
  }

  if (const auto partial_revokes = sysvars->all.find("partial_revokes");
      sysvars->all.end() != partial_revokes) {
    sysvars->partial_revokes = mysqlshdk::mysql::sysvar_to_bool(
        "partial_revokes", partial_revokes->second);
  }

  if (const auto port = sysvars->all.find("port"); sysvars->all.end() != port) {
    sysvars->port = shcore::lexical_cast<uint16_t>(port->second);
  }

  if (const auto server_uuid = sysvars->all.find("server_uuid");
      sysvars->all.end() != server_uuid) {
    sysvars->server_uuid = server_uuid->second;
  }

  if (const auto version = sysvars->all.find("version");
      sysvars->all.end() != version) {
    sysvars->version = version->second;
  }
}

const auto optional_string = [](const shcore::json::Value &o, const char *n) {
  return shcore::json::optional(o, n, true).value_or(std::string{});
};

const auto optional_uint = [](const shcore::json::Value &o, const char *n) {
  return shcore::json::optional_uint(o, n).value_or(0);
};

}  // namespace

Binlog binlog(const std::shared_ptr<mysqlshdk::db::ISession> &session,
              const Server_version &version, bool quiet) {
  Binlog binlog;

  try {
    auto keyword =
        version.is_maria_db
            ? "MASTER"
            : mysqlshdk::mysql::get_binary_logs_keyword(version.number, true);

    DBUG_EXECUTE_IF("dumper_dump_mariadb", {
      // We need the binlog query to not be affected by this dbug flag, because
      // it has to work even when running in mysql.
      keyword = mysqlshdk::mysql::get_binary_logs_keyword(
          session->get_server_version(), true);
    });

    const auto result =
        session->query(shcore::str_format("SHOW %s STATUS", keyword));

    if (const auto row = result->fetch_one()) {
      binlog.file.name = row->get_string(0);              // File
      binlog.file.position = row->get_uint(1);            // Position
      binlog.startup_options.do_db = row->get_string(2);  // Binlog_Do_DB
      binlog.startup_options.ignore_db =
          row->get_string(3);                     // Binlog_Ignore_DB
      binlog.gtid_executed = row->get_string(4);  // Executed_Gtid_Set
    }
  } catch (const mysqlshdk::db::Error &e) {
    if (e.code() == ER_SPECIFIC_ACCESS_DENIED_ERROR) {
      if (!quiet) {
        current_console()->print_warning(
            "Could not fetch the binary log information: " + e.format());
      }
    } else {
      throw;
    }
  }

  return binlog;
}

Binlog binlog(const std::shared_ptr<mysqlshdk::db::ISession> &session,
              bool quiet) {
  return binlog(session, server_version(session), quiet);
}

void serialize(const Binlog &binlog, shcore::JSON_dumper *dumper) {
  dumper->start_object();

  dumper->append("file", binlog.file.name);
  dumper->append("position", binlog.file.position);
  dumper->append("gtidExecuted", binlog.gtid_executed);

  dumper->end_object();
}

Binlog binlog(const shcore::json::Value &object) {
  Binlog binlog;

  binlog.file.name = optional_string(object, "file");
  binlog.file.position = optional_uint(object, "position");
  binlog.gtid_executed = optional_string(object, "gtidExecuted");

  return binlog;
}

Server_version server_version(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  const auto result = session->query("SELECT @@GLOBAL.VERSION");

  if (const auto row = result->fetch_one()) {
    return server_version(row->get_string(0));
  } else {
    THROW_ERROR(SHERR_DUMP_IC_FAILED_TO_FETCH_VERSION);
  }

  return {};
}

Server_version server_version(std::string_view ver) {
  using mysqlshdk::utils::Version;

  Server_version version;

  version.number = Version(ver);

  DBUG_EXECUTE_IF("dumper_dump_mariadb", {
    version.number = Version("10.4.18-MariaDB-1:10.4.18+maria~focal");
  });

  if (std::string::npos !=
      shcore::str_lower(version.number.get_extra()).find("mariadb")) {
    // we don't want the numbering used by MariaDB to interfere with various
    // conditions we have in our code, just fall-back to an old version
    version.number = Version("5.6.0-" + version.number.get_full());
    version.is_5_6 = true;
    version.is_maria_db = true;
  } else if (version.number < Version(5, 7, 0)) {
    version.is_5_6 = true;
  } else if (version.number < Version(8, 0, 0)) {
    version.is_5_7 = true;
  } else {
    version.is_8_0 = true;
  }

  return version;
}

Server_variables server_variables(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  Server_variables sysvars;

  {
    const auto result = session->query("SHOW GLOBAL VARIABLES");

    while (const auto row = result->fetch_one()) {
      sysvars.all.emplace(row->get_string(0), row->get_string(1));
    }
  }

  initialize_sysvars(&sysvars);

  return sysvars;
}

Replication_topology replication_topology(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  mysqlshdk::mysql::Instance instance{session};

  Replication_topology topology;

  topology.canonical_address = instance.get_canonical_address();

  topology.group_name =
      instance.get_sysvar_string("group_replication_group_name")
          .value_or(std::string{});

  topology.view_change_uuid =
      instance.get_sysvar_string("group_replication_view_change_uuid")
          .value_or(std::string{});

  if (session->query("SHOW SCHEMAS LIKE 'mysql_innodb_cluster_metadata'")
          ->fetch_one()) {
    const auto query = [&session](std::string_view sql, const char *table,
                                  const char *column) {
      try {
        const auto result = session->query(sql);

        if (const auto row = result->fetch_one()) {
          return row->get_string(0);
        }
      } catch (const mysqlshdk::db::Error &e) {
        log_warning(
            "Failed to fetch '%s' from mysql_innodb_cluster_metadata.%s: %s",
            column, table, e.format().c_str());
      }

      return std::string{};
    };

    topology.cluster_id = query(
        "SELECT cluster_id FROM mysql_innodb_cluster_metadata.v2_this_instance",
        "v2_this_instance", "cluster_id");

    if (topology.cluster_id.empty()) {
      topology.cluster_id = query(
          "SELECT cluster_id FROM mysql_innodb_cluster_metadata.instances "
          "WHERE CAST(mysql_server_uuid AS binary)=CAST(@@server_uuid AS "
          "binary)",
          "instances", "cluster_id");
    }

    topology.cluster_set_id = query(
        "SELECT clusterset_id FROM mysql_innodb_cluster_metadata.v2_cs_members",
        "v2_cs_members", "clusterset_id");
  }

  return topology;
}

void serialize(const Server_info &info, shcore::JSON_dumper *dumper,
               bool binlog, bool sysvars) {
  const auto append_non_empty = [dumper](const char *name,
                                         const std::string &value) {
    if (!value.empty()) {
      dumper->append(name, value);
    }
  };

  dumper->start_object();

  if (binlog) {
    dumper->append("binlog");
    serialize(info.binlog, dumper);
  }

  if (sysvars) {
    dumper->append("sysvars");
    dumper->start_object();

    for (const auto &var : info.sysvars.all) {
      dumper->append(var.first, var.second);
    }

    dumper->end_object();  // sysvars
  } else {
    // store just some basic information
    dumper->append("version", info.version.number.get_full());
    dumper->append("hostname", info.sysvars.hostname);
    dumper->append("port", info.sysvars.port);
    dumper->append("serverUuid", info.sysvars.server_uuid);
  }

  dumper->append("topology");
  dumper->start_object();

  dumper->append("canonicalAddress", info.topology.canonical_address);
  append_non_empty("groupName", info.topology.group_name);
  append_non_empty("viewChangeUuid", info.topology.view_change_uuid);
  append_non_empty("clusterId", info.topology.cluster_id);
  append_non_empty("clusterSetId", info.topology.cluster_set_id);

  dumper->end_object();  // topology

  dumper->end_object();
}

Server_info server_info(const shcore::json::Value &object) {
  Server_info info;

  if (const auto log = shcore::json::optional_object(object, "binlog");
      log.has_value()) {
    info.binlog = binlog(*log);
  }

  if (auto sysvars = shcore::json::optional_map(object, "sysvars");
      sysvars.has_value()) {
    info.sysvars.all = std::move(*sysvars);
    initialize_sysvars(&info.sysvars);
    info.version = server_version(info.sysvars.version);
  } else {
    if (const auto version = optional_string(object, "version");
        !version.empty()) {
      info.version = server_version(version);
    }

    info.sysvars.hostname = optional_string(object, "hostname");
    info.sysvars.server_uuid = optional_string(object, "serverUuid");
  }

  if (const auto topology = shcore::json::optional_object(object, "topology");
      topology.has_value()) {
    info.topology.canonical_address =
        optional_string(*topology, "canonicalAddress");
    info.topology.group_name = optional_string(*topology, "groupName");
    info.topology.view_change_uuid =
        optional_string(*topology, "viewChangeUuid");
    info.topology.cluster_id = optional_string(*topology, "clusterId");
    info.topology.cluster_set_id = optional_string(*topology, "clusterSetId");
  }

  return info;
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
