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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_COMMON_H_
#define MYSQLSHDK_LIBS_SSH_SSH_COMMON_H_

#include <errno.h>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

#ifndef _MSC_VER
#include <poll.h>
#include <unistd.h>
#endif

#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/error.h"

#ifdef _MSC_VER
typedef int socklen_t;
#endif

namespace mysqlshdk {
namespace ssh {

struct Ssh_session_info {
  const Ssh_connection_options &connection;
  const std::chrono::system_clock::time_point &time_created;
};

void ssh_close_socket(int socket);

std::string get_error();
void set_socket_non_blocking(int sock);
void init_libssh();

enum class Ssh_return_type {
  CONNECTION_FAILURE,
  CONNECTED,
  INVALID_AUTH_DATA,
  FINGERPRINT_MISMATCH,
  FINGERPRINT_CHANGED,
  FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING,
  FINGERPRINT_UNKNOWN
};

enum class Ssh_auth_return {
  AUTH_DENIED,
  AUTH_PARTIAL,
  AUTH_SUCCESS,
  AUTH_INFO,
  AUTH_NONE
};

class Ssh_tunnel_exception : public std::runtime_error {
 public:
  explicit Ssh_tunnel_exception(const std::string &message)
      : runtime_error(message) {}
  explicit Ssh_tunnel_exception(const char *message) : runtime_error(message) {}
  ~Ssh_tunnel_exception() noexcept override {}
};

class Ssh_auth_exception : public std::runtime_error {
 public:
  explicit Ssh_auth_exception(const std::string &message)
      : runtime_error(message) {}
  explicit Ssh_auth_exception(const char *message) : runtime_error(message) {}
  ~Ssh_auth_exception() noexcept override {}
};

// A semaphore limits access to a bunch of resources to different threads. A
// count value determines how many resources are available (and hence how many
// threads can use them at the same time).
struct Semaphore final {
 public:
  Semaphore();
  explicit Semaphore(int initial_count);
  Semaphore(const Semaphore &other) = delete;
  Semaphore(Semaphore &&other) = delete;
  Semaphore &operator=(const Semaphore &other) = delete;
  Semaphore &operator=(Semaphore &&other) = delete;

  void post();
  void wait();

 private:
  std::mutex m_mutex;
  std::condition_variable m_condition;
  int m_count;
};

/**
 * This is a multi-purpose base threading class. The usage is simple, a new
 * descendant class should implement a run() function with the code that
 * should work in a thread. Inside of the run() access to the whole class
 * functions and members can be done using this yet those functions and members
 * are not protected and it's up to the user to implement protection if needed.
 *
 */
class Ssh_thread {
 public:
  Ssh_thread();
  virtual ~Ssh_thread();

  Ssh_thread(const Ssh_thread &other) = delete;
  Ssh_thread(Ssh_thread &&other) = delete;
  Ssh_thread &operator=(const Ssh_thread &other) = delete;
  Ssh_thread &operator=(Ssh_thread &&other) = delete;

  void stop();
  bool is_running() const;
  void start();
  void join();
  void set_stop() { m_stop = true; }

 protected:
  virtual void run() = 0;

  std::atomic<bool> m_stop = {false};
  std::atomic<bool> m_finished = {true};

 private:
  void internal_run();

  std::thread m_thread;
  Semaphore m_initialization_sem;
};

}  // namespace ssh
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_SSH_SSH_COMMON_H_
