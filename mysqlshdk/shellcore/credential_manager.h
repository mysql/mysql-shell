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

#ifndef MYSQLSHDK_SHELLCORE_CREDENTIAL_MANAGER_H_
#define MYSQLSHDK_SHELLCORE_CREDENTIAL_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/shell_notifications.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/options.h"

namespace mysql {
namespace secret_store {
namespace api {

class Helper_interface;

}  // namespace api
}  // namespace secret_store
}  // namespace mysql

namespace shcore {

class Credential_manager : public NotificationObserver {
 public:
  enum class Save_passwords { ALWAYS, PROMPT, NEVER };

  Credential_manager(const Credential_manager &) = delete;
  Credential_manager(Credential_manager &&) = delete;
  Credential_manager &operator=(const Credential_manager &) = delete;
  Credential_manager &operator=(Credential_manager &&) = delete;

  static Credential_manager &get();

  void register_options(Options *options);

  void initialize();

  void handle_notification(const std::string &name,
                           const Object_bridge_ref &sender,
                           Value::Map_type_ref data) override;

  void set_helper(const std::string &helper);

  bool get_password(mysqlshdk::db::Connection_options *options) const;

  bool save_password(const mysqlshdk::db::Connection_options &options);

  bool remove_password(const mysqlshdk::db::Connection_options &options);

  // shell API

  std::vector<std::string> list_credential_helpers() const;

  void store_credential(const std::string &url, const std::string &credential);

  void delete_credential(const std::string &url);

  void delete_all_credentials();

  std::vector<std::string> list_credentials() const;

 private:
  Credential_manager();
  ~Credential_manager() = default;

  bool should_save_password(const std::string &url);

  void add_ignore_filter(const std::string &filter);

  bool is_ignored_url(const std::string &url) const;

  std::unique_ptr<::mysql::secret_store::api::Helper_interface> m_helper;
  std::string m_helper_string;
  Save_passwords m_save_passwords = Save_passwords::PROMPT;
  std::vector<std::string> m_ignore_filters;
  bool m_is_initialized = false;
};

}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_CREDENTIAL_MANAGER_H_
