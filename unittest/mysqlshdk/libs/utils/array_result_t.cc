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

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/array_result.h"

#include "unittest/gtest_clean.h"

namespace shcore {

shcore::Array_t get_data() {
  auto table = shcore::make_array();
  auto columns = shcore::make_array();
  columns->emplace_back("Instance");
  columns->emplace_back("Version");
  columns->emplace_back("Last Check-in");
  columns->emplace_back("R/O Port");
  columns->emplace_back("R/W Port");
  table->emplace_back(std::move(columns));

  auto row = shcore::make_array();
  row->emplace_back("localhost:3307");
  row->emplace_back("8.0.18");
  row->emplace_back(nullptr);
  row->emplace_back(33071);
  row->emplace_back(33072);
  table->emplace_back(std::move(row));

  row = shcore::make_array();
  row->emplace_back("localhost:3308");
  row->emplace_back("8.0.18");
  row->emplace_back(nullptr);
  row->emplace_back(33081);
  row->emplace_back(33082);
  table->emplace_back(std::move(row));

  return table;
}

void fetch_data(mysqlshdk::db::IResult *result) {
  EXPECT_TRUE(result->has_resultset());
  EXPECT_EQ(0, result->get_fetched_row_count());

  auto row = result->fetch_one();
  EXPECT_EQ(1, result->get_fetched_row_count());
  EXPECT_EQ(5, row->num_fields());
  EXPECT_EQ("localhost:3307", row->get_string(0));
  EXPECT_EQ("8.0.18", row->get_string(1));
  EXPECT_EQ("NULL", row->get_string(2));
  EXPECT_EQ("33071", row->get_string(3));
  EXPECT_EQ("33072", row->get_string(4));

  row = result->fetch_one();
  EXPECT_EQ(2, result->get_fetched_row_count());
  EXPECT_EQ(5, row->num_fields());
  EXPECT_EQ("localhost:3308", row->get_string(0));
  EXPECT_EQ("8.0.18", row->get_string(1));
  EXPECT_EQ("NULL", row->get_string(2));
  EXPECT_EQ("33081", row->get_string(3));
  EXPECT_EQ("33082", row->get_string(4));

  row = result->fetch_one();
  EXPECT_EQ(nullptr, row);

  EXPECT_FALSE(result->next_resultset());
  EXPECT_FALSE(result->has_resultset());
}

TEST(Array_as_result, positive_tests) {
  auto data = get_data();
  shcore::Array_as_result result(data);

  auto md = result.get_metadata();

  EXPECT_EQ("Instance", md[0].get_column_name());
  EXPECT_EQ("Version", md[1].get_column_name());
  EXPECT_EQ("Last Check-in", md[2].get_column_name());
  EXPECT_EQ("R/O Port", md[3].get_column_name());
  EXPECT_EQ("R/W Port", md[4].get_column_name());

  EXPECT_EQ("Instance", md[0].get_column_label());
  EXPECT_EQ("Version", md[1].get_column_label());
  EXPECT_EQ("Last Check-in", md[2].get_column_label());
  EXPECT_EQ("R/O Port", md[3].get_column_label());
  EXPECT_EQ("R/W Port", md[4].get_column_label());

  auto fn = result.field_names();

  EXPECT_EQ("Instance", fn->field_name(0));
  EXPECT_EQ("Version", fn->field_name(1));
  EXPECT_EQ("Last Check-in", fn->field_name(2));
  EXPECT_EQ("R/O Port", fn->field_name(3));
  EXPECT_EQ("R/W Port", fn->field_name(4));

  EXPECT_EQ(0, result.get_affected_row_count());
  EXPECT_EQ(0, result.get_auto_increment_value());
  EXPECT_EQ(0, result.get_fetched_row_count());
  EXPECT_EQ(0, result.get_warning_count());
  EXPECT_EQ("", result.get_info());

  fetch_data(&result);

  result.rewind();

  fetch_data(&result);
}

TEST(Array_as_result, negative_tests) {
  auto data = get_data();
  shcore::Array_as_result result(data);

  EXPECT_THROW_MSG(result.get_gtids(), std::logic_error, "Not implemented.");

  auto row = result.fetch_one();
  EXPECT_THROW_MSG(row->get_int(0), std::logic_error, "Not implemented.");
  EXPECT_THROW_MSG(row->get_uint(0), std::logic_error, "Not implemented.");
  EXPECT_THROW_MSG(row->get_float(0), std::logic_error, "Not implemented.");
  EXPECT_THROW_MSG(row->get_double(0), std::logic_error, "Not implemented.");
  EXPECT_THROW_MSG(row->get_string_data(0), std::logic_error,
                   "Not implemented.");
  EXPECT_THROW_MSG(row->get_bit(0), std::logic_error, "Not implemented.");
}

}  // namespace shcore
