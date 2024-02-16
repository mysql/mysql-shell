/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/api_options.h"

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/utils_connection.h"

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

const shcore::Option_pack_def<Recovery_progress_option>
    &Recovery_progress_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Recovery_progress_option>().optional(
          kRecoveryProgress, &Recovery_progress_option::set_recovery_progress);

  return opts;
}

Recovery_progress_style Recovery_progress_option::get_recovery_progress() {
  if (!m_recovery_progress.has_value()) {
    m_recovery_progress = isatty(STDOUT_FILENO)
                              ? Recovery_progress_style::PROGRESSBAR
                              : Recovery_progress_style::TEXTUAL;
  }

  return *m_recovery_progress;
}

void Recovery_progress_option::set_recovery_progress(int value) {
  // Validate recoveryProgress option UInteger [0, 2]
  if (value < 0 || value > 2) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [0, 2].",
                           value, kRecoveryProgress));
  }

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

const shcore::Option_pack_def<Force_options> &Force_options::options() {
  static const auto opts = shcore::Option_pack_def<Force_options>().optional(
      kForce, &Force_options::force);

  return opts;
}

const shcore::Option_pack_def<List_routers_options>
    &List_routers_options::options() {
  static const auto opts =
      shcore::Option_pack_def<List_routers_options>().optional(
          kOnlyUpgradeRequired, &List_routers_options::only_upgrade_required);

  return opts;
}

void Router_options_options::set_extended(uint64_t value) {
  // Validate extended option UInteger [0, 2] or Boolean.
  if (value > 2) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%" PRIu64
                           "' for option '%s'. It must be an integer in the "
                           "range [0, 2].",
                           value, kExtended));
  }

  extended = value;
}

const shcore::Option_pack_def<Router_options_options>
    &Router_options_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Router_options_options>()
          .optional(kRouter, &Router_options_options::router)
          .optional(kExtended, &Router_options_options::set_extended);

  return opts;
}

const shcore::Option_pack_def<Setup_account_options>
    &Setup_account_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Setup_account_options>()
          .optional(mysqlshdk::db::kPassword, &Setup_account_options::password)
          .optional(kDryRun, &Setup_account_options::dry_run)
          .optional(kUpdate, &Setup_account_options::update)
          .optional(kRequireCertIssuer,
                    &Setup_account_options::require_cert_issuer)
          .optional(kRequireCertSubject,
                    &Setup_account_options::require_cert_subject)
          .optional(kPasswordExpiration,
                    &Setup_account_options::set_password_expiration);

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
