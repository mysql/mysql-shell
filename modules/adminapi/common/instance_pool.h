/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_
#define MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_

#include <list>
#include <memory>
#include <set>
#include <string>
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
class Instance final : public mysqlshdk::mysql::Instance {
 public:
  // Simplified interface for creating Instances when a pool is not available

  // Session is prepared for executing non-trivial queries, like sql_mode
  // and autocommit being set to default values.
  static std::shared_ptr<Instance> connect(
      const mysqlshdk::db::Connection_options &copts, bool interactive = false);

  // Non-prepared version
  static std::shared_ptr<Instance> connect_raw(
      const mysqlshdk::db::Connection_options &copts, bool interactive = false);

 public:
  Instance() = default;

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
  void retain() noexcept;

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

  std::shared_ptr<mysqlshdk::db::IResult> query_udf(
      const std::string &sql, bool buffered = false) const override;

  void execute(const std::string &sql) const override;

  void prepare_session();

  void reconnect_if_needed(const char *what);

 public:
  /**
   * Try to acquire a shared lock on the instance.
   *
   * NOTE: Required lock service UDFs are automatically installed if needed.
   *
   * @param timeout maximum time in seconds to wait for the lock to be
   *        available if it cannot be obtained immediately. By default 0,
   *        meaning that it will not wait if the lock cannot be acquired
   *        immediately, issuing an error.
   *
   * @throw shcore::Exception if the lock cannot be acquired or any other error
   *        occur when trying to obtain the lock.
   *
   * @return int with the exit code of the function execution: 1 - a warning
   *         was issued because service lock UDFs could not be installed;
   *         2 - lock UDFs could not be installed (no warning);
   *         O - otherwise (success installing lock UDFs or already available).
   */
  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock_shared(
      std::chrono::seconds timeout = {});

  /**
   * Try to acquire an exclusive lock on the instance.
   *
   * NOTE: Required lock service UDFs are automatically installed if needed.
   *
   * @param timeout maximum time in seconds to wait for the lock to be
   *        available if it cannot be obtained immediately. By default 0,
   *        meaning that it will not wait if the lock cannot be acquired
   *        immediately, issuing an error.
   *
   * @throw shcore::Exception if the lock cannot be acquired or any other error
   *        occur when trying to obtain the lock.
   *
   * @return int with the exit code of the function execution: 1 - a warning
   *         was issued because service lock UDFs could not be installed;
   *         2 - lock UDFs could not be installed (no warning);
   *         O - otherwise (success installing lock UDFs or already available).
   */
  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock_exclusive(
      std::chrono::seconds timeout = {});

  /**
   * Check if the lock service is available and install if needed.
   *
   * NOTE: the locks are installed only if SRO is disabled
   *
   * @param can_disable_sro if true and in case SRO is enabled,
   *  toggles it temporarily to try and install the lock service.
   *
   * @return true if the lock service is already installed or if it was
   * installed successfully
   */
  bool ensure_lock_service_is_installed(bool can_disable_sro = false);

  /**
   * Check if the lock service is available
   *
   * @return true if the lock service is available
   */
  bool is_lock_service_installed() const;

 private:
  friend class Instance_pool;
  int m_retain_count = 1;
  Instance_pool *m_pool = nullptr;

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock(
      mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout = {});
};

class Scoped_instance_list final {
 public:
  explicit Scoped_instance_list(
      std::list<std::shared_ptr<Instance>> &&l) noexcept
      : m_list(std::move(l)) {}

  ~Scoped_instance_list() {
    for (const auto &i : m_list) {
      i->release();
    }
  }

  const std::list<std::shared_ptr<Instance>> &list() const { return m_list; }

 private:
  std::list<std::shared_ptr<Instance>> m_list;
};

struct Scoped_instance final {
  Scoped_instance() = default;

  Scoped_instance(const Scoped_instance &copy) noexcept : ptr(copy.ptr) {
    if (ptr) ptr->retain();
  }

  explicit Scoped_instance(std::shared_ptr<Instance> i) noexcept
      : ptr(std::move(i)) {}

