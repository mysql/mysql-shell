/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/api_options.h"

#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/debug.h"

namespace mysqlsh::dba {

const shcore::Option_pack_def<Timeout_option> &Timeout_option::options() {
  static const auto opts = shcore::Option_pack_def<Timeout_option>().optional(
      kTimeout, &Timeout_option::set_timeout);

  return opts;
}

void Timeout_option::set_timeout(int value) {
  if (value < 0) {
    throw shcore::Exception::argument_error(
        shcore::str_format("%s option must be >= 0", kTimeout));
  }

  timeout = value;
}

const shcore::Option_pack_def<Interactive_option>
    &Interactive_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Interactive_option>().optional(
          kInteractive, &Interactive_option::set_interactive, "",
          shcore::Option_extract_mode::CASE_INSENSITIVE,
          shcore::Option_scope::DEPRECATED);
  return opts;
}

void Interactive_option::set_interactive(bool interactive) {
  handle_deprecated_option(kInteractive, "");

  DBUG_EXECUTE_IF("dba_deprecated_option_fail",
                  { throw std::logic_error("debug"); });

  m_interactive = interactive;
}

bool Interactive_option::interactive() const {
  return m_interactive.value_or(current_shell_options()->get().wizards);
}

const shcore::Option_pack_def<Password_interactive_options>
    &Password_interactive_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Password_interactive_options>()
          .optional(mysqlshdk::db::kPassword,
                    &Password_interactive_options::set_password, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::DEPRECATED)
          .optional(mysqlshdk::db::kDbPassword,
                    &Password_interactive_options::set_password, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::DEPRECATED)
          .include<Interactive_option>();

  return opts;
}

const shcore::Option_pack_def<Wait_recovery_option>
    &Wait_recovery_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Wait_recovery_option>()
          .optional(kRecoveryProgress, &Wait_recovery_option::set_wait_recovery)
          .optional(kWaitRecovery, &Wait_recovery_option::set_wait_recovery, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::DEPRECATED);

  return opts;
}

const shcore::Option_pack_def<Recovery_progress_option>
    &Recovery_progress_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Recovery_progress_option>().optional(
          kRecoveryProgress, &Recovery_progress_option::set_recovery_progress);

  return opts;
}

Recovery_progress_style Wait_recovery_option::get_wait_recovery() {
  if (!m_wait_recovery.has_value()) {
    m_wait_recovery = isatty(STDOUT_FILENO)
                          ? Recovery_progress_style::PROGRESSBAR
                          : Recovery_progress_style::TEXTUAL;
  }

  return *m_wait_recovery;
}

Recovery_progress_style Recovery_progress_option::get_recovery_progress() {
  if (!m_recovery_progress.has_value()) {
    m_recovery_progress = isatty(STDOUT_FILENO)
                              ? Recovery_progress_style::PROGRESSBAR
                              : Recovery_progress_style::TEXTUAL;
  }

  return *m_recovery_progress;
}

namespace {
void validate_wait_recovery(int value) {
  // Validate waitRecovery option UInteger [0, 3]
  if (value < 0 || value > 3) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [0, 3].",
                           value, kWaitRecovery));
  }
}

void validate_recovery_progress(int value) {
  // Validate recoveryProgress option UInteger [0, 3]
  if (value < 0 || value > 2) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [0, 2].",
                           value, kRecoveryProgress));
  }
}
}  // namespace

void Wait_recovery_option::set_wait_recovery(const std::string &option,
                                             int value) {
  if (option == kWaitRecovery) {
    handle_deprecated_option(kWaitRecovery, kRecoveryProgress,
                             m_recovery_progress.has_value(), false);

    DBUG_EXECUTE_IF("dba_deprecated_option_fail",
                    { throw std::logic_error("debug"); });
  }

  // Validate waitRecovery option UInteger [0, 3]
  if (option == kWaitRecovery) {
    validate_wait_recovery(value);
  } else {
    // Validate recoveryProgress option UInteger [0, 2]
    validate_recovery_progress(value);
  }

  if (option == kWaitRecovery) {
    switch (value) {
      case 0:
        m_wait_recovery = Recovery_progress_style::NOWAIT;
        break;
      case 1:
        m_wait_recovery = Recovery_progress_style::NOINFO;
        break;
      case 2:
        m_wait_recovery = Recovery_progress_style::TEXTUAL;
        break;
      case 3:
        m_wait_recovery = Recovery_progress_style::PROGRESSBAR;
        break;
    }
  } else {
    switch (value) {
      case 0:
        m_recovery_progress = Recovery_progress_style::NOINFO;
        break;
      case 1:
        m_recovery_progress = Recovery_progress_style::TEXTUAL;
        break;
      case 2:
        m_recovery_progress = Recovery_progress_style::PROGRESSBAR;
        break;
    }

    m_wait_recovery = m_recovery_progress;
  }
}

