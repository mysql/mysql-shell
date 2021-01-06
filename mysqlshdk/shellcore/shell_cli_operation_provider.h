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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_PROVIDER_H_
#define MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_PROVIDER_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"

namespace shcore {
namespace cli {
/**
 * A factory function is needed to support providers that only exist when
 * the shell is started with specific arguments such as --cluster, --rs.
 *
 * Those objects should be supported in CLI even when they are not created,
 * i.e. to provide CLI help.
 */
using Provider_factory =
    std::function<std::shared_ptr<shcore::Cpp_object_bridge>(bool)>;

/**
 * Defines a CLI provider object which could:
 * - Return an object that exposes functions to CLI
 * - Hold nested Providers
 */
class Provider final {
 public:
  Provider() = default;
  explicit Provider(const Provider_factory &ff) : m_factory(ff) {}
  explicit Provider(
      const std::shared_ptr<shcore::Cpp_object_bridge> &target_object) {
    m_factory = [target_object](bool) { return target_object; };
  }

  std::shared_ptr<Provider> register_provider(const std::string &name,
                                              const Provider_factory &ff);

  std::shared_ptr<Provider> register_provider(
      const std::string &name,
      const std::shared_ptr<shcore::Cpp_object_bridge> &object);

  void remove_provider(const std::string &name);

  std::shared_ptr<Provider> get_provider(const std::string &name) {
    auto it = m_providers.find(name);
    if (it != m_providers.end()) {
      return it->second;
    }

    return {};
  }

  std::shared_ptr<shcore::Cpp_object_bridge> get_object(bool for_help) {
    if (m_factory) {
      return m_factory(for_help);
    }
    return {};
  }

  const std::map<std::string, std::shared_ptr<Provider>> &get_providers()
      const {
    return m_providers;
  }

 private:
  std::shared_ptr<Provider> register_provider(
      const std::string &name, std::shared_ptr<Provider> &&provider);

  Provider_factory m_factory;
  std::map<std::string, std::shared_ptr<Provider>> m_providers;
};
}  // namespace cli
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_SHELL_CLI_OPERATION_PROVIDER_H_
