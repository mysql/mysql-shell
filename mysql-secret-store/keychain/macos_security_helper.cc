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

#include "mysql-secret-store/keychain/macos_security_helper.h"

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysql::secret_store::common::Helper_exception;
using mysql::secret_store::common::Helper_exception_code;
using mysql::secret_store::common::get_helper_exception;

namespace mysql {
namespace secret_store {
namespace keychain {

namespace {

void handle_find_exception(const Helper_exception &ex) {
  if (std::string{ex.what()}.find("SecKeychainSearchCopyNext") !=
      std::string::npos) {
    throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
  } else {
    throw ex;
  }
}

}  // namespace

Macos_security_helper::Macos_security_helper()
    : common::Helper("keychain", shcore::get_long_version(),
                     MYSH_HELPER_COPYRIGHT) {}

void Macos_security_helper::check_requirements() { m_invoker.validate(); }

void Macos_security_helper::store(const common::Secret &secret) {
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

void Macos_security_helper::list(std::vector<common::Secret_id> *secrets) {
  for (const auto &entry : m_invoker.list()) {
    std::string url;
    if (!entry.account.empty() && !entry.service.empty()) {
      url = entry.account + "@" + entry.service;
    } else {
      url = entry.account + entry.service;
    }

    secrets->emplace_back(common::Secret_id{entry.type, url});
  }
}

Security_invoker::Entry Macos_security_helper::get_entry(
    const common::Secret_id &id) {
  Security_invoker::Entry entry;
  entry.type = id.secret_type;

  const auto at_position = id.url.find("@");

  if (std::string::npos == at_position) {
    entry.account = "";
    entry.service = id.url;
  } else {
    entry.account = id.url.substr(0, at_position);
    entry.service = id.url.substr(at_position + 1);
  }

  return entry;
}

}  // namespace keychain
}  // namespace secret_store
}  // namespace mysql
