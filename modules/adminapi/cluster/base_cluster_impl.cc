/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/cluster/base_cluster_impl.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_options.h"
#include "modules/adminapi/cluster/cluster_set_option.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/sql.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "shellcore/utils_help.h"

namespace mysqlsh {
namespace dba {

Base_cluster_impl::Base_cluster_impl(
    const std::string &cluster_name, std::shared_ptr<Instance> group_server,
    std::shared_ptr<MetadataStorage> metadata_storage)
    : m_cluster_name(cluster_name),
      m_target_server(group_server),
      m_metadata_storage(metadata_storage) {
  m_target_server->retain();

  m_admin_credentials.get(group_server->get_connection_options());
}

Base_cluster_impl::~Base_cluster_impl() {
  if (m_target_server) {
    m_target_server->release();
    m_target_server.reset();
  }

  if (m_primary_master) {
    m_primary_master->release();
    m_primary_master.reset();
  }
}

void Base_cluster_impl::disconnect() {
  if (m_target_server) {
    m_target_server->release();
    m_target_server.reset();
  }

  if (m_primary_master) {
    m_primary_master->release();
    m_primary_master.reset();
  }

  if (m_metadata_storage) {
    m_metadata_storage.reset();
  }
}

void Base_cluster_impl::target_server_invalidated() {
  if (m_target_server && m_primary_master) {
    m_target_server->release();
    m_target_server = m_primary_master;
    m_target_server->retain();
  } else {
    // find some other server to connect to?
  }
}

bool Base_cluster_impl::get_gtid_set_is_complete() const {
  shcore::Value flag;
  if (get_metadata_storage()->query_cluster_attribute(
          get_id(), k_cluster_attribute_assume_gtid_set_complete, &flag))
    return flag.as_bool();
  return false;
}

void Base_cluster_impl::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &channel_name, int timeout) const {
  assert(m_primary_master);

  std::string gtid_set =
      mysqlshdk::mysql::get_executed_gtid_set(*m_primary_master);

  bool sync_res = wait_for_gtid_set_safe(target_instance, gtid_set,
                                         channel_name, timeout, true);

  if (!sync_res) {
    throw shcore::Exception("Timeout reached waiting for transactions from " +
                                m_primary_master->descr() +
                                " to be applied on instance '" +
                                target_instance.descr() + "'",
                            SHERR_DBA_GTID_SYNC_TIMEOUT);
  }
}

namespace {
std::string format_repl_user_name(mysqlshdk::mysql::IInstance *slave,
                                  const std::string &user_prefix) {
  return user_prefix + "_" +
         std::to_string(*slave->get_sysvar_int("server_id"));
}
}  // namespace

mysqlshdk::mysql::Auth_options Base_cluster_impl::create_replication_user(
    mysqlshdk::mysql::IInstance *slave, const std::string &user_prefix,
    bool dry_run) {
  assert(m_primary_master);

  mysqlshdk::mysql::Auth_options creds;

  creds.user = format_repl_user_name(slave, user_prefix);

  try {
    std::string repl_password;

    // Create replication accounts for this instance at the master
    // replicaset unless the user provided one.

    auto console = mysqlsh::current_console();

    // Accounts are created at the master replicaset regardless of who will use
    // them, since they'll get replicated everywhere.

    // drop the replication user, for all hosts
    // we need to drop any user from any host in case a different instance was
    // the slave earlier
    if (dry_run)
      log_info("Drop %s at %s (dryRun)", creds.user.c_str(),
               m_primary_master->descr().c_str());
    else
      drop_replication_user(slave, user_prefix);

    log_info("Creating replication user %s@%% with random password at %s%s",
             creds.user.c_str(), m_primary_master->descr().c_str(),
             dry_run ? " (dryRun)" : "");

    // re-create replication with a new generated password
    if (!dry_run) {
      mysqlshdk::mysql::create_user_with_random_password(
          *m_primary_master, creds.user, {"%"},
          {std::make_tuple("REPLICATION SLAVE", "*.*", false)}, &repl_password);
    }

    creds.password = repl_password;
  } catch (const std::exception &e) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error while setting up replication account: %s", e.what()));
  }

