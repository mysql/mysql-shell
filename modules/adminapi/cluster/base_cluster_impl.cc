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
#include "modules/adminapi/common/errors.h"
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

Cluster_check_info Base_cluster_impl::check_preconditions(
    const std::string &function_name, FunctionAvailability *custom_func_avail) {
  // Makes sure the metadata state is re-loaded on each API call
  m_metadata_storage->invalidate_cached();
  return check_function_preconditions(
      api_class(get_type()) + "." + function_name, get_metadata_storage(),
      custom_func_avail);
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
  if (opts.has_scheme()) {
    if (opts.get_scheme() == main_opts.get_scheme()) {
      opts.clear_scheme();
    } else {
      // different scheme means it's an X protocol URI
      const auto error = make_unsupported_protocol_error();
      const auto endpoint = Connection_options(instance_def).uri_endpoint();
      detail::report_connection_error(error, endpoint);
      throw shcore::Exception::runtime_error(
          detail::connection_error_msg(error, endpoint));
    }
  }

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
  }
  CATCH_REPORT_AND_THROW_CONNECTION_ERROR(
      Connection_options(instance_def).uri_endpoint())
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
    // Prepare the setup_account execution
    op_setup.prepare();
    // Execute the setup_account command.
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

void Base_cluster_impl::set_instance_tag(const std::string &instance_def,
                                         const std::string &option,
                                         const shcore::Value &value) {
  // Connect to the target Instance and check if it belongs to the
  // cluster/replicaSet
  const auto target_instance =
      Scoped_instance(connect_target_instance(instance_def));
  const auto target_uuid = target_instance->get_uuid();
  const auto is_instance_on_md = get_metadata_storage()->is_instance_on_cluster(
      get_id(), target_instance->get_canonical_address());

  if (!is_instance_on_md) {
    std::string err_msg =
        "The instance '" + Connection_options(instance_def).uri_endpoint() +
        "' does not belong to the " + api_class(get_type()) + ".";
    throw shcore::Exception::runtime_error(err_msg);
  }
  get_metadata_storage()->set_instance_tag(target_uuid, option, value);
}

void Base_cluster_impl::set_cluster_tag(const std::string &option,
                                        const shcore::Value &value) {
  get_metadata_storage()->set_cluster_tag(get_id(), option, value);
}

void Base_cluster_impl::set_instance_option(const std::string &instance_def,
                                            const std::string &option,
                                            const shcore::Value &value) {
  check_preconditions("setInstanceOption");

  // Initialize pool with the metadata
  Instance_pool::Auth_options auth_opts;
  auth_opts.get(get_target_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);
  ipool->set_metadata(get_metadata_storage());

  // make sure metadata session is using the primary
  acquire_primary();
  auto finally_primary =
      shcore::on_leave_scope([this]() { release_primary(); });

  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value);
  if (opt_namespace.empty()) {
    // no namespace was provided
    _set_instance_option(instance_def, option, val);
  } else {
    set_instance_tag(instance_def, opt, val);
  }
}

void Base_cluster_impl::set_option(const std::string &option,
                                   const shcore::Value &value) {
  FunctionAvailability custom_func_avail = {
      k_min_gr_version,
      GRInstanceType::InnoDBCluster,
      ReplicationQuorum::State(ReplicationQuorum::States::Normal),
      ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO,
      {{metadata::kIncompatibleOrUpgrading, MDS_actions::RAISE_ERROR}}};

  std::string opt_namespace, opt;
  shcore::Value val;
  std::tie(opt_namespace, opt, val) =
      validate_set_option_namespace(option, value);

  FunctionAvailability *custom_precondition{nullptr};
  if (!opt_namespace.empty() && get_type() == Cluster_type::GROUP_REPLICATION) {
    custom_precondition = &custom_func_avail;
  }
  check_preconditions("setOption", custom_precondition);

  // Initialize pool with the metadata
  Instance_pool::Auth_options auth_opts;
  auth_opts.get(get_target_server()->get_connection_options());
  Scoped_instance_pool ipool(false, auth_opts);
  ipool->set_metadata(get_metadata_storage());

  // make sure metadata session is using the primary
  acquire_primary();
  auto finally_primary =
      shcore::on_leave_scope([this]() { release_primary(); });

  if (opt_namespace.empty()) {
    // no namespace was provided
    _set_option(option, val);
  } else {
    set_cluster_tag(opt, val);
  }
}

