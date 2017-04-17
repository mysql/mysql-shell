/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _UNITTEST_MOCKS_CORELIBS_DB_FAKE_ROW_H
#define _UNITTEST_MOCKS_CORELIBS_DB_FAKE_ROW_H

#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/db/column.h"
#include "unittest/mocks/mysqlshdk/libs/db/mock_result.h"
#include "mocks/gmock_clean.h"
#include <vector>

namespace testing {
class Mock_row : public mysqlshdk::db::IRow {
public:
  Mock_row() {}
  Mock_row(const std::vector<std::string>& names, const std::vector<mysqlshdk::db::Type>& types, const std::vector<std::string>&data);
  MOCK_CONST_METHOD0(size, size_t());
  MOCK_CONST_METHOD1(get_int, int64_t(int index));
  MOCK_CONST_METHOD1(get_uint, uint64_t(int index));
  MOCK_CONST_METHOD1(get_string, std::string(int index));
  MOCK_CONST_METHOD1(get_data, std::pair<const char*, size_t>(int index));
  MOCK_CONST_METHOD1(get_double, double(int index));
  MOCK_CONST_METHOD1(get_date, std::string(int index));

  MOCK_CONST_METHOD1(is_null, bool(int index));
  MOCK_CONST_METHOD1(is_int, bool(int index));
  MOCK_CONST_METHOD1(is_uint, bool(int index));
  MOCK_CONST_METHOD1(is_string, bool(int index));
  MOCK_CONST_METHOD1(is_double, bool(int index));
  MOCK_CONST_METHOD1(is_date, bool(int index));
  MOCK_CONST_METHOD1(is_binary, bool(int index));

private:
  std::vector<std::string> _names;
  std::vector<mysqlshdk::db::Type> _types;
  std::vector<std::string> _record;

  virtual size_t def_size() const;

  virtual int64_t def_get_int(int index) const;
  virtual uint64_t def_get_uint(int index) const;
  virtual std::string def_get_string(int index) const;
  virtual std::pair<const char*, size_t> def_get_data(int index) const;
  virtual double def_get_double(int index) const;
  virtual std::string def_get_date(int index) const;

  virtual bool def_is_null(int index) const;
  virtual bool def_is_int(int index) const;
  virtual bool def_is_uint(int index) const;
  virtual bool def_is_string(int index) const;
  virtual bool def_is_double(int index) const;
  virtual bool def_is_date(int index) const;
  virtual bool def_is_binary(int index) const;

  void enable_fake_engine();
};
}

#endif // MOCK_SESSION_H
