/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/common/dump/dump_version.h"

#include "mysqlshdk/include/shellcore/console.h"

#include "modules/util/load/load_errors.h"

namespace mysqlsh {
namespace dump {
namespace common {

namespace {

const mysqlshdk::utils::Version k_supported_dumper{k_dumper_version};

}  // namespace

void validate_dumper_version(const mysqlshdk::utils::Version &version) {
  if (version.get_major() > k_supported_dumper.get_major() ||
      (version.get_major() == k_supported_dumper.get_major() &&
       version.get_minor() > k_supported_dumper.get_minor())) {
    current_console()->print_error(
        "Dump format has version " + version.get_full() +
        " which is not supported by this version of MySQL Shell. Please "
        "upgrade MySQL Shell to use it.");
    THROW_ERROR(SHERR_LOAD_UNSUPPORTED_DUMP_VERSION);
  }

  if (version < k_supported_dumper) {
    current_console()->print_note(
        "Dump format has version " + version.get_full() +
        " and was created by an older version of MySQL Shell. If you "
        "experience problems using it, please recreate the dump using the "
        "current version of MySQL Shell and try again.");
  }
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
