/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

// This module implements basic primitives to use the MySQL Locking Service
// UDF Interface. For more information, see:
// https://dev.mysql.com/doc/en/locking-service-udf-interface.html

#ifndef MYSQLSHDK_LIBS_MYSQL_LOCK_SERVICE_H_
#define MYSQLSHDK_LIBS_MYSQL_LOCK_SERVICE_H_

#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

/**
 * Supported lock modes: SHARED (read), EXCLUSIVE (write).
 */
enum class Lock_mode {
  NONE,
  SHARED,
  EXCLUSIVE,
};

class [[nodiscard]] Lock_scoped {
  friend class Lock_scoped_list;

 public:
  explicit Lock_scoped(std::function<void()> c) noexcept
      : m_callback{std::move(c)} {}

  Lock_scoped() = default;
  Lock_scoped(std::nullptr_t) {}

  Lock_scoped(const Lock_scoped &) = delete;
  Lock_scoped &operator=(const Lock_scoped &) = delete;

  Lock_scoped(Lock_scoped && o) noexcept { *this = std::move(o); }
  Lock_scoped &operator=(Lock_scoped &&o) noexcept {
    if (this != &o) std::swap(m_callback, o.m_callback);
    return *this;
  }

  ~Lock_scoped() noexcept {
    if (!m_callback) return;
    try {
      m_callback();
    } catch (const std::exception &e) {
      log_error("Unexpected exception: %s", e.what());
    }
  }

  explicit operator bool() const noexcept { return m_callback.operator bool(); }

  void invoke() {
    if (!m_callback) return;
    std::exchange(m_callback, nullptr)();
  }

 private:
  std::function<void()> m_callback;
};

class [[nodiscard]] Lock_scoped_list {
 public:
  Lock_scoped_list() = default;
  Lock_scoped_list(const Lock_scoped_list &) = delete;
  Lock_scoped_list &operator=(const Lock_scoped_list &) = delete;

  Lock_scoped_list(Lock_scoped_list && o) noexcept { *this = std::move(o); }
  Lock_scoped_list &operator=(Lock_scoped_list &&o) noexcept {
    if (this != &o) std::swap(m_callbacks, o.m_callbacks);
    return *this;
  }

  ~Lock_scoped_list() noexcept { invoke(); }

  void invoke() noexcept {
    // from end to begin (mimics a stack)
    std::for_each(m_callbacks.rbegin(), m_callbacks.rend(), [](const auto &cb) {
      try {
        cb();
      } catch (const std::exception &e) {
        log_error("Unexpected exception: %s", e.what());
      }
    });
    m_callbacks.clear();
  }

  void push_back(std::function<void()> cb) {
    if (!cb) return;
    m_callbacks.push_back(std::move(cb));
  }

  void push_back(Lock_scoped lock) {
    if (!lock) return;
    m_callbacks.push_back(std::exchange(lock.m_callback, nullptr));
  }

  template <class TCapture>
  void push_back(Lock_scoped lock, TCapture && capture) {
    if (!lock) return;
    m_callbacks.push_back(
        [capture = std::forward<TCapture>(capture),
         cb = std::exchange(lock.m_callback, nullptr)]() { cb(); });
  }

 private:
  std::vector<std::function<void()>> m_callbacks;
};

/**
 * Convert Lock_mode enumeration values to string.
 *
 * @param mode Lock_mode value to convert to string.
 * @return string representing the Lock_mode value.
 */
std::string to_string(const Lock_mode mode);

/**
 * Install the lock service on the target instance.
 *
 * @param instance Instance object with the target instance to install the
 * lock service.
 */
void install_lock_service(mysqlshdk::mysql::IInstance *instance);

/**
 * Check if the lock service are installed on the target instance.
 *
 * @param instance Instance object with the target instance to check.
 * @return boolean true if lock service is installed, false otherwise.
 */
bool has_lock_service(const mysqlshdk::mysql::IInstance &instance);

/**
 * Uninstall the lock service from the target instance.
 *
 * @param instance Instance object with the target instance to uninstall the
 *        lock service
 */
void uninstall_lock_service(mysqlshdk::mysql::IInstance *instance);

/**
 * Try to acquire the specified lock on the target instance.
 *
 * The lock to acquire is identified by the specified namespace and lock name
 * (combination of both names). Two types of locks can be acquired: SHARED or
 * EXCLUSIVE. Multiple SHARED locks with the same identifier (namespace and
 * lock name) can be acquired by different instances, but an EXCLUSIVE lock
 * will fail to be acquired (i.e., if a SHARED lock is already hold on the
 * identifier by another instance). Only one EXCLUSIVE lock can be hold on an
 * identifier by different instances, meaning that no other locks can be
 * acquired on that identifier by other instances. A timeout can be specified
 * to indicate how much time (in seconds) to wait to acquire a lock before
 * giving up with an error.
 *
 * @param instance Instance object with the target instance to get the lock.
 * @param name_space string with the namespace the lock belong too.
 * @param lock_name string with the name of the lock to acquire.
 * @param lock_mode Type of lock to be acquired: Lock_mode::SHARED or
 *                  Lock_mode::EXCLUSIVE.
 * @param timeout positive int with the maximum time in seconds to wait for
 * the lock to be acquired. By default 0, meaning that it will fail with an
 * error if the specified lock cannot be immediately acquired.
 * @throws shcore::Exception if the lock cannot be acquired (wait timeout
 *         exceeded), or if the name_space or lock name are invalid (empty or
 *         length greater than 64 characters).
 */
void get_lock(const mysqlshdk::mysql::IInstance &instance,
              const std::string &name_space, const std::string &lock_name,
              Lock_mode lock_mode, unsigned int timeout = 0);

/**
 * Release all lock in the specified namespace for the given target instance.
 *
 * All locks in the specified namespace are released independently of their
 * type (Lock_mode).
 *
 * @param instance Instance object with the target instance to release locks.
 * @param name_space string with the namespace to release the locks from.
 * @throws shcore::Exception if the name_space is invalid (empty or length
 *         greater than 64 characters).
 */
void release_lock(const mysqlshdk::mysql::IInstance &instance,
                  const std::string &name_space);

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_LOCK_SERVICE_H_
