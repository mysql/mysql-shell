/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SCOPED_CONTEXTS_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SCOPED_CONTEXTS_H_

#include <memory>
#include <utility>
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlsh {

template <typename T>
class Global_scoped_object {
 public:
  explicit Global_scoped_object(
      const std::shared_ptr<T> &scoped_value,
      const std::function<void(const std::shared_ptr<T> &)> &deleter = {});
  ~Global_scoped_object();

  Global_scoped_object(const Global_scoped_object &) = delete;
  Global_scoped_object(Global_scoped_object &&) = delete;
  Global_scoped_object &operator=(const Global_scoped_object &) = delete;
  Global_scoped_object &operator=(Global_scoped_object &&) = delete;

  std::shared_ptr<T> get() const;

 private:
  std::function<void(const std::shared_ptr<T> &)> m_deleter;
  std::shared_ptr<T> m_scoped_value;
};

using Scoped_console = Global_scoped_object<mysqlsh::IConsole>;
using Scoped_shell_options = Global_scoped_object<mysqlsh::Shell_options>;
using Scoped_interrupt = Global_scoped_object<shcore::Interrupts>;
using Scoped_logger = Global_scoped_object<shcore::Logger>;

namespace detail {

template <class T>
std::decay_t<T> decay_copy(T &&v) {
  return std::forward<T>(v);
}

template <class Function, class... Args>
std::thread spawn_scoped_thread(Function &&f, Args &&... args) {
  auto thd_current_logger = shcore::current_logger(true);
  auto thd_current_shell_opts = mysqlsh::current_shell_options(true);
  auto thd_current_interrupt = shcore::current_interrupt(true);
  auto thd_current_console = mysqlsh::current_console(true);
  return std::thread(
      [f = decay_copy(std::forward<Function>(f)), thd_current_logger,
       thd_current_shell_opts, thd_current_interrupt,
       thd_current_console](const std::decay_t<Args> &... a) {
        mysqlsh::Scoped_logger logger(thd_current_logger);
        mysqlsh::Scoped_shell_options shell_opts(thd_current_shell_opts);
        mysqlsh::Scoped_interrupt interrupt(thd_current_interrupt);
        mysqlsh::Scoped_console console(thd_current_console);
        f(a...);
      },
      std::forward<Args>(args)...);
}

}  // namespace detail

template <class Function, class... Args>
std::thread spawn_scoped_thread(Function &&f, Args &&... args) {
  return detail::spawn_scoped_thread(std::forward<Function>(f),
                                     std::forward<Args>(args)...);
}

template <
    class Function, class C, class... Args,
    std::enable_if_t<std::is_member_pointer<std::decay_t<Function>>{}, int> = 0>
std::thread spawn_scoped_thread(Function &&f, C &&c, Args &&... args) {
  return detail::spawn_scoped_thread(
      [f = detail::decay_copy(std::forward<Function>(f)),
       c = std::forward<C>(c)](const std::decay_t<Args> &... a) {
        (const_cast<C *>(&c)->*f)(a...);
      },
      std::forward<Args>(args)...);
}

}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SCOPED_CONTEXTS_H_
