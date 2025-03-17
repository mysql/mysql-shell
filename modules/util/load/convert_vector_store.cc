/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#include "modules/util/load/convert_vector_store.h"

#include <stdexcept>

namespace mysqlsh {
namespace load {

std::string to_string(Convert_vector_store mode) {
  switch (mode) {
    case Convert_vector_store::AUTO:
      return "auto";
    case Convert_vector_store::CONVERT:
      return "convert";
    case Convert_vector_store::KEEP:
      return "keep";
    case Convert_vector_store::SKIP:
      return "skip";
  }

  throw std::logic_error{"to_string(Convert_vector_store)"};
}

Convert_vector_store to_convert_vector_store(std::string_view s) {
  if ("auto" == s) {
    return Convert_vector_store::AUTO;
  } else if ("convert" == s) {
    return Convert_vector_store::CONVERT;
  } else if ("keep" == s) {
    return Convert_vector_store::KEEP;
  } else if ("skip" == s) {
    return Convert_vector_store::SKIP;
  } else {
    // WL16802-FR2.1.1: throw on unknown values
    throw std::invalid_argument{"'auto', 'convert', 'keep', 'skip'"};
  }
}

}  // namespace load
}  // namespace mysqlsh
