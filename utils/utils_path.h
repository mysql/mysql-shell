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

#ifndef UTILS_UTILS_PATH_H_
#define UTILS_UTILS_PATH_H_

#include <algorithm>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "shellcore/common.h"

namespace shcore {

std::string SHCORE_PUBLIC join_path(const std::vector<std::string>& components);
std::pair<std::string, std::string> SHCORE_PUBLIC splitdrive(
    const std::string &path);

#ifdef WIN32
const char path_separator = '\\';
#else
const char path_separator = '/';
#endif

}  // namespace shcore

#endif  // UTILS_UTILS_PATH_H_
