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

#include <string>
#include <utility>

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_row.h"
#include "utils/utils_general.h"

namespace testing {
void Mock_row::init(const std::vector<std::string> &names,
                    const std::vector<mysqlshdk::db::Type> &types,
                    const std::vector<std::string> &data) {
  _names = std::move(names);
  _types = std::move(types);
  _record = std::move(data);
  enable_fake_engine();
}

void Mock_row::enable_fake_engine() {
  ON_CALL(*this, get_as_string(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_as_string));
  ON_CALL(*this, num_fields())
      .WillByDefault(Invoke(this, &Mock_row::def_num_fields));
  ON_CALL(*this, get_int(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_int));
  ON_CALL(*this, get_uint(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_uint));
  ON_CALL(*this, get_string(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_string));
  ON_CALL(*this, get_string_data(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_string_data));
  ON_CALL(*this, get_raw_data(_, _, _))
      .WillByDefault(Invoke(this, &Mock_row::def_get_raw_data));
  ON_CALL(*this, get_float(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_float));
  ON_CALL(*this, get_double(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_double));
  ON_CALL(*this, get_bit(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_bit));
  ON_CALL(*this, get_type(_))
      .WillByDefault(Invoke(this, &Mock_row::def_get_type));
  ON_CALL(*this, is_null(_))
      .WillByDefault(Invoke(this, &Mock_row::def_is_null));
}

uint32_t Mock_row::def_num_fields() const {
  return static_cast<uint32_t>(_names.size());
}

std::string Mock_row::def_get_as_string(uint32_t index) const {
  return _record[index];
}

mysqlshdk::db::Type Mock_row::def_get_type(uint32_t index) const {
  return _types[index];
}

int64_t Mock_row::def_get_int(uint32_t index) const {
  std::string tmp = _record[index];
  return std::stoi(tmp);
}

uint64_t Mock_row::def_get_uint(uint32_t index) const {
  std::string tmp = _record[index];
  return std::stoi(tmp);
}

std::string Mock_row::def_get_string(uint32_t index) const {
  return _record[index];
}

std::pair<const char *, size_t> Mock_row::def_get_string_data(
    uint32_t index) const {
  return std::pair<const char *, size_t>(_record[index].c_str(),
                                         _record[index].size());
}

void Mock_row::def_get_raw_data(uint32_t index, const char **out_data,
                                size_t *out_size) const {
  if (def_is_null(index)) {
    *out_data = nullptr;
    *out_size = 0;
  } else {
    *out_data = _record[index].c_str();
    *out_size = _record[index].size();
  }
}

float Mock_row::def_get_float(uint32_t index) const {
  std::string tmp = _record[index];
  return std::stof(tmp);
}

double Mock_row::def_get_double(uint32_t index) const {
  std::string tmp = _record[index];
  return std::stod(tmp);
}

uint64_t Mock_row::def_get_bit(uint32_t index) const {
  std::string tmp = _record[index];
  return std::stoull(tmp);
}

bool Mock_row::def_is_null(uint32_t index) const {
  return _record[index] == "___NULL___";
}

}  // namespace testing
