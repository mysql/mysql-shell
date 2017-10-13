/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef UNITTEST_TEST_UTILS_SHELL_TEST_WRAPPER_H_
#define UNITTEST_TEST_UTILS_SHELL_TEST_WRAPPER_H_

// Environment variables only

#include <mutex>
#include <queue>
#include <string>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace tests {
/*
 * This class wrapps a Mysql_shell instance providing the required
 * hooks to feed input and retrieve output for validation
 *
 */
class Shell_test_wrapper {
 public:
  Shell_test_wrapper();
  void reset();
  Shell_test_output_handler& get_output_handler();
  void execute(const std::string& line);
  mysqlsh::Shell_options& get_options();
  void trace_protocol();

 private:
  Shell_test_output_handler output_handler;
  mysqlsh::Shell_options _options;
  std::shared_ptr<mysqlsh::Mysql_shell> _interactive_shell;
};

template <typename T>
class Thread_queue {
 public:
  T pop() {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
      _cond.wait(mlock);
    }
    auto item = _queue.front();
    _queue.pop();
    return item;
  }

  void push(const T& item) {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(item);
    mlock.unlock();
    _cond.notify_one();
  }

  bool empty() {
    std::unique_lock<std::mutex> mlock(_mutex);
    return _queue.empty();
  }

 private:
  std::queue<T> _queue;
  std::mutex _mutex;
  std::condition_variable _cond;
};

#if 0
/*
 * This class provides a shell wrapper that runs in a thread.
 * NOTE: This class is not being used as using the shell in a thread
 * together with another shell on the main thread has random crashes
 * in some platforms, because the shell was never thought on being used
 * in a threaded way, and there are parts that share resources, i.e. singletons
 * which causes i.e. te global session to be deleted twice, even it's on a
 * a shared pointer.
 */
/*class Asynch_shell_test_wrapper {
 public:
  explicit Asynch_shell_test_wrapper(bool trace = false);

  ~Asynch_shell_test_wrapper();

  mysqlsh::Shell_options& get_options();

  void execute(const std::string& line);

  bool execute_sync(const std::string& line, float timeup = 0);

  void dump_out();
  void dump_err();
  bool wait_for_state(const std::string& state, float timeup = 0);
  void trace_protocol();
  std::string get_state();
  Shell_test_output_handler& get_output_handler();

  void start();
  void shutdown();

 private:
  Shell_test_wrapper _shell;
  bool _trace;

  std::shared_ptr<std::thread> _thread;
  std::string _state;
  bool _started;

  Thread_queue<std::string> _input_lines;
};*/
#endif
}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_SHELL_TEST_WRAPPER_H_
