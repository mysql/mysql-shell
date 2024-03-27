/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REQUEST_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REQUEST_H_

#include <utility>

#include "mysqlshdk/libs/rest/headers.h"
#include "mysqlshdk/libs/rest/request.h"

namespace mysqlshdk {
namespace rest {

class Signed_rest_service;

struct Signed_request : public Request {
 public:
  explicit Signed_request(Masked_string path, Headers headers = {},
                          Query query = {}, bool include_empty_values = false)
      : Request(std::move(path), std::move(headers), std::move(query),
                include_empty_values) {}

  const Headers &headers() const override;

  const Headers &unsigned_headers() const { return m_headers; }

 private:
  friend class Signed_rest_service;

  Signed_rest_service *m_service = nullptr;

  Headers m_signed_headers;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REQUEST_H_
