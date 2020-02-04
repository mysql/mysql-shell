/*
 * Copyright (c) 2018, 2020 Oracle and/or its affiliates. All rights reserved.
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
using Scoped_interrupt_handler = Global_scoped_object<shcore::Interrupt_helper>;
using Scoped_logger = Global_scoped_object<shcore::Logger>;
}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SCOPED_CONTEXTS_H_
