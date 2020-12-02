/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/syslog.h"

#include <cassert>
#include <mutex>
#include <unordered_map>

#include "mysqlshdk/libs/utils/syslog_system.h"

namespace shcore {

namespace {

constexpr auto k_syslog_name = "mysqlsh";

/**
 * Global system log, singleton.
 */
class Global_syslog final {
 public:
  Global_syslog() = default;

  Global_syslog(const Global_syslog &) = delete;
  Global_syslog(Global_syslog &&) = delete;

  Global_syslog &operator=(const Global_syslog &) = delete;
  Global_syslog &operator=(Global_syslog &&) = delete;

  ~Global_syslog() = default;

  /**
   * Initialize access to the system log.
   */
  void initialize() {
    const std::lock_guard<std::mutex> lock(m_init_mutex);

    if (0 == m_init_counter) {
      syslog::open(k_syslog_name);
    }

    ++m_init_counter;
  }

  /**
   * Logs given message to the system log using the specified log level.
   *
   * @param level log level
   * @param msg message to be logged
   */
  void log(syslog::Level level, const std::string &msg) const {
    // not checking if we're initialized, Syslog::log() protects from that, this
    // way we don't need to use mutex here
    syslog::log(level, msg.c_str());

    for (const auto &hook : m_hooks) {
      hook.first(level, msg.c_str(), hook.second);
    }
  }

  /**
   * Finalize access to the system log.
   */
  void finalize() {
    const std::lock_guard<std::mutex> lock(m_init_mutex);

    if (1 == m_init_counter) {
      syslog::close();
    }

    --m_init_counter;

    assert(m_init_counter >= 0);
  }

  void add_hook(syslog::callback_t hook, void *data) {
    remove_hook(hook, data);
    m_hooks.emplace(hook, data);
  }

  void remove_hook(syslog::callback_t hook, void *data) {
    auto range = m_hooks.equal_range(hook);

    for (auto it = range.first; it != range.second; ++it) {
      if (data == it->second) {
        m_hooks.erase(it);
        break;
      }
    }
  }

 private:
  std::mutex m_init_mutex;

  int m_init_counter = 0;

  std::unordered_multimap<syslog::callback_t, void *> m_hooks;
};

Global_syslog g_syslog;

}  // namespace

namespace syslog {

void add_hook(callback_t hook, void *data) { g_syslog.add_hook(hook, data); }

void remove_hook(callback_t hook, void *data) {
  g_syslog.remove_hook(hook, data);
}

}  // namespace syslog

Syslog::~Syslog() { finalize(); }

void Syslog::enable(bool flag) {
  if (flag) {
    initialize();
  } else {
    finalize();
  }
}

void Syslog::pause(bool flag) {
  if (flag) {
    ++m_paused;
  } else {
    --m_paused;

    assert(m_paused >= 0);
  }
}

void Syslog::log(syslog::Level level, const std::string &msg) const {
  if (active()) {
    g_syslog.log(level, msg);
  }
}

void Syslog::initialize() {
  if (!m_enabled) {
    g_syslog.initialize();

    m_enabled = true;
  }
}

void Syslog::finalize() {
  if (m_enabled) {
    g_syslog.finalize();

    m_enabled = false;
  }
}

}  // namespace shcore
