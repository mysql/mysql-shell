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

#include "mysql-secret-store/secret-service/secret_service_helper.h"

#include <utility>

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysql::secret_store::common::Helper_exception;
using mysql::secret_store::common::Helper_exception_code;
using mysql::secret_store::common::get_helper_exception;

namespace mysql {
namespace secret_store {
namespace secret_service {

namespace {

#define SECRET_TYPE_KEY "secret_type"
#define SECRET_URL_KEY "secret_url"

#define MAKE_ATTRIBUTE(key) "attribute." key

constexpr auto k_secret_store_key = "secret_store";
constexpr auto k_secret_type_key = SECRET_TYPE_KEY;
constexpr auto k_secret_url_key = SECRET_URL_KEY;

constexpr auto k_secret_type_attribute = MAKE_ATTRIBUTE(SECRET_TYPE_KEY);
constexpr auto k_secret_url_attribute = MAKE_ATTRIBUTE(SECRET_URL_KEY);

#undef MAKE_ATTRIBUTE
#undef SECRET_TYPE_KEY
#undef SECRET_URL_KEY

void parse_list(const std::string &list,
                std::vector<common::Secret_id> *secrets) {
  bool needs_type = false;
  bool needs_url = false;

  auto verify_secret_id = [&needs_type, &needs_url]() {
    if (needs_type || needs_url) {
      throw Helper_exception{"Failed to parse list of secrets"};
    }
  };

  if (!list.empty()) {
    for (const auto &line : shcore::str_split(list, "\r\n", -1, true)) {
      if (line[0] == '[') {
        verify_secret_id();
        secrets->emplace_back();
        needs_type = needs_url = true;
      } else {
        auto option = shcore::str_split(line, "=", -1, true);
        auto key = shcore::str_strip(option[0]);
        auto value = shcore::str_strip(option[1]);

        if (k_secret_type_attribute == key) {
          secrets->back().secret_type = value;
          needs_type = false;
        } else if (k_secret_url_attribute == key) {
          secrets->back().url = value;
          needs_url = false;
        }
      }
    }
  }

  verify_secret_id();
}

}  // namespace

Secret_service_helper::Secret_service_helper()
    : common::Helper("secret-service", shcore::get_long_version(),
                     MYSH_HELPER_COPYRIGHT) {}

void Secret_service_helper::check_requirements() {
  // secret-tool does not work in a headless system (it might call dbus-launch
  // which in turn initializes X11); the only safe way to check if secret-tool
  // works is to call actual command
  list();
}

void Secret_service_helper::store(const common::Secret &secret) {
  m_invoker.store(get_label(secret.id), get_attributes(secret.id),
                  secret.secret);
}

void Secret_service_helper::get(const common::Secret_id &id,
                                std::string *secret) {
  *secret = get(id);
}

void Secret_service_helper::erase(const common::Secret_id &id) {
  const auto attributes = get_attributes(id);
  // erase does not return an error if requested secret does not exist, call
  // get() to check that
  get(attributes);
  m_invoker.erase(attributes);
}

void Secret_service_helper::list(std::vector<common::Secret_id> *secrets) {
  parse_list(list(), secrets);
}

Secret_tool_invoker::Attributes Secret_service_helper::get_attributes(
    const common::Secret_id &id) {
  Secret_tool_invoker::Attributes attributes;

  attributes.emplace(std::make_pair(k_secret_store_key, name()));
  attributes.emplace(std::make_pair(k_secret_type_key, id.secret_type));
  attributes.emplace(std::make_pair(k_secret_url_key, id.url));

  return attributes;
}

std::string Secret_service_helper::get_label(const common::Secret_id &id) {
  return "Secret for " + id.url + " (" + id.secret_type + ")";
}

std::string Secret_service_helper::get(
    const Secret_tool_invoker::Attributes &attributes) {
  try {
    return m_invoker.get(attributes);
  } catch (const Helper_exception &ex) {
    if (ex.what() ==
        std::string{"Process \"secret-tool\" finished with exit code 1: "}) {
      throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
    } else {
      throw;
    }
  }
}

std::string Secret_service_helper::get(const common::Secret_id &id) {
  return get(get_attributes(id));
}

std::string Secret_service_helper::list() {
  return m_invoker.list({{k_secret_store_key, name()}});
}

}  // namespace secret_service
}  // namespace secret_store
}  // namespace mysql
