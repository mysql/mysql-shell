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

#include "modules/reports/utils.h"

#include <algorithm>
#include <utility>

#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"
#include "mysqlshdk/libs/db/mysqlx/expr_parser.h"
#include "mysqlshdk/libs/utils/array_result.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace reports {

namespace {
using Resultset_writer_member = std::string (Resultset_writer::*)();

std::string resultset_formatter(const shcore::Array_t &report,
                                Resultset_writer_member member) {
  shcore::Array_as_result result{report};
  Resultset_writer writer{&result};

  return (writer.*member)();
}

void merge_json_object(const shcore::Array_t &headers,
                       const shcore::Array_t &data,
                       const Column_definition &column,
                       shcore::Dictionary_t &&value) {
  // find the column name in the headers
  const auto pos = std::find_if(
      headers->begin(), headers->end(), [&column](const shcore::Value &v) {
        return (nullptr != column.name) && (v.get_string() == column.name);
      });
  // extra headers are only inserted if column name is found in headers, or
  // if headers are empty
  const auto insert_headers = (pos != headers->end()) || headers->empty();
  // if column name is not found, headers are going to be appended at the end,
  // otherwise they are going to be inserted in its place
  const auto insert_pos = pos == headers->end() ? pos : headers->erase(pos);

  shcore::Value::Array_type extra_headers;

  if (!column.mapping.empty()) {
    for (const auto &m : column.mapping) {
      extra_headers.emplace_back(m.name);
      data->emplace_back(std::move(value->find(m.id)->second));
    }
  } else {
    for (auto &v : *value) {
      extra_headers.emplace_back(std::move(v.first));
      data->emplace_back(std::move(v.second));
    }
  }

  if (insert_headers) {
    headers->insert(insert_pos, std::make_move_iterator(extra_headers.begin()),
                    std::make_move_iterator(extra_headers.end()));
  }
}

}  // namespace

std::string vertical_formatter(const shcore::Array_t &report) {
  return resultset_formatter(report, &Resultset_writer::write_vertical);
}

std::string table_formatter(const shcore::Array_t &report) {
  return resultset_formatter(report, &Resultset_writer::write_table);
}

std::string brief_formatter(const shcore::Array_t &report) {
  shcore::Array_as_result result{report};
  std::string output;

  while (result.has_resultset()) {
    const auto &columns = result.get_metadata();
    const auto prev_size = output.size();

    while (const auto row = result.fetch_one()) {
      for (size_t i = 0; i < columns.size(); ++i) {
        output.append(columns[i].get_column_name())
            .append(": ")
            .append(row->get_as_string(i))
            .append(", ");
      }
    }

    if (output.size() > prev_size) {
      // remove space
      output.pop_back();
      // remove comma
      output.pop_back();
      output.append(".\n");
    }

    result.next_resultset();
  }

  return output;
}

std::string status_formatter(const shcore::Array_t &report) {
  return resultset_formatter(report, &Resultset_writer::write_status);
}

shcore::Array_t create_report_from_json_object(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const std::string &query, const std::vector<Column_definition> &columns,
    const std::vector<Column_definition> &return_columns) {
  std::string final_query;

  if (!columns.empty()) {
    std::string items;

    for (const auto &c : columns) {
      items.append("'").append(c.id).append("',").append(c.query).append(",");
    }

    if (!items.empty()) {
      // remove last comma
      items.pop_back();
    }

    final_query = "SELECT json_object(" + items + ") ";
  }

  final_query += query;

  const auto result = session->query(final_query);
  auto report = shcore::make_array();

  const auto &use_headers = return_columns.empty() ? columns : return_columns;

  // headers
  {
    auto headers = shcore::make_array();

    for (const auto &c : use_headers) {
      headers->emplace_back(c.name);
    }

    report->emplace_back(std::move(headers));
  }

  // write data
  while (const auto row = result->fetch_one()) {
    if (!row->is_null(0)) {
      auto output_row = shcore::make_array();
      auto map = shcore::Value::parse(row->get_string(0)).as_map();

      if (!use_headers.empty()) {
        output_row->reserve(use_headers.size());

        for (const auto &c : use_headers) {
          auto &value = map->find(c.id)->second;

          if (shcore::Value_type::Map == value.type) {
            merge_json_object(report->at(0).as_array(), output_row, c,
                              value.as_map());
          } else {
            output_row->emplace_back(std::move(value));
          }
        }
      } else {
        merge_json_object(report->at(0).as_array(), output_row, {},
                          std::move(map));
      }

      report->emplace_back(std::move(output_row));
    }
  }

  return report;
}

Sql_condition::Sql_condition(const Sql_condition::Validator &ident,
                             const Sql_condition::Validator &op)
    : m_identifier(ident), m_operator(op) {}

std::string Sql_condition::identifier_validator(const std::string &ident) {
  return ident;
}

std::string Sql_condition::operator_validator(const std::string &ident) {
  static constexpr const char *allowed[] = {"==", "!=",   ">",  ">=", "<",
                                            "<=", "like", "&&", "||", "not"};

  if (std::end(allowed) ==
      std::find(std::begin(allowed), std::end(allowed), ident)) {
    throw std::invalid_argument("Disallowed operator: " + ident);
  }

  return "==" == ident ? "=" : ident;
}

std::string Sql_condition::parse(const std::string &condition) const {
  const auto expr = ::mysqlx::Expr_parser{condition}.expr();

  return validate_expression(*expr.get());
}

std::string Sql_condition::validate_expression(
    const Mysqlx::Expr::Expr &expr) const {
  switch (expr.type()) {
    case Mysqlx::Expr::Expr_Type::Expr_Type_IDENT: {
      const auto &ident = expr.identifier();
      const auto name =
          (ident.has_table_name() ? ident.table_name() + "." : "") +
          ident.name();
      return m_identifier(name);
    }

    case Mysqlx::Expr::Expr_Type::Expr_Type_LITERAL: {
      const auto &literal = expr.literal();

      switch (literal.type()) {
        case ::Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_NULL:
          return "NULL";

        case ::Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_BOOL:
          return literal.v_bool() ? "TRUE" : "FALSE";

        case ::Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_DOUBLE:
          return std::to_string(literal.v_double());

        case ::Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_SINT:
          return std::to_string(literal.v_signed_int());

        case ::Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_UINT:
          return std::to_string(literal.v_unsigned_int());

        case ::Mysqlx::Datatypes::Scalar_Type::Scalar_Type_V_OCTETS:
          return shcore::quote_sql_string(literal.v_octets().value());

        default:
          throw std::invalid_argument(
              "Unknown literal type: " +
              ::Mysqlx::Datatypes::Scalar_Type_Name(literal.type()));
      }
    }

    case Mysqlx::Expr::Expr_Type::Expr_Type_OPERATOR: {
      const auto &op = expr.operator_();
      const auto name = m_operator(op.name());

      if ("not" == name) {
        return name + "(" + validate_expression(op.param()[0]) + ")";
      } else {
        return "(" + validate_expression(op.param()[0]) + ")" + name + "(" +
               validate_expression(op.param()[1]) + ")";
      }
    }

    default:
      throw std::invalid_argument("Unknown expression type: " +
                                  ::Mysqlx::Expr::Expr_Type_Name(expr.type()));
  }
}

}  // namespace reports
}  // namespace mysqlsh
