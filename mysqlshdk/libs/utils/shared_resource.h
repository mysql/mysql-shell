/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_SHARED_RESOURCE_H_
#define MYSQLSHDK_LIBS_UTILS_SHARED_RESOURCE_H_

#include <memory>
#include <mutex>
#include <utility>

namespace shcore {

/**
 * A template class that manages a shared resource using a shared_ptr.
 *
 * Provides exclusive access to the resource via the acquire() method, which
 * returns an RAII object. As long as the RAII object exists, further calls to
 * acquire() will block until it is destroyed.
 */
template <typename T>
class Shared_resource final
    : public std::enable_shared_from_this<Shared_resource<T>> {
 public:
  /**
   * RAII object that provides exclusive access to the managed resource.
   *
   * While this object exists, the mutex is locked, blocking other acquire()
   * calls. It also keeps the Shared_resource alive.
   */
  class Exclusive_resource final {
   public:
    Exclusive_resource() = default;

    explicit Exclusive_resource(std::shared_ptr<Shared_resource<T>> owner)
        : m_owner(std::move(owner)), m_lock(m_owner->m_mutex) {}

    Exclusive_resource(const Exclusive_resource &) = delete;
    Exclusive_resource(Exclusive_resource &&) = delete;

    Exclusive_resource &operator=(const Exclusive_resource &) = delete;
    Exclusive_resource &operator=(Exclusive_resource &&) = delete;

    T *operator->() const { return m_owner->m_resource.get(); }
    T &operator*() const { return *m_owner->m_resource; }

    T *get() const { return m_owner ? m_owner->m_resource.get() : nullptr; }

    explicit operator bool() const noexcept { return !!m_owner; }

    operator std::shared_ptr<T>() const {
      return m_owner ? m_owner->m_resource : nullptr;
    }

   private:
    std::shared_ptr<Shared_resource<T>> m_owner;
    std::unique_lock<std::mutex> m_lock;
  };

  explicit Shared_resource(std::shared_ptr<T> resource)
      : m_resource(std::move(resource)) {}

  Shared_resource(const Shared_resource &) = delete;
  Shared_resource(Shared_resource &&) = delete;

  Shared_resource &operator=(const Shared_resource &) = delete;
  Shared_resource &operator=(Shared_resource &&) = delete;

  /**
   * Acquires exclusive access to the resource.
   *
   * Blocks if another Exclusive_resource is currently active.
   * Returns an Exclusive_resource that unlocks when destroyed.
   */
  Exclusive_resource acquire() {
    return Exclusive_resource(this->shared_from_this());
  }

 private:
  std::shared_ptr<T> m_resource;
  std::mutex m_mutex;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_SHARED_RESOURCE_H_
