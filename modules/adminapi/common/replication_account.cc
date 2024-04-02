/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/replication_account.h"

#include <cassert>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/server_features.h"
#include "modules/adminapi/dba/api_options.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "utils/debug.h"
#include "utils/utils_general.h"

namespace mysqlsh::dba {

namespace {
constexpr std::string_view k_cluster_read_replica_user_name{
    "mysql_innodb_replica_"};

int64_t extract_server_id(std::string_view account_user) {
  auto pos = account_user.find_last_of('_');
  if (pos == std::string_view::npos) return -1;

  account_user = account_user.substr(pos + 1);

  return shcore::lexical_cast<int64_t>(account_user, -1);
}
}  // namespace

std::string Replication_account::make_replication_user_name(
    uint32_t server_id, std::string_view user_prefix, bool server_id_hexa) {
  if (server_id_hexa)
    return shcore::str_format("%.*s%x", static_cast<int>(user_prefix.length()),
                              user_prefix.data(), server_id);

  return shcore::str_format("%.*s%u", static_cast<int>(user_prefix.length()),
                            user_prefix.data(), server_id);
}

Replication_account::Auth_options_host
Replication_account::create_replication_user(
    const mysqlshdk::mysql::IInstance &target,
    std::string_view auth_cert_subject, Account_type account_type,
    bool only_on_target, mysqlshdk::mysql::Auth_options creds,
    bool print_recreate_note, bool dry_run) const {
  // for Clusters
  if (topo_holds<Cluster_impl>()) {
    const auto &cluster = topo_as<Cluster_impl>();

    const mysqlshdk::mysql::IInstance *primary = &target;

    auto host = get_replication_user_host();

    if (creds.user.empty()) {
      creds.user = Replication_account::make_replication_user_name(
          target.get_server_id(),
          account_type == Replication_account::Account_type::READ_REPLICA
              ? k_cluster_read_replica_user_name
              : Replication_account::k_group_recovery_user_prefix);
    }

    std::shared_ptr<mysqlshdk::mysql::IInstance> primary_master;
    if (!only_on_target) {
      primary_master = cluster.get_primary_master();
      primary = primary_master.get();
    }

    log_info("Creating replication account '%s'@'%s' for instance '%s'",
             creds.user.c_str(), host.c_str(), target.descr().c_str());

    {
      // Get all hosts for the account:
      //
      // There may be left-over accounts in the target instance that must be
      // cleared-out, especially because when using the MySQL Communication
      // Stack a new account is created on both ends (joiner and donor) to allow
      // GCS establish a connection before joining the member to the group and
      // if the account already exists with a different host that is reachable,
      // GCS will fail to establish a connection and the member won't be able to
      // join the group. For that reason, we must ensure all of those recovery
      // accounts are dropped and not only the one of the
      // 'replicationAllowedHost'
      std::vector<std::string> recovery_user_hosts =
          mysqlshdk::mysql::get_all_hostnames_for_user(*primary, creds.user);

      // Check if the replication user already exists and delete it if it does,
      // before creating it again.
      for (const auto &hostname : recovery_user_hosts) {
        if (!primary->user_exists(creds.user, hostname)) continue;

        auto note = shcore::str_format(
            "User '%s'@'%s' already existed at instance '%s'. It will be "
            "deleted and created again with a new password.",
            creds.user.c_str(), hostname.c_str(), primary->descr().c_str());

        if (print_recreate_note) {
          current_console()->print_note(note);
        } else {
          log_debug("%s", note.c_str());
        }

        if (!dry_run) {
          primary->drop_user(creds.user, hostname);
        }
      }
    }

    mysqlshdk::gr::Create_recovery_user_options user_options;
    {
      // Check if clone is available on ALL cluster members, to avoid a failing
      // SQL query because BACKUP_ADMIN is not supported
      // Do the same for MySQL commStack, being the grant
      // GROUP_REPLICATION_STREAM
      auto lowest_version = cluster.get_lowest_instance_version();

      auto clone_available_all_members = supports_mysql_clone(lowest_version);
      auto mysql_comm_stack_available_all_members =
          supports_mysql_communication_stack(lowest_version);

      auto target_version = target.get_version();

      user_options.clone_supported =
          clone_available_all_members && supports_mysql_clone(target_version);

      if (account_type == Replication_account::Account_type::READ_REPLICA) {
        user_options.auto_failover = true;
      } else {
        user_options.auto_failover = false;
        user_options.mysql_comm_stack_supported =
            mysql_comm_stack_available_all_members &&
            supports_mysql_communication_stack(target_version);
      }
    }

    // setup password, cert issuer and/or subject
    auto auth_type = cluster.query_cluster_auth_type();
    validate_instance_member_auth_options("cluster", false, auth_type,
                                          auth_cert_subject);

    switch (auth_type) {
      case Replication_auth_type::PASSWORD:
      case Replication_auth_type::CERT_ISSUER_PASSWORD:
      case Replication_auth_type::CERT_SUBJECT_PASSWORD:
        user_options.requires_password = true;
        user_options.password = creds.password;
        break;
      default:
        user_options.requires_password = false;
        break;
    }

    switch (auth_type) {
      case Replication_auth_type::CERT_SUBJECT:
      case Replication_auth_type::CERT_SUBJECT_PASSWORD:
        user_options.cert_subject = auth_cert_subject;
        [[fallthrough]];
      case Replication_auth_type::CERT_ISSUER:
      case Replication_auth_type::CERT_ISSUER_PASSWORD:
        user_options.cert_issuer = cluster.query_cluster_auth_cert_issuer();
        break;
      default:
        break;
    }

    if (dry_run) {
      //"simulate" response
      mysqlshdk::mysql::Auth_options ret_creds;
      ret_creds.password = user_options.password;
      ret_creds.user = creds.user;

      return {std::move(ret_creds), std::move(host)};
    }

    // create user

    std::vector<std::string> hosts;
    hosts.push_back(host);

    return {mysqlshdk::gr::create_recovery_user(creds.user, *primary, hosts,
                                                user_options),
            std::move(host)};
  }

  // for ReplicaSets
  if (topo_holds<Replica_set_impl>()) {
    const auto &rset = topo_as<Replica_set_impl>();

    auto master = rset.get_primary_master().get();

    std::string host = "%";
    if (shcore::Value allowed_host;
        !dry_run &&
        rset.get_metadata_storage()->query_cluster_attribute(
            rset.get_id(), k_cluster_attribute_replication_allowed_host,
            &allowed_host) &&
        allowed_host.get_type() == shcore::String &&
        !allowed_host.as_string().empty()) {
      host = allowed_host.as_string();
    }

    creds = mysqlshdk::mysql::Auth_options{};
    creds.user = Replication_account::make_replication_user_name(
        target.get_server_id(),
        Replication_account::k_async_recovery_user_prefix);

    try {
      // Create replication accounts for this instance at the master
      // replicaset unless the user provided one.

      // Accounts are created at the master replicaset regardless of who will
      // use them, since they'll get replicated everywhere.

      log_info("Creating replication user %s@%s at '%s'%s", creds.user.c_str(),
               host.c_str(), master->descr().c_str(),
               dry_run ? " (dryRun)" : "");

      if (dry_run) return {creds, host};

      // re-create replication with a new generated password

      // Check if the replication user already exists and delete it if it does,
      // before creating it again.
      mysqlshdk::mysql::drop_all_accounts_for_user(*master, creds.user);

      mysqlshdk::mysql::IInstance::Create_user_options user_options;
      user_options.grants.push_back({"REPLICATION SLAVE", "*.*", false});

      auto auth_type = rset.query_cluster_auth_type();

      // setup password
      bool requires_password{false};
      switch (auth_type) {
        case Replication_auth_type::PASSWORD:
        case Replication_auth_type::CERT_ISSUER_PASSWORD:
        case Replication_auth_type::CERT_SUBJECT_PASSWORD:
          requires_password = true;
          break;
        default:
          break;
      }

      // setup cert issuer and/or subject
      switch (auth_type) {
        case Replication_auth_type::CERT_SUBJECT:
        case Replication_auth_type::CERT_SUBJECT_PASSWORD:
          user_options.cert_subject = auth_cert_subject;
          [[fallthrough]];
        case Replication_auth_type::CERT_ISSUER:
        case Replication_auth_type::CERT_ISSUER_PASSWORD:
          user_options.cert_issuer = rset.query_cluster_auth_cert_issuer();
          break;
        default:
          break;
      }

      if (requires_password) {
        std::string repl_password;
        mysqlshdk::mysql::create_user_with_random_password(
            *master, creds.user, {host}, user_options, &repl_password);

        creds.password = std::move(repl_password);
      } else {
        mysqlshdk::mysql::create_user(*master, creds.user, {host},
                                      user_options);
      }

    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Error while setting up replication account: %s", e.what()));
    }

    return {creds, host};
  }

  assert(!"Unknown topology type");
  throw std::logic_error("Unknown topology type");
}

Replication_account::Auth_options_host
Replication_account::create_cluster_replication_user(
    const mysqlshdk::mysql::IInstance &target, std::string_view account_host,
    Replication_auth_type member_auth_type, std::string_view auth_cert_issuer,
    std::string_view auth_cert_subject, bool dry_run) {
  assert(topo_holds<Cluster_set_impl>());
  if (!topo_holds<Cluster_set_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cset = topo_as<Cluster_set_impl>();

  // we use server_id as an arbitrary unique number within the clusterset. Once
  // the user is created, no relationship between the server_id and the number
  // should be assumed. The only way to obtain the replication user name for a
  // cluster is through the metadata.
  auto repl_user = Replication_account::make_replication_user_name(
      target.get_server_id(), k_cluster_set_async_user_name, true);

  std::string repl_host("%");
  if (account_host.empty()) {
    shcore::Value allowed_host;
    if (cset.get_metadata_storage()->query_cluster_set_attribute(
            cset.get_id(), k_cluster_attribute_replication_allowed_host,
            &allowed_host) &&
        allowed_host.get_type() == shcore::String &&
        !allowed_host.as_string().empty()) {
      repl_host = allowed_host.as_string();
    }
  } else {
    repl_host = account_host;
  }

  log_info(
      "Creating async replication account '%s'@'%s' for new cluster at '%s'",
      repl_user.c_str(), repl_host.c_str(), target.descr().c_str());

  auto primary = cset.get_metadata_storage()->get_md_server();

  if (dry_run) {
    mysqlshdk::mysql::Auth_options repl_auth;
    repl_auth.user = std::move(repl_user);

    return {std::move(repl_auth), std::move(repl_host)};
  }
  // If the user already exists, it may have a different password causing an
  // authentication error. Also, it may have the wrong or incomplete set of
  // required grants. For those reasons, we must simply drop the account if
  // already exists to ensure a new one is created.
  mysqlshdk::mysql::drop_all_accounts_for_user(*primary, repl_user);

  std::vector<std::string> hosts;
  hosts.push_back(repl_host);

  mysqlshdk::gr::Create_recovery_user_options options;
  options.clone_supported = true;
  options.auto_failover = true;

  switch (member_auth_type) {
    case Replication_auth_type::PASSWORD:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      options.requires_password = true;
      break;
    default:
      options.requires_password = false;
      break;
  }

  switch (member_auth_type) {
    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      options.cert_subject = auth_cert_subject;
      [[fallthrough]];
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      options.cert_issuer = auth_cert_issuer;
      break;
    default:
      break;
  }

  auto repl_auth =
      mysqlshdk::gr::create_recovery_user(repl_user, *primary, hosts, options);

  return {std::move(repl_auth), std::move(repl_host)};
}

Replication_account::User_host Replication_account::recreate_replication_user(
    const mysqlshdk::mysql::IInstance &target,
    std::string_view auth_cert_subject, bool dry_run) const {
  auto repl_account = create_replication_user(
      target, auth_cert_subject,
      Replication_account::Account_type::GROUP_REPLICATION, false, {}, false,
      dry_run);

  mysqlshdk::mysql::Replication_credentials_options options;
  options.password = repl_account.auth.password.value_or("");

  mysqlshdk::mysql::change_replication_credentials(
      target, mysqlshdk::gr::k_gr_recovery_channel, repl_account.auth.user,
      options);

  return {repl_account.auth.user, repl_account.host};
}

void Replication_account::drop_all_replication_users() {
  // for Clusters
  if (topo_holds<Cluster_impl>()) {
    auto primary = topo_as<Cluster_impl>().get_primary_master();
    assert(primary);

    auto rec_prefix =
        std::string(Replication_account::k_group_recovery_user_prefix) + "%";
    auto cs_prefix = std::string(k_cluster_set_async_user_name) + "%";
    auto read_replica_prefix =
        std::string(k_cluster_read_replica_user_name) + "%";

    auto result = primary->queryf(
        "SELECT DISTINCT user from mysql.user WHERE user LIKE ? OR user LIKE ? "
        "OR user LIKE ?",
        rec_prefix, cs_prefix, read_replica_prefix);

    std::vector<std::string> accounts_to_drop;
    while (auto row = result->fetch_one()) {
      accounts_to_drop.push_back(row->get_string(0));
    }

    for (const auto &account : accounts_to_drop) {
      log_info("Removing account '%s'", account.c_str());
      try {
        mysqlshdk::mysql::drop_all_accounts_for_user(*primary, account);
      } catch (const std::exception &e) {
        mysqlsh::current_console()->print_warning(
            shcore::str_format("Error dropping accounts for user '%s': %s",
                               account.c_str(), e.what()));
      }
    }

    return;
  }

  // for ReplicaSets
  if (topo_holds<Replica_set_impl>()) {
    auto primary = topo_as<Replica_set_impl>().get_primary_master();
    assert(primary);

    auto rs_prefix =
        std::string(Replication_account::k_async_recovery_user_prefix) + "%";

    auto result = primary->queryf(
        "SELECT DISTINCT user from mysql.user WHERE user LIKE ?", rs_prefix);

    std::vector<std::string> accounts_to_drop;
    while (auto row = result->fetch_one())
      accounts_to_drop.push_back(row->get_string(0));

    for (const auto &account : accounts_to_drop) {
      log_info("Removing account '%s'", account.c_str());
      try {
        mysqlshdk::mysql::drop_all_accounts_for_user(*primary, account);
      } catch (const std::exception &e) {
        mysqlsh::current_console()->print_warning(
            shcore::str_format("Error dropping accounts for user '%s': %s",
                               account.c_str(), e.what()));
      }
    }

    return;
  }

  assert(!"Unknown topology type");
  throw std::logic_error("Unknown topology type");
}

void Replication_account::ensure_metadata_has_recovery_accounts(
    bool allow_non_standard_format) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  auto endpoints =
      cluster.get_metadata_storage()->get_instances_with_recovery_accounts(
          cluster.get_id());

  if (endpoints.empty()) return;

  log_info("Fixing instances missing the replication recovery account...");

  auto console = mysqlsh::current_console();

  cluster.execute_in_members(
      {}, cluster.get_cluster_server()->get_connection_options(), {},
      [this, &console, &endpoints, &allow_non_standard_format, &cluster](
          const std::shared_ptr<Instance> &instance,
          const mysqlshdk::gr::Member &gr_member) {
        auto recovery_user = mysqlshdk::mysql::get_replication_user(
            *instance, mysqlshdk::gr::k_gr_recovery_channel);

        log_debug("Fixing recovering account '%s' in instance '%s'",
                  recovery_user.c_str(), instance->descr().c_str());

        bool recovery_is_valid = allow_non_standard_format;
        if (!recovery_is_valid && !recovery_user.empty()) {
          recovery_is_valid =
              shcore::str_beginswith(
                  recovery_user,
                  Replication_account::k_group_recovery_old_user_prefix) ||
              shcore::str_beginswith(
                  recovery_user,
                  Replication_account::k_group_recovery_user_prefix);
        }

        if (recovery_user.empty())
          throw std::logic_error(shcore::str_format(
              "Recovery user account not found for server address '%s' with "
              "UUID %s",
              instance->descr().c_str(), instance->get_uuid().c_str()));

        if (!recovery_is_valid) {
          console->print_error(shcore::str_format(
              "Unsupported recovery account '%s' has been found for instance "
              "'%s'. Operations such as "
              "<Cluster>.<<<resetRecoveryAccountsPassword>>>() and "
              "<Cluster>.<<<addInstance>>>() may fail. Please remove and add "
              "the instance back to the Cluster to ensure a supported "
              "recovery account is used.",
              recovery_user.c_str(), instance->descr().c_str()));
          return true;
        }

        auto it = endpoints.find(instance->get_uuid());
        if ((it == endpoints.end()) || (it->second == recovery_user))
          return true;

        cluster.get_metadata_storage()->update_instance_repl_account(
            gr_member.uuid, Cluster_type::GROUP_REPLICATION,
            Replica_type::GROUP_MEMBER, recovery_user,
            get_replication_user_host());

        return true;
      },
      [](const Cluster_impl::Instance_md_and_gr_member &md) {
        return (md.second.state == mysqlshdk::gr::Member_state::ONLINE ||
                md.second.state == mysqlshdk::gr::Member_state::RECOVERING);
      });
}

void Replication_account::update_replication_allowed_host(
    const std::string &host) {
  // for Clusters
  if (topo_holds<Cluster_impl>()) {
    const auto &cluster = topo_as<Cluster_impl>();

    bool upgraded = false;
    auto primary = cluster.get_primary_master();

  retry:
    for (const Instance_metadata &instance : cluster.get_instances()) {
      auto account = cluster.get_metadata_storage()->get_instance_repl_account(
          instance.uuid, Cluster_type::GROUP_REPLICATION,
          Replica_type::GROUP_MEMBER);

      if (account.first.empty()) {
        if (upgraded)
          throw shcore::Exception::runtime_error(
              "Unable to perform account management upgrade for the cluster.");

        upgraded = true;
        current_console()->print_note(
            "Legacy cluster account management detected, will update it "
            "first.");
        ensure_metadata_has_recovery_accounts(false);
        goto retry;

      } else if (account.second != host) {
        log_info("Re-creating account for %s: %s@%s -> %s@%s",
                 instance.endpoint.c_str(), account.first.c_str(),
                 account.second.c_str(), account.first.c_str(), host.c_str());
        clone_user(*primary, account.first, account.second, account.first,
                   host);

        // drop all (other) hosts in case the account was created at an old
        // version with multiple accounts per user
        auto hosts = mysqlshdk::mysql::get_all_hostnames_for_user(
            *primary, account.first);
        for (const auto &h : hosts) {
          if (host != h) primary->drop_user(account.first, h, true);
        }

        cluster.get_metadata_storage()->update_instance_repl_account(
            instance.uuid, Cluster_type::GROUP_REPLICATION,
            Replica_type::GROUP_MEMBER, account.first, host);
      } else {
        log_info("Skipping account recreation for %s: %s@%s == %s@%s",
                 instance.endpoint.c_str(), account.first.c_str(),
                 account.second.c_str(), account.first.c_str(), host.c_str());
      }
    }

    return;
  }

  // for ClusterSets
  if (topo_holds<Cluster_set_impl>()) {
    const auto &cset = topo_as<Cluster_set_impl>();

    for (const Cluster_set_member_metadata &cluster : cset.get_clusters()) {
      auto account = cset.get_metadata_storage()->get_cluster_repl_account(
          cluster.cluster.cluster_id);

      if (account.second == host) {
        log_info("Skipping account recreation for cluster %s: %s@%s == %s@%s",
                 cluster.cluster.cluster_name.c_str(), account.first.c_str(),
                 account.second.c_str(), account.first.c_str(), host.c_str());
        continue;
      }

      auto primary = cset.get_primary_master();

      log_info("Re-creating account for cluster %s: %s@%s -> %s@%s",
               cluster.cluster.cluster_name.c_str(), account.first.c_str(),
               account.second.c_str(), account.first.c_str(), host.c_str());
      clone_user(*primary, account.first, account.second, account.first, host);

      primary->drop_user(account.first, account.second, true);

      cset.get_metadata_storage()->update_cluster_repl_account(
          cluster.cluster.cluster_id, account.first, host);
    }

    return;
  }

  // for ReplicaSets
  if (topo_holds<Replica_set_impl>()) {
    const auto &rset = topo_as<Replica_set_impl>();

    for (const Instance_metadata &instance :
         rset.get_instances_from_metadata()) {
      auto account = rset.get_metadata_storage()->get_instance_repl_account(
          instance.uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);
      bool maybe_adopted = false;

      if (account.first.empty()) {
        account = {Replication_account::make_replication_user_name(
                       instance.server_id,
                       Replication_account::k_async_recovery_user_prefix),
                   "%"};
        maybe_adopted = true;
      }

      auto primary = rset.get_primary_master();

      if (account.second != host) {
        log_info("Re-creating account for %s: %s@%s -> %s@%s",
                 instance.endpoint.c_str(), account.first.c_str(),
                 account.second.c_str(), account.first.c_str(), host.c_str());
        clone_user(*primary, account.first, account.second, account.first,
                   host);

        primary->drop_user(account.first, account.second, true);

        rset.get_metadata_storage()->update_instance_repl_account(
            instance.uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE,
            account.first, host);
      } else {
        log_info("Skipping account recreation for %s: %s@%s == %s@%s",
                 instance.endpoint.c_str(), account.first.c_str(),
                 account.second.c_str(), account.first.c_str(), host.c_str());
      }

      if (maybe_adopted) {
        auto ipool = current_ipool();

        Scoped_instance target(
            ipool->connect_unchecked_endpoint(instance.endpoint));

        // If the replicaset could have been adopted, then also ensure the
        // channel is using the right account
        std::string repl_user = mysqlshdk::mysql::get_replication_user(
            *target, k_replicaset_channel_name);
        if (repl_user != account.first && !instance.primary_master) {
          current_console()->print_info(shcore::str_format(
              "Changing replication user at %s to %s",
              instance.endpoint.c_str(), account.first.c_str()));

          std::string password;
          mysqlshdk::mysql::set_random_password(*primary, account.first, {host},
                                                &password);

          mysqlshdk::mysql::stop_replication_receiver(
              *target, k_replicaset_channel_name);

          mysqlshdk::mysql::Replication_credentials_options options;
          options.password = password;

          mysqlshdk::mysql::change_replication_credentials(
              *target, k_replicaset_channel_name, account.first, options);

          mysqlshdk::mysql::start_replication_receiver(
              *target, k_replicaset_channel_name);
        }
      }
    }

    return;
  }

  assert(!"Unknown topology type");
  throw std::logic_error("Unknown topology type");
}

void Replication_account::restore_recovery_account_all_members(
    bool reset_password) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  // if we don't need to reset the password (for example the recovery accounts
  // only use certificates), all we need to do is to update the replication
  // credentials
  if (!reset_password) {
    log_info("Restoring recovery accounts with certificates only.");

    cluster.execute_in_members(
        [&cluster](const std::shared_ptr<Instance> &instance,
                   const Cluster_impl::Instance_md_and_gr_member &info) {
          if (info.second.state != mysqlshdk::gr::Member_state::ONLINE)
            return true;

          auto recovery_user =
              cluster.get_metadata_storage()->get_instance_repl_account_user(
                  instance->get_uuid(), Cluster_type::GROUP_REPLICATION,
                  Replica_type::GROUP_MEMBER);

          mysqlshdk::mysql::Replication_credentials_options options;

          mysqlshdk::mysql::change_replication_credentials(
              *instance, mysqlshdk::gr::k_gr_recovery_channel, recovery_user,
              options);

          return true;
        },
        [](const shcore::Error &,
           const Cluster_impl::Instance_md_and_gr_member &) { return true; });

    return;
  }

