/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "modules/util/dump/common_errors.h"

#include <exception>

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/response.h"

#include "modules/util/dump/progress_thread.h"

namespace mysqlsh {
namespace dump {

void translate_current_exception(const Progress_thread &progress) {
  const auto exception = std::current_exception();

  if (exception) {
    const auto stage = progress.current_stage();

    if (!stage) {
      // no context is available, just rethrow
      std::rethrow_exception(exception);
    }

    const auto context = "While '" + stage->description() + "': ";

    try {
      std::rethrow_exception(exception);
    } catch (const mysqlshdk::rest::Response_error &e) {
      throw mysqlshdk::rest::to_exception(e, context);
    } catch (const mysqlshdk::rest::Connection_error &e) {
      THROW_ERROR(SHERR_DL_COMMON_CONNECTION_ERROR, context.c_str(), e.what());
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

}  // namespace dump
}  // namespace mysqlsh
