/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "mysqlshdk/shellcore/shell_cli_operation_provider.h"

namespace shcore {
namespace cli {
std::shared_ptr<Provider> Provider::register_provider(
    const std::string &name, const Provider_factory &ff) {
  return register_provider(name, std::make_shared<Provider>(ff));
}

std::shared_ptr<Provider> Provider::register_provider(
    const std::string &name,
    const std::shared_ptr<shcore::Cpp_object_bridge> &object) {
  return register_provider(name, std::make_shared<Provider>(object));
}

std::shared_ptr<Provider> Provider::register_provider(
    const std::string &name, std::shared_ptr<Provider> &&provider) {
  auto result = m_providers.emplace(name, provider);

  if (!result.second) {
    throw std::invalid_argument(
        "Shell CLI operation provider already registered under name " + name);
  }

  return result.first->second;
}

void Provider::remove_provider(const std::string &name) {
  auto it = m_providers.find(name);
  if (it != m_providers.end()) m_providers.erase(it);
}

}  // namespace cli
}  // namespace shcore