void Recovery_progress_option::set_recovery_progress(int value) {
  // Validate recoveryProgress option UInteger [0, 2]
  validate_recovery_progress(value);

  switch (value) {
    case 0:
      m_recovery_progress = Recovery_progress_style::NOINFO;
      break;
    case 1:
      m_recovery_progress = Recovery_progress_style::TEXTUAL;
      break;
    default:
      m_recovery_progress = Recovery_progress_style::PROGRESSBAR;
      break;
  }
}

void Password_interactive_options::set_password(const std::string &option,
                                                const std::string &value) {
  if (option == mysqlshdk::db::kDbPassword) {
    handle_deprecated_option(mysqlshdk::db::kDbPassword,
                             mysqlshdk::db::kPassword, password.has_value(),
                             true);
  }

  if (option == mysqlshdk::db::kPassword) {
    handle_deprecated_option(mysqlshdk::db::kPassword, "");

    DBUG_EXECUTE_IF("dba_deprecated_option_fail",
                    { throw std::logic_error("debug"); });
  }

  password = value;
}

const shcore::Option_pack_def<Force_interactive_options>
    &Force_interactive_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Force_interactive_options>()
          .optional(kForce, &Force_interactive_options::force)
          .include<Interactive_option>();

  return opts;
}

const shcore::Option_pack_def<List_routers_options>
    &List_routers_options::options() {
  static const auto opts =
      shcore::Option_pack_def<List_routers_options>().optional(
          kOnlyUpgradeRequired, &List_routers_options::only_upgrade_required);

  return opts;
}

const shcore::Option_pack_def<Setup_account_options>
    &Setup_account_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Setup_account_options>()
          .optional(kDryRun, &Setup_account_options::dry_run)
          .optional(kUpdate, &Setup_account_options::update)
          .optional(kRequireCertIssuer,
                    &Setup_account_options::require_cert_issuer)
          .optional(kRequireCertSubject,
                    &Setup_account_options::require_cert_subject)
          .optional(kPasswordExpiration,
                    &Setup_account_options::set_password_expiration)
          .include<Password_interactive_options>();

  return opts;
}

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_PASSWORD_EXPIRATION,
    "@li passwordExpiration: Password expiration setting for the account. "
    "May be set to the number of days for expiration, 'NEVER' to disable "
    "expiration and 'DEFAULT' to use the system default.");

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_REQUIRE_CERT_ISSUER,
    "@li requireCertIssuer: Optional SSL certificate issuer for the account.");

REGISTER_HELP(OPT_SETUP_ACCOUNT_OPTIONS_REQUIRE_CERT_SUBJECT,
              "@li requireCertSubject: Optional SSL certificate subject for "
              "the account.");

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_DRY_RUN,
    "@li dryRun: boolean value used to enable a dry run of the account setup "
    "process. Default value is False.");

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_DRY_RUN_DETAIL,
    "If dryRun is used, the function will display information about the "
    "permissions to be granted to `user` account without actually creating "
    "and/or performing any changes to it.");

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_UPDATE,
    "@li update: boolean value that must be enabled to allow updating the "
    "privileges and/or password of existing accounts. Default value is False.");

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_UPDATE_DETAIL,
    "To change authentication options for an existing account, set `update` to "
    "`true`. It is possible to change password without affecting "
    "certificate options or vice-versa but certificate options can only be "
    "changed together.");

REGISTER_HELP(
    OPT_SETUP_ACCOUNT_OPTIONS_INTERACTIVE_DETAIL,
    "The interactive option can be used to explicitly enable or disable the "
    "interactive prompts that help the user through the account setup "
    "process.");

void Setup_account_options::set_password_expiration(
    const shcore::Value &value) {
  switch (value.get_type()) {
    case shcore::Null:
      password_expiration.reset();
      break;

    case shcore::String: {
      const auto &s = value.get_string();
      if (shcore::str_caseeq(s, "NEVER")) {
        password_expiration = -1;
        break;
      } else if (shcore::str_caseeq(s, "DEFAULT") || s.empty()) {
        password_expiration.reset();
        break;
      }
      [[fallthrough]];
    }

    case shcore::Integer:
      [[fallthrough]];
    case shcore::UInteger:
      try {
        password_expiration = value.as_uint();
        if (*password_expiration > 0) break;
      } catch (...) {
      }
      throw shcore::Exception::value_error(
          std::string("Option '") + kPasswordExpiration +
          "' UInteger, 'NEVER' or 'DEFAULT' expected, but value is '" +
          value.as_string() + "'");
      break;

    default:
      throw shcore::Exception::type_error(
          std::string("Option '") + kPasswordExpiration +
          "' UInteger, 'NEVER' or 'DEFAULT' expected, but value is " +
          type_name(value.get_type()));
  }
}

}  // namespace mysqlsh::dba
