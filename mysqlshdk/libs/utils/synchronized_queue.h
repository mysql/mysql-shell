/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>

namespace shcore {
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

  void push(const T &r) {
    {
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      m_queue.push_back(r);
    }
    m_task_ready.notify_one();
  }

  void push(T &&r) {
    {
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      m_queue.emplace_back(std::move(r));
    }
    m_task_ready.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_task_ready.wait(lock, [this]() { return !m_queue.empty(); });
    auto r = m_queue.front();
    m_queue.pop_front();
    return r;
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
      for (int64_t i = 0; i < n; i++) {
        m_queue.emplace_back(T());
      }
    }
    m_task_ready.notify_all();
  }

 private:
  std::mutex m_queue_mutex;
  std::condition_variable m_task_ready;
  std::deque<T> m_queue;
};
}  // namespace shcore

#endif /* MYSQLSHDK_LIBS_UTILS_SYNCHRONIZED_QUEUE_H_ */
