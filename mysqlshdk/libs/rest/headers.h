/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_HEADERS_H_
#define MYSQLSHDK_LIBS_REST_HEADERS_H_

#include <map>
#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace rest {

/**
 * Maps HTTP headers to associated values, allows for case-insensitive access.
 * If the value string is empty, a header with no content is going to be sent.
 */
using Headers =
    std::map<std::string, std::string, shcore::Case_insensitive_comparator>;

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_HEADERS_H_
