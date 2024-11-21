/*
 * Copyright (c) 2018, 2024 Oracle and/or its affiliates.
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

#include "shellcore/shell_init.h"

#include <curl/curl.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {

namespace {

void init_openssl_modules() {
#ifdef BUNDLE_OPENSSL_MODULES
  constexpr auto k_env_var = "OPENSSL_MODULES";

  // don't override path set by the user
  if (!::getenv(k_env_var)) {
    shcore::setenv(
        k_env_var,
        shcore::path::join_path(shcore::get_library_folder(), "ossl-modules"));
  }
#endif
}

}  // namespace

void thread_init() { mysql_thread_init(); }

void thread_end() { mysql_thread_end(); }

void global_init() {
  mysql_library_init(0, nullptr, nullptr);

  thread_init();

  srand(time(0));
  curl_global_init(CURL_GLOBAL_ALL);
  init_openssl_modules();
}

void global_end() {
  thread_end();
  mysql_library_end();
}

Mysql_thread::Mysql_thread() {
  if (mysql_thread_init()) {
    throw std::runtime_error(
        "Cannot allocate specific memory for the MySQL thread.");
  }
}

Mysql_thread::~Mysql_thread() { mysql_thread_end(); }

}  // namespace mysqlsh
