/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/router.h"
#include "modules/adminapi/common/setup_account.h"
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
  // NOTE: These lines can be removed once Cluster_impl starts using
  // acquire_primary()
  auto master = m_primary_master;
  if (!master) master = m_target_server;

  assert(master);

  std::string gtid_set = mysqlshdk::mysql::get_executed_gtid_set(*master);

  bool sync_res = wait_for_gtid_set_safe(target_instance, gtid_set,
                                         channel_name, timeout, true);

  if (!sync_res) {
    throw shcore::Exception(
        "Timeout reached waiting for transactions from " + master->descr() +
            " to be applied on instance '" + target_instance.descr() + "'",
        SHERR_DBA_GTID_SYNC_TIMEOUT);
  }
}

std::string Base_cluster_impl::get_replication_user_name(
    mysqlshdk::mysql::IInstance *target_instance,
    const std::string &user_prefix) const {
  return user_prefix +
         std::to_string(*target_instance->get_sysvar_int("server_id"));
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

/**
 * Helper method that has common code for setup_router_account and
 * setup_admin_account methods
 * @param user the user part of the account
 * @param host The host part of the account
 * @param interactive the value of the interactive flag
 * @param update the value of the update flag
 * @param dry_run the value of the dry_run flag
 * @param password the password for the account
 * @param type The type of account to create, Admin or Router
 */
void Base_cluster_impl::setup_account_common(
    const std::string &username, const std::string &host, bool interactive,
    bool update, bool dry_run,
    const mysqlshdk::utils::nullable<std::string> &password,
    const Setup_account_type &type) {
  // NOTE: GR by design guarantees that the primary instance is always the one
  // with the lowest instance version. A similar (although not explicit)
  // guarantee exists on Semi-sync replication, replication from newer master to
  // older slaves might not be possible but is generally not supported. See:
  // https://dev.mysql.com/doc/refman/en/replication-compatibility.html
  //
  // By using the primary instance to validate
  // the list of privileges / build the list of grants to be given to the
  // new/existing user we are sure that privileges that appeared in recent
  // versions (like REPLICATION_APPLIER) are only checked/granted in case all
  // cluster members support it. This ensures that there is no issue when the
  // DDL statements are replicated throughout the cluster since they won't
  // contain unsupported grants.

  // The pool is initialized with the metadata using the current session
  auto metadata = std::make_shared<MetadataStorage>(get_target_server());
  Instance_pool::Auth_options auth_opts;
  auth_opts.get(get_target_server()->get_connection_options());
  Scoped_instance_pool ipool(interactive, auth_opts);
  ipool->set_metadata(metadata);

  const auto primary_instance = acquire_primary();
  auto finally_primary =
      shcore::on_leave_scope([this]() { release_primary(); });

  // get the metadata version to build an accurate list of grants
  mysqlshdk::utils::Version metadata_version;
  if (!metadata->check_version(&metadata_version)) {
    throw std::logic_error("Internal error, metadata not found.");
  }

  {
    std::vector<std::string> grant_list;
    // get list of grants
    switch (type) {
      case Setup_account_type::ROUTER:
        grant_list = create_router_grants(shcore::make_account(username, host),
                                          metadata_version);
        break;
      case Setup_account_type::ADMIN:
        grant_list = create_admin_grants(shcore::make_account(username, host),
                                         primary_instance->get_version());
        break;
    }

    Setup_account op_setup(username, host, interactive, update, dry_run,
                           password, grant_list, *primary_instance);
    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope([&op_setup]() { op_setup.finish(); });
    // Prepare the setup_router_account execution
    op_setup.prepare();
    // Execute the setup_router_account command.
    op_setup.execute();
  }
}

void Base_cluster_impl::setup_admin_account(
    const std::string &username, const std::string &host, bool interactive,
    bool update, bool dry_run,
    const mysqlshdk::utils::nullable<std::string> &password) {
  setup_account_common(username, host, interactive, update, dry_run, password,
                       Setup_account_type::ADMIN);
}

void Base_cluster_impl::setup_router_account(
    const std::string &username, const std::string &host, bool interactive,
    bool update, bool dry_run,
    const mysqlshdk::utils::nullable<std::string> &password) {
  setup_account_common(username, host, interactive, update, dry_run, password,
                       Setup_account_type::ROUTER);
}

}  // namespace dba
}  // namespace mysqlsh
