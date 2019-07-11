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

#include "modules/reports/native_report.h"

#include <utility>

namespace mysqlsh {
namespace reports {

namespace {

static constexpr auto k_report_key = "report";

shcore::Dictionary_t create_report_response(shcore::Array_t &&report) {
  const auto response = shcore::make_dict();

  response->emplace(k_report_key, std::move(report));

  return response;
}

}  // namespace

shcore::Dictionary_t Native_report::report(
    const std::shared_ptr<ShellBaseSession> &session,
    const shcore::Array_t &argv, const shcore::Dictionary_t &options) {
  m_session = session->get_core_session();
  parse(argv, options);
  return create_report_response(execute());
}

}  // namespace reports
}  // namespace mysqlsh