  return creds;
}

mysqlshdk::mysql::Auth_options Base_cluster_impl::refresh_replication_user(
    mysqlshdk::mysql::IInstance *slave, const std::string &user_prefix,
    bool dry_run) {
  assert(m_primary_master);

  mysqlshdk::mysql::Auth_options creds;

  creds.user = format_repl_user_name(slave, user_prefix);

  try {
    // Create replication accounts for this instance at the master
    // replicaset unless the user provided one.

    auto console = mysqlsh::current_console();

    log_info("Resetting password for %s@%% at %s", creds.user.c_str(),
             m_primary_master->descr().c_str());
    // re-create replication with a new generated password
    if (!dry_run) {
      std::string repl_password;
      mysqlshdk::mysql::set_random_password(*m_primary_master, creds.user,
                                            {"%"}, &repl_password);
      creds.password = repl_password;
    }
  } catch (const std::exception &e) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error while resetting password for replication account: %s",
        e.what()));
  }

  return creds;
}

void Base_cluster_impl::drop_replication_user(
    mysqlshdk::mysql::IInstance *slave, const std::string &user_prefix) {
  assert(m_primary_master);

  std::string user = format_repl_user_name(slave, user_prefix);

  log_info("Dropping account %s@%% at %s", user.c_str(),
           m_primary_master->descr().c_str());
  try {
    m_primary_master->drop_user(user, "%", true);
  } catch (mysqlshdk::db::Error &e) {
    auto console = current_console();
    console->print_warning(shcore::str_format(
        "%s: Error dropping account %s@%s: %s",
        m_primary_master->descr().c_str(), user.c_str(),
        slave->get_canonical_hostname().c_str(), e.format().c_str()));
    // ignore the error and move on
  }
}

std::string Base_cluster_impl::get_replication_user(
    mysqlshdk::mysql::IInstance *target_instance,
    const std::string &user_prefix) const {
  return format_repl_user_name(target_instance, user_prefix);
}

void Base_cluster_impl::set_target_server(
    const std::shared_ptr<Instance> &instance) {
  disconnect();

  m_target_server = instance;
  m_target_server->retain();

  m_metadata_storage = std::make_shared<MetadataStorage>(m_target_server);
}

std::shared_ptr<Instance> Base_cluster_impl::connect_target_instance(
    const std::string &instance_def) {
  assert(m_target_server);

  auto bad_target = []() {
    current_console()->print_error(
        "Target instance must be given as host:port. Credentials will be taken"
        " from the main session and, if given, must match them.");

    return std::invalid_argument("Invalid target instance specification");
  };

  auto ipool = current_ipool();

  // check that the instance_def
  mysqlshdk::db::Connection_options opts(instance_def);

  // clear allowed values
  opts.clear_host();
  opts.clear_port();
  opts.clear_socket();
  // check if credentials given and if so, ensure they match the target session
  const auto &main_opts(m_target_server->get_connection_options());
  if (opts.has_user() && opts.get_user() == main_opts.get_user())
    opts.clear_user();
  if (opts.has_password() && opts.get_password() == main_opts.get_password())
    opts.clear_password();
  if (opts.has_scheme() && opts.get_scheme() == main_opts.get_scheme())
    opts.clear_scheme();

  if (opts.has_data()) throw bad_target();

  opts = mysqlshdk::db::Connection_options(instance_def);
  opts.clear_user();
  opts.set_user(main_opts.get_user());
  opts.clear_password();
  opts.set_password(main_opts.get_password());
  opts.clear_scheme();
  opts.set_scheme(main_opts.get_scheme());

  try {
    return ipool->connect_unchecked(opts);
  } catch (const shcore::Exception &e) {
    log_warning("Could not connect to target instance %s: %s",
                instance_def.c_str(), e.format().c_str());
    throw;
  }
}

shcore::Value Base_cluster_impl::list_routers(bool only_upgrade_required) {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["routers"] = router_list(get_metadata_storage().get(), get_id(),
                                   only_upgrade_required);

  return shcore::Value(dict);
}
}  // namespace dba
}  // namespace mysqlsh
