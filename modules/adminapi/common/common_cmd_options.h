/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_COMMON_CMD_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_COMMON_CMD_OPTIONS_H_

#include <optional>
#include <string>
#include <string_view>

#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/types/option_pack.h"

namespace mysqlsh::dba {

/*
 * This listing should be of the more common used, and more specifically shared,
 * command parameters. If a parameter is only used on a specific command, it
 * should be declared there, in an anonymous namespace and not "bleed" onto
 * others.
 *
 * Also, take into consideration that the Option declaration doesn't automaticly
 * connect it to a field in a type. That is made using
 * shcore::Option_pack_builder.
 *
 * In other words, the workflow should be:
 *  - declare option with type, name, scope and/or extraction mode
 *  - declare a field in a type (or multiple types) that groups all the options
 * of a command
 *     - NOTE: the field type must coincide with the type of the option
 *  - using a shcore::Option_pack_builder, connect the Option to a field.
 *
 * This way we have a clear distinction between the option information and its
 * storage.
 */

inline constexpr shcore::Option_data<std::optional<bool>> kOptionForce{"force"};

inline constexpr shcore::Option_data<bool> kOptionDryRun{"dryRun"};

inline constexpr shcore::Option_data<uint64_t> kOptionExtended{"extended"};

inline constexpr shcore::Option_data<int> kOptionTimeout{
    "timeout", [](std::string_view name, int &value) {
      if (value >= 0) return;
      throw shcore::Exception::argument_error(
          shcore::str_format("%.*s option must be >= 0",
                             static_cast<int>(name.length()), name.data()));
    }};

inline constexpr shcore::Option_data<std::optional<Recovery_progress_style>>
    kOptionRecoveryProgress{"recoveryProgress"};

inline constexpr shcore::Option_data<std::optional<Member_recovery_method>>
    kOptionRecoveryMethod{"recoveryMethod"};

inline constexpr shcore::Option_data<std::optional<std::string>>
    kOptionPassword{"password"};

inline constexpr shcore::Option_data<std::optional<std::string>>
    kOptionCommunicationStack{
        "communicationStack",
        [](std::string_view name, std::optional<std::string> &value) {
          if (value.has_value())
            value = shcore::str_upper(shcore::str_strip(*value));

          if (!value.has_value() || value->empty()) {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value for '%.*s' option. String value cannot be "
                "empty.",
                static_cast<int>(name.length()), name.data()));
          }

          if (std::find(kCommunicationStackValidValues.begin(),
                        kCommunicationStackValidValues.end(),
                        *value) != kCommunicationStackValidValues.end())
            return;

          auto valid_values =
              shcore::str_join(kCommunicationStackValidValues, ", ");
          throw shcore::Exception::argument_error(shcore::str_format(
              "Invalid value for '%.*s' option. Supported values: %s.",
              static_cast<int>(name.length()), name.data(),
              valid_values.c_str()));
        }};

inline constexpr shcore::Option_data<Cluster_ssl_mode> kOptionMemberSslMode{
    "memberSslMode"};

inline constexpr shcore::Option_data<Replication_auth_type>
    kOptionMemberAuthType{"memberAuthType"};

inline constexpr shcore::Option_data<std::string> kOptionCertIssuer{
    "certIssuer", [](std::string_view name, std::string &value) {
      if (!value.empty()) return;
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%.*s' option. Value cannot be an "
          "empty string.",
          static_cast<int>(name.length()), name.data()));
    }};
inline constexpr shcore::Option_data<std::string> kOptionCertSubject{
    "certSubject", [](std::string_view name, std::string &value) {
      if (!value.empty()) return;
      throw shcore::Exception::argument_error(shcore::str_format(
          "Invalid value for '%.*s' option. Value cannot be an "
          "empty string.",
          static_cast<int>(name.length()), name.data()));
    }};

inline constexpr shcore::Option_data<std::optional<std::string>> kOptionLabel{
    "label"};

inline constexpr shcore::Option_data<std::optional<std::string>>
    kOptionIpAllowlist{
        "ipAllowlist",
        [](std::string_view name, std::optional<std::string> &value) {
          if (!value.has_value()) return;
          if (!shcore::str_strip_view(*value).empty()) return;

          throw shcore::Exception::argument_error(shcore::str_format(
              "Invalid value for %.*s: string value cannot be empty.",
              static_cast<int>(name.length()), name.data()));
        }};

inline constexpr shcore::Option_data<std::optional<std::string>>
    kOptionLocalAddress{
        "localAddress",
        [](std::string_view name, std::optional<std::string> &value) {
          if (value.has_value()) value = shcore::str_strip(*value);

          if (!value.has_value() || value->empty()) {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value for %.*s, string value cannot be empty.",
                static_cast<int>(name.length()), name.data()));
          }

          if (value.value() == ":") {
            throw shcore::Exception::argument_error(shcore::str_format(
                "Invalid value for %.*s. If ':' is specified then at "
                "least a non-empty host or port must be specified: "
                "'<host>:<port>' or '<host>:' or ':<port>'.",
                static_cast<int>(name.length()), name.data()));
          }
        }};

inline constexpr shcore::Option_data<bool> kOptionGtidSetIsComplete{
    "gtidSetIsComplete"};

inline constexpr shcore::Option_data<std::string> kOptionReplicationAllowedHost{
    "replicationAllowedHost"};

inline constexpr shcore::Option_data<std::string> kOptionRename{"rename"};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_COMMON_CMD_OPTIONS_H_
