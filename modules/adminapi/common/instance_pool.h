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

#ifndef MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_
#define MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_

#include <string>

#include "modules/adminapi/common/cluster_types.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

class MetadataStorage;
class Instance_pool;
class Instance : public mysqlshdk::mysql::Instance {
 public:
  Instance(){};
  explicit Instance(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  Instance(Instance_pool *owner,
           const std::shared_ptr<mysqlshdk::db::ISession> &session);

  // Increment retain count of the instance, so that the pool won't give it
  // to someone else. Ownership of the instance still belongs to the pool.
  // Call it when keeping a reference to an Instance that obtained from
  // elsewhere.
  void retain();

  // Decrement retain count of the instance. Once the count goes to 0,
  // it will be returned to the pool, which can give it to someone else or
  // close it. If the instance does not belong to a pool, it will be closed.
  // Call if when no longer using the Instance, if the Instance was obtained
  // from the pool or if it was keeping a retain()ed reference.
  void release();

  // Take ownership of the instance from the pool.
  // retain()/release() rules stay unchanged.
  // Call it if the Instance should no longer be managed by the pool.
  void steal();

  std::shared_ptr<mysqlshdk::db::IResult> query(
      const std::string &sql, bool buffered = false) const override;

  void execute(const std::string &sql) const override;

 private:
  friend class Instance_pool;
  int m_retain_count = 1;
  Instance_pool *m_pool = nullptr;

  void log_sql(const std::string &sql) const;
};

struct Scoped_instance {
  Scoped_instance() {}

  Scoped_instance(const Scoped_instance &copy) : ptr(copy.ptr) {
    if (ptr) ptr->retain();
  }

  explicit Scoped_instance(const std::shared_ptr<Instance> &i) : ptr(i) {}

  void operator=(const Scoped_instance &si) {
    if (ptr) ptr->release();
    ptr = si.ptr;
    if (ptr) ptr->retain();
  }

  ~Scoped_instance() {
    if (ptr) ptr->release();
  }

  operator bool() const { return ptr.get() != nullptr; }
  operator std::shared_ptr<Instance>() const { return ptr; }
  operator Instance &() const { return *ptr.get(); }

  Instance *operator->() const { return ptr.get(); }
  Instance &operator*() const { return *ptr.get(); }

  Instance *get() const { return ptr.get(); }

  std::shared_ptr<Instance> ptr;
};

/**
 * A pool of DB sessions/instances.
 *
 * Use by allocating one before calling a long task that needs several DB
 * connections is about to start. The pool will provide sessions as they're
 * acquired, automatically creating or recycling them as needed. All sessions
 * are closed when the pool is destroyed (after the task is done).
 */
class Instance_pool {
 public:
  using Auth_options = mysqlshdk::mysql::Auth_options;

  explicit Instance_pool(bool allow_password_prompt);

  void set_default_auth_options(const Auth_options &auth_opts);

  const Auth_options &default_auth_opts() const { return m_default_auth_opts; }

  void set_metadata(const std::shared_ptr<MetadataStorage> &md);

  ~Instance_pool();

  std::shared_ptr<MetadataStorage> find_metadata_server(
      const std::shared_ptr<mysqlshdk::db::ISession> &session);

  // Add an externally created, unmanaged instance to the pool.
  // The caller is still responsible for calling release() on it.
  std::shared_ptr<Instance> adopt(const std::shared_ptr<Instance> &instance);

  // Connect to the specified instance without doing any checks
  std::shared_ptr<Instance> connect_unchecked(
      const mysqlshdk::db::Connection_options &opts);

  // Same as above, but by uuid
  std::shared_ptr<Instance> connect_unchecked_uuid(const std::string &uuid);

  std::shared_ptr<Instance> connect_unchecked_endpoint(
      const std::string &endpoint, bool allow_url = false);

  // Connect to the PRIMARY of the replicaset
  std::shared_ptr<Instance> connect_group_primary(
      const std::string &group_name);

  // Connect to a SECONDARY of the replicaset
  std::shared_ptr<Instance> connect_group_secondary(
      const std::string &group_name);

  // Connect to any member within a quorum of the replicaset
  std::shared_ptr<Instance> connect_group_member(const std::string &group_name);

  void check_group_member(const mysqlshdk::mysql::IInstance &instance,
                          bool allow_recovering,
                          std::string *out_member_id = nullptr,
                          std::string *out_group_name = nullptr,
                          bool *out_single_primary_mode = nullptr,
                          bool *out_is_primary = nullptr);

  std::shared_ptr<Instance> connect_cluster_member_of(
      const std::shared_ptr<Instance> &instance);

 private:
  std::shared_ptr<MetadataStorage> m_metadata;

  friend class Instance;
  struct Pool_entry {
    std::shared_ptr<Instance> instance;
    bool leased = false;
  };

  std::shared_ptr<Instance> add_leased_instance(
      std::shared_ptr<Instance> instance);
  void return_instance(Instance *instance);
  std::shared_ptr<Instance> forget_instance(Instance *instance);

  void refresh_metadata_cache();

  std::string label_for_server_uuid(const std::string &uuid);

  std::shared_ptr<Instance> try_connect_primary_through_member(
      const std::string &member_uuid);

  void set_auth_opts(const Auth_options &auth,
                     mysqlshdk::db::Connection_options *opts);

  std::list<Pool_entry> m_pool;
  Auth_options m_default_auth_opts;
  struct Metadata_cache;
  Metadata_cache *m_mdcache = nullptr;
  // UUID of servers that were a primary until recently
  std::set<std::string> m_recent_primaries;
  bool m_allow_password_prompt;
};

/**
 * Creates and registers an instance pool in the current scope.
 * Once the scope leaves, all sessions that are still owned by the pool will
 * be closed.
 *
 * The current active pool can be accessed globally through the
 * current_ipool() method.
 */
class Scoped_instance_pool {
 public:
  explicit Scoped_instance_pool(const std::shared_ptr<Instance_pool> &pool);

  Scoped_instance_pool(const std::shared_ptr<MetadataStorage> &metadata,
                       bool interactive,
                       const Instance_pool::Auth_options &def_auth)
      : Scoped_instance_pool(std::make_shared<Instance_pool>(interactive)) {
    m_pool->set_metadata(metadata);
    m_pool->set_default_auth_options(def_auth);
  }

  Scoped_instance_pool(bool interactive,
                       const Instance_pool::Auth_options &def_auth)
      : Scoped_instance_pool(std::make_shared<Instance_pool>(interactive)) {
    m_pool->set_default_auth_options(def_auth);
  }

  ~Scoped_instance_pool();

  Scoped_instance_pool(const Scoped_instance_pool &) = delete;
  Scoped_instance_pool(Scoped_instance_pool &&) = delete;
  Scoped_instance_pool &operator=(const Scoped_instance_pool &) = delete;
  Scoped_instance_pool &operator=(Scoped_instance_pool &&) = delete;

  std::shared_ptr<Instance_pool> get() const;

  Instance_pool *operator->() { return m_pool.get(); }
  Instance_pool &operator*() { return *m_pool.get(); }

 private:
  std::shared_ptr<Instance_pool> m_pool;
};

std::shared_ptr<Instance_pool> current_ipool();

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_