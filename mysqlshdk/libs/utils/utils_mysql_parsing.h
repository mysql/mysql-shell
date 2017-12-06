/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _UTILS_MYSQL_PARSING_H_
#define _UTILS_MYSQL_PARSING_H_

#include "scripting/common.h"

#define SPACES " \t\r\n"

#include <string>
#include <vector>
#include <stack>
#include <initializer_list>

namespace shcore {
namespace mysql {
namespace splitter {
class SHCORE_PUBLIC Delimiters final {
public:
  using delim_type_t = std::string;

  Delimiters() = default;
  Delimiters(const std::initializer_list<delim_type_t> &delimiters);

  std::size_t size() const;
  void set_main_delimiter(delim_type_t delimiter);
  const delim_type_t& get_main_delimiter() const;

  delim_type_t& operator[](std::size_t pos);

private:
  delim_type_t main_delimiter;
  std::vector<delim_type_t> additional_delimiters;
};

class SHCORE_PUBLIC Statement_range final {
public:
  explicit Statement_range(std::size_t begin, std::size_t end,
      Delimiters::delim_type_t delimiter);

  std::size_t offset() const;
  std::size_t length() const;
  const Delimiters::delim_type_t& get_delimiter() const;

private:
  std::size_t m_begin;
  std::size_t m_end;
  Delimiters::delim_type_t m_delimiter;
};

// String SQL parsing functions (from WB)
const unsigned char* skip_leading_whitespace(const unsigned char *head, const unsigned char *tail);
bool is_line_break(const unsigned char *head, const unsigned char *line_break);
std::vector<Statement_range> SHCORE_PUBLIC determineStatementRanges(const char *sql, size_t length,
    Delimiters &delimiters, const std::string &line_break,
    std::stack<std::string> &input_context_stack);
}
}
}

#endif
