/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_ASYNC_REPLICATION_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_ASYNC_REPLICATION_OPTIONS_H_

#include <string>
#include <vector>
#include "modules/mod_common.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/version.h"
#include "scripting/types.h"

namespace mysqlsh {
namespace dba {

struct Async_replication_options {
  enum Unpack_target {
    NONE,   // none
    CREATE  // all options OK
  };

  Async_replication_options() : target(NONE) {}

  explicit Async_replication_options(Unpack_target t) : target(t) {}

  template <typename Unpacker>
  Unpacker &unpack(Unpacker *options) {
    do_unpack(options);
    return *options;
  }

  Unpack_target target;

  mysqlshdk::utils::nullable<mysqlshdk::mysql::Auth_options> repl_credentials;

  mysqlshdk::utils::nullable<int> master_connect_retry;
  mysqlshdk::utils::nullable<int> master_retry_count;
  mysqlshdk::utils::nullable<int> master_delay;

 private:
  template <typename Unpacker>
  void do_unpack(Unpacker * /*unpacker*/) {
    switch (target) {
      case NONE:
        break;

      case CREATE:
        // unpacker->optional_obj_group("replChannel", &repl_credentials);
        break;
    }
  }
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ASYNC_REPLICATION_OPTIONS_H_
