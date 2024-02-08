/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/setup_account.h"

#include "modules/adminapi/common/accounts.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace mysqlsh::dba {

Setup_account::Setup_account(std::string name, std::string host,
                             Setup_account_options options,
                             std::vector<std::string> grants,
                             const mysqlshdk::mysql::IInstance &primary_server,
                             Cluster_type purpose) noexcept
    : m_name(std::move(name)),
      m_host(std::move(host)),
      m_options(std::move(options)),
      m_purpose(purpose),
      m_privilege_list(std::move(grants)),
      m_primary_server(primary_server) {}

void Setup_account::prompt_for_password() {
  auto console = mysqlsh::current_console();
  console->print_info();
  console->print_info(shcore::str_format(
      "Missing the password for new account %s@%s. Please provide one.",
      m_name.c_str(), m_host.c_str()));
  m_options.password = prompt_new_account_password();
  console->print_info();
}

void Setup_account::prepare() {
  const auto console = mysqlsh::current_console();
  if (m_options.dry_run) {
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
                                              current_host, m_purpose,
                                              &error_info)) {
    console->print_error(error_info);
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Account currently in use (%s) does not have enough privileges to "
        "execute the operation.",
        shcore::make_account(current_user, current_host).c_str()));
  }

  // Check if user exists on instance and throw error if it does and update
  // option was not set to True
  m_user_exists = m_primary_server.user_exists(m_name, m_host);
  if (m_user_exists && !m_options.update) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Could not proceed with the operation because account "
        "%s@%s already exists. Enable the 'update' option "
        "to update the existing account's privileges.",
        m_name.c_str(), m_host.c_str()));
  }

  // Also throw an error if user does not exist and update is enabled
  if (!m_user_exists && m_options.update) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Could not proceed with the operation because account "
        "%s@%s does not exist and the 'update' option is enabled",
        m_name.c_str(), m_host.c_str()));
  }

  // Prompt for the password in case it is necessary
  if (!m_user_exists && !m_options.password.has_value()) {
    if (current_shell_options()->get().wizards) {
      prompt_for_password();
    } else if (!m_options.require_cert_issuer.has_value() &&
               !m_options.require_cert_subject.has_value()) {
      // new user, password was not provided and not interactive mode, so we
      // must throw an error
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Could not proceed with the operation because neither password nor "
          "client certificate options were specified to create account %s@%s. "
          "Provide one using the 'password', '%s' and/or '%s' options.",
          m_name.c_str(), m_host.c_str(), kRequireCertIssuer,
          kRequireCertSubject));
    }
  }
}

shcore::Value Setup_account::execute() {
  // Determine the recovery user each of the online instances and reset its
  // password

  const auto console = mysqlsh::current_console();
  // Create the user account
  create_account();

  if (m_options.dry_run && !m_privilege_list.empty()) {
    console->print_info("The following grants would be executed: ");
  }

  // Give the grants
  for (const auto &grant : m_privilege_list) {
    if (m_options.dry_run) {
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
  if (m_options.dry_run) {
    console->print_info("dryRun finished.");
    console->print_info();
  }
  return shcore::Value();
}

void Setup_account::create_account() {
  // NOTE: Use 'CREATE IF NOT EXISTS' because DDL is replicated as SBR. This
  // makes it work even if the account doesn't exists on some cluster
  // instances but exists on others.
  const auto console = mysqlsh::current_console();
  const char *action = m_user_exists ? "Updating" : "Creating";
  console->print_info();
  console->print_info(shcore::str_format("%s user %s@%s.", action,
                                         m_name.c_str(), m_host.c_str()));

  std::string sql;

  // If the user already exists, just update the password otherwise create a new
  // account
  if (m_user_exists) {
    std::string what;
    if (m_options.password.has_value()) {
      what = "user password";
    }
    if (m_options.require_cert_issuer.has_value() ||
        m_options.require_cert_subject.has_value()) {
      if (!what.empty()) what += " and ";
      what += "client certificate options";
    }
    console->print_info("Updating " + what + ".");
    sql = "ALTER USER";
  } else {
    sql = "CREATE USER IF NOT EXISTS";
  }

  if (m_options.dry_run) return;

  sql += shcore::sqlformat(" ?@?", m_name, m_host);
  if (m_options.password.has_value()) {
    sql += shcore::sqlformat(" IDENTIFIED BY /*((*/ ? /*))*/",
                             m_options.password.value());
  }
  if (m_options.require_cert_issuer.has_value() ||
      m_options.require_cert_subject.has_value()) {
    if (!m_options.require_cert_issuer.value_or("").empty() ||
        !m_options.require_cert_subject.value_or("").empty()) {
      sql += " REQUIRE";
    } else {
      sql += " REQUIRE NONE";
    }
    if (!m_options.require_cert_issuer.value_or("").empty()) {
      sql += shcore::sqlformat(" ISSUER ?", *m_options.require_cert_issuer);
    }
    if (!m_options.require_cert_subject.value_or("").empty()) {
      sql += shcore::sqlformat(" SUBJECT ?", *m_options.require_cert_subject);
    }
  }
  if (m_options.password_expiration.has_value()) {
    if (*m_options.password_expiration < 0) {
      sql += " PASSWORD EXPIRE NEVER";
    } else {
      sql += shcore::sqlformat(" PASSWORD EXPIRE INTERVAL ? DAY",
                               *m_options.password_expiration);
    }
  }

  m_primary_server.execute(sql);
}

}  // namespace mysqlsh::dba
