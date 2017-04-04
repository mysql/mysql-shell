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

#include "unittest/mocks/mysqlshdk/libs/db/mock_row.h"
#include "utils/utils_general.h"

#include <utility>

namespace testing {
  Mock_row::Mock_row(const std::vector<std::string>& names,
                     const std::vector<mysqlshdk::db::Type>& types,
                     const std::vector<std::string>& data):
                     _names(names),
                     _types(types),
                     _record(data)
  {
    enable_fake_engine();
  }

  void Mock_row::enable_fake_engine() {
    ON_CALL(*this, size()).WillByDefault(Invoke(this, &Mock_row::def_size));
    ON_CALL(*this, get_int(_)).WillByDefault(Invoke(this, &Mock_row::def_get_int));
    ON_CALL(*this, get_uint(_)).WillByDefault(Invoke(this, &Mock_row::def_get_uint));
    ON_CALL(*this, get_string(_)).WillByDefault(Invoke(this, &Mock_row::def_get_string));
    ON_CALL(*this, get_data(_)).WillByDefault(Invoke(this, &Mock_row::def_get_data));
    ON_CALL(*this, get_double(_)).WillByDefault(Invoke(this, &Mock_row::def_get_double));
    ON_CALL(*this, get_date(_)).WillByDefault(Invoke(this, &Mock_row::def_get_date));
    ON_CALL(*this, is_null(_)).WillByDefault(Invoke(this, &Mock_row::def_is_null));
    ON_CALL(*this, is_int(_)).WillByDefault(Invoke(this, &Mock_row::def_is_int));
    ON_CALL(*this, is_uint(_)).WillByDefault(Invoke(this, &Mock_row::def_is_uint));
    ON_CALL(*this, is_string(_)).WillByDefault(Invoke(this, &Mock_row::def_is_string));
    ON_CALL(*this, is_double(_)).WillByDefault(Invoke(this, &Mock_row::def_is_double));
    ON_CALL(*this, is_date(_)).WillByDefault(Invoke(this, &Mock_row::def_is_date));
  }

  size_t Mock_row::def_size() const {
    return _names.size();
  }

  int64_t Mock_row::def_get_int(int index) const {
    std:: string tmp = _record[index];
    return std::stoi(tmp);
  }

  uint64_t Mock_row::def_get_uint(int index) const{
    std:: string tmp = _record[index];
    return std::stoi(tmp);
  }

  std::string Mock_row::def_get_string(int index) const{
    return _record[index];
  }

  std::pair<const char*, size_t> Mock_row::def_get_data(int index) const{
    return std::pair<const char*, size_t>(_record[index].c_str(), _record[index].size());
  }

  double Mock_row::def_get_double(int index) const{
    std:: string tmp = _record[index];
    return std::stod(tmp);
  }

  std::string Mock_row::def_get_date(int index) const{
    return _record[index];
  }


  bool Mock_row::def_is_null(int index) const{
    return _record[index] == "null";
  }

  bool Mock_row::def_is_int(int index) const{
    std:: string tmp0 = _record[index];
    int tmp1 = std::stoi(tmp0);
    std::string tmp2 = std::to_string(tmp1);
    return tmp2 == _record[index];
  }

  bool Mock_row::def_is_uint(int index) const{
    std::string tmp0 = _record[index];
    int tmp1 = std::stoi(tmp0);
    std::string tmp2 = std::to_string(tmp1);
    return tmp2 == _record[index];
  }

  bool Mock_row::def_is_string(int index) const{
    return true;
  }

  bool Mock_row::def_is_double(int index) const{
    std::string tmp0 = _record[index];
    double tmp1 = std::stod(tmp0);
    std::string tmp2 = std::to_string(tmp1);
    return tmp2 == _record[index];
  }

  bool Mock_row::def_is_binary(int index) const{
    return false; // TODO : Add proper date comparison logic
  }

  bool Mock_row::def_is_date(int index) const{
    return false; // TODO : Add proper date comparison logic
  }
}