  cluster.execute_in_members(
      [this, &cluster](const std::shared_ptr<Instance> &instance,
                       const Cluster_impl::Instance_md_and_gr_member &info) {
        if (info.second.state != mysqlshdk::gr::Member_state::ONLINE)
          return true;

        try {
          cluster.reset_recovery_password(instance);
        } catch (const std::exception &e) {
          // If we can't change the recovery account password for some
          // reason, we must re-create it
          log_info(
              "Unable to reset the recovery password of instance '%s', "
              "re-creating a new account: %s",
              instance->descr().c_str(), e.what());

          auto cert_subject =
              cluster.query_cluster_instance_auth_cert_subject(*instance);

          recreate_replication_user(*instance, cert_subject);
        }

        return true;
      },
      [](const shcore::Error &,
         const Cluster_impl::Instance_md_and_gr_member &) { return true; });
}

void Replication_account::create_local_replication_user(
    const mysqlshdk::mysql::IInstance &target,
    std::string_view auth_cert_subject,
    const Group_replication_options &gr_options,
    bool propagate_credentials_donors, bool dry_run) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  // When using the 'MySQL' communication stack, the account must already
  // exist in the joining member since authentication is based on MySQL
  // credentials so the account must exist in both ends before GR
  // starts. For those reasons, we must create the account with binlog
  // disabled before starting GR, otherwise we'd create errant
  // transaction
  mysqlshdk::mysql::Suppress_binary_log nobinlog(target);

  if (target.get_sysvar_bool("super_read_only", false) && !dry_run) {
    target.set_sysvar("super_read_only", false);
  }

  auto repl_account = create_replication_user(
      target, auth_cert_subject,
      Replication_account::Account_type::GROUP_REPLICATION, true,
      gr_options.recovery_credentials.value_or(
          mysqlshdk::mysql::Auth_options{}),
      dry_run);

  // Change GR's recovery replication credentials in all possible
  // donors so they are able to connect to the joining member. GR will pick a
  // suitable donor (that we cannot determine) and it needs to be able to
  // connect and authenticate to the joining member.
  // NOTE: Instances in RECOVERING must be skipped since won't be used as donor
  // and the change source command would fail anyway
  if (propagate_credentials_donors && !dry_run)
    cluster.change_recovery_credentials_all_members(repl_account.auth);

  DBUG_EXECUTE_IF("fail_recovery_mysql_stack", {
    // Revoke the REPLICATION_SLAVE to make the distributed recovery fail
    auto primary = cluster.get_primary_master();
    primary->execute("REVOKE REPLICATION SLAVE on *.* from " +
                     repl_account.auth.user);
    target.execute("REVOKE REPLICATION SLAVE on *.* from " +
                   repl_account.auth.user);
  });
}

