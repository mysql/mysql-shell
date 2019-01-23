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

#include "modules/adminapi/common/group_replication_options.h"

#include <set>
#include <string>
#include <vector>
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {

/**
 * Validate the value specified for the groupName option.
 *
 * @param options Map type value with containing the specified options.
 * @throw ArgumentError if the value is empty.
 */
void validate_group_name_option(std::string group_name) {
  // Minimal validation is performed here, the rest is already currently
  // handled at the mysqlprovision level (including the logic to automatically
  // set the group name when not specified)
  group_name = shcore::str_strip(group_name);
  if (group_name.empty())
    throw shcore::Exception::argument_error(
        "Invalid value for groupName, string value cannot be empty.");
}

const char *kMemberSSLModeAuto = "AUTO";
const char *kMemberSSLModeRequired = "REQUIRED";
const char *kMemberSSLModeDisabled = "DISABLED";
const std::set<std::string> kMemberSSLModeValues = {
    kMemberSSLModeAuto, kMemberSSLModeDisabled, kMemberSSLModeRequired};

void validate_ssl_instance_options(std::string ssl_mode) {
  // Validate use of SSL options for the cluster instance and issue an
  // exception if invalid.
  ssl_mode = shcore::str_upper(ssl_mode);
  if (kMemberSSLModeValues.count(ssl_mode) == 0) {
    std::string valid_values = shcore::str_join(kMemberSSLModeValues, ",");
    throw shcore::Exception::argument_error(
        "Invalid value for memberSslMode option. "
        "Supported values: " +
        valid_values + ".");
  }
}

/**
 * Validate the value specified for the localAddress option.
 *
 * @param options Map type value with containing the specified options.
 * @throw ArgumentError if the value is empty or no host and port is specified
 *        (i.e., value is ":").
 */
void validate_local_address_option(std::string local_address) {
  // Minimal validation is performed here, the rest is already currently
  // handled at the mysqlprovision level (including the logic to automatically
  // set the host and port when not specified).

  local_address = shcore::str_strip(local_address);
  if (local_address.empty())
    throw shcore::Exception::argument_error(
        "Invalid value for localAddress, string value cannot be empty.");
  if (local_address.compare(":") == 0)
    throw shcore::Exception::argument_error(
        "Invalid value for localAddress. If ':' is specified then at least a "
        "non-empty host or port must be specified: '<host>:<port>' or "
        "'<host>:' or ':<port>'.");
}

/**
 * Validate the value specified for the exitStateAction option is supported on
 * the target instance
 *
 * @param session object which represents the session to the instance
 * @throw RuntimeError if the value is not supported on the target instance
 */
void validate_exit_state_action_supported(
    const mysqlshdk::utils::Version &version, std::string exit_state_action) {
  exit_state_action = shcore::str_strip(exit_state_action);

  if (exit_state_action.empty())
    throw shcore::Exception::argument_error(
        "Invalid value for exitStateAction, string value cannot be empty.");

  if (!is_group_replication_option_supported(version, kExitStateAction)) {
    throw shcore::Exception::runtime_error(
        "Option 'exitStateAction' not supported on target server "
        "version: '" +
        version.get_full() + "'");
  }
}

/**
 * Validate if the consistency option is supported the target instance
 * version. The actual value is validated by the GR plugin.
 *
 * @param version version of the target instance
 * @throw RuntimeError if the value is not supported on the target instance
 * @throw argument_error if the value provided is empty
 */
void validate_consistency_supported(
    const mysqlshdk::utils::Version &version,
    const mysqlshdk::utils::nullable<std::string> &consistency) {
  if (!consistency.is_null()) {
    if (shcore::str_strip(*consistency).empty()) {
      throw shcore::Exception::argument_error(
          "Invalid value for consistency, string value cannot be "
          "empty.");
    }
    if (!is_group_replication_option_supported(version, kConsistency)) {
      throw std::runtime_error(
          "Option 'consistency' not supported on target server "
          "version: '" +
          version.get_full() + "'");
    }
  }
}

/**
 * Validate if the expelTimeout option is supported in the target instance
 * version and within the accepted range. The actual value is validated by
 * the GR plugin.
 *
 * @param session object which represents the session to the instance
 * @param expel_timeout nullable object with the value of expelTimeout
 * @throw RuntimeError if the value is not supported on the target instance
 * @throw argument_error if the value provided not within the valid range.
 */
void validate_expel_timeout_supported(
    const mysqlshdk::utils::Version &version,
    const mysqlshdk::utils::nullable<std::int64_t> &expel_timeout) {
  if (!expel_timeout.is_null()) {
    if ((*expel_timeout) < 0 || (*expel_timeout) > 3600) {
      throw shcore::Exception::argument_error(
          "Invalid value for expelTimeout, integer value must be in the range: "
          "[0, 3600]");
    }
    if (!is_group_replication_option_supported(version, kExpelTimeout)) {
      throw std::runtime_error(
          "Option 'expelTimeout' not supported on target server "
          "version: '" +
          version.get_full() + "'");
    }
  }
}

/**
 * Validate the value specified for the autoRejoinTries option is supported on
 * the target instance version. The actual value is validated by the GR plugin.
 *
 * @param version version of the target instance
 * @throw RuntimeError if the value is not supported on the target instance
 */
void validate_auto_rejoin_tries_supported(
    const mysqlshdk::utils::Version &version) {
  // The rejoinRetries option shall only be allowed if the target MySQL
  // server version is >= 8.0.16.
  if (!is_group_replication_option_supported(version, kAutoRejoinTries)) {
    throw shcore::Exception::runtime_error(
        "Option 'autoRejoinTries' not supported on target server "
        "version: '" +
        version.get_full() + "'");
  }
}

/**
 * Validate the value specified for the memberWeight option is supported on
 * the target instance
 *
 * @param options Map type value with containing the specified options.
 * @throw RuntimeError if the value is not supported on the target instance
 */
void validate_member_weight_supported(
    const mysqlshdk::utils::Version &version) {
  // The memberWeight option shall only be allowed if the target MySQL
  // server version is >= 5.7.20 if 5.0, or >= 8.0.11 if 8.0.

  if (!is_group_replication_option_supported(version, kMemberWeight)) {
    throw shcore::Exception::runtime_error(
        "Option 'memberWeight' not supported on target server "
        "version: '" +
        version.get_full() + "'");
  }
}

/**
 * Validate the value specified for the groupSeeds option.
 *
 * @param options Map type value with containing the specified options.
 * @throw ArgumentError if the value is empty.
 */
void validate_group_seeds_option(std::string group_seeds) {
  // Minimal validation is performed here the rest is already currently
  // handled at the mysqlprovision level (including the logic to automatically
  // set the group seeds when not specified)
  group_seeds = shcore::str_strip(group_seeds);
  if (group_seeds.empty())
    throw shcore::Exception::argument_error(
        "Invalid value for groupSeeds, string value cannot be empty.");
}

/**
 * Validates the ipWhitelist option
 *
 * Checks if the given ipWhitelist is valid for use in the AdminAPI.
 *
 * @param ip_whitelist The ipWhitelist to validate
 * @param hostnames_supported boolean value to indicate whether hostnames are
 * supported or not in the ipWhitelist. (supported since version 8.0.4)
 *
 * @throws argument_error if the ipWhitelist is empty
 * @throws argument_error if the subnet in CIDR notation is not valid
 * @throws argument_error if the address section is not an IPv4 address
 * @throws argument_error if the address is IPv6
 * @throws argument_error if hostnames_supported is true but the address does
 * not resolve to a valid IPv4 address
 * @throws argument error if hostnames_supports is false and the address it nos
 * a valid IPv4 address
 */
void validate_ip_whitelist_option(const std::string &ip_whitelist,
                                  bool hostnames_supported) {
  // Validate if the ipWhiteList value is not empty
  if (shcore::str_strip(ip_whitelist).empty())
    throw shcore::Exception::argument_error(
        "Invalid value for ipWhitelist: string value cannot be empty.");

  // Iterate over the ipWhitelist values
  std::vector<std::string> ip_whitelist_list =
      shcore::str_split(ip_whitelist, ",", -1);

  for (std::string value : ip_whitelist_list) {
    // Strip any blank chars from the ip_whitelist value
    value = shcore::str_strip(value);
    std::string full_value = value;

    // Check if a subnet using CIDR notation was used and validate its value
    // and separate the address
    //
    // CIDR notation is a compact representation of an IP address and its
    // associated routing prefix. The notation is constructed from an IP
    // address, a slash ('/') character, and a decimal number. The number is the
    // count of leading 1 bits in the routing mask, traditionally called the
    // network mask. The IP address is expressed according to the standards of
    // IPv4 or IPv6.

    int cidr = 0;
    if (mysqlshdk::utils::Net::strip_cidr(&value, &cidr)) {
      if ((cidr < 1) || (cidr > 32))
        throw shcore::Exception::argument_error(
            "Invalid value for ipWhitelist '" + full_value +
            "': subnet value "
            "in CIDR notation is not valid.");

      // Check if value is an hostname: hostname/cidr is not allowed
      if (!mysqlshdk::utils::Net::is_ipv4(value))
        throw shcore::Exception::argument_error(
            "Invalid value for ipWhitelist '" + full_value +
            "': CIDR "
            "notation can only be used with IPv4 addresses.");
    }

    // Check if the ipWhiteList option is IPv6
    if (mysqlshdk::utils::Net::is_ipv6(value))
      throw shcore::Exception::argument_error(
          "Invalid value for ipWhitelist '" + value +
          "': IPv6 not "
          "supported.");

    // Validate if the ipWhiteList option is IPv4
    if (!mysqlshdk::utils::Net::is_ipv4(value)) {
      // group_replication_ip_whitelist only support hostnames in server
      // >= 8.0.4
      if (hostnames_supported) {
        try {
          mysqlshdk::utils::Net::resolve_hostname_ipv4(value);
        } catch (mysqlshdk::utils::net_error &error) {
          throw shcore::Exception::argument_error(
              "Invalid value for ipWhitelist '" + value +
              "': address does "
              "not resolve to a valid IPv4 address.");
        }
      } else {
        throw shcore::Exception::argument_error(
            "Invalid value for ipWhitelist '" + value +
            "': string value is "
            "not a valid IPv4 address.");
      }
    }
  }
}

// ----

void Group_replication_options::do_unpack(shcore::Option_unpacker *unpacker) {
  switch (target) {
    case NONE:
      break;

    // Here it CREATE falls back into the JOIN case as they
    // share the same options except for groupName
    case CREATE:
      unpacker->optional(kGroupName, &group_name);
    case JOIN:
      unpacker->optional(kMemberSslMode, &ssl_mode)
          .optional(kIpWhitelist, &ip_whitelist)
          .optional(kLocalAddress, &local_address)
          .optional(kGroupSeeds, &group_seeds)
          .optional(kExitStateAction, &exit_state_action)
          .optional_exact(kMemberWeight, &member_weight)
          .optional(kFailoverConsistency, &consistency)
          .optional(kConsistency, &consistency)
          .optional_exact(kExpelTimeout, &expel_timeout)
          .optional(kAutoRejoinTries, &auto_rejoin_tries);

      break;

    case REJOIN:
      unpacker->optional(kMemberSslMode, &ssl_mode)
          .optional(kIpWhitelist, &ip_whitelist);
      break;
  }
}

void Group_replication_options::check_option_values(
    const mysqlshdk::utils::Version &version) {
  // Validate group name option
  if (!group_name.is_null()) {
    validate_group_name_option(*group_name);
  }

  if (!ip_whitelist.is_null() &&
      shcore::str_casecmp(*ip_whitelist, "AUTOMATIC") != 0) {
    // if the ipWhitelist option was provided, we know it is a valid value
    // since we've already done the validation above.
    // Skip validation if default 'AUTOMATIC' is used.
    bool hostnames_supported = false;

    // Validate ip whitelist option
    if (version >= mysqlshdk::utils::Version(8, 0, 4)) {
      hostnames_supported = true;
    }

    validate_ip_whitelist_option(*ip_whitelist, hostnames_supported);
  }

  if (!ssl_mode.is_null()) {
    validate_ssl_instance_options(*ssl_mode);
  }

  if (!local_address.is_null()) {
    validate_local_address_option(*local_address);
  }

  // Validate group seeds option
  if (!group_seeds.is_null()) {
    validate_group_seeds_option(*group_seeds);
  }

  // Validate if the exitStateAction option is supported on the target
  // instance and if is not empty.
  // The validation for the value set is handled at the group-replication
  // level
  if (!exit_state_action.is_null()) {
    validate_exit_state_action_supported(version, *exit_state_action);
  }

  // Validate if the memberWeight option is supported on the target
  // instance and if it used in the optional parameters.
  // The validation for the value set is handled at the group-replication
  // level
  if (!member_weight.is_null()) {
    validate_member_weight_supported(version);
  }

  if (!consistency.is_null()) {
    // Validate if the consistency option is supported on the target
    // instance and if is not empty.
    // The validation for the value set is handled at the group-replication
    // level
    validate_consistency_supported(version, consistency);
  }

  if (!expel_timeout.is_null()) {
    // Validate if the expelTimeout option is supported on the target
    // instance and if it is within the valid range [0, 3600].
    validate_expel_timeout_supported(version, expel_timeout);
  }

  if (!auto_rejoin_tries.is_null()) {
    // Validate if the auto_rejoin_tries option is supported on the target
    validate_auto_rejoin_tries_supported(version);
  }
}

void Group_replication_options::read_option_values(
    const mysqlshdk::mysql::IInstance &instance) {
  if (group_name.is_null()) {
    group_name = instance.get_sysvar_string("group_replication_group_name");
  }

  if (ssl_mode.is_null()) {
    ssl_mode = instance.get_sysvar_string("group_replication_ssl_mode");
  }

  if (group_seeds.is_null()) {
    group_seeds = instance.get_sysvar_string("group_replication_group_seeds");

    // Set group_seeds to NULL if value read is empty (to be overridden).
    if (!group_seeds.is_null() && group_seeds->empty()) group_seeds.reset();
  }

  if (ip_whitelist.is_null()) {
    ip_whitelist = instance.get_sysvar_string("group_replication_ip_whitelist");
  }

  if (local_address.is_null()) {
    local_address =
        instance.get_sysvar_string("group_replication_local_address");
  }

  mysqlshdk::utils::Version version = instance.get_version();

  if (member_weight.is_null() &&
      version >= mysqlshdk::utils::Version(8, 0, 2)) {
    member_weight = instance.get_sysvar_int("group_replication_member_weight");
  }

  if (exit_state_action.is_null() &&
      version >= mysqlshdk::utils::Version(8, 0, 12)) {
    exit_state_action =
        instance.get_sysvar_string("group_replication_exit_state_action");
  }

  if (expel_timeout.is_null() &&
      version >= mysqlshdk::utils::Version(8, 0, 13)) {
    expel_timeout =
        instance.get_sysvar_int("group_replication_member_expel_timeout");
  }

  if (consistency.is_null() && version >= mysqlshdk::utils::Version(8, 0, 14)) {
    consistency = instance.get_sysvar_string("group_replication_consistency");
  }

  if (auto_rejoin_tries.is_null() &&
      version >= mysqlshdk::utils::Version(8, 0, 16)) {
    auto_rejoin_tries =
        instance.get_sysvar_int("group_replication_autorejoin_tries");
  }
}

}  // namespace dba
}  // namespace mysqlsh
