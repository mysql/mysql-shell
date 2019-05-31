/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_INIT_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_INIT_H_

namespace mysqlsh {

/*
 * Call once before using shell library to initialize global state, including
 * libmysqlclient.
 */
void global_init();

void thread_init();

void thread_end();

/*
 * Call once when done using shell library, from the same thread that was used
 * to call global_init().
 */
void global_end();

/**
 * This class is used for proper libmysqlclient data structures initialization
 * and deinitialization (using RAII) when connecting to MySQL Server from
 * threads.
 */
class Mysql_thread final {
 public:
  Mysql_thread();
  Mysql_thread(const Mysql_thread &other) = delete;
  Mysql_thread(Mysql_thread &&other) = delete;

  Mysql_thread &operator=(const Mysql_thread &other) = delete;
  Mysql_thread &operator=(Mysql_thread &&other) = delete;

  ~Mysql_thread();
};
}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_SHELL_INIT_H_
