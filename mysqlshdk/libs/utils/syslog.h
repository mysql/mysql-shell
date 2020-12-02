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

#ifndef MYSQLSHDK_LIBS_UTILS_SYSLOG_H_
#define MYSQLSHDK_LIBS_UTILS_SYSLOG_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/utils/syslog_level.h"

namespace shcore {

/**
 * Provides access to the system log. Should not be shared between threads.
 */
class Syslog final {
 public:
  Syslog() = default;

  Syslog(const Syslog &) = delete;
  Syslog(Syslog &&) = default;

  Syslog &operator=(const Syslog &) = delete;
  Syslog &operator=(Syslog &&) = default;

  ~Syslog();

  /**
   * Checks whether this instance is active, that is: enabled and not paused.
   */
  inline bool active() const noexcept { return m_enabled && (0 == m_paused); }

  /**
   * Enables/disables the system logging functionality.
   *
   * @param flag true to enable the system log
   */
  void enable(bool flag);

  /**
   * Pauses/resumes logging to the system log.
   *
   * Can be called multiple times, an equal number of pause and resume calls is
   * required in order to resume logging.
   *
   * @param flag true to temporarily pause the logging
   */
  void pause(bool flag);

  /**
   * Logs given message to the system log using the specified log level.
   *
   * @param level log level
   * @param msg message to be logged
   */
  void log(syslog::Level level, const std::string &msg) const;

 private:
  void initialize();

  void finalize();

  bool m_enabled = false;

  int m_paused = 0;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_SYSLOG_H_
