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

#include "mysqlshdk/libs/db/utils/utils.h"
#include <vector>

#include "mysqlshdk/libs/db/row.h"

namespace mysqlshdk {
namespace db {

std::string to_string(const IRow &row) {
  std::string text;
  if (row.num_fields() > 0) {
    text.append(row.get_as_string(0));
    for (uint32_t i = 1; i < row.num_fields(); ++i) {
      text.append("\t");
      text.append(row.get_as_string(i));
    }
  }
  return text;
}

void print(std::ostream &s, const IRow &row) {
  if (row.num_fields() > 0) {
    s << row.get_as_string(0);
    for (uint32_t i = 1; i < row.num_fields(); ++i) {
      s << "\t";
      s << row.get_as_string(i);
    }
  }
}

void dump(std::ostream &s, IResult *result) {
  auto &columns = result->get_metadata();

  if (columns.size() > 0) {
    s << columns[0].get_column_label();
    for (size_t i = 1; i < columns.size(); ++i) {
      s << "\t";
      s << columns[i].get_column_label();
    }
    s << "\n";
  }
  while (const auto row = result->fetch_one()) {
    print(s, *row);
    s << "\n";
  }
}

}  // namespace db
}  // namespace mysqlshdk
