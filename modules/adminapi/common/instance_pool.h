/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include <list>
#include <memory>
#include <set>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/lock_service.h"
#include "mysqlshdk/libs/utils/threads.h"

namespace mysqlsh {
namespace dba {

namespace topology {
class Node;
}

class MetadataStorage;
class Instance_pool;
class Instance : public mysqlshdk::mysql::Instance {
 public:
  // Simplified interface for creating Instances when a pool is not available

  // Session is prepared for executing non-trivial queries, like sql_mode
  // and autocommit being set to default values.
  static std::shared_ptr<Instance> connect(
      const mysqlshdk::db::Connection_options &copts, bool interactive = false,
      bool show_tls_deprecation = false);

  // Non-prepared version
  static std::shared_ptr<Instance> connect_raw(
      const mysqlshdk::db::Connection_options &copts, bool interactive = false,
      bool show_tls_deprecation = false);

 public:
  Instance() {}

  // Wrap a session object. release() will not be effective.
  explicit Instance(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  // Wrap a session for the pool. The session will be released back to the pool
  // or closed when Instance is released
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

  void prepare_session();

  /**
   * Try to acquire a shared lock on the instance.
   *
   * NOTE: Required lock service UDFs are automatically installed if needed.
   *
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
  int get_lock_shared(unsigned int timeout = 0,
                      bool skip_fail_install_warn = false);

  /**
   * Try to acquire an exclusive lock on the instance.
   *
   * NOTE: Required lock service UDFs are automatically installed if needed.
   *
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
  int get_lock_exclusive(unsigned int timeout = 0,
                         bool skip_fail_install_warn = false);

  /**
   * Release all locks on the instance.
   *
   * @param no_throw boolean indicating if exceptions are thrown in case a
   *                 failure occur releasing locks. By default, true meaning
   *                 that no exception is thrown.
   *
   */
  void release_lock(bool no_throw = true) const;

 private:
  friend class Instance_pool;
  int m_retain_count = 1;
  Instance_pool *m_pool = nullptr;

  void log_sql(const std::string &sql) const;
  void log_sql_error(const shcore::Error &e) const;

  int ensure_lock_service_udfs_installed(bool skip_fail_install_warn);
  int get_lock(mysqlshdk::mysql::Lock_mode mode, unsigned int timeout = 0,
               bool skip_fail_install_warn = false);
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

  std::shared_ptr<MetadataStorage> get_metadata() { return m_metadata; }

  ~Instance_pool();

  std::shared_ptr<MetadataStorage> find_metadata_server(
      const std::shared_ptr<mysqlshdk::db::ISession> &session);

  // Add an externally created, unmanaged instance to the pool.
  // The caller is still responsible for calling release() on it.
  std::shared_ptr<Instance> adopt(const std::shared_ptr<Instance> &instance);

  // Connect to the specified instance without doing any checks
  std::shared_ptr<Instance> connect_unchecked(
      const mysqlshdk::db::Connection_options &opts,
      bool show_tls_deprecation = false);

  // Same as above, but by uuid
  std::shared_ptr<Instance> connect_unchecked_uuid(const std::string &uuid);

  std::shared_ptr<Instance> connect_unchecked_endpoint(
      const std::string &endpoint, bool allow_url = false);

  // Connect to the node. If node is a group, picks any member from it.
  std::shared_ptr<Instance> connect_unchecked(const topology::Node *node);

  // Connect to the PRIMARY of the cluster
  std::shared_ptr<Instance> connect_primary(const topology::Node *node);

  std::shared_ptr<Instance> connect_async_cluster_primary(
      Cluster_id cluster_id);

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

  std::shared_ptr<Instance> connect_primary_master_for_member(
      const std::string &server_uuid);

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

template <class InputIter>
std::list<shcore::Dictionary_t> execute_in_parallel(
    InputIter begin, InputIter end,
    std::function<void(const Scoped_instance &instance)> fn) {
  std::list<shcore::Dictionary_t> errors;

  mysqlshdk::utils::map_reduce<bool, shcore::Dictionary_t>(
      begin, end,
      [&fn](const Scoped_instance &inst) -> shcore::Dictionary_t {
        mysqlsh::Mysql_thread thdinit;

        try {
          fn(inst);
        } catch (const shcore::Exception &e) {
          log_debug("%s: %s", inst->descr().c_str(), e.format().c_str());

          shcore::Dictionary_t dict = e.error();
          // NOTE: Ensure the UUID is cached on the instance before calling
          //       execute_in_parallel() otherwise it might fail here trying
          //       to query an unavailable instance.
          dict->set("uuid", shcore::Value(inst->get_uuid()));
          dict->set("from", shcore::Value(inst->descr()));
          dict->set("errmsg", shcore::Value(e.format()));
          return dict;
        } catch (const std::exception &e) {
          log_debug("%s: %s", inst->descr().c_str(), e.what());

          return shcore::make_dict("errmsg", shcore::Value(e.what()), "from",
                                   shcore::Value(inst->descr()));
        }
        return nullptr;
      },
      // Collects returned into a list (return value ignored)
      [&errors](bool total, shcore::Dictionary_t partial) -> bool {
        if (partial) errors.push_back(partial);
        return total;
      });

  return errors;
}

/**
 * Try to acquire a shared lock on all the given instances.
 *
 * NOTE: If it fails to acquire the lock on an instance, then the lock is
 *       released on all previous instances.
 *
 * @param instances List of instances to get the lock.
 * @param timeout maximum time in seconds to wait for the lock to be
 *        available if it cannot be obtained immediately. By default 0,
 *        meaning that it will not wait if the lock cannot be acquired
 *        immediately, issuing an error.
 * @param skip_uuid UUID of an instance to be ignored from the list (no lock
 *        acquired). By default "", no instance skipped.
 *
 * @throw shcore::Exception if the lock cannot be acquired or any other error
 *        occur when trying to obtain the lock.
 */
void get_instance_lock_shared(const std::list<Scoped_instance> &instances,
                              unsigned int timeout = 0,
                              const std::string &skip_uuid = "");

/**
 * Try to acquire an exclusive lock on all the given instances.
 *
 * NOTE: If it fails to acquire the lock on an instance, then the lock is
 *       released on all previous instances.
 *
 * @param instances List of instances to get the lock.
 * @param timeout maximum time in seconds to wait for the lock to be
 *        available if it cannot be obtained immediately. By default 0,
 *        meaning that it will not wait if the lock cannot be acquired
 *        immediately, issuing an error.
 * @param skip_uuid UUID of an instance to be ignored from the list (no lock
 *        acquired). By default "", no instance skipped.
 *
 * @throw shcore::Exception if the lock cannot be acquired or any other error
 *        occur when trying to obtain the lock.
 */
void get_instance_lock_exclusive(const std::list<Scoped_instance> &instances,
                                 unsigned int timeout = 0,
                                 const std::string &skip_uuid = "");

/**
 * Release all instance locks for all the given instances.
 *
 * @param instances List of instances to release the locks.
 * @param skip_uuid UUID of an instance to be ignored from the list (no lock
 *        released). By default "", no instance skipped.
 *
 */
void release_instance_lock(const std::list<Scoped_instance> &instances,
                           const std::string &skip_uuid = "");

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_
