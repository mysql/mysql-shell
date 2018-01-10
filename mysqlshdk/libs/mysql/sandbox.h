/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_MYSQL_SANDBOX_H_
#define MYSQLSHDK_LIBS_MYSQL_SANDBOX_H_

#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/mysql/mycnf.h"

namespace mysqlshdk {
namespace mysql {
namespace sandbox {

void reconfigure(const std::string &sandbox_dir, int port,
                 const std::vector<mycnf::Option> &mycnf_options);

}  // namespace sandbox
}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_SANDBOX_H_
