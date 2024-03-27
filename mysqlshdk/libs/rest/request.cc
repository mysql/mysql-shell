/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

std::string add_query_parameters(const std::string &path, const Query &query,
                                 bool include_empty_values) {
  std::string p = path;

  if (!query.empty()) {
    p.reserve(256);
    p += '?';
    p += www_form_urlencoded(query, include_empty_values);
  }

  return p;
}

}  // namespace

std::string type_name(Type method) {
  switch (method) {
    case Type::DELETE:
      return "delete";

    case Type::GET:
      return "get";

    case Type::HEAD:
      return "head";

    case Type::PATCH:
      return "patch";

    case Type::POST:
      return "post";

    case Type::PUT:
      return "put";
  }

  throw std::logic_error("Unknown method received");
}

std::string www_form_urlencoded(const Query &query, bool include_empty_values) {
  std::string q;

  if (!query.empty()) {
    q.reserve(256);

    for (const auto &param : query) {
      q += param.first;

      if (include_empty_values || param.second.has_value()) {
        q += '=';
      }

      if (param.second.has_value()) {
        q += shcore::pctencode(param.second.value());
      }

      q += '&';
    }

    // remove the last ampersand
    q.pop_back();
  }

  return q;
}

Request::Request(Masked_string path, Headers headers, Query query,
                 bool include_empty_values)
    : m_path(std::move(path)),
      m_headers(std::move(headers)),
      m_query(std::move(query)),
      m_full_path(
          add_query_parameters(m_path.real(), m_query, include_empty_values)) {}

}  // namespace rest
}  // namespace mysqlshdk