std::tuple<std::string, std::string, shcore::Value>
Base_cluster_impl::validate_set_option_namespace(
    const std::string &option, const shcore::Value &value) const {
  shcore::Value val = value;
  // Check if we're using namespaces
  auto tokens = shcore::str_split(option, ":", 1);
  if (tokens.size() == 2) {
    // colon cannot be first char or last char, we don't support empty
    // namespaces nor empty namespace options
    std::string opt_namespace = tokens[0];
    std::string opt = tokens[1];
    if (opt_namespace.empty() || opt.empty()) {
      throw shcore::Exception::argument_error("'" + option +
                                              "' is not a valid identifier.");
    }
    // option is of type namespace:option
    // For now since we don't allow namespaces other than 'tag', throw an error
    // if the namespace is not a tag
    if (opt_namespace != "tag") {
      throw shcore::Exception::argument_error("Namespace '" + opt_namespace +
                                              "' not supported.");
    } else {
      // tag namespace.
      if (!shcore::is_valid_identifier(opt)) {
        throw shcore::Exception::argument_error(
            "'" + opt + "' is not a valid tag identifier.");
      }
      // Even if it is a valid identifier, if it starts with _ we need to make
      // sure it is one of the allowed built-in tags
      if (opt[0] == '_') {
        built_in_tags_map_t::const_iterator c_it =
            k_supported_set_option_tags.find(opt);
        if (c_it == k_supported_set_option_tags.cend()) {
          throw shcore::Exception::argument_error(
              "'" + opt + "' is not a valid built-in tag.");
        }
        // If the type of the value is not Null, check that it can be converted
        // to the expected type
        if (value.type != shcore::Value_type::Null) {
          try {
            switch (c_it->second) {
              case shcore::Value_type::Bool:
                val = shcore::Value(value.as_bool());
                break;
              default:
                // so far, built-in tags are only expected to be booleans.
                // Any other type should throw an exception. This exception
                // will be caught on the catch clause, and re-created with a
                // more informative message.
                throw shcore::Exception::type_error(
                    "Unsupported built-in tag type.");
            }
          } catch (const shcore::Exception &) {
            throw shcore::Exception::type_error(shcore::str_format(
                "Built-in tag '%s' is expected to be of type %s, but is %s",
                c_it->first.c_str(), type_name(c_it->second).c_str(),
                type_name(value.type).c_str()));
          }
        }
      }
    }
    return std::make_tuple(opt_namespace, opt, val);
  } else {
    // not using namespaces
    return std::make_tuple("", option, val);
  }
}

shcore::Value Base_cluster_impl::get_cluster_tags() {
  shcore::Dictionary_t res = shcore::make_dict();

  // lambda function that does the repetitive work of creating an array of
  // dictionaries from a string representation of a JSON object.
  auto helper_func = [](std::string tags) -> shcore::Array_t {
    shcore::Array_t result = shcore::make_array();
    if (!tags.empty()) {
      auto tags_value = shcore::Value::parse(tags);
      auto tags_map = tags_value.as_map();
      auto it = tags_map->begin();
      while (it != tags_map->end()) {
        auto dict = shcore::make_dict();
        (*dict)["option"] = shcore::Value(it->first);
        (*dict)["value"] = it->second;
        result->emplace_back(shcore::Value(dict));
        it++;
      }
    }
    return result;
  };

  std::string cluster_tags = get_metadata_storage()->get_cluster_tags(get_id());

  // Fill cluster tags
  (*res)["global"] = shcore::Value(helper_func(cluster_tags));

  // Fill the cluster instance tags
  std::vector<Instance_metadata> instance_defs =
      get_metadata_storage()->get_all_instances(get_id());

  // get the list of tags each instance
  for (const auto &instance_def : instance_defs) {
    std::string instance_tags =
        get_metadata_storage()->get_instance_tags(instance_def.uuid);
    (*res)[instance_def.label] = shcore::Value(helper_func(instance_tags));
  }
  return shcore::Value(res);
}

}  // namespace dba
}  // namespace mysqlsh
