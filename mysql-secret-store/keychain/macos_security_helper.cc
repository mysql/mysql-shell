/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#include "mysql-secret-store/keychain/macos_security_helper.h"

#include <string>
#include <string_view>
#include <utility>

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysql::secret_store::common::get_helper_exception;
using mysql::secret_store::common::Helper_exception;
using mysql::secret_store::common::Helper_exception_code;

namespace mysql {
namespace secret_store {
namespace keychain {

namespace {

constexpr std::string_view k_mysqlsh_scheme = "mysqlsh:/";
constexpr std::string_view k_password_type = "password";

void handle_find_exception(const Helper_exception &ex) {
  if (std::string{ex.what()}.find("SecKeychainSearchCopyNext") !=
      std::string::npos) {
    throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
  } else {
    throw ex;
  }
}

bool validate_string(const std::string &str) {
  return std::string::npos == str.find_first_of("\x00\x0A", 0, 2);
}

}  // namespace

Macos_security_helper::Macos_security_helper()
    : common::Helper("keychain", shcore::get_long_version(),
                     MYSH_HELPER_COPYRIGHT) {}

void Macos_security_helper::check_requirements() { m_invoker.validate(); }

void Macos_security_helper::store(const common::Secret &secret) {
  if (!validate_string(secret.id.id)) {
    throw get_helper_exception(Helper_exception_code::INVALID_SECRET_ID);
  }

  if (!validate_string(secret.secret)) {
    throw get_helper_exception(Helper_exception_code::INVALID_SECRET);
  }

  m_invoker.store(get_entry(secret.id), secret.secret);
}

void Macos_security_helper::get(const common::Secret_id &id,
                                std::string *secret) {
  try {
    *secret = m_invoker.get(get_entry(id));
  } catch (const Helper_exception &ex) {
    handle_find_exception(ex);
  }
}

void Macos_security_helper::erase(const common::Secret_id &id) {
  try {
    m_invoker.erase(get_entry(id));
  } catch (const Helper_exception &ex) {
    handle_find_exception(ex);
  }
}

void Macos_security_helper::list(std::vector<common::Secret_id> *secrets,
                                 std::optional<std::string> secret_type) {
  const std::string mysqlsh_scheme{k_mysqlsh_scheme};

  for (auto &entry : m_invoker.list()) {
    std::string id;

    if (k_password_type == entry.type) {
      if (!entry.account.empty() && !entry.service.empty()) {
        id = entry.account + "@" + entry.service;
      } else {
        id = entry.account + entry.service;
      }
    } else {
      if (entry.service != mysqlsh_scheme + entry.type) {
        throw Helper_exception{"Invalid entry detected"};
      }

      id = std::move(entry.account);
    }

    if (!secret_type.has_value() || *secret_type == entry.type) {
      secrets->emplace_back(
          common::Secret_id{std::move(entry.type), std::move(id)});
    }
  }
}

Security_invoker::Entry Macos_security_helper::get_entry(
    const common::Secret_id &id) {
  Security_invoker::Entry entry;
  entry.type = id.secret_type;

  if (k_password_type == id.secret_type) {
    if (const auto at_position = id.id.find("@");
        std::string::npos == at_position) {
      entry.account = "";
      entry.service = id.id;
    } else {
      entry.account = id.id.substr(0, at_position);
      entry.service = id.id.substr(at_position + 1);
    }
  } else {
    entry.account = id.id;
    entry.service.reserve(k_mysqlsh_scheme.length() + id.secret_type.length());
    entry.service = k_mysqlsh_scheme;
    entry.service += id.secret_type;
  }

  return entry;
}

}  // namespace keychain
}  // namespace secret_store
}  // namespace mysql
