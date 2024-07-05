/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "mysql-secret-store/core/program.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "shellcore/interrupt_helper.h"

namespace mysql {
namespace secret_store {
namespace client {

std::unique_ptr<common::Helper> get_helper();

}  // namespace client
}  // namespace secret_store
}  // namespace mysql

int main(int argc, char *argv[]) {
  using mysql::secret_store::client::get_helper;
  using mysql::secret_store::core::Program;
  shcore::Signal_interrupt_helper helper;
  mysqlsh::Scoped_interrupt interrupt_handler(
      shcore::Interrupts::create(&helper));

  return Program(get_helper()).run(argc, argv);
}
