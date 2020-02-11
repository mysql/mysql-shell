/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_UTILS_H_
#define MYSQLSHDK_LIBS_STORAGE_UTILS_H_

#include <string>

namespace mysqlshdk {
namespace storage {
namespace utils {

/**
 * Provides the scheme part of the given URI.
 *
 * @param uri URI
 *
 * @returns Scheme of the given URI or empty string if there is no scheme.
 *
 * @throws std::invalid_argument If URI contains an empty scheme.
 */
std::string get_scheme(const std::string &uri);

/**
 * Strips the scheme part from the given URI.
 *
 * If the URI does not have a scheme it is returned unchanged.
 * If the scheme parameter is not empty, it must match the scheme in the given
 * URI.
 *
 * @param uri URI
 * @param scheme expected scheme
 *
 * @returns URI without the scheme.
 *
 * @throws std::invalid_argument If URI contains an empty scheme.
 * @throws std::invalid_argument If the scheme parameter is not empty and does
 *         not match the scheme in the given URI.
 */
std::string strip_scheme(const std::string &uri,
                         const std::string &scheme = "");

/**
 * Checks if scheme matches the expected value.
 *
 * @param scheme A scheme to be tested.
 * @param expected An expected value.
 *
 * @returns true If scheme matches the expected value.
 */
bool scheme_matches(const std::string &scheme, const char *expected);

}  // namespace utils
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_UTILS_H_
