/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/secret-store-api/helper_invoker.h"

#include <vector>

#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/secret-store-api/logger.h"

namespace mysql {
namespace secret_store {
namespace api {

namespace {

constexpr auto k_secret = "\"Secret\"";

std::string hide_secret(const std::string &s) {
  const auto pos = s.find(k_secret);

  if (pos == std::string::npos) {
    return s;
  } else {
    const auto start = s.find('"', pos + strlen(k_secret));
    const auto end = mysqlshdk::utils::span_quoted_string_dq(s, start);

    return s.substr(0, start + 1) + "****" + s.substr(end - 1);
  }
}

}  // namespace

Helper_invoker::Helper_invoker(const Helper_name &name) : m_name{name} {}

bool Helper_invoker::store(const std::string &input) const {
  std::string output;
  return store(input, &output);
}

bool Helper_invoker::store(const std::string &input,
                           std::string *output) const {
  return invoke("store", input, output);
}

bool Helper_invoker::get(const std::string &input, std::string *output) const {
  return invoke("get", input, output);
}

bool Helper_invoker::erase(const std::string &input) const {
  std::string output;
  return erase(input, &output);
}

bool Helper_invoker::erase(const std::string &input,
                           std::string *output) const {
  return invoke("erase", input, output);
}

bool Helper_invoker::list(std::string *output) const {
  return invoke("list", {}, output);
}

bool Helper_invoker::version() const {
  std::string output;
  return version(&output);
}

bool Helper_invoker::version(std::string *output) const {
  return invoke("version", {}, output);
}

bool Helper_invoker::invoke(const char *command, const std::string &input,
                            std::string *output) const {
  try {
    std::string path = m_name.path();
    const char *const args[] = {path.c_str(), command, nullptr};

    logger::log("Invoking helper");
    logger::log("  Command line: " + path + " " + command);
    logger::log("  Input: " + hide_secret(input));

    shcore::Process_launcher app{args};

    app.start();

    if (!input.empty()) {
      app.write(input.c_str(), input.length());
      app.finish_writing();
    }

    *output = shcore::str_strip(app.read_all());

    const auto exit_code = app.wait();

    logger::log("  Output: " + hide_secret(*output));
    logger::log("  Exit code: " + std::to_string(exit_code));

    return exit_code == 0;
  } catch (const std::exception &ex) {
    *output = std::string{"Exception caught while running helper command '"} +
              command + "': " + ex.what();
    logger::log("  Output: " + hide_secret(*output));
    return false;
  }
}

}  // namespace api
}  // namespace secret_store
}  // namespace mysql
