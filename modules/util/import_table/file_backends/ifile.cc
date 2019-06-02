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

#include "modules/util/import_table/file_backends/ifile.h"

#include "modules/util/import_table/file_backends/file.h"
#include "modules/util/import_table/file_backends/http.h"
#include "modules/util/import_table/file_backends/oci_object_storage.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace import_table {

std::unique_ptr<IFile> make_file_handler(const std::string &filepath) {
  if (shcore::str_beginswith(filepath, "oci+os://")) {
    return shcore::make_unique<Oci_object_storage>(filepath);
  } else if (shcore::str_beginswith(filepath, "http://") ||
             shcore::str_beginswith(filepath, "https://")) {
    return shcore::make_unique<Http_get>(filepath);
  }
  // implicit file://
  return shcore::make_unique<File>(filepath);
}

}  // namespace import_table
}  // namespace mysqlsh
