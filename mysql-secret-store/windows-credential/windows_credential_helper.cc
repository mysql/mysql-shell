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

#include "mysql-secret-store/windows-credential/windows_credential_helper.h"

#include <WinCred.h>

#include <memory>
#include <utility>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysql::secret_store::common::Helper_exception;
using mysql::secret_store::common::Helper_exception_code;
using mysql::secret_store::common::get_helper_exception;

namespace mysql {
namespace secret_store {
namespace windows_credential {

namespace {

constexpr auto k_company_name = "Oracle";
constexpr auto k_name_separator = "|";

constexpr DWORD k_credential_type = CRED_TYPE_GENERIC;  // generic credential

}  // namespace

Windows_credential_helper::Windows_credential_helper()
    : common::Helper("windows-credential", shcore::get_long_version(),
                     MYSH_HELPER_COPYRIGHT),
      m_persist_mode{CRED_PERSIST_LOCAL_MACHINE} {}

void Windows_credential_helper::check_requirements() {
  DWORD maximum_persist[CRED_TYPE_MAXIMUM];

  if (!CredGetSessionTypes(CRED_TYPE_MAXIMUM, maximum_persist)) {
    throw Helper_exception{"Failed to check persistent mode: " +
                           shcore::get_last_error()};
  }

  if (maximum_persist[k_credential_type] < CRED_PERSIST_LOCAL_MACHINE) {
    throw Helper_exception{"Credentials cannot be persisted"};
  }

  m_persist_mode = maximum_persist[k_credential_type];
}

void Windows_credential_helper::store(const common::Secret &secret) {
  auto url_keyword = get_url_keyword();
  auto secret_type_keyword = get_secret_type_keyword();

  CREDENTIAL_ATTRIBUTE attributes[2];

  attributes[0].Keyword = (LPSTR)url_keyword.c_str();
  attributes[0].Flags = 0;
  attributes[0].ValueSize = secret.id.url.length();
  attributes[0].Value = (LPBYTE)secret.id.url.c_str();

  attributes[1].Keyword = (LPSTR)secret_type_keyword.c_str();
  attributes[1].Flags = 0;
  attributes[1].ValueSize = secret.id.secret_type.length();
  attributes[1].Value = (LPBYTE)secret.id.secret_type.c_str();

  auto name = get_name(secret.id);
  CREDENTIAL cred;

  cred.Flags = 0;  // no flags
  cred.Type = k_credential_type;
  cred.TargetName = (LPSTR)name.c_str();  // unique name
  cred.Comment = nullptr;                 // no comments!
  cred.LastWritten = {0, 0};              // ignored for write operations
  cred.CredentialBlobSize = secret.secret.length();
  cred.CredentialBlob = (LPBYTE)secret.secret.c_str();
  cred.Persist = m_persist_mode;
  cred.AttributeCount = shcore::array_size(attributes);
  cred.Attributes = attributes;
  cred.TargetAlias = nullptr;  // ignored in case of CRED_TYPE_GENERIC
  cred.UserName = nullptr;     // ignored in case of CRED_TYPE_GENERIC

  if (!CredWrite(&cred, 0)) {
    throw Helper_exception{"Failed to store the secret: " +
                           shcore::get_last_error()};
  }
}

void Windows_credential_helper::get(const common::Secret_id &id,
                                    std::string *secret) {
  auto name = get_name(id);
  PCREDENTIAL cred = nullptr;

  if (!CredRead(name.c_str(), k_credential_type, 0, &cred)) {
    if (GetLastError() == ERROR_NOT_FOUND) {
      throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
    } else {
      throw Helper_exception{"Failed to get the secret: " +
                             shcore::get_last_error()};
    }
  } else {
    const std::unique_ptr<CREDENTIAL, decltype(&CredFree)> deleter{cred,
                                                                   CredFree};
    *secret = std::string(reinterpret_cast<char *>(cred->CredentialBlob),
                          cred->CredentialBlobSize);
  }
}

void Windows_credential_helper::erase(const common::Secret_id &id) {
  auto name = get_name(id);

  if (!CredDelete(name.c_str(), k_credential_type, 0)) {
    if (GetLastError() == ERROR_NOT_FOUND) {
      throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
    } else {
      throw Helper_exception{"Failed to erase the secret: " +
                             shcore::get_last_error()};
    }
  }
}

void Windows_credential_helper::list(std::vector<common::Secret_id> *secrets) {
  PCREDENTIAL *credentials = nullptr;
  DWORD count = 0;
  auto filter = get_search_pattern();

  if (!CredEnumerate(filter.c_str(), 0, &count, &credentials)) {
    if (GetLastError() == ERROR_NOT_FOUND) {
      // empty list, not an error
      return;
    } else {
      throw Helper_exception{"Failed to list the secrets: " +
                             shcore::get_last_error()};
    }
  } else {
    const std::unique_ptr<PCREDENTIAL, decltype(&CredFree)> deleter{credentials,
                                                                    CredFree};
    const auto url_keyword = get_url_keyword();
    const auto secret_type_keyword = get_secret_type_keyword();

    for (DWORD i = 0; i < count; ++i) {
      std::string url;
      std::string secret_type;

      for (DWORD j = 0; j < credentials[i]->AttributeCount; ++j) {
        const auto &attribute = credentials[i]->Attributes[j];
        const auto keyword = std::string{attribute.Keyword};
        auto value = std::string(reinterpret_cast<char *>(attribute.Value),
                                 attribute.ValueSize);

        if (keyword == url_keyword) {
          url = std::move(value);
        } else if (keyword == secret_type_keyword) {
          secret_type = std::move(value);
        }
      }

      if (url.empty() || secret_type.empty()) {
        throw Helper_exception{"Invalid entry detected"};
      }

      secrets->emplace_back(common::Secret_id{secret_type, url});
    }
  }
}

std::string Windows_credential_helper::get_name_prefix() const {
  return shcore::str_join(std::vector<std::string>{k_company_name, name()},
                          k_name_separator);
}

std::string Windows_credential_helper::get_name(
    const common::Secret_id &id) const {
  return shcore::str_join(
      std::vector<std::string>{get_name_prefix(), id.secret_type, id.url},
      k_name_separator);
}

std::string Windows_credential_helper::get_search_pattern() const {
  return shcore::str_join(std::vector<std::string>{get_name_prefix(), "*"},
                          k_name_separator);
}

std::string Windows_credential_helper::get_keyword_prefix() const {
  return shcore::str_join(
      std::vector<std::string>{k_company_name, name() + "-"}, "_");
}

std::string Windows_credential_helper::get_url_keyword() const {
  return get_keyword_prefix() + "url";
}

std::string Windows_credential_helper::get_secret_type_keyword() const {
  return get_keyword_prefix() + "secret-type";
}

}  // namespace windows_credential
}  // namespace secret_store
}  // namespace mysql
