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

#include "mysqlshdk/libs/mysql/script.h"

#include <cassert>
#include <memory>
#include <stack>

#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"

namespace mysqlshdk {
namespace mysql {

size_t execute_sql_script(std::shared_ptr<mysqlshdk::db::ISession> session,
                          const std::string &script) {
  shcore::mysql::splitter::Delimiters delimiters({";"});
  std::stack<std::string> parsing_context_stack;

  auto ranges = shcore::mysql::splitter::determineStatementRanges(
      script.data(), script.length(), delimiters, "\n", parsing_context_stack);
  size_t range_index = 0;
  for (; range_index < ranges.size(); range_index++) {
    assert(!ranges[range_index].get_delimiter().empty());
    session->query(script.substr(ranges[range_index].offset(),
                                 ranges[range_index].length()));
  }
  return range_index;
}

}  // namespace mysql
}  // namespace mysqlshdk
