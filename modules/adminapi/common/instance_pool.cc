/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/instance_pool.h"

#include <errmsg.h>
#include <mysql.h>
#include <stack>

#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/global_topology.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/include/scripting/types.h"  // exceptions
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh {
namespace dba {

namespace {
// Constants with the names used to lock instances.
static constexpr char k_lock[] = "AdminAPI_lock";
static constexpr char k_lock_name_instance[] = "AdminAPI_instance";

std::shared_ptr<mysqlshdk::db::ISession> connect_session(
    const mysqlshdk::db::Connection_options &opts, bool interactive) {
  if (SessionType::X == opts.get_session_type()) {
    throw make_unsupported_protocol_error();
  }

  try {
    return establish_mysql_session(opts, interactive);
  } catch (const shcore::Exception &e) {
    if (CR_VERSION_ERROR == e.code() ||
        (CR_SERVER_LOST == e.code() &&
         0 == strcmp("Lost connection to MySQL server at 'waiting for initial "
                     "communication packet', system error: 110",
                     e.what()))) {
      throw make_unsupported_protocol_error();
    } else {
      throw;
    }
  }
}

}  // namespace

// default SQL_MODE as of 8.0.19
static constexpr const char *k_default_sql_mode =
    "ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,"
    "NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION";

std::shared_ptr<Instance> Instance::connect_raw(
    const mysqlshdk::db::Connection_options &opts, bool interactive) {
  return std::make_shared<Instance>(connect_session(opts, interactive));
}

std::shared_ptr<Instance> Instance::connect(
    const mysqlshdk::db::Connection_options &opts, bool interactive) {
  const auto instance = connect_raw(opts, interactive);

  instance->prepare_session();

  return instance;
}

void Instance::prepare_session() {
  // prepare the session to be used by the AdminAPI

  // change autocommit to the default value, which we assume throughout the
  // code, but can be changed globally by the user
  // Bug#30202883 and Bug#30324461 are caused by the lack of this
  set_sysvar("autocommit", static_cast<int64_t>(1),
             mysqlshdk::mysql::Var_qualifier::SESSION);

  // change sql_mode to the default value, in case the user has some
  // non-standard and incompatible setting
  set_sysvar("sql_mode", std::string(k_default_sql_mode),
             mysqlshdk::mysql::Var_qualifier::SESSION);

  // Handle the consistency levels != "EVENTUAL" and
  // "BEFORE_ON_PRIMARY_FAILOVER" on the target instance session:
  //
  // Any query executed on a member that is still RECOVERING and has a
  // consistency level of BEFORE, AFTER or BEFORE_AND_AFTER will result in an
  // error. As documented: "You can only use the consistency levels BEFORE,
  // AFTER and BEFORE_AND_AFTER on ONLINE members, attempting to use them on
  // members in other states causes a session error." For that reason, we must
  // set the target instance consistency level to EVENTUAL on those cases to
  // avoid the error for any query executed using that session.
  //
  // This handling must be done to all AdminAPI sessions to ensure that
  // concurrent command calls do not results in errors due to higher consistency
  // levels (BUG#30394258, BUG#30401048)
  if (get_version() >= mysqlshdk::utils::Version("8.0.14")) {
    set_sysvar("group_replication_consistency", std::string("EVENTUAL"),
               mysqlshdk::mysql::Var_qualifier::SESSION);
  }

  // Cache the hostname, port, and UUID to avoid errors accessing this data if
  // the instance fails during an operation.
  get_canonical_address();
  get_uuid();
}

Instance::Instance(const std::shared_ptr<mysqlshdk::db::ISession> &session)
    : mysqlshdk::mysql::Instance(session), m_retain_count(-1) {}

Instance::Instance(Instance_pool *owner,
                   const std::shared_ptr<mysqlshdk::db::ISession> &session)
    : mysqlshdk::mysql::Instance(session), m_pool(owner) {}

void Instance::retain() {
  DBUG_TRACE;

  if (m_retain_count > -1) m_retain_count++;
}

void Instance::release() {
  DBUG_TRACE;

  if (m_retain_count > -1) {
    if (m_retain_count <= 1) {
      if (m_pool) {
        refresh();  // clear cached values

        m_pool->return_instance(this);
      } else {
        get_session()->close();
      }
    } else {
      m_retain_count--;
    }
  }
}

// The instance is expected to have been acquired directly by the caller and
// nobody else holds references to it (no Scoped_instances either).
void Instance::steal() {
  DBUG_TRACE;

  if (m_pool) m_pool->forget_instance(this);
  m_pool = nullptr;
}

/**
 * Log the input SQL statement.
 *
 * SQL is logged with the INFO level and according the value set for the
 * dba.logSql shell option:
 *   - 0: no SQL logging;
 *   - 1: log SQL statements, except SELECT and SHOW;
 *   - 2: log all SQL statements;
 * Passwords are hidden, i.e., replaced by ****, assuming that they are
 * properly identified in the SQL statement.
 *
 * @param sql string with the target SQL statement to log.
 *
 */
void Instance::log_sql(const std::string &sql) const {
  if (current_shell_options()->get().dba_log_sql > 1 ||
      (current_shell_options()->get().dba_log_sql == 1 &&
       !shcore::str_ibeginswith(sql, "select") &&
       !shcore::str_ibeginswith(sql, "show"))) {
    log_info(
        "%s: %s", get_connection_options().uri_endpoint().c_str(),
        shcore::str_subvars(
            sql, [](const std::string &) { return "****"; }, "/*((*/", "/*))*/")
            .c_str());
  }
}

void Instance::log_sql_error(const shcore::Error &e) const {
  if (current_shell_options()->get().dba_log_sql > 0) {
    log_info("%s: -> %s", descr().c_str(), e.format().c_str());
  }
}

std::shared_ptr<mysqlshdk::db::IResult> Instance::query(const std::string &sql,
                                                        bool buffered) const {
  log_sql(sql);
  try {
    return mysqlshdk::mysql::Instance::query(sql, buffered);
  } catch (const shcore::Error &e) {
    log_sql_error(e);
    throw;
  }
}

void Instance::execute(const std::string &sql) const {
  log_sql(sql);
  try {
    mysqlshdk::mysql::Instance::execute(sql);
  } catch (const shcore::Error &e) {
    log_sql_error(e);
    throw;
  }
}

/**
 * Check if the lock service UDFs is available and install if needed.
 *
 * @param skip_fail_install_warn boolean value that controls if a warning is
 *        printed in case the lock service UDFs failed to be installed. This
 *        can be useful to avoid multiple warnings to be repeated for the same
 *        operation. By default false, meaning that the warning can be
 *        printed.
 * @return int with the exit code of the function execution: 1 - a warning
 *         was issued because service lock UDFs could not be installed;
 *         2 - lock UDFs could not be installed (no warning);
 *         O - otherwise (success installing lock UDFs or already available).
 */
int Instance::ensure_lock_service_udfs_installed(bool skip_fail_install_warn) {
  auto console = current_console();
  int exit_code = 0;

  // Install Locking Service UDFs if needed.
  if (!mysqlshdk::mysql::has_lock_service_udfs(*this)) {
    log_debug("Installing lock service UDFs.");
    try {
      // Check if instance is read-only otherwise it will fail to install
      // the locking UDFs (e.g., it might happen for clusters created with an
      // older MySQL Shell version).
      bool super_read_only = *get_sysvar_bool("super_read_only");
      if (super_read_only) {
        // Temporarly disable super_read_only to install UDFs.
        set_sysvar("super_read_only", false,
                   mysqlshdk::mysql::Var_qualifier::GLOBAL);
      }

      // Re-enable super_read_only at the end (even if install UDFs fail).
      shcore::on_leave_scope restore_read_only([super_read_only, this]() {
        if (super_read_only)
          set_sysvar("super_read_only", true,
                     mysqlshdk::mysql::Var_qualifier::GLOBAL);
      });

      mysqlshdk::mysql::install_lock_service_udfs(this);
    } catch (const std::exception &err) {
      // Nlogicotify the user in case the MySQL lock service cannot be installed
      // (not available) and continue (without concurrent execution support).
      log_warning("Failed to install lock service UDFs on %s: %s",
                  descr().c_str(), err.what());
      if (!skip_fail_install_warn) {
        console->print_warning(
            "Concurrent execution of ReplicaSet operations is not supported "
            "because the required MySQL lock service UDFs could not be "
            "installed on instance '" +
            descr() + "'.");
        console->print_info(
            "Make sure the MySQL lock service plugin is available on all "
            "instances if you want to be able to execute some operations at "
            "the same time. The operation will continue without concurrent "
            "execution support.");
        console->print_info();

        // Return 1, if failed to install and a warning was printed.
        exit_code = 1;
      } else {
        // Return 2, if failed to install, no warning printed.
        exit_code = 2;
      }
    }
  }

  return exit_code;
}

/**
 * Try to acquire a specific lock on the instance.
 *
 * NOTE: Required lock service UDFs are automatically installed if needed.
 *
 * @param mode type of lock to acquire: READ (shared) or WRITE (exclusive).
 * @param timeout maximum time in seconds to wait for the lock to be
 *        available if it cannot be obtained immediately. By default 0,
 *        meaning that it will not wait if the lock cannot be acquired
 *        immediately, issuing an error.
 * @param skip_fail_install_warn boolean value that controls if a warning is
 *        printed in case the lock service UDFs failed to be installed. This
 *        can be useful to avoid multiple warnings to be repeated for the same
 *        operation. By default false, meaning that the warning can be
 *        printed.
 *
 * @throw shcore::Exception if the lock cannot be acquired or any other error
 *        occur when trying to obtain the lock.
 *
 * @return int with the exit code of the function execution: 1 - a warning
 *         was issued because service lock UDFs could not be installed;
 *         2 - lock UDFs could not be installed (no warning);
 *         O - otherwise (success installing lock UDFs or already available).
 */

int Instance::get_lock(mysqlshdk::mysql::Lock_mode mode, unsigned int timeout,
                       bool skip_fail_install_warn) {
  auto console = current_console();

  // Install Locking Service UDFs if needed.
  int exit_code = ensure_lock_service_udfs_installed(skip_fail_install_warn);

  // Return immediatly if lock_service UDFs failed to install
  // (no need to try to acquire any lock).
  if (exit_code != 0) return exit_code;

  DBUG_EXECUTE_IF("dba_locking_timeout_one", { timeout = 1; });

  // Try to acquire the specified lock.
  // NOTE: Only one lock per namespace is used because lock release is
  //       performed by namespace.
  try {
    log_debug("Acquiring %s lock ('%s', '%s') on %s.",
              mysqlshdk::mysql::to_string(mode).c_str(), k_lock_name_instance,
              k_lock, descr().c_str());
    mysqlshdk::mysql::get_lock(*this, k_lock_name_instance, k_lock, mode,
                               timeout);
  } catch (const shcore::Error &err) {
    // Abort the operation in case the required lock cannot be acquired.
    log_debug("Failed to get %s lock ('%s', '%s'): %s",
              mysqlshdk::mysql::to_string(mode).c_str(), k_lock_name_instance,
              k_lock, err.what());
    console->print_error(
        "The operation cannot be executed because it failed to acquire the "
        "lock on instance '" +
        descr() +
        "'. Another operation requiring exclusive access to the instance is "
        "still in progress, please wait for it to finish and try again.");
    throw shcore::Exception(
        "Failed to acquire lock on instance '" + descr() + "'",
        SHERR_DBA_LOCK_GET_FAILED);
  }

  return exit_code;
}

int Instance::get_lock_shared(unsigned int timeout,
                              bool skip_fail_install_warn) {
  return get_lock(mysqlshdk::mysql::Lock_mode::SHARED, timeout,
                  skip_fail_install_warn);
}

int Instance::get_lock_exclusive(unsigned int timeout,
                                 bool skip_fail_install_warn) {
  return get_lock(mysqlshdk::mysql::Lock_mode::EXCLUSIVE, timeout,
                  skip_fail_install_warn);
}

void Instance::release_lock(bool no_throw) const {
  // Release all instance locks in the k_lock_name_instance namespace.
  // NOTE: Only perform the operation if the lock service UDFs are available
  //       otherwise do nothing (ignore if concurrent execution is not
  //       supported, e.g., lock service plugin not available).
  try {
    if (mysqlshdk::mysql::has_lock_service_udfs(*this)) {
      log_debug("Releasing locks for '%s' on %s.", k_lock_name_instance,
                descr().c_str());
      mysqlshdk::mysql::release_lock(*this, k_lock_name_instance);
    }
  } catch (const shcore::Error &error) {
    if (no_throw) {
      // Ignore any error trying to release locks (e.g., might have not been
      // previously acquired due to lack of permissions).
      log_error("Unable to release '%s' locks for '%s': %s",
                k_lock_name_instance, descr().c_str(), error.what());
    } else {
      throw;
    }
  }
}

struct Instance_pool::Metadata_cache {
  std::vector<Instance_metadata> instances;
  std::vector<Cluster_metadata> clusters;

  const Cluster_metadata *try_get_cluster(const Cluster_id &id) const {
    for (const auto &c : clusters) {
      if (c.cluster_id == id) return &c;
    }
    return nullptr;
  }

  const Instance_metadata *get_cluster_primary(const Cluster_id &id) const {
    for (const auto &i : instances) {
      if (i.cluster_id == id && i.primary_master && !i.invalidated) return &i;
    }
    throw shcore::Exception("Async cluster has no PRIMARY",
                            SHERR_DBA_ASYNC_PRIMARY_UNDEFINED);
  }

  const Instance_metadata *get_instance_with_uuid(
      const std::string &uuid) const {
    for (const auto &i : instances) {
      if (i.uuid == uuid) return &i;
    }
    // not supposed to happen
    throw std::logic_error("internal error");
  }
};

Instance_pool::Instance_pool(bool allow_password_prompt)
    : m_allow_password_prompt(allow_password_prompt) {
  DBUG_TRACE;

  m_mdcache = new Metadata_cache();
}

Instance_pool::~Instance_pool() {
  DBUG_TRACE;

  delete m_mdcache;
  for (const auto &inst : m_pool) {
#ifndef NDEBUG
    if (inst.leased) {
      std::cerr << inst.instance->descr() << " ("
                << inst.instance->get_session()->get_connection_id()
                << ") not returned to pool. " << inst.instance->m_retain_count
                << " refs\n";

      DBUG_EXECUTE_IF("ipool_trap_session_leak", DEBUG_TRAP;);
    }
#endif

    inst.instance->close_session();
  }
  m_pool.clear();
}

void Instance_pool::set_default_auth_options(const Auth_options &opts) {
  m_default_auth_opts = opts;
}

void Instance_pool::set_auth_opts(const Auth_options &auth,
                                  mysqlshdk::db::Connection_options *opts) {
  if (auth.user.empty()) {
    if (m_default_auth_opts.user.empty())
      throw std::invalid_argument("Missing authentication info");
    m_default_auth_opts.set(opts);
  } else {
    auth.set(opts);
  }
}

void Instance_pool::set_metadata(const std::shared_ptr<MetadataStorage> &md) {
  DBUG_TRACE;

  m_metadata = md;
  if (m_metadata) refresh_metadata_cache();
}

/**
 * Find a suitable metadata server that contains the server the session is
 * connected to.
 *
 * The following is considered in order:
 * - the target server
 * - any member of the group of the target server
 * - any member of a replica group
 * - the original target server if nothing better is found
 *
 * A server is suitable if it's ONLINE and part of a majority.
 *
 * The process is aborted and an exception thrown if the original instance or
 * group is found to not belong to the cluster anymore.
 */
std::shared_ptr<MetadataStorage> Instance_pool::find_metadata_server(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  DBUG_TRACE;
  auto console = current_console();

  auto instance = std::make_shared<Instance>(session);

  log_info("Looking for a suitable metadata server for instance %s",
           instance->descr().c_str());

  auto md = std::make_shared<MetadataStorage>(instance);

  if (!md->check_version()) {
    console->print_warning("InnoDB cluster metadata does not exist at " +
                           session->get_connection_options().uri_endpoint());
    throw shcore::Exception("Metadata schema missing",
                            SHERR_DBA_METADATA_MISSING);
  }

  auto old_md = m_metadata;
  set_metadata(md);
  try {
    auto member = connect_cluster_member_of(instance);
    member->steal();
    set_metadata(old_md);
    if (member == instance) {
      return md;
    } else {
      md = std::make_shared<MetadataStorage>(member);
      return md;
    }
  } catch (...) {
    set_metadata(old_md);
    throw;
  }
}

void Instance_pool::refresh_metadata_cache() {
  DBUG_TRACE;
  if (!m_metadata) throw std::logic_error("metadata object not set");

  log_debug("Refreshing metadata cache");
  m_mdcache->instances = m_metadata->get_all_instances();
  m_mdcache->clusters = m_metadata->get_all_clusters();
}

std::shared_ptr<Instance> Instance_pool::adopt(
    const std::shared_ptr<Instance> &instance) {
  DBUG_TRACE;
  return add_leased_instance(instance);
}

// Connect to the specified instance without doing any checks
std::shared_ptr<Instance> Instance_pool::connect_unchecked(
    const mysqlshdk::db::Connection_options &opts) {
  DBUG_TRACE;
  for (auto &inst : m_pool) {
    if (!inst.leased && inst.instance->get_connection_options() == opts) {
      inst.leased = true;
      return inst.instance;
    }
  }

  auto session = connect_session(opts, m_allow_password_prompt);
  auto instance =
      add_leased_instance(std::make_shared<Instance>(this, session));
  instance->prepare_session();
  return instance;
}

std::shared_ptr<Instance> Instance_pool::connect_unchecked_endpoint(
    const std::string &endpoint, bool allow_url) {
  DBUG_TRACE;
  mysqlshdk::db::Connection_options opts(endpoint);

  if (allow_url) {
    if (!opts.has_user()) {
      m_default_auth_opts.set(&opts);
    }
  } else {
    // check that there's only a host:port
    mysqlshdk::db::Connection_options tmp(opts);
    tmp.clear_host();
    tmp.clear_port();
    tmp.clear_socket();
    if (tmp.has_data())
      throw std::invalid_argument(
          "Target instance must be specified as host:port");
    m_default_auth_opts.set(&opts);
  }

  try {
    return connect_unchecked(opts);
  }
  CATCH_AND_THROW_CONNECTION_ERROR(endpoint)
}

std::shared_ptr<Instance> Instance_pool::connect_unchecked_uuid(
    const std::string &uuid) {
  DBUG_TRACE;
  Auth_options auth = m_default_auth_opts;

  for (auto &inst : m_pool) {
    Auth_options iauth;
    iauth.get(inst.instance->get_connection_options());

    if (!inst.leased && inst.instance->get_uuid() == uuid && iauth == auth) {
      inst.leased = true;
      return inst.instance;
    }
  }

  for (const auto &inst : m_mdcache->instances) {
    if (inst.uuid == uuid) {
      if (inst.endpoint.empty())
        throw shcore::Exception(
            "missing endpoint information for instance " + uuid,
            SHERR_DBA_METADATA_INFO_MISSING);

      mysqlshdk::db::Connection_options opts(inst.endpoint);
      set_auth_opts(auth, &opts);

      try {
        return connect_unchecked(opts);
      }
      CATCH_AND_THROW_CONNECTION_ERROR(inst.endpoint)
    }
  }
  throw shcore::Exception("Unable to find metadata for instance " + uuid,
                          SHERR_DBA_MEMBER_METADATA_MISSING);
}

std::shared_ptr<Instance> Instance_pool::connect_unchecked(
    const topology::Node *node) {
  DBUG_TRACE;

  const Cluster_metadata *cluster =
      m_mdcache->try_get_cluster(node->cluster_id);
  DBUG_ASSERT(cluster);
  if (!cluster) {
    throw shcore::Exception::logic_error("Invalid node " + node->label);
  }

  if (cluster->type == Cluster_type::GROUP_REPLICATION) {
    assert(0);
    return {};
  } else {
    return connect_unchecked_uuid(node->get_primary_member()->uuid);
  }
}

std::shared_ptr<Instance> Instance_pool::connect_primary_master_for_member(
    const std::string &server_uuid) {
  DBUG_TRACE;

  std::string my_group_name;
  std::vector<Cluster_metadata> replicasets;

  if (!m_metadata) throw std::logic_error("Metadata object required");

  const Instance_metadata *md = m_mdcache->get_instance_with_uuid(server_uuid);

  const Cluster_metadata *cluster_md =
      m_mdcache->try_get_cluster(md->cluster_id);

  if (cluster_md && cluster_md->type == Cluster_type::ASYNC_REPLICATION) {
    return connect_async_cluster_primary(cluster_md->cluster_id);
  } else {
    throw std::logic_error("internal error");
  }

  throw shcore::Exception("No PRIMARY node found",
                          SHERR_DBA_ACTIVE_CLUSTER_NOT_FOUND);
}

std::shared_ptr<Instance> Instance_pool::connect_async_cluster_primary(
    Cluster_id cluster_id) {
  const Instance_metadata *md = m_mdcache->get_cluster_primary(cluster_id);
  return connect_unchecked_endpoint(md->endpoint);
}

void Instance_pool::check_group_member(
    const mysqlshdk::mysql::IInstance &instance, bool allow_recovering,
    std::string *out_member_id, std::string *out_group_name,
    bool *out_single_primary_mode, bool *out_is_primary) {
  DBUG_TRACE;
  mysqlshdk::gr::Member_state member_state;
  bool has_quorum;

  if (!mysqlshdk::gr::get_group_information(
          instance, &member_state, out_member_id, out_group_name,
          out_single_primary_mode, &has_quorum, out_is_primary)) {
    throw shcore::Exception(
        "Group Replication is not running at instance " + instance.descr(),
        SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING);
  }

  if (!has_quorum) {
    throw shcore::Exception("Member " +
                                label_for_server_uuid(instance.get_uuid()) +
                                " is not part of a majority group",
                            SHERR_DBA_GROUP_MEMBER_NOT_IN_QUORUM);
  }

  if (member_state == mysqlshdk::gr::Member_state::ONLINE) {
  } else if (member_state == mysqlshdk::gr::Member_state::RECOVERING &&
             allow_recovering) {
  } else {
    throw shcore::Exception(
        "member " + label_for_server_uuid(instance.get_uuid()) +
            " is in state " + mysqlshdk::gr::to_string(member_state),
        SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);
  }
}

std::shared_ptr<Instance> Instance_pool::connect_primary(
    const topology::Node *node) {
  DBUG_TRACE;

  const Cluster_metadata *cluster =
      m_mdcache->try_get_cluster(node->cluster_id);
  DBUG_ASSERT(cluster);
  if (!cluster) {
    throw shcore::Exception::logic_error("Invalid cluster " + node->label);
  }

  if (cluster->type == Cluster_type::GROUP_REPLICATION) {
    return connect_group_primary(cluster->group_name);
  } else {
    auto server = static_cast<const topology::Server *>(node);

    return connect_unchecked_uuid(server->get_primary_member()->uuid);
  }
}

std::shared_ptr<Instance> Instance_pool::connect_group_primary(
    const std::string &group_name) {
  DBUG_TRACE;
  std::vector<std::string> candidates;
  // first try to find someone that was the primary last time we saw it
  for (const auto &i : m_mdcache->instances) {
    if (i.group_name == group_name) {
      if (m_recent_primaries.find(i.uuid) != m_recent_primaries.end()) {
        // instance was a PRIMARY last time we saw it, try using it as a
        // starting point to find the current PRIMARY
        auto instance = try_connect_primary_through_member(i.uuid);
        if (instance) {
          return instance;
        }
      } else {
        candidates.push_back(i.uuid);
      }
    }
  }

  if (candidates.empty()) {
    log_warning("Could not find any members in metadata for group_name %s",
                group_name.c_str());

    throw shcore::Exception(
        "No managed members found for the requested group_name",
        SHERR_DBA_METADATA_MISSING);
  }

  // we don't know any possible primaries, so just try everyone one by one
  for (const auto &uuid : candidates) {
    auto instance = try_connect_primary_through_member(uuid);
    if (instance) {
      return instance;
    }
  }
  // there are no primaries anywhere
  throw shcore::Exception(
      "Could not connect to a PRIMARY member of group " + group_name,
      SHERR_DBA_GROUP_HAS_NO_PRIMARY);
}

std::shared_ptr<Instance> Instance_pool::connect_group_secondary(
    const std::string &group_name) {
  DBUG_TRACE;
  int num_reachable = 0;

  for (const auto &i : m_mdcache->instances) {
    if (i.group_name == group_name) {
      std::shared_ptr<Instance> instance;
      try {
        instance = connect_unchecked_uuid(i.uuid);
        num_reachable++;
      } catch (const shcore::Exception &e) {
        log_warning("%s: %s", i.endpoint.c_str(), e.format().c_str());
        if (e.code() > CR_ERROR_LAST) {
          num_reachable++;
        }
        continue;
      }

      bool single_primary;
      bool is_primary;
      try {
        check_group_member(*instance, false, nullptr, nullptr, &single_primary,
                           &is_primary);
      } catch (const shcore::Exception &e) {
        instance->release();
        log_warning("%s: %s", i.endpoint.c_str(), e.what());
        continue;
      }

      if (!is_primary) return instance;

      instance->release();

      if (!single_primary) {
        throw shcore::Exception::runtime_error(
            "Target cluster is configured for multi-primary mode and has no "
            "SECONDARY members");
      }
    }
  }

  if (num_reachable == 0)
    throw shcore::Exception(
        "Could not connect to any SECONDARY member of the target InnoDB "
        "cluster",
        SHERR_DBA_GROUP_UNREACHABLE);

  throw shcore::Exception(
      "Could not find any SECONDARY member in the target InnoDB cluster",
      SHERR_DBA_GROUP_UNAVAILABLE);
}

std::shared_ptr<Instance> Instance_pool::connect_group_member(
    const std::string &group_name) {
  DBUG_TRACE;
  int num_reachable = 0;

  for (const auto &i : m_mdcache->instances) {
    if (i.group_name == group_name) {
      std::shared_ptr<Instance> instance;
      try {
        instance = connect_unchecked_uuid(i.uuid);
        num_reachable++;
      } catch (const shcore::Exception &e) {
        log_warning("%s: %s", i.endpoint.c_str(), e.format().c_str());
        if (e.code() > CR_ERROR_LAST) {
          num_reachable++;
        }
        continue;
      }

      try {
        check_group_member(*instance, true);
      } catch (const shcore::Exception &e) {
        instance->release();
        log_warning("%s: %s", i.endpoint.c_str(), e.what());
        continue;
      }
      return instance;
    }
  }

  if (num_reachable == 0)
    throw shcore::Exception(
        "Could not connect to any member of group " + group_name,
        SHERR_DBA_GROUP_UNREACHABLE);

  throw shcore::Exception(
      "Could not find any available member in group " + group_name,
      SHERR_DBA_GROUP_UNAVAILABLE);
}

std::shared_ptr<Instance> Instance_pool::try_connect_primary_through_member(
    const std::string &member_uuid) {
  DBUG_TRACE;
  bool has_quorum;
  bool single_primary;
  std::vector<mysqlshdk::gr::Member> members;

  try {
    std::shared_ptr<Instance> instance(connect_unchecked_uuid(member_uuid));

    try {
      // info from the candidate member
      members =
          mysqlshdk::gr::get_members(*instance, &single_primary, &has_quorum);
    } catch (...) {
      instance->release();
      throw;
    }
    if (has_quorum) {
      for (const auto &m : members) {
        if (m.uuid == member_uuid) {
          // if this member is a primary, return it
          if (m.role == mysqlshdk::gr::Member_role::PRIMARY) {
            m_recent_primaries.insert(member_uuid);
            return instance;
          }
          // otherwise, we remove from list of recent primaries
          m_recent_primaries.erase(member_uuid);
        } else {
          // new primary is in the list of peers, return a session to it
          if (m.role == mysqlshdk::gr::Member_role::PRIMARY) {
            m_recent_primaries.insert(m.uuid);
            instance->release();
            // we don't need to check if the target is a member because we
            // just checked it from this fresh member list... there's a race
            // condition possible, but checking again here won't help with
            // that
            return connect_unchecked_uuid(m.uuid);
          }
        }
      }
    } else {
      throw shcore::Exception(
          "Target cluster does not have enough ONLINE members to form a "
          "quorum.",
          SHERR_DBA_GROUP_HAS_NO_QUORUM);
    }

    // member_uuid is probably either out of the group or in a partition with
    // no quorum
    instance->release();

    return {};
  } catch (const shcore::Error &err) {
    // ignore any DB connect errors that could be due to lack of availability
    if (err.code() >= CR_MIN_ERROR && err.code() <= CR_MAX_ERROR) {
      return {};
    }
    throw;
  }
}

std::shared_ptr<Instance> Instance_pool::connect_cluster_member_of(
    const std::shared_ptr<Instance> &instance) {
  DBUG_TRACE;

  const Instance_metadata *i =
      m_mdcache->get_instance_with_uuid(instance->get_uuid());
  const Cluster_metadata *c = m_mdcache->try_get_cluster(i->cluster_id);
  if (!c) throw std::logic_error("internal error");

  if (c->type == Cluster_type::GROUP_REPLICATION) {
    std::string group_name;
    try {
      check_group_member(*instance, false, nullptr, &group_name);
      return instance;
    } catch (const shcore::Exception &e) {
      log_warning("%s: %s", instance->descr().c_str(), e.what());
    }
    return connect_group_member(group_name);
  } else {
    return instance;
  }
}

std::shared_ptr<Instance> Instance_pool::add_leased_instance(
    std::shared_ptr<Instance> instance) {
  DBUG_TRACE;
  Pool_entry entry;
  entry.instance = instance;
  entry.leased = true;
  m_pool.emplace_back(entry);
  return instance;
}

void Instance_pool::return_instance(Instance *instance) {
  DBUG_TRACE;
  for (auto i = m_pool.begin(); i != m_pool.end(); ++i) {
    if (i->instance.get() == instance) {
      if (!i->leased) throw std::logic_error("Returning unleased instance");
      i->leased = false;
      break;
    }
  }
}

std::shared_ptr<Instance> Instance_pool::forget_instance(Instance *instance) {
  DBUG_TRACE;
  for (auto i = m_pool.begin(); i != m_pool.end(); ++i) {
    if (i->instance.get() == instance) {
      auto ptr = i->instance;
      m_pool.erase(i);
      return ptr;
    }
  }
  throw std::logic_error("Trying to steal non-managed instance");
}

std::string Instance_pool::label_for_server_uuid(const std::string &uuid) {
  DBUG_TRACE;
  for (const auto &i : m_mdcache->instances) {
    if (i.uuid == uuid) return i.label;
  }
  return uuid;
}

namespace {

template <typename T>
class Scoped_storage {
 public:
  std::shared_ptr<T> get() const {
    if (m_objects.empty())
      throw std::logic_error("Instance_pool not initialized!");
    return m_objects.top();
  }

  void push(const std::shared_ptr<T> &object) {
    assert(object);
    m_objects.push(object);
  }

  void pop(const std::shared_ptr<T> &object) {
    assert(!m_objects.empty() && m_objects.top() == object);
    (void)object;
    m_objects.pop();
  }

 private:
  std::stack<std::shared_ptr<T>> m_objects;
};

Scoped_storage<Instance_pool> g_ipool_storage;

}  // namespace

Scoped_instance_pool::Scoped_instance_pool(
    const std::shared_ptr<Instance_pool> &ipool)
    : m_pool{ipool} {
  g_ipool_storage.push(m_pool);
}

Scoped_instance_pool::~Scoped_instance_pool() { g_ipool_storage.pop(m_pool); }

std::shared_ptr<Instance_pool> Scoped_instance_pool::get() const {
  return m_pool;
}

std::shared_ptr<Instance_pool> current_ipool() {
  DBUG_TRACE;
  return g_ipool_storage.get();
}

void get_instance_lock_shared(const std::list<Scoped_instance> &instances,
                              unsigned int timeout,
                              const std::string &skip_uuid) {
  shcore::Scoped_callback_list revert_list;
  bool skip_warn = false;
  for (const auto &instance : instances) {
    // Skip the instance with the given UUID (if not empty).
    if (!skip_uuid.empty() && instance->get_uuid() == skip_uuid) {
      continue;
    }

    try {
      if (instance->get_lock_shared(timeout, skip_warn) == 1) {
        // Only print warning once if concurrent execution is not supported.
        skip_warn = true;
      }

      // Add the corresponding release operation to the revert list.
      revert_list.push_front([=]() { instance->release_lock(); });
    } catch (...) {
      // Release any previously acquired lock.
      revert_list.call();
      throw;
    }
  }

  revert_list.cancel();
}

void get_instance_lock_exclusive(const std::list<Scoped_instance> &instances,
                                 unsigned int timeout,
                                 const std::string &skip_uuid) {
  shcore::Scoped_callback_list revert_list;
  bool skip_warn = false;
  for (const auto &instance : instances) {
    // Skip the instance with the given UUID (if not empty).
    if (!skip_uuid.empty() && instance->get_uuid() == skip_uuid) {
      continue;
    }

    try {
      if (instance->get_lock_exclusive(timeout, skip_warn) == 1) {
        // Only print warning once if concurrent execution is not supported.
        skip_warn = true;
      }

      // Add the corresponding release operation to the revert list.
      revert_list.push_front([=]() { instance->release_lock(); });
    } catch (...) {
      // Release any previously acquired lock.
      revert_list.call();
      throw;
    }
  }

  revert_list.cancel();
}

void release_instance_lock(const std::list<Scoped_instance> &instances,
                           const std::string &skip_uuid) {
  for (const auto &instance : instances) {
    // Skip the instance with the given UUID (if not empty).
    if (!skip_uuid.empty() && instance->get_uuid() == skip_uuid) {
      continue;
    }

    try {
      instance->release_lock();
    } catch (const std::exception &err) {
      // Ignore any error trying to release locks and continue.
      log_info("Failed to release instance locks on %s: %s",
               instance->descr().c_str(), err.what());
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
