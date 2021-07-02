/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_THREAD_POOL_H_
#define MYSQLSHDK_LIBS_UTILS_THREAD_POOL_H_

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "mysqlshdk/libs/utils/synchronized_queue.h"

namespace shcore {

/**
 * A pool of threads which allows to execute an operation in a pool, and
 * process the result of that operation in the calling thread.
 */
class Thread_pool final {
 public:
  using Priority = Queue_priority;
  using Producer = std::function<std::string()>;
  using Processor = std::function<void(std::string &&)>;

  enum class Async_state {
    IDLE,
    PRODUCING,
    PROCESSING,
    DONE,
    TERMINATED,
  };

  Thread_pool() = delete;

  /**
   * Sets the number of threads the pool is going to use.
   *
   * @param threads Number of threads to use.
   */
  explicit Thread_pool(uint64_t threads);

  Thread_pool(const Thread_pool &) = delete;
  Thread_pool(Thread_pool &&) = delete;

  Thread_pool &operator=(const Thread_pool &) = delete;
  Thread_pool &operator=(Thread_pool &&) = delete;

  ~Thread_pool();

  /**
   * Initializes and starts the threads.
   */
  void start_threads();

  /**
   * Adds a task to be executed.
   *
   * @param produce_data Operation which is going to be executed in the thread
   *        pool.
   * @param process_data Operation which is going to be executed in the calling
   *        thread using the result of produce_data.
   *
   * @throws logic_error If called after a call to tasks_done().
   */
  void add_task(Producer &&produce_data, Processor &&process_data,
                Priority priority = Priority::MEDIUM);

  /**
   * Notifies the thread pool that all tasks have been added.
   *
   * @throws logic_error If called twice.
   */
  void tasks_done();

  /**
   * Terminates execution of all threads, if a call to process() has been made,
   * it will exit as well.
   *
   * Terminate is not instantaneous, all the calls which are being executed need
   * to finish first.
   */
  void terminate();

  /**
   * Waits for all the tasks to finish.
   *
   * Internally, it executes all the process_data calls using the current
   * thread.
   *
   * @throws any Exception which was reported by the produce_data or
   *         process_data calls.
   */
  void process();

  /**
   * Calls process() in a separate thread, the current state of the processing
   * can be checked at any time using the returned value.
   *
   * @returns value which can be used to check the current state of asynchronous
   *          processing
   */
  volatile const Async_state &process_async();

  /**
   * Waits for the asynchronous process() call to finish.
   *
   * @throws any Exception which was reported by process() call.
   */
  void wait_for_process();

 private:
  struct Task {
    Producer produce_data;
    Processor process_data;
  };

  void emergency_shutdown();

  void wait_for_worker_threads();

  void wait_for_async_thread();

  void rethrow();

  void kill_threads();

  void maybe_shutdown_main_thread();

  void shutdown_main_thread();

  uint64_t m_threads;

  std::atomic<uint64_t> m_active_threads;

  std::vector<std::thread> m_workers;

  std::vector<std::exception_ptr> m_worker_exceptions;

  volatile bool m_worker_interrupt = false;

  volatile bool m_all_tasks_pushed = false;

  Synchronized_queue<Task> m_worker_tasks;

  Synchronized_queue<std::function<void()>> m_main_thread_tasks;

  volatile Async_state m_async_state = Async_state::IDLE;

  std::unique_ptr<std::thread> m_async_thread;

  std::exception_ptr m_async_exception = nullptr;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_THREAD_POOL_H_
