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

#include "mysqlshdk/libs/rest/error_codes.h"

#include "mysqlshdk/libs/rest/response.h"

namespace mysqlshdk {
namespace rest {

shcore::Exception to_exception(const Response_error &error,
                               const std::string &context) {
  return shcore::Exception(context + error.format(),
                           54'000 + static_cast<int>(error.status_code()));
}

void translate_current_exception(const std::string &context) {
  const auto exception = std::current_exception();

  if (exception) {
    try {
      std::rethrow_exception(exception);
    } catch (const Response_error &e) {
      throw to_exception(e, context);
    } catch (const shcore::Exception &e) {
      const auto error = e.error();
      const auto msg = context + error->get_string("message", "Unknown error");
      error->set("message", shcore::Value{msg});
      throw shcore::Exception(msg, e.code(), error);
    } catch (...) {
      throw;
    }
  }
}

}  // namespace rest
}  // namespace mysqlshdk
