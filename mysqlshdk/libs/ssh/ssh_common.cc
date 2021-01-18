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

#include "mysqlshdk/libs/ssh/ssh_common.h"

#include <fcntl.h>
#include <libssh/callbacks.h>
#include <libssh/sftp.h>
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace {

int mutex_init(void **priv) {
  *priv = new std::mutex();
  return 0;
}

int mutex_destroy(void **lock) {
  delete static_cast<std::mutex *>(*lock);
  *lock = nullptr;
  return 0;
}

int mutex_lock(void **lock) {
  static_cast<std::mutex *>(*lock)->lock();
  return 0;
}

int mutex_unlock(void **lock) {
  static_cast<std::mutex *>(*lock)->unlock();
  return 0;
}

unsigned long get_thread_id(void) {
  std::hash<std::thread::id> hasher;
  return static_cast<unsigned long>(hasher(std::this_thread::get_id()));
}

struct ssh_threads_callbacks_struct std_threads = {
    "threads_stdthread", &mutex_init,   &mutex_destroy,
    &mutex_lock,         &mutex_unlock, &get_thread_id};

ssh_threads_callbacks_struct *ssh_threads_get_std_threads(void) {
  return &std_threads;
}

void libssh_log_callback(int priority, const char *function, const char *buffer,
                         void *UNUSED(userdata)) {
  // There's a chance that the logger will try to log
  // before actually the logger will be ready.
  // and what's worse we can do nothing with it, so the only solution is to just
  // skip the log since there's no error log here, there should be no problem
  // with it.
  auto logger = shcore::current_logger(true);
  if (!logger) return;

  switch (priority) {
    case SSH_LOG_TRACE:
      log_debug3("libssh: %s %s", function, buffer);
      break;
    case SSH_LOG_DEBUG:
      log_debug2("libssh: %s %s", function, buffer);
      break;
    case SSH_LOG_WARN:
      // Even these are only enabled when Shell is in one of the DEBUG log
      // levels, warnings are reported as such in the shell log
      log_warning("libssh: %s %s", function, buffer);
      break;
    case SSH_LOG_INFO:
      // Even these are only enabled when Shell is in DEBUG2 log
      // levels, SSH INFOs are reported as debug in the shell log
      log_debug("libssh: %s %s", function, buffer);
      break;
    case SSH_LOG_NONE:
    default:
      break;
  }
}

/**
 * Hook to have the SSH log level updated according to the Shell Log Level
 */
void update_libssh_log_level(shcore::Logger::LOG_LEVEL shell_log_level,
                             [[maybe_unused]] void *user_data = nullptr) {
  int level = SSH_LOG_NONE;
  switch (shell_log_level) {
    // SSH Logging is too chatty, for that reason all of these levels disable
    // logging on libSSH
    case shcore::Logger::LOG_LEVEL::LOG_NONE:
    case shcore::Logger::LOG_LEVEL::LOG_INTERNAL_ERROR:
    case shcore::Logger::LOG_LEVEL::LOG_ERROR:
    case shcore::Logger::LOG_LEVEL::LOG_WARNING:
    case shcore::Logger::LOG_LEVEL::LOG_INFO:
      level = SSH_LOG_NONE;
      break;
    case shcore::Logger::LOG_LEVEL::LOG_DEBUG:
      level = SSH_LOG_WARN;
      break;
    case shcore::Logger::LOG_LEVEL::LOG_DEBUG2:
      //  SSH_LOG_INFO: NOT USED, Since INFO is too chatty as well we enable it
      // together at shell level Debug2
      level = SSH_LOG_DEBUG;
      break;
    case shcore::Logger::LOG_LEVEL::LOG_DEBUG3:
      level = SSH_LOG_TRACE;
      break;
  }
  ssh_set_log_level(level);
}

}  // namespace

namespace mysqlshdk {
namespace ssh {

void ssh_close_socket(int socket) {
#if _MSC_VER
  closesocket(socket);
#else
  close(socket);
#endif
}

std::string get_error() { return std::string(strerror(errno)); }

void set_socket_non_blocking(int sock) {
#ifdef _MSC_VER
  u_long mode = 1;
  int result = ioctlsocket(sock, FIONBIO, &mode);
  if (result != NO_ERROR) {
    ssh_close_socket(sock);
    throw Ssh_tunnel_exception("unable to set socket nonblock: " + get_error());
  }
#else
  if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
    close(sock);
    throw Ssh_tunnel_exception("unable to set socket nonblock: " + get_error());
  }
#endif
}

static void setup_libssh() {
  ssh_threads_set_callbacks(ssh_threads_get_std_threads());
  update_libssh_log_level(shcore::current_logger()->get_log_level());
  shcore::current_logger()->attach_log_level_hook(update_libssh_log_level);
  ssh_set_log_callback(libssh_log_callback);
  ssh_init();
}

void init_libssh() {
  static std::once_flag ssh_init_once;

  std::call_once(ssh_init_once, [] { setup_libssh(); });
}

void Ssh_thread::internal_run() {
  m_initialization_sem.post();
  m_finished = false;
  run();
  m_finished = true;
}

Ssh_thread::Ssh_thread() {}

Ssh_thread::~Ssh_thread() {
  try {
    stop();
  } catch (...) {
    log_error("SSH: thread: Unable to stop SSHThread.");
  }
}

void Ssh_thread::stop() {
  m_stop = true;
  if (m_thread.joinable()) m_thread.join();
}

bool Ssh_thread::is_running() const { return !m_finished; }

void Ssh_thread::start() {
  if (m_finished) {
    m_stop = false;
    m_thread = mysqlsh::spawn_scoped_thread([this] { internal_run(); });
    m_initialization_sem.wait();
  }
}

void Ssh_thread::join() { m_thread.join(); }

Semaphore::Semaphore() : Semaphore(0) {}

Semaphore::Semaphore(int initial_count) : m_count(initial_count) {}

void Semaphore::post() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_count++;
  m_condition.notify_one();
}

void Semaphore::wait() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait(lock, [this]() { return m_count > 0; });
  m_count--;
}

}  // namespace ssh
}  // namespace mysqlshdk
