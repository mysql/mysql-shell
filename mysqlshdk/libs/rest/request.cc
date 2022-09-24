/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/rest/request.h"

namespace mysqlshdk {
namespace rest {

namespace {
std::string add_query_parameters(const std::string &path, const Query &query) {
  std::string p = path;

  if (!query.empty()) {
    p.reserve(256);
    p += '?';

    for (const auto &param : query) {
      p += param.first;

      if (param.second.has_value()) {
        p += '=';
        p += shcore::pctencode(param.second.value());
      }

      p += '&';
    }

    // remove the last ampersand
    p.pop_back();
  }

  return p;
}

}  // namespace

Request::Request(Masked_string path, Headers headers, Query query)
    : m_path(std::move(path)),
      m_headers(std::move(headers)),
      m_query(std::move(query)),
      m_full_path(add_query_parameters(m_path.real(), m_query)) {}

}  // namespace rest
}  // namespace mysqlshdk
