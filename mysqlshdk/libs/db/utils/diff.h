/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_UTILS_DIFF_H_
#define MYSQLSHDK_LIBS_DB_UTILS_DIFF_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/mutable_result.h"

namespace mysqlshdk {
namespace db {

enum class Row_difference {
  Identical,      //< All fields of the row are the same between left and right
  Fields_differ,  //< One or more fields differ between left and right
  Row_missing,    //< Row from the left is missing on the right
  Row_added       //< Row from the right is missing on the left
};

int compare(const IRow &lrow, const IRow &rrow);

size_t find_different_row_fields(const IRow &lrow, const IRow &rrow,
                                 std::function<bool(int)> callback);

size_t find_different_row_fields(
    const Row_ref_by_name &lrow, const Row_ref_by_name &rrow,
    std::function<bool(const std::string &)> callback);

size_t find_different_rows(
    IResult *left, IResult *right,
    std::function<bool(const IRow *, const IRow *, Row_difference)> callback,
    bool call_for_all = false);

size_t find_different_rows(
    IResult *left, IResult *right,
    std::function<bool(const Row_ref_by_name &, const Row_ref_by_name &,
                       Row_difference)>
        callback,
    bool call_for_all = false);

size_t find_different_rows_with_key_indexes(
    IResult *left, IResult *right, const std::vector<uint32_t> &key_fields,
    std::function<bool(const IRow *, const IRow *, Row_difference)> callback,
    bool call_for_all = false);

size_t find_different_rows_with_key_names(
    IResult *left, IResult *right,
    const std::vector<std::string> &key_field_names,
    std::function<bool(const IRow *, const IRow *, Row_difference)> callback,
    bool call_for_all = false);

size_t find_different_rows_with_key_names(
    IResult *left, IResult *right,
    const std::vector<std::string> &key_field_names,
    std::function<bool(const Row_ref_by_name &, const Row_ref_by_name &,
                       Row_difference)>
        callback,
    bool call_for_all = false);

std::unique_ptr<Mutable_result> merge_sorted(
    IResult *left, IResult *right, const std::vector<uint32_t> &key_fields);

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_UTILS_DIFF_H_
