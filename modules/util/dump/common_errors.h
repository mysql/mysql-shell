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

#ifndef MODULES_UTIL_DUMP_COMMON_ERRORS_H_
#define MODULES_UTIL_DUMP_COMMON_ERRORS_H_

#include "mysqlshdk/libs/rest/error_codes.h"

#include "modules/util/dump/errors.h"

#define SHERR_DL_COMMON_MIN 54000

#define SHERR_DL_COMMON_FIRST SHERR_DL_COMMON_MIN

#define SHERR_DL_COMMON_CONNECTION_ERROR 54000
#define SHERR_DL_COMMON_CONNECTION_ERROR_MSG "%sConnection error: %s."

#define SHERR_DL_COMMON_LAST 54000

#define SHERR_DL_COMMON_MAX 54999

namespace mysqlsh {
namespace dump {

class Progress_thread;

/**
 * Rethrows current exception, translating it to shcore::Exception, including
 * information about the current stage of execution.
 *
 * Does nothing if currently there is no exception.
 *
 * @param progress Used to obtain information about the current stage.
 */
void translate_current_exception(const Progress_thread &progress);

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_COMMON_ERRORS_H_
