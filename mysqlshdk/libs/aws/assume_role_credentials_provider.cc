/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/assume_role_credentials_provider.h"

#include <cassert>
#include <chrono>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/aws/aws_profiles.h"
#include "mysqlshdk/libs/aws/container_credentials_provider.h"
#include "mysqlshdk/libs/aws/env_credentials_provider.h"
#include "mysqlshdk/libs/aws/imds_credentials_provider.h"

namespace mysqlshdk {
namespace aws {

namespace {

const std::string k_role_arn{"role_arn"};
const std::string k_web_identity_token_file{"web_identity_token_file"};

const std::string k_credential_source{"credential_source"};
const std::string k_source_profile{"source_profile"};

const std::string k_duration_seconds{"duration_seconds"};
const std::string k_external_id{"external_id"};
const std::string k_mfa_serial{"mfa_serial"};
const std::string k_role_session_name{"role_session_name"};

bool uses_assume_role(const Profile *profile) {
  assert(profile);
  // if profile has both these settings, then role is assumed using a different
  // provider
  return profile->has(k_role_arn) && !profile->get(k_role_arn)->empty() &&
         (!profile->has(k_web_identity_token_file) ||
          profile->get(k_web_identity_token_file)->empty());
}

bool uses_source_profile(const Profile *profile) {
  assert(profile);
  const auto has_source_profile = profile->has(k_source_profile) &&
                                  !profile->get(k_source_profile)->empty();
  const auto has_credential_source =
      profile->has(k_credential_source) &&
      !profile->get(k_credential_source)->empty();

  if (has_source_profile && has_credential_source) {
    throw std::invalid_argument("Profile '" + profile->name() +
                                "' contains both '" + k_source_profile +
                                "' and '" + k_credential_source + "' settings");
  }

  if (!has_source_profile && !has_credential_source) {
    throw std::invalid_argument("Profile '" + profile->name() +
                                "' contains neither '" + k_source_profile +
                                "' nor '" + k_credential_source + "' settings");
  }

  return has_source_profile;
}

void validate_settings(const Profile *profile) {
  assert(profile);

  if (profile->has(k_mfa_serial)) {
    throw std::invalid_argument("Profile '" + profile->name() +
                                "' is using multi-factor authentication device "
                                "to assume a role, this is not supported");
  }

  if (profile->has(k_duration_seconds)) {
    try {
      // check if value is convertible to integer
      Settings::as_int(*profile->get(k_duration_seconds));
    } catch (const std::exception &e) {
      throw std::invalid_argument(
          "Profile '" + profile->name() + "' contains invalid value of '" +
          k_duration_seconds + "' setting: " + e.what());
    }
  }
}

void validate_profile_chain(const Settings &s,
                            const std::string &starting_profile) {
  std::unordered_set<const Profile *> visited;
  std::vector<const Profile *> chain;

  const auto print_chain = [&chain]() {
    return shcore::str_join(chain, " -> ",
                            [](const Profile *p) { return p->name(); });
  };

  const auto handle_error = [&print_chain](const std::string &msg) {
    log_error("%s, profile chain: %s", msg.c_str(), print_chain().c_str());
    throw std::invalid_argument(msg);
  };

  if (const auto p = s.config_profile(starting_profile)) {
    chain.emplace_back(p);
    visited.emplace(p);
  } else {
    handle_error("Profile '" + starting_profile + "' does not exist");
  }

  while (true) {
    if (!uses_assume_role(chain.back())) {
      // the last profile does not use assume role, end of chain
      break;
    }

    if (!uses_source_profile(chain.back())) {
      // the last profile uses credential source, end of chain
      break;
    }

    const auto &profile = *chain.back()->get(k_source_profile);
    auto p = s.config_profile(profile);

    if (!p) {
      // if there's no config profile, try with profile from credentials file
      p = s.credentials_profile(profile);
    }

    if (!p) {
      handle_error("Profile '" + profile + "' referenced from profile '" +
                   chain.back()->name() + "' does not exist");
    }

    // check for a cycle
    if (visited.count(p)) {
      // profile can reference itself, as long as it has static credentials
      if (chain.back() != p || !s.has_static_credentials(profile)) {
        // add profile to chain to make the log message more verbose
        chain.emplace_back(p);
        handle_error("Profiles in the config file '" +
                     s.get_string(Setting::CONFIG_FILE) + "' create a cycle");
      }

      // profile references itself, end of chain
      break;
    } else {
      chain.emplace_back(p);
      visited.emplace(p);
    }
  }

  log_debug("Profile chain used by assume role: %s", print_chain().c_str());
}

}  // namespace

Assume_role_credentials_provider::Assume_role_credentials_provider(
    const Settings &settings, const std::string &profile)
    : Aws_credentials_provider({"assume role (config file: " +
                                    settings.get_string(Setting::CONFIG_FILE) +
                                    ", profile: " + profile + ")",
                                "AccessKeyId", "SecretAccessKey"}) {
  const auto p = settings.config_profile(profile);

  if (!p) {
    log_warning(
        "The config file at '%s' does not contain a profile named: '%s'.",
        settings.get_string(Setting::CONFIG_FILE).c_str(), profile.c_str());
    return;
  }

  if (!uses_assume_role(p)) {
    log_info("Profile '%s' is not configured to assume a role.",
             profile.c_str());
    return;
  }

  validate_settings(p);

  if (uses_source_profile(p)) {
    handle_source_profile(p, settings);
  } else {
    handle_credential_source(p, settings);
  }

  m_profile = p;
  m_region = settings.get_string(Setting::REGION);
}

bool Assume_role_credentials_provider::available() const noexcept {
  return m_profile;
}

Aws_credentials_provider::Credentials
Assume_role_credentials_provider::fetch_credentials() {
  if (!m_provider) {
    initialize_sts();
  }

  return assume_role();
}

void Assume_role_credentials_provider::handle_source_profile(
    const Profile *profile, const Settings &settings) {
  validate_profile_chain(settings, profile->name());

  const auto source = profile->get(k_source_profile);
  assert(source);

  if (settings.has_static_credentials(*source) ||
      !uses_assume_role(settings.config_profile(*source))) {
    m_resolver.add_profile_providers(settings, *source);
  } else {
    m_resolver.add(
        std::make_unique<Assume_role_credentials_provider>(settings, *source));
  }
}

void Assume_role_credentials_provider::handle_credential_source(
    const Profile *profile, const Settings &settings) {
  const auto source = profile->get(k_credential_source);
  assert(source);
  std::unique_ptr<Aws_credentials_provider> provider;

  if ("Environment" == *source) {
    provider = std::make_unique<Env_credentials_provider>();
  } else if ("Ec2InstanceMetadata" == *source) {
    provider = std::make_unique<Imds_credentials_provider>(settings);
  } else if ("EcsContainer" == *source) {
    provider = std::make_unique<Container_credentials_provider>();
  } else {
    throw std::invalid_argument("Profile '" + profile->name() +
                                "' has invalid value of '" +
                                k_credential_source + "' setting: " + *source);
  }

  m_resolver.add(std::move(provider));
}

void Assume_role_credentials_provider::initialize_sts() {
  try {
    m_provider = m_resolver.resolve();
    m_sts = std::make_unique<Sts_client>(m_region, m_provider.get());
  } catch (const std::exception &e) {
    const auto &name = m_profile->name();
    // erase the profile, so we won't try to resolve credentials again
    m_profile = nullptr;

    throw std::runtime_error(
        "Could not obtain credentials to assume role using profile '" + name +
        "': " + e.what());
  }
}

Aws_credentials_provider::Credentials
Assume_role_credentials_provider::assume_role() {
  try {
    Assume_role_request request;

    request.arn = *m_profile->get(k_role_arn);

    if (const auto name = m_profile->get(k_role_session_name)) {
      request.session_name = *name;
    } else {
      request.session_name = "mysqlsh-session-";
      request.session_name += std::to_string(
          std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());
    }

    log_debug3("AWS assume role - session name: %s",
               request.session_name.c_str());

    if (const auto duration = m_profile->get(k_duration_seconds)) {
      request.duration_seconds = Settings::as_int(*duration);
      log_debug3("AWS assume role - custom duration: %ds",
                 *request.duration_seconds);
    }

    if (const auto id = m_profile->get(k_external_id)) {
      request.external_id = *id;
      log_debug3("AWS assume role - external ID: %s",
                 request.external_id.c_str());
    }

    auto response = m_sts->assume_role(request);
    Credentials creds;

    creds.access_key_id = std::move(response.access_key_id);
    creds.secret_access_key = std::move(response.secret_access_key);
    creds.session_token = std::move(response.session_token);
    creds.expiration = std::move(response.expiration);

    return creds;
  } catch (const std::exception &e) {
    throw std::runtime_error("Could not assume role using profile '" +
                             m_profile->name() + "': " + e.what());
  }
}

}  // namespace aws
}  // namespace mysqlshdk
