/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

namespace shcore {

class Signal_interrupt_helper : public Interrupt_helper {
 public:
  Signal_interrupt_helper() = default;

  Signal_interrupt_helper(const Signal_interrupt_helper &) = delete;
  Signal_interrupt_helper(Signal_interrupt_helper &&) = delete;

  Signal_interrupt_helper &operator=(const Signal_interrupt_helper &) = delete;
  Signal_interrupt_helper &operator=(Signal_interrupt_helper &&) = delete;

  ~Signal_interrupt_helper() override = default;

  void setup(Interrupts *) override;
};

}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_INTERRUPT_HELPER_H_
