/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_DIALECT_H_
#define MODULES_UTIL_IMPORT_TABLE_DIALECT_H_

#include <string>

#include "mysqlshdk/include/scripting/types/option_pack.h"

namespace shcore {

class Option_unpacker;

}  // namespace shcore

namespace mysqlsh {
namespace import_table {

/**
 * Store field- and line-handling options for LOAD DATA INFILE
 */
struct Dialect {
  static const shcore::Option_pack_def<Dialect> &options();

  std::string lines_terminated_by{"\n"};   // string
  std::string fields_escaped_by{"\\"};     // char
  std::string fields_terminated_by{"\t"};  // string
  std::string fields_enclosed_by{};        // char
  bool fields_optionally_enclosed = false;
  std::string lines_starting_by{""};  // string

  void set_dialect(const std::string &option, const std::string &name);

  bool operator==(const Dialect &d) const;

  /**
   * Build part of LOAD DATA LOCAL INFILE SQL statement describing import file
   * dialect.
   * @return Part of SQL statement
   */
  std::string build_sql() const;

  /**
   * Returns default dialect for LOAD DATA FILE.
   */
  static Dialect default_() { return Dialect(); }

  /**
   * Returns dialect for LOAD DATA FILE that describes JSON documents file
   * format.
   */
  static Dialect json();

  /**
   * Returns dialect for LOAD DATA FILE that describes CSV (comma-separated
   * values) file format.
   */
  static Dialect csv();

  /**
   * Returns dialect for LOAD DATA FILE that describes TSV (tab-separated
   * values) file format.
   */
  static Dialect tsv();

  /**
   * Returns dialect for LOAD DATA FILE that describes unix variant of CSV
   * (comma-separated values) file format.
   */
  static Dialect csv_unix();

  /**
   * Returns dialect for LOAD DATA FILE that describes unix variant of CSV
   * (comma-separated values) file format which is compliant with RFC4180.
   */
  static Dialect csv_rfc_unix();

 private:
  void on_unpacked_options();
  /**
   * Validate provided options
   */
  void validate() const;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_DIALECT_H_