void Replication_account::create_replication_users_at_instance(
    const mysqlshdk::mysql::IInstance &target) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  // When using the 'MySQL' communication stack and the recovery accounts
  // require certificates, we need to make sure that every account for all
  // current members also exists in the target (new) instance. To avoid having
  // errant transactions when GR starts, we do this with binlog disabled.
  mysqlshdk::mysql::Suppress_binary_log nobinlog(target);

  if (target.get_sysvar_bool("super_read_only", false))
    target.set_sysvar("super_read_only", false);

  cluster.execute_in_members(
      [this, &target](const std::shared_ptr<Instance> &instance,
                      const Cluster_impl::Instance_md_and_gr_member &info) {
        if ((info.second.state != mysqlshdk::gr::Member_state::ONLINE) &&
            (info.second.state != mysqlshdk::gr::Member_state::RECOVERING))
          return true;

        auto user_hosts = get_replication_user(*instance, false);

        for (const auto &host : user_hosts.hosts) {
          if (target.user_exists(user_hosts.user, host)) {
            log_info(
                "User '%s'@'%s' already existed at instance '%s'. It will be "
                "deleted and created again.",
                user_hosts.user.c_str(), host.c_str(), target.descr().c_str());

            target.drop_user(user_hosts.user, host);
          }

          log_info("Copying user '%s'@'%s' to instance '%s'.",
                   user_hosts.user.c_str(), host.c_str(),
                   target.descr().c_str());
          clone_user(*instance, target, user_hosts.user, host);
        }

        return true;
      },
      [](const shcore::Error &,
         const Cluster_impl::Instance_md_and_gr_member &) { return true; });
}

