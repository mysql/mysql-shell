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

#include "mysqlshdk/libs/utils/thread_pool.h"

#include "mysqlshdk/include/shellcore/scoped_contexts.h"

namespace shcore {

Thread_pool::Thread_pool(uint64_t threads)
    : m_threads(threads),
      m_active_threads(threads),
      m_workers(threads),
      m_worker_exceptions(threads) {}

Thread_pool::~Thread_pool() {
  kill_threads();
  wait_for_async_thread();
}

void Thread_pool::start_threads() {
  for (auto i = decltype(m_threads){0}; i < m_threads; ++i) {
    m_workers[i] = mysqlsh::spawn_scoped_thread(
        [this](auto id) {
          try {
            while (true) {
              auto task = m_worker_tasks.pop();

              if (m_worker_interrupt) {
                return;
              }

              if (!task.produce_data) {
                break;
              }

              auto data = task.produce_data();

              m_main_thread_tasks.push(
                  [data = std::move(data),
                   process_data = std::move(task.process_data)]() mutable {
                    process_data(std::move(data));
                  });

              if (m_worker_interrupt) {
                return;
              }
            }

            maybe_shutdown_main_thread();
          } catch (...) {
            m_worker_exceptions[id] = std::current_exception();
            emergency_shutdown();
          }
        },
        i);
  }
}

void Thread_pool::add_task(Producer &&fetch_data, Processor &&process_data,
                           Priority priority) {
  if (!m_all_tasks_pushed) {
    m_worker_tasks.push({std::move(fetch_data), std::move(process_data)},
                        priority);
  } else {
    throw std::logic_error(
        "Cannot add a task after the worker queue has been shut down");
  }
}

void Thread_pool::tasks_done() {
  if (!m_all_tasks_pushed) {
    m_all_tasks_pushed = true;
    m_worker_tasks.shutdown(m_threads);
  } else {
    throw std::logic_error("Worker queue is already shut down");
  }
}

void Thread_pool::terminate() { emergency_shutdown(); }

void Thread_pool::process() {
  try {
    while (true) {
      auto task = m_main_thread_tasks.pop();

      if (m_worker_interrupt || !task) {
        break;
      }

      task();

      if (m_worker_interrupt) {
        break;
      }
    }

    if (!m_worker_interrupt) {
      m_async_state = Async_state::DONE;
    }
  } catch (...) {
    kill_threads();
    throw;
  }

  wait_for_worker_threads();
  rethrow();
}

volatile const Thread_pool::Async_state &Thread_pool::process_async() {
  m_async_state = Async_state::PRODUCING;

  m_async_thread =
      std::make_unique<std::thread>(mysqlsh::spawn_scoped_thread([this]() {
        try {
          process();
        } catch (...) {
          m_async_exception = std::current_exception();
        }
      }));

  return m_async_state;
}

void Thread_pool::wait_for_process() {
  wait_for_async_thread();

  if (m_async_exception) {
    std::rethrow_exception(m_async_exception);
  }
}

void Thread_pool::emergency_shutdown() {
  if (!m_worker_interrupt) {
    m_worker_interrupt = true;
    m_async_state = Async_state::TERMINATED;
    m_worker_tasks.shutdown(m_threads);
    shutdown_main_thread();
  }
}

void Thread_pool::wait_for_worker_threads() {
  for (auto &worker : m_workers) {
    worker.join();
  }

  m_workers.clear();
}

void Thread_pool::wait_for_async_thread() {
  if (m_async_thread) {
    m_async_thread->join();
    m_async_thread.reset();
  }
}

void Thread_pool::rethrow() {
  for (const auto &exc : m_worker_exceptions) {
    if (exc) {
      std::rethrow_exception(exc);
    }
  }
}

void Thread_pool::kill_threads() {
  emergency_shutdown();
  wait_for_worker_threads();
}

void Thread_pool::maybe_shutdown_main_thread() {
  if (--m_active_threads == 0) {
    m_async_state = Async_state::PROCESSING;
    shutdown_main_thread();
  }
}

void Thread_pool::shutdown_main_thread() { m_main_thread_tasks.shutdown(1); }

}  // namespace shcore
