/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_DUMP_UTILS_H_
#define MODULES_UTIL_DUMP_DUMP_UTILS_H_

#include <string>

namespace mysqlsh {
namespace dump {

// Encodes/decodes filenames for schema/table dumps
// All strings must be UTF-8

std::string encode_schema_basename(const std::string &schema);

std::string encode_table_basename(const std::string &schema,
                                  const std::string &table);

std::string encode_partition_basename(const std::string &schema,
                                      const std::string &table,
                                      const std::string &partition);

std::string get_schema_filename(const std::string &basename);

std::string get_schema_filename(const std::string &basename,
                                const std::string &ext);

std::string get_table_filename(const std::string &basename);

std::string get_table_data_filename(const std::string &basename,
                                    const std::string &ext);

std::string get_table_data_filename(const std::string &basename,
                                    const std::string &ext, size_t index,
                                    bool last_chunk);

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_DUMP_UTILS_H_
