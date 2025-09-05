/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/common_errors.h"

#include <cassert>
#include <exception>
#include <utility>

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/response.h"

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/progress_thread.h"

namespace mysqlsh {
namespace dump {

namespace {

void translate_exception(const std::exception_ptr exception,
                         const Progress_thread &progress) {
  assert(exception);

  const auto stage = progress.current_stage();

  if (!stage || !stage->has_terminated()) {
    // no context is available, or exception happened after stage has
    // finished, just rethrow
    std::rethrow_exception(exception);
  }

  const auto context = "While '" + stage->description() + "': ";

  try {
    mysqlshdk::rest::translate_current_exception(context);
  } catch (const mysqlshdk::rest::Connection_error &e) {
    THROW_ERROR(SHERR_DL_COMMON_CONNECTION_ERROR, context.c_str(), e.what());
  }
}

}  // namespace

void translate_current_exception(std::string_view context,
                                 const Progress_thread &progress) {
  if (const auto exception = std::current_exception()) {
    try {
      translate_exception(exception, progress);
    } catch (...) {
      log_exception(context, std::current_exception());
      throw;
    }
  }
}

void log_exception(std::string_view context,
                   const std::exception_ptr exception) {
  // we log the exception only when in JSON mode
  if ("off" == mysqlsh::current_shell_options()->get().wrap_json) {
    return;
  }

  assert(exception);

  try {
    try {
      std::rethrow_exception(exception);
    } catch (const shcore::Exception &) {
      throw;
    } catch (const shcore::Error &e) {
      throw shcore::Exception(e.what(), e.code());
    } catch (const std::runtime_error &e) {
      throw shcore::Exception::runtime_error(e.what());
    } catch (const std::invalid_argument &e) {
      throw shcore::Exception::argument_error(e.what());
    } catch (const std::logic_error &e) {
      throw shcore::Exception::logic_error(e.what());
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error(e.what());
    } catch (...) {
      throw shcore::Exception::runtime_error("Unknown error");
    }
  } catch (const shcore::Exception &ex) {
    auto exception_info = shcore::make_dict();

    exception_info->emplace("exception", ex.error());
    exception_info->emplace("context", context);

    current_console()->print_error(
        shcore::str_format(
            "An exception has been reported while running %.*s: %s",
            static_cast<int>(context.size()), context.data(),
            ex.format().c_str()),
        shcore::make_dict("exceptionInfo", std::move(exception_info)));
  }
}

}  // namespace dump
}  // namespace mysqlsh
