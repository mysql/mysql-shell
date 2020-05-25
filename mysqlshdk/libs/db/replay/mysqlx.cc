/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/replay/mysqlx.h"

#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace replay {

namespace {

class Argument_visitor : public xcl::Argument_visitor {
 public:
  Argument_visitor() = delete;

  explicit Argument_visitor(const std::string &query) : m_sql(query, 0) {}

  Argument_visitor(const Argument_visitor &) = delete;

  Argument_visitor(Argument_visitor &&) = default;

  Argument_visitor &operator=(const Argument_visitor &) = delete;

  Argument_visitor &operator=(Argument_visitor &&) = default;

  ~Argument_visitor() override = default;

  std::string query() const { return m_sql.str(); }

  void visit_null() override { m_sql << nullptr; }

  void visit_integer(const int64_t value) override { m_sql << value; }

  void visit_uinteger(const uint64_t value) override { m_sql << value; }

  void visit_double(const double value) override { m_sql << value; }

  void visit_float(const float value) override { m_sql << value; }

  void visit_bool(const bool value) override { m_sql << value; }

  void visit_object(const xcl::Argument_value::Object &) override {
    throw std::logic_error("type not implemented");
  }

  void visit_uobject(const xcl::Argument_value::Unordered_object &) override {
    throw std::logic_error("type not implemented");
  }

  void visit_array(const xcl::Argument_value::Arguments &) override {
    throw std::logic_error("type not implemented");
  }

  void visit_string(const std::string &value) override { m_sql << value; }

  void visit_octets(const std::string &value) override { m_sql << value; }

  void visit_decimal(const std::string &value) override { m_sql << value; }

 private:
  shcore::sqlstring m_sql;
};

}  // namespace

std::string query(const std::string &q, const ::xcl::Argument_array &args) {
  Argument_visitor v{q};
  int i = 0;

  for (const auto &value : args) {
    try {
      value.accept(&v);
    } catch (const std::exception &e) {
      throw std::invalid_argument(shcore::str_format(
          "%s while substituting placeholder value at index #%i", e.what(), i));
    }

    ++i;
  }

  try {
    return v.query();
  } catch (const std::exception &e) {
    throw std::invalid_argument(
        "Insufficient number of values for placeholders in query");
  }
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