std::string Replication_account::create_recovery_account(
    const mysqlshdk::mysql::IInstance &primary,
    const mysqlshdk::mysql::IInstance &target,
    const Replication_auth_options &repl_auth_options,
    std::string_view repl_allowed_host) {
  auto repl_account_host =
      repl_allowed_host.empty() ? std::string_view{"%"} : repl_allowed_host;

  mysqlshdk::mysql::Auth_options repl_account;
  repl_account.user = Replication_account::make_replication_user_name(
      target.get_server_id(),
      Replication_account::k_group_recovery_user_prefix);

  log_info("Creating recovery account '%s'@'%.*s' for instance '%s'",
           repl_account.user.c_str(),
           static_cast<int>(repl_account_host.size()), repl_account_host.data(),
           target.descr().c_str());

  auto target_version = target.get_version();

  mysqlshdk::gr::Create_recovery_user_options user_options;
  user_options.clone_supported = supports_mysql_clone(target_version);
  user_options.auto_failover = false;
  user_options.mysql_comm_stack_supported =
      supports_mysql_communication_stack(target_version);

  switch (repl_auth_options.member_auth_type) {
    case Replication_auth_type::PASSWORD:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      user_options.requires_password = true;
      break;
    default:
      user_options.requires_password = false;
      break;
  }

  switch (repl_auth_options.member_auth_type) {
    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      user_options.cert_subject = repl_auth_options.cert_subject;
      [[fallthrough]];
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      user_options.cert_issuer = repl_auth_options.cert_issuer;
      break;
    default:
      break;
  }

  if (target.user_exists(repl_account.user, repl_account_host)) {
    auto pwd_section =
        user_options.requires_password ? " with a new password" : "";
    mysqlsh::current_console()->print_note(shcore::str_format(
        "User '%s'@'%.*s' already existed at instance '%s'. It will be deleted "
        "and created again%s.",
        repl_account.user.c_str(), static_cast<int>(repl_account_host.size()),
        repl_account_host.data(), target.descr().c_str(), pwd_section));

    primary.drop_user(repl_account.user, repl_account_host);
  }

  repl_account = mysqlshdk::gr::create_recovery_user(
      repl_account.user, primary, {std::string{repl_account_host}},
      user_options);

  // Configure GR's recovery channel with the newly created account
  log_debug("Changing GR recovery user details for '%s'",
            target.descr().c_str());

  try {
    mysqlshdk::mysql::Replication_credentials_options options;
    options.password = repl_account.password.value_or("");

    mysqlshdk::mysql::change_replication_credentials(
        target, mysqlshdk::gr::k_gr_recovery_channel, repl_account.user,
        options);
  } catch (const std::exception &e) {
    current_console()->print_warning(
        shcore::str_format("Error updating recovery credentials for %s: %s",
                           target.descr().c_str(), e.what()));
  }

  return repl_account.user;
}

