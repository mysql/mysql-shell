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

#include "mysqlshdk/libs/rest/signed/signed_request.h"

#include <ctime>

#include "mysqlshdk/libs/rest/signed/signed_rest_service.h"

namespace mysqlshdk {
namespace rest {

const Headers &Signed_request::headers() const {
  if (m_service && m_service->should_sign_request(*this)) {
    auto self = const_cast<Signed_request *>(this);
    const auto now = time(nullptr);

    if (!m_signed_headers.empty() && !m_service->valid_headers(*this, now)) {
      self->m_signed_headers.clear();
    }

    if (m_signed_headers.empty()) {
      self->m_signed_headers = m_service->make_headers(*this, now);
    }

    return m_signed_headers;
  } else {
    return m_headers;
  }
}

}  // namespace rest
}  // namespace mysqlshdk
