/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/setup_account.h"

#include "modules/adminapi/common/accounts.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace mysqlsh {
namespace dba {

Setup_account::Setup_account(
    const std::string &name, const std::string &host, bool interactive,
    bool update, bool dry_run,
    const mysqlshdk::utils::nullable<std::string> &password,
    std::vector<std::string> grants,
    const mysqlshdk::mysql::IInstance &primary_server)
    : m_interactive(interactive),
      m_update(update),
      m_dry_run(dry_run),
      m_password(password),
      m_name(name),
      m_host(host),
      m_privilege_list(std::move(grants)),
      m_primary_server(primary_server) {}

void Setup_account::prompt_for_password() {
  auto console = mysqlsh::current_console();
  console->print_info();
  console->print_info(shcore::str_format(
      "Missing the password for new account %s@%s. Please provide one.",
      m_name.c_str(), m_host.c_str()));
  m_password = prompt_new_account_password();
  console->print_info();
}

void Setup_account::prepare() {
  const auto console = mysqlsh::current_console();
  if (m_dry_run) {
    console->print_note(
        "dryRun option was specified. Validations will be executed, "
        "but no changes will be applied.");
  }

  // Validate the primary session user has Innodb admin privileges to run the
  // operation

  std::string current_user, current_host;
  m_primary_server.get_current_user(&current_user, &current_host);

  std::string error_info;
  if (!validate_cluster_admin_user_privileges(m_primary_server, current_user,
                                              current_host, &error_info)) {
    console->print_error(error_info);
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Account currently in use (%s) does not have enough privileges to "
        "execute the operation.",
        shcore::make_account(current_user, current_host).c_str()));
  }

  // Check if user exists on instance and throw error if it does and update
  // option was not set to True
  m_user_exists = m_primary_server.user_exists(m_name, m_host);
  if (m_user_exists && !m_update) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Could not proceed with the operation because account "
        "%s@%s already exists. Enable the 'update' option "
        "to update the existing account's privileges.",
        m_name.c_str(), m_host.c_str()));
  }
  // Also throw an error if user does not exist and update is enabled
  if (!m_user_exists && m_update) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Could not proceed with the operation because account "
        "%s@%s does not exist and the 'update' option is enabled",
        m_name.c_str(), m_host.c_str()));
  }

  // Prompt for the password in case it is necessary
  if (!m_user_exists && m_password.is_null()) {
    if (m_interactive) {
      prompt_for_password();
    } else {
      // new user, password was not provided and not interactive mode, so we
      // must throw an error
      throw shcore::Exception::runtime_error(
          shcore::str_format("Could not proceed with the operation because no "
                             "password was specified to create account "
                             "%s@%s. Provide one using the 'password' option.",
                             m_name.c_str(), m_host.c_str()));
    }
  }
}

shcore::Value Setup_account::execute() {
  // Determine the recovery user each of the online instances and reset its
  // password

  const auto console = mysqlsh::current_console();
  // Create the user account
  create_account();

  if (m_dry_run && !m_privilege_list.empty()) {
    console->print_info("The following grants would be executed: ");
  }

  // Give the grants
  for (const auto &grant : m_privilege_list) {
    if (m_dry_run) {
      console->print_info(grant);
    } else {
      m_primary_server.execute(grant);
    }
  }

  // Account created/updated successfully
  const std::string action = m_user_exists ? "updated" : "created";
  console->print_info(shcore::str_format("Account %s@%s was successfully %s.",
                                         m_name.c_str(), m_host.c_str(),
                                         action.c_str()));
  console->print_info();
  if (m_dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
  return shcore::Value();
}

void Setup_account::rollback() {
  // Do nothing.
}

void Setup_account::finish() {
  // Do nothing
}
void Setup_account::create_account() {
  // NOTE: Use 'CREATE IF NOT EXISTS' because DDL is replicated as SBR. This
  // makes it work even if the account doesn't exists on some cluster
  // instances but exists on others.
  const auto console = mysqlsh::current_console();
  const std::string action = m_user_exists ? "Updating" : "Creating";
  console->print_info(shcore::str_format("%s user %s@%s.", action.c_str(),
                                         m_name.c_str(), m_host.c_str()));
  if (!m_dry_run) {
    m_primary_server.executef("CREATE USER IF NOT EXISTS ?@?", m_name, m_host);
  }

  // Set/change the account password
  if (!m_password.is_null()) {
    if (m_user_exists) {
      console->print_info("Updating user password.");
    } else {
      console->print_info("Setting user password.");
    }
    if (!m_dry_run) {
      m_primary_server.executef("ALTER USER ?@? IDENTIFIED BY /*((*/ ? /*))*/",
                                m_name, m_host, m_password.get_safe());
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