std::string Replication_account::create_recovery_account(
    const mysqlshdk::mysql::IInstance &target_instance,
    std::string_view auth_cert_subject) {
  // create the new replication user
  auto auth_host = create_replication_user(
      target_instance, auth_cert_subject,
      Replication_account::Account_type::GROUP_REPLICATION);

  // Configure GR's recovery channel with the newly created account
  log_debug("Changing GR recovery user details for '%s'",
            target_instance.descr().c_str());

  try {
    mysqlshdk::mysql::Replication_credentials_options options;
    options.password = auth_host.auth.password.value_or("");

    mysqlshdk::mysql::change_replication_credentials(
        target_instance, mysqlshdk::gr::k_gr_recovery_channel,
        auth_host.auth.user, options);
  } catch (const std::exception &e) {
    current_console()->print_warning(
        shcore::str_format("Error updating recovery credentials for '%s': %s",
                           target_instance.descr().c_str(), e.what()));

    drop_replication_user(&target_instance, {}, {}, 0, false);
  }

  return auth_host.auth.user;
}

Replication_account::User_host Replication_account::recreate_recovery_account(
    const mysqlshdk::mysql::IInstance &primary,
    const mysqlshdk::mysql::IInstance &target,
    const std::string_view auth_cert_subject) {
  // Get the recovery account
  auto user_hosts = get_replication_user(target, false);

  // Recovery account is coming from the Metadata
  const auto &host = user_hosts.hosts.front();

  // Ensure the replication user is deleted if exists before creating it
  // again
  primary.drop_user(user_hosts.user, host);

  log_debug("Re-creating temporary recovery user '%s'@'%s' at instance '%s'.",
            user_hosts.user.c_str(), host.c_str(), primary.descr().c_str());

  return recreate_replication_user(target, auth_cert_subject);
}

