/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/utils/diff.h"
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/mutable_result.h"

namespace mysqlshdk {
namespace db {

int compare_decimal_str(const std::string &n1, const std::string &n2) {
  // we assume n1 and n2 are valid MySQL decimal values
  if (n1.empty() || n2.empty()) throw std::invalid_argument("invalid argument");

  double d = std::stod(n1) - std::stod(n2);
  if (d < 0)
    return -1;
  else if (d > 0)
    return 1;
  else
    return 0;
}

int compare_field(const IRow &lrow, const IRow &rrow, uint32_t field) {
  assert(lrow.get_type(field) == rrow.get_type(field));

  if (lrow.is_null(field) && !rrow.is_null(field))
    return 1;
  else if (lrow.is_null(field) && rrow.is_null(field))
    return 0;
  else if (rrow.is_null(field))
    return -1;

  switch (lrow.get_type(field)) {
    case Type::Null:
      // not supposed to reach here
      return 0;

    case Type::Date:
    case Type::DateTime:
    case Type::Time:
    case Type::Geometry:
    case Type::Json:
    case Type::Enum:
    case Type::Set:
    case Type::String:
    case Type::Bytes: {
      int r = lrow.get_string(field).compare(rrow.get_string(field));
      if (r < 0)
        return -1;
      else if (r > 0)
        return 1;
      else
        return 0;
    }

    case Type::Decimal:  // compare Decimal as strings to not mess precision
      return compare_decimal_str(lrow.get_string(field),
                                 rrow.get_string(field));

    case Type::Integer:
      if (lrow.get_int(field) == rrow.get_int(field)) return 0;
      if (lrow.get_int(field) < rrow.get_int(field)) return -1;
      return 1;

    case Type::UInteger:
      if (lrow.get_uint(field) == rrow.get_uint(field)) return 0;
      if (lrow.get_uint(field) < rrow.get_uint(field)) return -1;
      return 1;

    case Type::Float:
      // comparing -inf/+inf/nan are left as undefined behaviour, since they're
      // not well defined in mysql either
      if (lrow.get_float(field) == rrow.get_float(field)) return 0;
      if (lrow.get_float(field) < rrow.get_float(field)) return -1;
      return 1;

    case Type::Double:
      if (lrow.get_double(field) == rrow.get_double(field)) return 0;
      if (lrow.get_double(field) < rrow.get_double(field)) return -1;
      return 1;

    case Type::Bit:
      return lrow.get_as_string(field).compare(rrow.get_as_string(field));
  }
  return 0;
}

/*
 * Compare 2 rows to determine relative ordering
 *
 * @param  lrow        row to compare
 * @param  rrow        row to be compared with
 * @return             -1, 0, +1
 *
 * This will compare two rows, field by field and return -1, 0 or +1
 * if lrow < rrow, lrow == rrow or lrow > rrow
 */
int compare(const IRow &lrow, const IRow &rrow) {
  assert(lrow.num_fields() == rrow.num_fields());

  for (uint32_t i = 0; i < lrow.num_fields(); i++) {
    int r = compare_field(lrow, rrow, i);
    if (r != 0) return r;
  }
  return 0;
}

/*
 * Compare 2 rows to determine relative ordering, considering their key values
 *
 * @param  lrow        row to compare
 * @param  rrow        row to be compared with
 * @param  key_fields  vector of index columns that are part of the key
 * @return             -2, -1, 0, +1, +2
 *
 * Return value is:
 *   0 if the rows are identical
 *   -1 if lrow.key < rrow.key
 *   +1 if lrow.key > rrow.key
 *   -2 if lrow < rrow
 *   +2 if lrow > rrow
 */
int compare(const IRow &lrow, const IRow &rrow,
            const std::vector<bool> &key_fields) {
  assert(lrow.num_fields() == rrow.num_fields() &&
         key_fields.size() == lrow.num_fields());
  int difference = 0;
  for (uint32_t i = 0, c = lrow.num_fields(); i < c; i++) {
    int r = compare_field(lrow, rrow, i);
    if (r != 0) {
      if (key_fields[i]) {
        return r << 1;
      }
      difference = r;
    }
  }
  return difference;
}

size_t find_different_row_fields(const IRow &lrow, const IRow &rrow,
                                 std::function<bool(int)> callback) {
  assert(lrow.num_fields() == rrow.num_fields());
  size_t c = 0;
  for (uint32_t i = 0; i < lrow.num_fields(); i++) {
    if (compare_field(lrow, rrow, i) != 0) {
      c++;
      if (!callback(i)) break;
    }
  }
  return c;
}

/**
 * Iterate fields of two rows with identical structure and call callback
 * on fields with different values.
 */
size_t find_different_row_fields(
    const Row_ref_by_name &lrow, const Row_ref_by_name &rrow,
    std::function<bool(const std::string &)> callback) {
  assert(lrow.num_fields() == rrow.num_fields());
  size_t c = 0;
  for (uint32_t i = 0; i < lrow.num_fields(); i++) {
    if (compare_field(*lrow, *rrow, i) != 0) {
      c++;
      if (!callback(lrow.field_name(i))) break;
    }
  }
  return c;
}

static void reset_result(IResult *r) {
  Mutable_result *rs = dynamic_cast<Mutable_result *>(r);
  if (rs != nullptr) rs->reset();
}

/**
 * Iterate rows from two sorted result objects, compare them and call the
 * callback for any different rows found. If call_for_all is true, the callback
 * will also be called for identical rows.
 *
 * If the callback returns false, the iteration will stop.
 *
 * Since it's not possible to tell for sure that rows from each result
 * represent the same record, this function can't tell whether rows
 * are Different. If that is needed, use the version which takes a key_fields
 * list.
 *
 * @param  left     Result to be compared
 * @param  right    Result to be compared with
 * @param  callback Function to call for each different row
 * @param  call_on_identical  If true, the callback is always called.
 * @return          Total number of rows processed.
 */
size_t find_different_rows(
    IResult *left, IResult *right,
    std::function<bool(const IRow *, const IRow *, Row_difference)> callback,
    bool call_on_identical) {
  if (left->get_metadata() != right->get_metadata())
    throw std::invalid_argument("Compared results have different fields");

  std::unique_ptr<IResult, void (*)(IResult *)> lr(left, reset_result);
  std::unique_ptr<IResult, void (*)(IResult *)> rr(right, reset_result);
  size_t count = 0;
  const auto *lrow = left->fetch_one();
  const auto *rrow = right->fetch_one();
  while (lrow || rrow) {
    int d = !lrow ? 1 : (!rrow ? -1 : compare(*lrow, *rrow));
    ++count;
    switch (d) {
      case 0:
        if (call_on_identical &&
            !callback(lrow, rrow, Row_difference::Identical))
          return count;
        lrow = left->fetch_one();
        rrow = right->fetch_one();
        break;
      case -1:
        if (!callback(lrow, nullptr, Row_difference::Row_missing)) return count;
        lrow = left->fetch_one();
        break;
      case 1:
        if (!callback(nullptr, rrow, Row_difference::Row_added)) return count;
        rrow = right->fetch_one();
        break;
    }
  }
  return count;
}

size_t find_different_rows(
    IResult *left, IResult *right,
    std::function<bool(const Row_ref_by_name &, const Row_ref_by_name &,
                       Row_difference)>
        callback,
    bool call_on_identical) {
  if (left->get_metadata() != right->get_metadata())
    throw std::invalid_argument("Compared results have different fields");

  std::unique_ptr<IResult, void (*)(IResult *)> lr(left, reset_result);
  std::unique_ptr<IResult, void (*)(IResult *)> rr(right, reset_result);
  size_t count = 0;
  auto lrow = left->fetch_one_named();
  auto rrow = right->fetch_one_named();
  while (lrow || rrow) {
    int d = !lrow ? 1 : (!rrow ? -1 : compare(*lrow, *rrow));
    ++count;
    switch (d) {
      case 0:
        if (call_on_identical &&
            !callback(lrow, rrow, Row_difference::Identical))
          return count;
        lrow = left->fetch_one_named();
        rrow = right->fetch_one_named();
        break;
      case -1:
        if (!callback(lrow, {}, Row_difference::Row_missing)) return count;
        lrow = left->fetch_one_named();
        break;
      case 1:
        if (!callback({}, rrow, Row_difference::Row_added)) return count;
        rrow = right->fetch_one_named();
        break;
    }
  }
  return count;
}

template <typename Row_type, typename Get_row>
static size_t find_different_rows(
    IResult *left, IResult *right, const std::vector<bool> &keys,
    std::function<bool(const Row_type, const Row_type, Row_difference)>
        callback,
    bool call_on_identical, Get_row get_row) {
  std::unique_ptr<IResult, void (*)(IResult *)> lr(left, reset_result);
  std::unique_ptr<IResult, void (*)(IResult *)> rr(right, reset_result);
  size_t count = 0;
  auto lrow = get_row(left);
  auto rrow = get_row(right);
  while (lrow || rrow) {
    int d = !lrow ? 2 : (!rrow ? -2 : compare(*lrow, *rrow, keys));
    ++count;
    switch (d) {
      case 0:
        if (call_on_identical &&
            !callback(lrow, rrow, Row_difference::Identical))
          return count;
        lrow = get_row(left);
        rrow = get_row(right);
        break;
      case -2:
        if (!callback(lrow, {}, Row_difference::Row_missing)) return count;
        lrow = get_row(left);
        break;
      case 2:
        if (!callback({}, rrow, Row_difference::Row_added)) return count;
        rrow = get_row(right);
        break;
      case -1:
      case 1:
        if (!callback(lrow, rrow, Row_difference::Fields_differ)) return count;
        lrow = get_row(left);
        rrow = get_row(right);
        break;
    }
  }
  return count;
}

/**
 * Iterate rows from two sorted result objects, compare them and call the
 * callback for any different rows found. If call_for_all is true, the callback
 * will also be called for identical rows.
 *
 * If the callback returns false, the iteration will stop.
 *
 * key_fields is used to match rows from both results. If the values of the
 * key fields are the same, the rows are considered to be the same and will
 * be compared field by field. Otherwise, they're considered different rows.
 *
 * @param  left         Result to be compared
 * @param  right        Result to be compared with
 * @param  key_fields   List of column indexes for the row ids
 * @param  callback     Function to call for each different row
 * @param  call_on_identical  If true, the callback is always called.
 * @return              Total number of rows processed.
 */
size_t find_different_rows_with_key_indexes(
    IResult *left, IResult *right, const std::vector<uint32_t> &key_fields,
    std::function<bool(const IRow *, const IRow *, Row_difference)> callback,
    bool call_on_identical) {
  if (left->get_metadata() != right->get_metadata())
    throw std::invalid_argument("Compared results have different fields");

  std::vector<bool> keys(left->get_metadata().size(), false);
  for (auto i : key_fields) {
    if (i >= keys.size())
      throw std::invalid_argument("Invalid key_field index value");
    keys[i] = true;
  }

  return find_different_rows(
      left, right, keys, callback, call_on_identical,
      [](IResult *result) { return result->fetch_one(); });
}

size_t find_different_rows_with_key_names(
    IResult *left, IResult *right,
    const std::vector<std::string> &key_field_names,
    std::function<bool(const IRow *, const IRow *, Row_difference)> callback,
    bool call_on_identical) {
  if (left->get_metadata() != right->get_metadata())
    throw std::invalid_argument("Compared results have different fields");

  std::vector<bool> keys(left->get_metadata().size(), false);
  std::size_t c = 0;
  for (const auto &n : key_field_names) {
    int i = 0;
    for (const auto &col : left->get_metadata()) {
      if (col.get_column_label() == n) {
        c++;
        keys[i] = true;
        break;
      }
      ++i;
    }
  }
  if (c != key_field_names.size())
    throw std::invalid_argument("Invalid value in key_field_name");

  return find_different_rows(
      left, right, keys, callback, call_on_identical,
      [](IResult *result) { return result->fetch_one(); });
}

size_t find_different_rows_with_key_names(
    IResult *left, IResult *right,
    const std::vector<std::string> &key_field_names,
    std::function<bool(const Row_ref_by_name &, const Row_ref_by_name &,
                       Row_difference)>
        callback,
    bool call_on_identical) {
  if (left->get_metadata() != right->get_metadata())
    throw std::invalid_argument("Compared results have different fields");

  std::vector<bool> keys(left->get_metadata().size(), false);
  std::size_t c = 0;
  for (const auto &n : key_field_names) {
    int i = 0;
    for (const auto &col : left->get_metadata()) {
      if (col.get_column_label() == n) {
        c++;
        keys[i] = true;
        break;
      }
      ++i;
    }
  }
  if (c != key_field_names.size())
    throw std::invalid_argument("Invalid value in key_field_names");

  return find_different_rows(
      left, right, keys, callback, call_on_identical,
      [](IResult *result) { return result->fetch_one_named(); });
}

std::unique_ptr<Mutable_result> merge_sorted(
    IResult *left, IResult *right, const std::vector<uint32_t> &key_fields) {
  if (left->get_metadata() != right->get_metadata())
    throw std::invalid_argument("Results to merge have different fields");
  auto res = std::make_unique<Mutable_result>(left->get_metadata());

  const auto *lrow = left->fetch_one();
  const auto *rrow = right->fetch_one();

  while (lrow && rrow) {
    int r = 0;
    for (auto field : key_fields) {
      r = compare_field(*lrow, *rrow, field);
      if (r != 0) break;
    }
    if (r == 0) r = compare(*lrow, *rrow);
    if (r < 0) {
      res->add_row(std::make_unique<Row_copy>(*lrow));
      lrow = left->fetch_one();
    } else {
      res->add_row(std::make_unique<Row_copy>(*rrow));
      rrow = right->fetch_one();
    }
  }

  while (rrow) {
    res->add_row(std::make_unique<Row_copy>(*rrow));
    rrow = right->fetch_one();
  }
  while (lrow) {
    res->add_row(std::make_unique<Row_copy>(*lrow));
    lrow = left->fetch_one();
  }

  return res;
}

}  // namespace db
}  // namespace mysqlshdk
