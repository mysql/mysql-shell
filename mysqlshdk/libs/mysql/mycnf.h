/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_MYSQL_MYCNF_H_
#define MYSQLSHDK_LIBS_MYSQL_MYCNF_H_

#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace mysql {
namespace mycnf {

using Option = std::pair<std::string, utils::nullable<std::string>>;

void update_options(const std::string &path, const std::string &section,
                     const std::vector<Option> &mycnf_options);

}  // namespace mycnf
}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_MYCNF_H_
