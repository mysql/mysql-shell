/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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
#include "modules/adminapi/common/sql.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/include/scripting/types.h"  // exceptions
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh {
namespace dba {

namespace {

// default SQL_MODE as of 8.0.19
constexpr const char *k_default_sql_mode =
    "ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,"
    "NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION";

// Constants with the names used to lock instances.
constexpr char k_lock[] = "AdminAPI_lock";
constexpr char k_lock_name_instance[] = "AdminAPI_instance";

int64_t default_adminapi_connect_timeout() {
  return mysqlsh::current_shell_options()->get().dba_connect_timeout * 1000;
}

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
         // TODO(alfredo) - when connection timeout is enabled, the error
         // returned for a timeout seems to be this too. This error handling
         // should probably be removed from here and the connect timeout setting
         // should be moved from connect_raw to this function
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

std::shared_ptr<Instance> Instance::connect_raw(
    const mysqlshdk::db::Connection_options &opts, bool interactive) {
  mysqlshdk::db::Connection_options op(opts);

  if (!op.has_value(mysqlshdk::db::kConnectTimeout)) {
    op.set(mysqlshdk::db::kConnectTimeout,
           std::to_string(default_adminapi_connect_timeout()));
  }

  return std::make_shared<Instance>(connect_session(op, interactive));
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

void Instance::retain() noexcept {
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

std::shared_ptr<mysqlshdk::db::IResult> Instance::query(const std::string &sql,
                                                        bool buffered) const {
  return mysqlshdk::mysql::Instance::query(sql, buffered);
}

std::shared_ptr<mysqlshdk::db::IResult> Instance::query_udf(
    const std::string &sql, bool buffered) const {
  return mysqlshdk::mysql::Instance::query_udf(sql, buffered);
}

void Instance::execute(const std::string &sql) const {
  mysqlshdk::mysql::Instance::execute(sql);
}

bool Instance::ensure_lock_service_is_installed(bool can_disable_sro) {
  if (is_lock_service_installed()) return true;

  try {
    // Check if instance is read-only otherwise it will fail to install
    // the lock service (e.g., it might happen for clusters created with an
    // older MySQL Shell version).
    bool super_read_only = get_sysvar_bool("super_read_only", false);
    if (super_read_only) {
      if (!can_disable_sro) {
        log_info(
            "Unable to install the lock service because the instance '%s' has "
            "super_read_only enabled.",
            descr().c_str());
        return false;
      }

      set_sysvar("super_read_only", false,
                 mysqlshdk::mysql::Var_qualifier::GLOBAL);
    }

    shcore::on_leave_scope restore_read_only([super_read_only, this]() {
      if (super_read_only)
        set_sysvar("super_read_only", true,
                   mysqlshdk::mysql::Var_qualifier::GLOBAL);
    });

    log_debug("Installing the lock service on '%s'", descr().c_str());
    mysqlshdk::mysql::install_lock_service(this);

    return true;

  } catch (const std::exception &err) {
    log_warning("Failed to install the lock service on '%s': %s",
                descr().c_str(), err.what());

    return false;
  }
}

bool Instance::is_lock_service_installed() const {
  return mysqlshdk::mysql::has_lock_service(*this);
}

/**
 * Try to acquire a specific lock on the instance.
 *
 * @param mode type of lock to acquire: READ (shared) or WRITE (exclusive).
 * @param timeout maximum time in seconds to wait for the lock to be
 *        available if it cannot be obtained immediately. By default 0,
 *        meaning that it will not wait if the lock cannot be acquired
 *        immediately, issuing an error.
 *
 * @throw shcore::Exception if the lock cannot be acquired. The method tries to
 * acquire the lock only when the lock service is available)
 *
 * @return the requested lock (which might be empty if it couldn't be obtained)
 */

mysqlshdk::mysql::Lock_scoped Instance::get_lock(
    mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout) {
  if (auto has_lock_service = is_lock_service_installed(); !has_lock_service) {
    try {
      switch (get_gr_instance_type(*this)) {
        case TargetType::Standalone:
          has_lock_service = ensure_lock_service_is_installed(true);
          break;
        default:
          break;
      }

    } catch (const shcore::Error &e) {
      log_debug("Unable to check and install the lock service: %s", e.what());
    }

    if (!has_lock_service) {
      log_warning(
          "The required MySQL Locking Service isn't installed on instance "
          "'%s'. The operation will continue without concurrent execution "
          "protection.",
          descr().c_str());

      return nullptr;
    }
  }

  DBUG_EXECUTE_IF("dba_locking_timeout_one",
                  { timeout = std::chrono::seconds{1}; });

  // Try to acquire the specified lock.
  // NOTE: Only one lock per namespace is used because lock release is
  //       performed by namespace.
  try {
    log_debug("Acquiring %s lock ('%s', '%s') on '%s'.",
              mysqlshdk::mysql::to_string(mode).c_str(), k_lock_name_instance,
              k_lock, descr().c_str());
    mysqlshdk::mysql::get_lock(*this, k_lock_name_instance, k_lock, mode,
                               timeout.count());
  } catch (const shcore::Error &err) {
    // Abort the operation in case the required lock cannot be acquired.
    log_info("Failed to get %s lock ('%s', '%s') on '%s': %s",
             mysqlshdk::mysql::to_string(mode).c_str(), k_lock_name_instance,
             k_lock, descr().c_str(), err.what());

    if (err.code() == ER_LOCKING_SERVICE_TIMEOUT) {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "lock on instance '%s'. Another operation requiring access to the "
          "instance is still in progress, please wait for it to finish and try "
          "again.",
          descr().c_str()));
      throw shcore::Exception(
          shcore::str_format("Failed to acquire lock on instance '%s'",
                             descr().c_str()),
          SHERR_DBA_LOCK_GET_FAILED);
    } else {
      current_console()->print_error(shcore::str_format(
          "The operation cannot be executed because it failed to acquire the "
          "lock on instance '%s': %s",
          descr().c_str(), err.what()));

      throw;
    }
  }

  auto release_cb = [this]() {
    // Release all instance locks in the k_lock_name_instance namespace.
    // NOTE: Only perform the operation if the lock service is
    // available
    //       otherwise do nothing (ignore if concurrent execution is not
    //       supported, e.g., lock service plugin not available).
    try {
      log_debug("Releasing locks for '%s' on %s.", k_lock_name_instance,
                descr().c_str());
      mysqlshdk::mysql::release_lock(*this, k_lock_name_instance);

    } catch (const shcore::Error &error) {
      // Ignore any error trying to release locks (e.g., might have not
      // been previously acquired due to lack of permissions).
      log_error("Unable to release '%s' locks for '%s': %s",
                k_lock_name_instance, descr().c_str(), error.what());
    }
  };

  return mysqlshdk::mysql::Lock_scoped{std::move(release_cb)};
}

mysqlshdk::mysql::Lock_scoped Instance::get_lock_shared(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::SHARED, timeout);
}

mysqlshdk::mysql::Lock_scoped Instance::get_lock_exclusive(
    std::chrono::seconds timeout) {
  return get_lock(mysqlshdk::mysql::Lock_mode::EXCLUSIVE, timeout);
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

  const std::string &get_cluster_name(const std::string &group_name) const {
    for (const auto &c : clusters) {
      if (c.group_name == group_name) return c.cluster_name;
    }
    throw shcore::Exception(
        "Could not find metadata for cluster with group_name '" + group_name +
            "'",
        SHERR_DBA_METADATA_MISSING);
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

void Instance_pool::set_default_auth_options(Auth_options opts) {
  m_default_auth_opts = std::move(opts);
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

void Instance_pool::set_metadata(std::shared_ptr<MetadataStorage> md) {
  DBUG_TRACE;

  m_metadata = std::move(md);
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

  log_debug("Refreshing metadata cache from '%s'",
            m_metadata->get_md_server()->descr().c_str());
  m_mdcache->instances = m_metadata->get_all_instances();
  m_mdcache->clusters = m_metadata->get_all_clusters(true);

  for (const auto &i : m_mdcache->instances) {
    auto it = std::find_if(
        m_mdcache->clusters.begin(), m_mdcache->clusters.end(),
        [&i](const auto &c) { return (c.cluster_id == i.cluster_id); });
    if (it == m_mdcache->clusters.end())
      log_debug("I) %s %s", i.label.c_str(), i.endpoint.c_str());
    else
      log_debug("I) %s %s (%s)", i.label.c_str(), i.endpoint.c_str(),
                it->cluster_name.c_str());
  }

  for (const auto &i : m_mdcache->clusters) {
    log_debug("C) %s '%s'", i.group_name.c_str(), i.cluster_name.c_str());
  }
  log_debug("DONE!");
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

  return Instance::connect(opts, m_allow_password_prompt);
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
  assert(cluster);
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
          instance, &member_state, out_member_id, out_group_name, nullptr,
          out_single_primary_mode, &has_quorum, out_is_primary)) {
    throw shcore::Exception(
        "Group Replication is not running at instance " + instance.descr(),
        SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING);
  }

  if (!has_quorum && member_state != mysqlshdk::gr::Member_state::OFFLINE) {
    throw shcore::Exception("Member " +
                                label_for_server_uuid(instance.get_uuid()) +
                                " is not part of a majority group",
                            SHERR_DBA_GROUP_MEMBER_NOT_IN_QUORUM);
  }

  if ((member_state == mysqlshdk::gr::Member_state::ONLINE) ||
      (member_state == mysqlshdk::gr::Member_state::RECOVERING &&
       allow_recovering))
    return;

  throw shcore::Exception(
      "member " + label_for_server_uuid(instance.get_uuid()) + " is in state " +
          mysqlshdk::gr::to_string(member_state),
      SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);
}

std::shared_ptr<Instance> Instance_pool::connect_primary(
    const topology::Node *node) {
  DBUG_TRACE;

  const Cluster_metadata *cluster =
      m_mdcache->try_get_cluster(node->cluster_id);
  assert(cluster);
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

  try {
    // first try to find someone that was the primary last time we saw it
    for (const auto &i : m_mdcache->instances) {
      if (i.group_name == group_name) {
        if (m_recent_primaries.find(i.uuid) != m_recent_primaries.end()) {
          // instance was a PRIMARY last time we saw it, try using it as a
          // starting point to find the current PRIMARY
          try {
            auto instance = try_connect_primary_through_member(i.uuid);
            if (instance) {
              return instance;
            }
          } catch (const shcore::Exception &e) {
            log_warning("Could not connect to %s: %s", i.endpoint.c_str(),
                        e.format().c_str());

            candidates.push_back(i.uuid);
          }
        } else {
          candidates.push_back(i.uuid);
        }
      }
    }

    if (candidates.empty()) {
      const auto &cname = m_mdcache->get_cluster_name(group_name);

      log_warning(
          "Could not find any members in metadata for cluster '%s', group_name "
          "%s",
          cname.c_str(), group_name.c_str());

      throw shcore::Exception(
          "No managed members found for cluster '" + cname + "'",
          SHERR_DBA_METADATA_MISSING);
    }

    // we don't know any possible primaries, so just try everyone one by one
    for (const auto &uuid : candidates) {
      try {
        auto instance = try_connect_primary_through_member(uuid);
        if (instance) {
          return instance;
        }
      } catch (const shcore::Exception &err) {
        if (err.code() != SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING) throw;
      }
    }
  } catch (const shcore::Exception &e) {
    if (e.code() == SHERR_DBA_GROUP_HAS_NO_QUORUM) {
      throw shcore::Exception("Cluster '" +
                                  m_mdcache->get_cluster_name(group_name) +
                                  "' has no quorum",
                              SHERR_DBA_GROUP_HAS_NO_QUORUM);
    }
    throw;
  }
  // there are no primaries anywhere
  throw shcore::Exception("Could not connect to a PRIMARY member of cluster '" +
                              m_mdcache->get_cluster_name(group_name) + "'",
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
            "Target cluster '" + m_mdcache->get_cluster_name(group_name) +
            "' is configured for multi-primary mode and has no "
            "SECONDARY members");
      }
    }
  }

  if (num_reachable == 0)
    throw shcore::Exception(
        "Could not connect to any SECONDARY member of the target InnoDB "
        "cluster '" +
            m_mdcache->get_cluster_name(group_name) + "'",
        SHERR_DBA_GROUP_UNREACHABLE);

  throw shcore::Exception(
      "Could not find any SECONDARY member in the target InnoDB cluster '" +
          m_mdcache->get_cluster_name(group_name) + "'",
      SHERR_DBA_GROUP_UNAVAILABLE);
}