bool Replication_account::drop_replication_user(
    const mysqlshdk::mysql::IInstance *target, std::string endpoint,
    std::string server_uuid, uint32_t server_id, bool dry_run) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  assert(target || (!endpoint.empty() && !server_uuid.empty()));

  const auto &cluster = topo_as<Cluster_impl>();
  auto primary = cluster.get_primary_master();

  std::string recovery_user, recovery_user_host;
  bool from_metadata = false;

  // Get the endpoint, uuid and server_id if empty
  if (target) {
    if (endpoint.empty()) {
      endpoint = target->get_canonical_address();
    }
    if (server_uuid.empty()) {
      server_uuid = target->get_uuid();
    }
    if (server_id == 0) {
      server_id = target->get_id();
    }
  }

  // Check if the recovery account created for the instance when it
  // joined the cluster, is in use by any other member. If not, it is a
  // leftover that must be dropped. This scenario happens when an instance is
  // added to a cluster using clone as the recovery method but waitRecovery is
  // zero or the user cancelled the monitoring of the recovery so the instance
  // will use the seed's recovery account leaving its own account unused.
  //
  // TODO(anyone): BUG#30031815 requires that we skip if the primary instance
  // is being removed. It causes a primary election and we cannot know which
  // instance will be the primary to perform the cleanup
  if (!mysqlshdk::utils::are_endpoints_equal(
          endpoint, primary->get_canonical_address())) {
    // Generate the recovery account username that was created for the
    // instance
    auto user = Replication_account::make_replication_user_name(
        server_id, Replication_account::k_group_recovery_user_prefix);

    // Get the Cluster's allowedHost
    std::string host = "%";

    if (shcore::Value allowed_host;
        cluster.get_metadata_storage()->query_cluster_attribute(
            cluster.get_id(), k_cluster_attribute_replication_allowed_host,
            &allowed_host) &&
        allowed_host.get_type() == shcore::String &&
        !allowed_host.as_string().empty()) {
      host = allowed_host.as_string();
    }

    // The account must not be in use by any other member of the cluster
    if (cluster.get_metadata_storage()->count_recovery_account_uses(user) ==
        0) {
      log_info("Dropping recovery account '%s'@'%s' for instance '%.*s'",
               user.c_str(), host.c_str(), static_cast<int>(endpoint.length()),
               endpoint.data());

      if (!dry_run) {
        try {
          if (m_undo.has_value()) {
            m_undo->add_snapshot_for_drop_user(*cluster.get_primary_master(),
                                               user, host);
          }
          cluster.get_primary_master()->drop_user(user, host, true);
        } catch (...) {
          mysqlsh::current_console()->print_warning(shcore::str_format(
              "Error dropping recovery account '%s'@'%s' for instance '%.*s': "
              "%s",
              user.c_str(), host.c_str(), static_cast<int>(endpoint.length()),
              endpoint.data(), format_active_exception().c_str()));
        }
      }
    } else {
      log_info(
          "Recovery account '%s'@'%s' for instance '%.*s' still in use in "
          "the Cluster: Skipping its removal",
          user.c_str(), host.c_str(), static_cast<int>(endpoint.length()),
          endpoint.data());
    }
  }

  if (target) {
    // Attempt to get the recovery account
    try {
      auto user_hosts = get_replication_user(*target, false);

      recovery_user = std::move(user_hosts.user);
      from_metadata = user_hosts.from_md;

      if (from_metadata) {
        recovery_user_host = std::exchange(user_hosts.hosts.front(), {});
      }
    } catch (const std::exception &) {
      mysqlsh::current_console()->print_note(shcore::str_format(
          "The recovery user name for instance '%s' does not match the "
          "expected format for users created automatically by InnoDB Cluster. "
          "Skipping its removal.",
          target->descr().c_str()));
    }

  } else {
    // Get the user from the Metadata
    std::tie(recovery_user, recovery_user_host) =
        cluster.get_metadata_storage()->get_instance_repl_account(
            server_uuid, Cluster_type::GROUP_REPLICATION,
            Replica_type::GROUP_MEMBER);

    if (recovery_user.empty()) {
      log_warning("Could not obtain recovery account metadata for '%.*s'",
                  static_cast<int>(endpoint.length()), endpoint.data());
      return false;
    }

    from_metadata = true;
  }

  // Drop the recovery account
  if (!from_metadata) {
    log_info("Removing replication user '%s'", recovery_user.c_str());
    if (!dry_run) {
      try {
        auto hosts = mysqlshdk::mysql::get_all_hostnames_for_user(
            *primary, recovery_user);

        for (const auto &host : hosts) {
          if (m_undo.has_value()) {
            m_undo->add_snapshot_for_drop_user(*primary, recovery_user, host);
          }
          primary->drop_user(recovery_user, host, true);
        }
      } catch (const std::exception &e) {
        mysqlsh::current_console()->print_warning(shcore::str_format(
            "Error dropping recovery accounts for user %s: %s",
            recovery_user.c_str(), e.what()));
        return false;
      }
    }

    return true;
  }

  /*
  Since clone copies all the data, including mysql.slave_master_info
  (where the CHANGE MASTER credentials are stored) an instance may be
  using the info stored in the donor's mysql.slave_master_info.

  To avoid such situation, we re-issue the CHANGE MASTER query after
  clone to ensure the right account is used. However, if the monitoring
  process is interrupted, that won't be done.

  The approach to tackle this issue is to store the donor recovery account
  in the target instance MD.instances table before doing the new CHANGE
  and only update it to the right account after a successful CHANGE
  MASTER. With this approach we can ensure that the account is not removed
  if it is being used.

  As so were we must query the Metadata to check whether the
  recovery user of that instance is registered for more than one instance
  and if that's the case then it won't be dropped.
  */
  if (cluster.get_metadata_storage()->count_recovery_account_uses(
          recovery_user) != 1)
    return true;

  log_info("Dropping recovery account '%s'@'%s' for instance '%.*s'",
           recovery_user.c_str(), recovery_user_host.c_str(),
           static_cast<int>(endpoint.length()), endpoint.data());

  if (!dry_run) {
    try {
      if (m_undo.has_value()) {
        m_undo->add_snapshot_for_drop_user(*primary, recovery_user,
                                           recovery_user_host);
      }
      primary->drop_user(recovery_user, recovery_user_host, true);
    } catch (...) {
      mysqlsh::current_console()->print_warning(shcore::str_format(
          "Error dropping recovery account '%s'@'%s' for instance '%.*s': %s",
          recovery_user.c_str(), recovery_user_host.c_str(),
          static_cast<int>(endpoint.length()), endpoint.data(),
          format_active_exception().c_str()));
    }

    // Also update metadata
    try {
      auto trx_undo = Transaction_undo::create();

      cluster.get_metadata_storage()->update_instance_repl_account(
          server_uuid, Cluster_type::GROUP_REPLICATION,
          Replica_type::GROUP_MEMBER, "", "", trx_undo.get());

      if (m_undo.has_value()) m_undo->add(std::move(trx_undo));
    } catch (const std::exception &e) {
      log_warning("Could not update recovery account metadata for '%.*s': %s",
                  static_cast<int>(endpoint.length()), endpoint.data(),
                  e.what());
    }
  }

  return true;
}

