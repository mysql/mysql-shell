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

#include "mysql-secret-store/secret-service/secret_tool_invoker.h"

#include "mysql-secret-store/include/helper.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysql {
namespace secret_store {
namespace secret_service {

using mysql::secret_store::common::Helper_exception;

namespace {

constexpr auto k_secret_tool = "secret-tool";

void set_attributes(const Secret_tool_invoker::Attributes &attributes,
                    std::vector<std::string> *args) {
  for (const auto &kv : attributes) {
    args->emplace_back(kv.first);
    args->emplace_back(kv.second);
  }
}

}  // namespace

void Secret_tool_invoker::store(const std::string &label,
                                const Attributes &attributes,
                                const std::string &password) {
  std::vector<std::string> args = {"store", "--label=" + label};
  set_attributes(attributes, &args);
  args.emplace_back("xdg:schema");
  args.emplace_back("org.freedesktop.Secret.Generic");
  invoke(args, true, password);
}

std::string Secret_tool_invoker::get(const Attributes &attributes) {
  std::vector<std::string> args = {"lookup"};
  set_attributes(attributes, &args);
  return invoke(args);
}

void Secret_tool_invoker::erase(const Attributes &attributes) {
  std::vector<std::string> args = {"clear"};
  set_attributes(attributes, &args);
  invoke(args);
}

std::string Secret_tool_invoker::list(const Attributes &attributes) {
  std::vector<std::string> args = {"search", "--all"};
  set_attributes(attributes, &args);
  return invoke(args);
}

std::string Secret_tool_invoker::invoke(const std::vector<std::string> &args,
                                        bool has_password,
                                        const std::string &password) const {
  std::vector<const char *> process_args;
  process_args.emplace_back(k_secret_tool);

  for (const auto &arg : args) {
    process_args.emplace_back(arg.c_str());
  }

  process_args.emplace_back(nullptr);

  shcore::Process_launcher app{&process_args[0]};

  app.start();

  if (has_password) {
    app.write(password.c_str(), password.length());
    app.finish_writing();
  }

  std::string output = shcore::str_strip(app.read_all());
  const auto exit_code = app.wait();

  if (0 != exit_code) {
    throw Helper_exception{"Process \"" + std::string{process_args[0]} +
                           "\" finished with exit code " +
                           std::to_string(exit_code) + ": " + output};
  }

  return output;
}

}  // namespace secret_service
}  // namespace secret_store
}  // namespace mysql
