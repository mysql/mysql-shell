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

#ifndef MODULES_UTIL_DUMP_ERRORS_H_
#define MODULES_UTIL_DUMP_ERRORS_H_

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#if __cplusplus > 201703L
// C++20 has __VA_OPT__, THROW_ERROR0() is not needed, should be replaced with
// THROW_ERROR()
#define THROW_ERROR(code, ...) \
  throw shcore::Exception(     \
      shcore::str_format(code##_MSG __VA_OPT__(, ) __VA_ARGS__), code)
#else
// this uses shcore::str_format() even though it's not needed in order to detect
// errors when message with formatting is used mistakenly with THROW_ERROR0()
// macro
#define THROW_ERROR0(code) \
  throw shcore::Exception(shcore::str_format(code##_MSG), code)
#define THROW_ERROR(code, ...) \
  throw shcore::Exception(shcore::str_format(code##_MSG, __VA_ARGS__), code)
#endif

#endif  // MODULES_UTIL_DUMP_ERRORS_H_