void Replication_account::drop_replication_user(const Cluster_impl &cluster) {
  assert(topo_holds<Cluster_set_impl>());
  if (!topo_holds<Cluster_set_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cset = topo_as<Cluster_set_impl>();

  auto [repl_user, repl_host] =
      cset.get_metadata_storage()->get_cluster_repl_account(cluster.get_id());

  /*
    Since clone copies all the data, including mysql.slave_master_info (where
    the CHANGE MASTER credentials are stored) an instance may be using the
    info stored in the donor's mysql.slave_master_info.

    To avoid such situation, we re-issue the CHANGE MASTER query after
    clone to ensure the right account is used. However, if the monitoring
    process is interrupted or waitRecovery = 0, that won't be done.

    The approach to tackle this issue is to store the donor recovery account
    in the target instance MD.instances table before doing the new CHANGE
    and only update it to the right account after a successful CHANGE MASTER.
    With this approach we can ensure that the account is not removed if it is
    being used.

   As so were we must query the Metadata to check whether the
    recovery user of that instance is registered for more than one instance
    and if that's the case then it won't be dropped.
  */
  if (cset.get_metadata_storage()->count_recovery_account_uses(repl_user,
                                                               true) != 1)
    return;

  auto primary = cset.get_metadata_storage()->get_md_server();
  log_info("Dropping replication account '%s'@'%s' for cluster '%s'",
           repl_user.c_str(), repl_host.c_str(), cluster.get_name().c_str());

  try {
    if (m_undo)
      m_undo->add_snapshot_for_drop_user(*primary, repl_user, repl_host);

    primary->drop_user(repl_user, repl_host.c_str(), true);
  } catch (const std::exception &) {
    current_console()->print_warning(shcore::str_format(
        "Error dropping replication account '%s'@'%s' for cluster '%s'",
        repl_user.c_str(), repl_host.c_str(), cluster.get_name().c_str()));
  }

  // Also update metadata
  try {
    Transaction_undo *trx_undo = nullptr;
    if (m_undo) {
      auto u = Transaction_undo::create();
      trx_undo = u.get();
      m_undo->add(std::move(u));
    }

    cset.get_metadata_storage()->update_cluster_repl_account(cluster.get_id(),
                                                             "", "", trx_undo);
  } catch (const std::exception &e) {
    log_warning("Could not update replication account metadata for '%s': %s",
                cluster.get_name().c_str(), e.what());
  }
}

void Replication_account::drop_replication_user(
    std::string_view server_uuid, const mysqlshdk::mysql::IInstance *target) {
  assert(topo_holds<Replica_set_impl>());
  if (!topo_holds<Replica_set_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &rset = topo_as<Replica_set_impl>();

  auto primary = rset.get_primary_master();
  assert(primary);

  auto account = rset.get_metadata_storage()->get_instance_repl_account(
      server_uuid, Cluster_type::ASYNC_REPLICATION, Replica_type::NONE);

  if (account.first.empty()) {
    if (target) {
      account = {Replication_account::make_replication_user_name(
                     target->get_server_id(),
                     Replication_account::k_async_recovery_user_prefix),
                 ""};
    } else {
      // If the instance is unreachable and the replication account info is not
      // stored in the MD schema we cannot attempt to obtain the instance's
      // server_id to determine the account username. In this particular
      // scenario, we must just log that the account couldn't be removed
      log_info(
          "Unable to drop instance's replication account from the ReplicaSet: "
          "Instance '%.*s' is unreachable, unable to determine its replication "
          "account.",
          static_cast<int>(server_uuid.length()), server_uuid.data());
    }
  }

  log_info("Dropping account %s@%s at %s", account.first.c_str(),
           account.second.c_str(), primary->descr().c_str());
  try {
    if (account.second.empty())
      mysqlshdk::mysql::drop_all_accounts_for_user(*primary, account.first);
    else
      primary->drop_user(account.first, account.second, true);
  } catch (const shcore::Error &e) {
    current_console()->print_warning(shcore::str_format(
        "%s: Error dropping account %s@%s: %s", primary->descr().c_str(),
        account.first.c_str(), account.second.c_str(), e.format().c_str()));
    // ignore the error and move on
  }
}

void Replication_account::drop_read_replica_replication_user(
    const mysqlshdk::mysql::IInstance &target, bool dry_run) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();
  assert(cluster.get_primary_master());

  auto target_instance_uuid = target.get_uuid();

  auto [repl_user, repl_user_host] =
      cluster.get_metadata_storage()->get_read_replica_repl_account(
          target_instance_uuid);

  log_info(
      "Dropping read-replica replication account '%s'@'%s' for instance '%s'",
      repl_user.c_str(), repl_user_host.c_str(), target.descr().c_str());

  if (dry_run) return;

  try {
    if (m_undo.has_value()) {
      m_undo->add_snapshot_for_drop_user(*cluster.get_primary_master(),
                                         repl_user, repl_user_host);
    }
    cluster.get_primary_master()->drop_user(repl_user, repl_user_host.c_str(),
                                            true);
  } catch (const std::exception &) {
    mysqlsh::current_console()->print_warning(shcore::str_format(
        "Error dropping read-replica replication account '%s'@'%s' for "
        "instance '%s'",
        repl_user.c_str(), repl_user_host.c_str(), target.descr().c_str()));
  }

  // Also update metadata
  try {
    cluster.get_metadata_storage()->update_read_replica_repl_account(
        target_instance_uuid, "", "");
  } catch (const std::exception &e) {
    log_warning(
        "Could not update read-replica replication account metadata for "
        "instance '%s': %s",
        target.descr().c_str(), e.what());
  }
}

void Replication_account::drop_replication_users() {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  auto ipool = current_ipool();

  cluster.execute_in_members(
      [this](const std::shared_ptr<Instance> &instance,
             const Cluster_impl::Instance_md_and_gr_member &info) {
        try {
          drop_replication_user(instance.get(), "", "", 0, false);
        } catch (const std::exception &e) {
          current_console()->print_warning(
              shcore::str_format("Could not drop internal user for '%s': %s",
                                 info.first.endpoint.c_str(), e.what()));
        }
        return true;
      },
      [this](const shcore::Error &connect_error,
             const Cluster_impl::Instance_md_and_gr_member &info) {
        // drop user based on MD lookup (will not work if the instance is still
        // using legacy account management)
        if (!drop_replication_user(nullptr, info.first.endpoint,
                                   info.first.uuid, info.first.server_id,
                                   false)) {
          current_console()->print_warning(shcore::str_format(
              "Could not drop internal user for '%s': %s",
              info.first.endpoint.c_str(), connect_error.format().c_str()));
        }
        return true;
      });

  cluster.execute_in_read_replicas(
      [this](const std::shared_ptr<Instance> &instance,
             const Instance_metadata &) {
        try {
          drop_read_replica_replication_user(*instance);
        } catch (...) {
          current_console()->print_warning(shcore::str_format(
              "Could not drop internal user for '%s': %s",
              instance->descr().c_str(), format_active_exception().c_str()));
        }
        return true;
      },
      [](const shcore::Error &connect_error, const Instance_metadata &info) {
        current_console()->print_warning(shcore::str_format(
            "Could not drop internal user for '%s': %s", info.endpoint.c_str(),
            connect_error.format().c_str()));
        return true;
      });
}

Replication_account::User_hosts Replication_account::get_replication_user(
    const mysqlshdk::mysql::IInstance &target_instance, bool is_read_replica) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  // First check the metadata for the recovery user
  const auto [recovery_user, recovery_user_host] =
      cluster.get_metadata_storage()->get_instance_repl_account(
          target_instance.get_uuid(), Cluster_type::GROUP_REPLICATION,
          is_read_replica ? Replica_type::READ_REPLICA
                          : Replica_type::GROUP_MEMBER);

  // new recovery user format (stored in Metadata), use that
  if (!recovery_user.empty()) {
    std::vector<std::string> recovery_user_hosts;
    recovery_user_hosts.push_back(recovery_user_host);

    return {recovery_user, std::move(recovery_user_hosts), true};
  }

  // Assuming the account was created by an older version of the shell, which
  // did not store account name in metadata
  log_info(
      "No recovery account details in metadata for instance '%s', assuming old "
      "account style",
      target_instance.descr().c_str());

  auto unrecorded_recovery_user = mysqlshdk::mysql::get_replication_user(
      target_instance, mysqlshdk::gr::k_gr_recovery_channel);
  assert(!unrecorded_recovery_user.empty());

  if (shcore::str_beginswith(
          unrecorded_recovery_user,
          Replication_account::k_group_recovery_old_user_prefix)) {
    log_info("Found old account style recovery user '%s'",
             unrecorded_recovery_user.c_str());
    // old accounts were created for several hostnames
    auto recovery_user_hosts = mysqlshdk::mysql::get_all_hostnames_for_user(
        *(cluster.get_cluster_server()), unrecorded_recovery_user);

    return {unrecorded_recovery_user, std::move(recovery_user_hosts), false};
  }

  if (shcore::str_beginswith(
          unrecorded_recovery_user,
          Replication_account::k_group_recovery_user_prefix)) {
    // either the transaction to store the user failed or the metadata was
    // manually changed to remove it.
    throw shcore::Exception::metadata_error(shcore::str_format(
        "The replication recovery account in use by '%s' is not stored in the "
        "metadata. Use cluster.rescan() to update the metadata.",
        target_instance.descr().c_str()));
  }

  // account not created by InnoDB cluster
  throw shcore::Exception::runtime_error(
      shcore::str_format("Recovery user '%s' not created by InnoDB Cluster",
                         unrecorded_recovery_user.c_str()));
}

