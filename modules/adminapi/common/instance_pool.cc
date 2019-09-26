/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <mysql.h>
#include <stack>

#include "modules/adminapi/common/dba_errors.h"
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

Instance::Instance(const std::shared_ptr<mysqlshdk::db::ISession> &session)
    : mysqlshdk::mysql::Instance(session) {}

Instance::Instance(Instance_pool *owner,
                   const std::shared_ptr<mysqlshdk::db::ISession> &session)
    : mysqlshdk::mysql::Instance(session), m_pool(owner) {}

void Instance::retain() {
  DBUG_TRACE;

  m_retain_count++;
}

void Instance::release() {
  DBUG_TRACE;

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

std::shared_ptr<mysqlshdk::db::IResult> Instance::query(const std::string &sql,
                                                        bool buffered) const {
  log_sql(sql);
  return mysqlshdk::mysql::Instance::query(sql, buffered);
}

void Instance::execute(const std::string &sql) const {
  log_sql(sql);
  mysqlshdk::mysql::Instance::execute(sql);
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

  try {
    auto session = establish_mysql_session(opts, m_allow_password_prompt);
    return add_leased_instance(std::make_shared<Instance>(this, session));
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        opts.uri_endpoint() + ": " + e.what(), e.code(), e.sqlstate());
  } catch (const shcore::Exception &e) {
    // Include the uri endpoint in the error message if another
    // shcore::Exception is issued (e.g., ER_ACCESS_DENIED_ERROR).
    throw shcore::Exception::mysql_error_with_code(
        opts.uri_endpoint() + ": " + e.what(), e.code());
  }
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
  return connect_unchecked(opts);
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

      return connect_unchecked(opts);
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
      } catch (shcore::Exception &e) {
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
      } catch (shcore::Exception &e) {
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
      } catch (shcore::Exception &e) {
        log_warning("%s: %s", i.endpoint.c_str(), e.format().c_str());
        if (e.code() > CR_ERROR_LAST) {
          num_reachable++;
        }
        continue;
      }

      try {
        check_group_member(*instance, true);
      } catch (shcore::Exception &e) {
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
  } catch (mysqlshdk::db::Error &err) {
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
    } catch (shcore::Exception &e) {
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

}  // namespace dba
}  // namespace mysqlsh
