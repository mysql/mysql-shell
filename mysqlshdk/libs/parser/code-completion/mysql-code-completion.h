/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#pragma once

#include <string_view>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/parser/code-completion/mysql_code_completion_result.h"
#include "mysqlshdk/libs/parser/parsers-common.h"

namespace parsers {
class MySQLParser;
}  // namespace parsers

PARSERS_PUBLIC_TYPE parsers::Sql_completion_result getCodeCompletion(
    size_t offset, std::string_view active_schema, bool uppercase_keywords,
    bool filtered, parsers::MySQLParser *parser);

PARSERS_PUBLIC_TYPE void resetPreconditions(parsers::MySQLParser *parser);
