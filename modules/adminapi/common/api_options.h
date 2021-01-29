/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_API_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_API_OPTIONS_H_

#include <string>

#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

struct Timeout_option {
  static const shcore::Option_pack_def<Timeout_option> &options();

  void set_timeout(int value);

  int timeout = 0;
};

struct Interactive_option {
  static const shcore::Option_pack_def<Interactive_option> &options();
  bool interactive() const;

 private:
  mysqlshdk::null_bool m_interactive;
};

struct Password_interactive_options : public Interactive_option {
  static const shcore::Option_pack_def<Password_interactive_options> &options();
  void set_password(const std::string &option, const std::string &value);

  mysqlshdk::null_string password;
};

struct Force_interactive_options : public Interactive_option {
  static const shcore::Option_pack_def<Force_interactive_options> &options();

  mysqlshdk::null_bool force;
};

struct List_routers_options {
  static const shcore::Option_pack_def<List_routers_options> &options();

  bool only_upgrade_required = false;
};

struct Setup_account_options : public Password_interactive_options {
  static const shcore::Option_pack_def<Setup_account_options> &options();

  bool dry_run = false;
  bool update = false;
};

}  // namespace dba
}  // namespace mysqlsh
#endif  // MODULES_ADMINAPI_COMMON_API_OPTIONS_H_
