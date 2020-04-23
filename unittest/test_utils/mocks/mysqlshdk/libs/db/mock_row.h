/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_ROW_H_
#define UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_ROW_H_

#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/row.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"

namespace testing {
class Mock_row : public mysqlshdk::db::IRow {
 public:
  Mock_row() {}
  Mock_row(const std::vector<std::string> &names,
           const std::vector<mysqlshdk::db::Type> &types,
           const std::vector<std::string> &data);
  void init(const std::vector<std::string> &names,
            const std::vector<mysqlshdk::db::Type> &types,
            const std::vector<std::string> &data);

  MOCK_CONST_METHOD1(get_as_string, std::string(uint32_t index));

  MOCK_CONST_METHOD0(num_fields, uint32_t());
  MOCK_CONST_METHOD1(get_int, int64_t(uint32_t index));
  MOCK_CONST_METHOD1(get_uint, uint64_t(uint32_t index));
  MOCK_CONST_METHOD1(get_string, std::string(uint32_t index));
  MOCK_CONST_METHOD1(get_string_data,
                     std::pair<const char *, size_t>(uint32_t index));
  MOCK_CONST_METHOD3(get_raw_data, void(uint32_t index, const char **out_data,
                                        size_t *out_size));
  MOCK_CONST_METHOD1(get_float, float(uint32_t index));
  MOCK_CONST_METHOD1(get_double, double(uint32_t index));
  MOCK_CONST_METHOD1(get_bit, uint64_t(uint32_t index));

  MOCK_CONST_METHOD1(get_type, mysqlshdk::db::Type(uint32_t index));
  MOCK_CONST_METHOD1(is_null, bool(uint32_t index));

 private:
  std::vector<std::string> _names;
  std::vector<mysqlshdk::db::Type> _types;
  std::vector<std::string> _record;

  virtual std::string def_get_as_string(uint32_t index) const;
  virtual uint32_t def_num_fields() const;

  virtual int64_t def_get_int(uint32_t index) const;
  virtual uint64_t def_get_uint(uint32_t index) const;
  virtual std::string def_get_string(uint32_t index) const;
  virtual std::pair<const char *, size_t> def_get_string_data(
      uint32_t index) const;
  virtual void def_get_raw_data(uint32_t index, const char **out_data,
                                size_t *out_size) const;
  virtual float def_get_float(uint32_t index) const;
  virtual double def_get_double(uint32_t index) const;
  virtual uint64_t def_get_bit(uint32_t index) const;

  virtual mysqlshdk::db::Type def_get_type(uint32_t index) const;
  virtual bool def_is_null(uint32_t index) const;

  void enable_fake_engine();
};
}  // namespace testing

#endif  // UNITTEST_MOCKS_MYSQLSHDK_LIBS_DB_MOCK_ROW_H_
