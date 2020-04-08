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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HELPER_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HELPER_H_

#include "mysqlshdk/include/shellcore/interrupt_handler.h"

#ifdef _WIN32
class Interrupt_windows_helper {
  std::thread m_scoped_thread;

 public:
  Interrupt_windows_helper();
  Interrupt_windows_helper(const Interrupt_windows_helper &) = delete;
  Interrupt_windows_helper(Interrupt_windows_helper &&) = delete;
  Interrupt_windows_helper &operator=(const Interrupt_windows_helper &) =
      delete;
  Interrupt_windows_helper &operator=(Interrupt_windows_helper &&) = delete;
  ~Interrupt_windows_helper();
};
#endif

class Interrupt_helper : public shcore::Interrupt_helper {
 public:
  void setup() override;

  void block() override;

  void unblock(bool) override;
};

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HELPER_H_
