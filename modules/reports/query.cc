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

#include "modules/reports/query.h"

#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "modules/mod_shell_reports.h"
#include "modules/mod_utils.h"
#include "modules/reports/native_report.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace reports {

namespace {

class Query_report : public Native_report {
 public:
  class Config {
   public:
    static const char *name() { return "query"; }
    static Report::Type type() { return Report::Type::LIST; }
    static const char *brief() {
      return "Executes the SQL statement given as arguments.";
    }

    static Report::Argc argc() { return {1, Report::k_asterisk}; }
  };

 private:
  void parse(const shcore::Array_t &argv,
             const shcore::Dictionary_t &) override {
    m_query.clear();

    for (const auto &a : *argv) {
      m_query += a.as_string() + " ";
    }

    m_query += ";";
  }

  shcore::Array_t execute() const override {
    // note: we're expecting a single resultset
    const auto result = m_session->query(m_query);
    auto report = shcore::make_array();

    // write headers
    {
      auto headers = shcore::make_array();

      for (const auto &column : result->get_metadata()) {
        headers->emplace_back(column.get_column_label());
      }

      report->emplace_back(std::move(headers));
    }

    // write data
    while (const auto row = result->fetch_one()) {
      auto data = shcore::make_array();
      auto values = get_row_values(*row);

      data->reserve(values.size());
      data->insert(data->end(), std::make_move_iterator(values.begin()),
                   std::make_move_iterator(values.end()));

      report->emplace_back(std::move(data));
    }

    return report;
  }

  std::string m_query;
};

}  // namespace

void register_query_report(Shell_reports *reports) {
  Native_report::register_report<Query_report>(reports);
}

}  // namespace reports
}  // namespace mysqlsh
