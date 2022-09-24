/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace uri {

bool IUri_parsable::is_allowed_scheme(const std::string &name) const {
  const auto &schemes = allowed_schemes();
  return schemes.empty() || schemes.find(name) != schemes.end();
}

void IUri_parsable::validate_allowed_scheme(const std::string &scheme) const {
  if (!is_allowed_scheme(scheme)) {
    const auto &schemes = allowed_schemes();
    throw std::invalid_argument(shcore::str_format(
        "Invalid scheme [%s], supported schemes include: %s", scheme.c_str(),
        shcore::str_join(schemes.begin(), schemes.end(), ", ").c_str()));
  }
}

Uri_serializable::Uri_serializable(
    const std::unordered_set<std::string> &allowed_schemes)
    : m_allowed_schemes(allowed_schemes) {}

const std::unordered_set<std::string> &Uri_serializable::allowed_schemes()
    const {
  return m_allowed_schemes;
}

std::string Uri_serializable::as_uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  return Uri_encoder().encode_uri(*this, format);
}

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk