/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_SYNCHRONIZED_QUEUE_H_
#define MYSQLSHDK_LIBS_UTILS_SYNCHRONIZED_QUEUE_H_

#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace shcore {

enum class Queue_priority { LOW = 1, MEDIUM, HIGH };

/**
 * Multiple producer, multiple consumer synchronized FIFO queue.
 */
template <class T>
class Synchronized_queue final {
 public:
  Synchronized_queue() = default;
  Synchronized_queue(const Synchronized_queue &other) = delete;
  Synchronized_queue(Synchronized_queue &&other) = delete;

  Synchronized_queue &operator=(const Synchronized_queue &other) = delete;
  Synchronized_queue &operator=(Synchronized_queue &&other) = delete;

  ~Synchronized_queue() = default;

  template <class U = T>
  void push(U &&r, Queue_priority p = Queue_priority::MEDIUM) {
    {
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      unsynchronized_push(std::forward<U>(r), map_priority(p));
    }
    m_task_ready.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_task_ready.wait(lock, [this]() { return !unsynchronized_empty(); });
    return unsynchronized_pop();
  }

  std::optional<T> try_pop(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    if (m_task_ready.wait_for(lock, timeout,
                              [this]() { return !unsynchronized_empty(); })) {
      return unsynchronized_pop();
    }
    return {};
  }

  /**
   * Method that push to the queue n guard objects that signals to consumer
   * threads to complete operation.
   *
   * @param n number of consumer threads.
   */
  void shutdown(int64_t n) {
    {
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      for (int64_t i = 0; i < n; i++)
        unsynchronized_push(T(), k_shutdown_priority);
    }
    m_task_ready.notify_all();
  }

  size_t size() const { return m_size; }

 private:
  using Priority_t = std::underlying_type_t<Queue_priority>;

  static constexpr Priority_t map_priority(Queue_priority p) {
    return static_cast<Priority_t>(p);
  }

  template <class U>
  inline void unsynchronized_push(U &&u, Priority_t p) {
    m_queues[k_max_priority - p].emplace_back(std::forward<U>(u));
    ++m_size;
  }

  inline T unsynchronized_pop() {
    for (auto &queue : m_queues) {
      if (!queue.empty()) {
        auto r = std::move(queue.front());
        queue.pop_front();
        --m_size;
        return r;
      }
    }

    throw std::logic_error("Trying to pop from an empty queue.");
  }

  inline bool unsynchronized_empty() const { return 0 == m_size; }

  static constexpr Priority_t k_shutdown_priority = 0;
  static constexpr Priority_t k_max_priority =
      map_priority(Queue_priority::HIGH);

  mutable std::mutex m_queue_mutex;
  std::condition_variable m_task_ready;
  std::array<std::deque<T>, 4> m_queues;
  std::atomic<std::size_t> m_size{0};
};
}  // namespace shcore

#endif /* MYSQLSHDK_LIBS_UTILS_SYNCHRONIZED_QUEUE_H_ */
