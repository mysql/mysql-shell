/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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
#include "modules/adminapi/common/common_cmd_options.h"
#include "modules/adminapi/common/execute.h"
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
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Recovery_progress_option> b;

    b.optional_as<int>(
        kOptionRecoveryProgress, &Recovery_progress_option::m_recovery_progress,
        [](const auto &value) -> decltype(kOptionRecoveryProgress)::Type {
          if (value < 0 || value > 2) {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value '%d' for option '%.*s'. It must be an integer "
                "in the range [0, 2].",
                value, static_cast<int>(kOptionRecoveryProgress.name.length()),
                kOptionRecoveryProgress.name.data()));
          }

          switch (value) {
            case 0:
              return Recovery_progress_style::MINIMAL;
            case 1:
              return Recovery_progress_style::TEXTUAL;
            default:
              return Recovery_progress_style::PROGRESS_BAR;
          }

          return std::nullopt;
        });

    return b.build();
  });

  return opts;
}

Recovery_progress_style Recovery_progress_option::get_recovery_progress()
    const {
  if (!m_recovery_progress.has_value()) {
    m_recovery_progress = isatty(STDOUT_FILENO)
                              ? Recovery_progress_style::PROGRESS_BAR
                              : Recovery_progress_style::TEXTUAL;
  }

  return *m_recovery_progress;
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

const shcore::Option_pack_def<Execute_options> &Execute_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Execute_options>()
          .optional(kExecuteExclude, &Execute_options::set_exclude)
          .optional(kDryRun, &Execute_options::dry_run)
          .include<Timeout_option>();

  return opts;
}

void Execute_options::set_exclude(const shcore::Value &value) {
  exclude = Execute::convert_to_instances_def(value, true);
}

const shcore::Option_pack_def<Create_routing_guideline_options>
    &Create_routing_guideline_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_routing_guideline_options>()
          .include<Force_options>();

  return opts;
}

const shcore::Option_pack_def<Import_routing_guideline_options>
    &Import_routing_guideline_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Import_routing_guideline_options> b;

    b.include<Force_options>();

    b.optional_as<std::string>(
        kOptionRename, [](Import_routing_guideline_options &options,
                          std::string_view, const std::string &value) {
          if (options.force.has_value()) {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Options '%s' and '%s' are mutually exclusive.", kForce,
                std::string(kOptionRename.name).c_str()));
          }

          if (value.empty()) {
            throw shcore::Exception::argument_error(
                shcore::str_format("Option '%s' cannot be empty.",
                                   std::string(kOptionRename.name).c_str()));
          }

          options.rename = value;
        });

    return b.build();
  });

  return opts;
}

}  // namespace mysqlsh::dba