  void operator=(const Scoped_instance &si) {
    if (ptr) ptr->release();
    ptr = si.ptr;
    if (ptr) ptr->retain();
  }

  ~Scoped_instance() {
    if (ptr) ptr->release();
  }

  explicit operator bool() const noexcept { return ptr.operator bool(); }
  operator std::shared_ptr<Instance>() const noexcept { return ptr; }
  operator Instance &() const noexcept { return *ptr.get(); }

  Instance *operator->() const noexcept { return ptr.get(); }
  Instance &operator*() const noexcept { return *ptr.get(); }

  Instance *get() const noexcept { return ptr.get(); }

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
class Instance_pool final {
 public:
  using Auth_options = mysqlshdk::mysql::Auth_options;

  explicit Instance_pool(bool allow_password_prompt);

  void set_default_auth_options(Auth_options auth_opts);

  const Auth_options &default_auth_opts() const { return m_default_auth_opts; }

  void set_metadata(std::shared_ptr<MetadataStorage> md);

  std::shared_ptr<MetadataStorage> get_metadata() { return m_metadata; }

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

  std::shared_ptr<Instance> try_connect_cluster_primary_with_fallback(
      const Cluster_id &cluster_id,
      Cluster_availability *out_cluster_availability);

  void check_group_member(const mysqlshdk::mysql::IInstance &instance,
                          bool allow_recovering,
                          std::string *out_member_id = nullptr,
                          std::string *out_group_name = nullptr,
                          bool *out_single_primary_mode = nullptr,
                          bool *out_is_primary = nullptr);

  std::shared_ptr<Instance> connect_cluster_member_of(
      const std::shared_ptr<Instance> &instance);

  void refresh_metadata_cache();

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
class Scoped_instance_pool final {
 public:
  explicit Scoped_instance_pool(std::shared_ptr<Instance_pool> pool);

  Scoped_instance_pool(std::shared_ptr<MetadataStorage> metadata,
                       bool interactive, Instance_pool::Auth_options def_auth)
      : Scoped_instance_pool(std::make_shared<Instance_pool>(interactive)) {
    m_pool->set_metadata(std::move(metadata));
    m_pool->set_default_auth_options(std::move(def_auth));
  }

  Scoped_instance_pool(bool interactive,
                       const Instance_pool::Auth_options &def_auth)
      : Scoped_instance_pool(std::make_shared<Instance_pool>(interactive)) {
    m_pool->set_default_auth_options(def_auth);
  }

  ~Scoped_instance_pool() noexcept;

  Scoped_instance_pool(const Scoped_instance_pool &) = delete;
  Scoped_instance_pool(Scoped_instance_pool &&) = delete;
  Scoped_instance_pool &operator=(const Scoped_instance_pool &) = delete;
  Scoped_instance_pool &operator=(Scoped_instance_pool &&) = delete;

  std::shared_ptr<Instance_pool> get() const noexcept { return m_pool; }

  Instance_pool *operator->() { return m_pool.get(); }
  Instance_pool &operator*() { return *m_pool.get(); }

 private:
  std::shared_ptr<Instance_pool> m_pool;
};

std::shared_ptr<Instance_pool> current_ipool();

template <class InputIter>
std::list<shcore::Dictionary_t> execute_in_parallel(
    InputIter begin, InputIter end,
    std::function<void(const std::shared_ptr<Instance> &instance)> fn) {
  std::list<shcore::Dictionary_t> errors;

  mysqlshdk::utils::map_reduce<bool, shcore::Dictionary_t>(
      begin, end,
      [&fn](const std::shared_ptr<Instance> &inst) -> shcore::Dictionary_t {
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
[[nodiscard]] mysqlshdk::mysql::Lock_scoped_list get_instance_lock_shared(
    const std::list<std::shared_ptr<Instance>> &instances,
    std::chrono::seconds timeout = {}, std::string_view skip_uuid = "");

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
[[nodiscard]] mysqlshdk::mysql::Lock_scoped_list get_instance_lock_exclusive(
    const std::list<std::shared_ptr<Instance>> &instances,
    std::chrono::seconds timeout = {}, std::string_view skip_uuid = "");

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_INSTANCE_POOL_H_