Replication_account::Auth_options_host
Replication_account::refresh_replication_user(
    const mysqlshdk::mysql::IInstance &target, bool dry_run) {
  // for ReplicaSets
  if (topo_holds<Replica_set_impl>()) {
    const auto &rset = topo_as<Replica_set_impl>();

    auto primary = rset.get_primary_master();
    assert(primary);

    Replication_account::Auth_options_host auth_options;
    {
      auto account = rset.get_metadata_storage()->get_instance_repl_account(
          target.get_uuid(), Cluster_type::ASYNC_REPLICATION,
          Replica_type::NONE);

      auth_options.auth.user = std::move(account.first);
      auth_options.host = std::move(account.second);
    }

    if (auth_options.auth.user.empty()) {
      auth_options.auth.user = Replication_account::make_replication_user_name(
          target.get_server_id(),
          Replication_account::k_async_recovery_user_prefix);

      auth_options.host = "%";
    }

    try {
      // Create replication accounts for this instance at the master
      // replicaset unless the user provided one.

      log_info("Resetting password for %s@%s at '%s'",
               auth_options.auth.user.c_str(), auth_options.host.c_str(),
               primary->descr().c_str());

      if (!dry_run) {
        std::string repl_password;
        mysqlshdk::mysql::set_random_password(*primary, auth_options.auth.user,
                                              {auth_options.host},
                                              &repl_password);

        auth_options.auth.password = std::move(repl_password);
      }
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Error while resetting password for replication account: %s",
          e.what()));
    }

    return auth_options;
  }

  assert(!"Unknown topology type");
  throw std::logic_error("Unknown topology type");
}

Replication_account::Auth_options_host
Replication_account::refresh_replication_user(
    const mysqlshdk::mysql::IInstance &target, const Cluster_id &cluster_id,
    bool dry_run) {
  assert(topo_holds<Cluster_impl>() || topo_holds<Cluster_set_impl>());
  if (!topo_holds<Cluster_impl>() && !topo_holds<Cluster_set_impl>())
    throw std::logic_error("Unknown topology type");

  Replication_account::Auth_options_host auth_options;

  if (topo_holds<Cluster_impl>()) {
    const auto &cluster = topo_as<Cluster_impl>();

    auto account =
        cluster.get_metadata_storage()->get_cluster_repl_account(cluster_id);

    auth_options.auth.user = std::move(account.first);
    auth_options.host = std::move(account.second);
  } else {
    const auto &cset = topo_as<Cluster_set_impl>();

    auto account =
        cset.get_metadata_storage()->get_cluster_repl_account(cluster_id);

    auth_options.auth.user = std::move(account.first);
    auth_options.host = std::move(account.second);
  }

  try {
    log_info("Resetting password for %s@%s at '%s'",
             auth_options.auth.user.c_str(), auth_options.host.c_str(),
             target.descr().c_str());

    if (!dry_run) {
      std::string repl_password;
      mysqlshdk::mysql::set_random_password(
          target, auth_options.auth.user, {auth_options.host}, &repl_password);

      auth_options.auth.password = std::move(repl_password);
    }
  } catch (const std::exception &e) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error while resetting password for replication account: %s",
        e.what()));
  }

  return auth_options;
}

std::unordered_map<uint32_t, std::string>
Replication_account::get_mismatched_recovery_accounts() {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  std::unordered_map<uint32_t, std::string> accounts;

  // don't run the test if the metadata needs an upgrade
  if (cluster.get_metadata_storage()->installed_version() !=
      mysqlsh::dba::metadata::current_version())
    return accounts;

  cluster.get_metadata_storage()->iterate_recovery_account(
      [&accounts](uint32_t server_id, std::string recovery_account) {
        // must match either the new prefix or the old one
        auto account = shcore::str_format(
            "%.*s%u",
            static_cast<int>(
                Replication_account::k_group_recovery_user_prefix.size()),
            Replication_account::k_group_recovery_user_prefix.data(),
            server_id);
        if (account == recovery_account) return true;

        account = shcore::str_format(
            "%.*s%u",
            static_cast<int>(
                Replication_account::k_group_recovery_old_user_prefix.size()),
            Replication_account::k_group_recovery_old_user_prefix.data(),
            server_id);
        if (account == recovery_account) return true;

        accounts[server_id] = std::move(recovery_account);
        return true;
      });

  return accounts;
}

std::vector<Replication_account::User_host>
Replication_account::get_unused_recovery_accounts(
    const std::unordered_map<uint32_t, std::string>
        &mismatched_recovery_accounts) {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  const auto &cluster = topo_as<Cluster_impl>();

  // don't run the test if the metadata needs an upgrade
  if (cluster.get_metadata_storage()->installed_version() !=
      mysqlsh::dba::metadata::current_version())
    return {};

  auto recovery_users =
      cluster.get_metadata_storage()->get_recovery_account_users();

  std::vector<User_host> accounts;
  mysqlshdk::mysql::iterate_users(
      *cluster.get_cluster_server(), "mysql\\_innodb\\_cluster\\_%",
      [&accounts, &mismatched_recovery_accounts, &recovery_users](
          std::string user, std::string host) {
        if (std::find(recovery_users.begin(), recovery_users.end(), user) !=
            recovery_users.end())
          return true;

        auto server_id = extract_server_id(user);
        if (server_id < 0) return true;

        if (mismatched_recovery_accounts.find(server_id) ==
            mismatched_recovery_accounts.end()) {
          accounts.push_back({std::move(user), std::move(host)});
        }

        return true;
      });

  return accounts;
}

std::string Replication_account::get_replication_user_host() const {
  assert(topo_holds<Cluster_impl>());
  if (!topo_holds<Cluster_impl>())
    throw std::logic_error("Unknown topology type");

  auto md = topo_as<Cluster_impl>().get_metadata_storage();

  shcore::Value allowed_host;
  if (md->query_cluster_attribute(topo_as<Cluster_impl>().get_id(),
                                  k_cluster_attribute_replication_allowed_host,
                                  &allowed_host) &&
      allowed_host.get_type() == shcore::String) {
    if (auto host = allowed_host.as_string(); !host.empty()) return host;
  }

  return "%";
}

}  // namespace mysqlsh::dba