std::shared_ptr<Instance> Instance_pool::connect_group_member(
    const std::string &group_name) {
  DBUG_TRACE;
  int num_reachable = 0;
  bool seen_no_quorum = false;

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
        if (e.code() == SHERR_DBA_GROUP_MEMBER_NOT_IN_QUORUM) {
          seen_no_quorum = true;
        }
        continue;
      }
      return instance;
    }
  }

  if (num_reachable == 0)
    throw shcore::Exception("Could not connect to any member of cluster '" +
                                m_mdcache->get_cluster_name(group_name) + "'",
                            SHERR_DBA_GROUP_UNREACHABLE);

  if (seen_no_quorum)
    throw shcore::Exception("Cluster '" +
                                m_mdcache->get_cluster_name(group_name) +
                                "' has no quorum",
                            SHERR_DBA_GROUP_HAS_NO_QUORUM);

  throw shcore::Exception("Could not find any available member in cluster '" +
                              m_mdcache->get_cluster_name(group_name) + "'",
                          SHERR_DBA_GROUP_UNAVAILABLE);
}

std::shared_ptr<Instance>
Instance_pool::try_connect_cluster_primary_with_fallback(
    const Cluster_id &cluster_id,
    Cluster_availability *out_cluster_availability) {
  DBUG_TRACE;

  // The metadata is refreshed when the pool is instantiated (with every
  // command call), refresh again only if the instances/clusters cache is empty
  if (m_mdcache->instances.empty() || m_mdcache->clusters.empty()) {
    refresh_metadata_cache();
  }

  auto cluster_md = m_mdcache->try_get_cluster(cluster_id);
  if (!cluster_md) {
    throw shcore::Exception(
        shcore::str_format("Could not find metadata for Cluster '%s'",
                           cluster_id.c_str()),
        SHERR_DBA_METADATA_MISSING);
  }

  std::shared_ptr<Instance> best;

  std::shared_ptr<Instance> secondary;
  std::shared_ptr<Instance> not_in_quorum;
  std::shared_ptr<Instance> not_online;

  int num_offline = 0;
  int num_total = 0;
  for (const auto &i : m_mdcache->instances) {
    if (i.group_name != cluster_md->group_name) continue;

    num_total++;

    std::shared_ptr<Instance> instance;

    try {
      instance = connect_unchecked_uuid(i.uuid);
    } catch (const shcore::Exception &e) {
      log_warning("%s: %s", i.endpoint.c_str(), e.format().c_str());
      // client errors are expected for non-running servers etc, but server
      // side errors are not
      if (e.code() > CR_ERROR_LAST) {
        current_console()->print_warning("While connecting to " + i.endpoint +
                                         ": " + e.format());
      }
      continue;
    }

    try {
      bool is_single_primary = false;
      bool is_primary = false;
      check_group_member(*instance, true, nullptr, nullptr, &is_single_primary,
                         &is_primary);

      if (!is_single_primary || is_primary) {
        *out_cluster_availability = Cluster_availability::ONLINE;
        best = instance;
        break;
      } else {
        if (!secondary)
          secondary = instance;
        else
          instance->release();
      }
    } catch (const shcore::Exception &e) {
      switch (e.code()) {
        case SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING:
        case SHERR_DBA_GROUP_MEMBER_NOT_ONLINE:
          num_offline++;

          if (!not_online)
            not_online = instance;
          else
            instance->release();
          break;
        case SHERR_DBA_GROUP_MEMBER_NOT_IN_QUORUM:
          if (!not_in_quorum)
            not_in_quorum = instance;
          else
            instance->release();
          break;
        default:
          assert(0);
          current_console()->print_error(
              "Unexpected error checking membership status of " + i.endpoint +
              ":" + e.format());
          break;
      }
    }
  }

  if (!best && secondary) {
    best = secondary;
    *out_cluster_availability = Cluster_availability::ONLINE_NO_PRIMARY;
  }
  if (!best && not_in_quorum) {
    best = not_in_quorum;
    *out_cluster_availability = Cluster_availability::NO_QUORUM;
  }
  if (!best && not_online) {
    best = not_online;
    if (num_offline == num_total)
      *out_cluster_availability = Cluster_availability::OFFLINE;
    else
      *out_cluster_availability = Cluster_availability::SOME_UNREACHABLE;
  }

  if (best != secondary && secondary) secondary->release();
  if (best != not_in_quorum && not_in_quorum) not_in_quorum->release();
  if (best != not_online && not_online) not_online->release();

  if (!best) {
    *out_cluster_availability = Cluster_availability::UNREACHABLE;
  }

  return best;
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
    } catch (const std::exception &e) {
      instance->release();
      if (shcore::str_beginswith(
              e.what(), "Group replication does not seem to be active"))
        throw shcore::Exception(e.what(),
                                SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING);
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

    // member_uuid is probably either out of the group or in a partition
    // with no quorum
    instance->release();

    return {};
  } catch (const shcore::Error &err) {
    // ignore any DB connect errors that could be due to lack of
    // availability
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

Scoped_instance_pool::Scoped_instance_pool(std::shared_ptr<Instance_pool> ipool)
    : m_pool{std::move(ipool)} {
  g_ipool_storage.push(m_pool);
}

Scoped_instance_pool::~Scoped_instance_pool() noexcept {
  g_ipool_storage.pop(m_pool);
}

std::shared_ptr<Instance_pool> current_ipool() {
  DBUG_TRACE;
  return g_ipool_storage.get();
}

[[nodiscard]] mysqlshdk::mysql::Lock_scoped_list get_instance_lock_shared(
    const std::list<std::shared_ptr<Instance>> &instances,
    std::chrono::seconds timeout, std::string_view skip_uuid) {
  mysqlshdk::mysql::Lock_scoped_list locks;
  for (const auto &instance : instances) {
    // Skip the instance with the given UUID (if not empty).
    if (!skip_uuid.empty() && instance->get_uuid() == skip_uuid) continue;

    try {
      // Add the corresponding release operation to the revert list.
      if (auto i_lock = instance->get_lock_shared(timeout); i_lock)
        locks.push_back(std::move(i_lock), instance);

    } catch (...) {
      // Release any previously acquired lock.
      locks.invoke();
      throw;
    }
  }

  return locks;
}

[[nodiscard]] mysqlshdk::mysql::Lock_scoped_list get_instance_lock_exclusive(
    const std::list<std::shared_ptr<Instance>> &instances,
    std::chrono::seconds timeout, std::string_view skip_uuid) {
  mysqlshdk::mysql::Lock_scoped_list locks;
  for (const auto &instance : instances) {
    // Skip the instance with the given UUID (if not empty).
    if (!skip_uuid.empty() && instance->get_uuid() == skip_uuid) continue;

    try {
      // Add the corresponding release operation to the revert list.
      if (auto i_lock = instance->get_lock_exclusive(timeout); i_lock)
        locks.push_back(std::move(i_lock), instance);

    } catch (...) {
      // Release any previously acquired lock.
      locks.invoke();
      throw;
    }
  }

  return locks;
}

}  // namespace dba
}  // namespace mysqlsh
