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

#include "mysqlshdk/libs/aws/aws_settings.h"

#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/aws/aws_config_file.h"

namespace mysqlshdk {
namespace aws {

namespace {

std::string config_file_path(const std::string &filename) {
  static const auto s_home_dir = shcore::get_home_dir();
  return shcore::path::join_path(s_home_dir, ".aws", filename);
}

}  // namespace

Settings::Settings() : m_cache("cache") {
  m_settings[Setting::PROFILE] = {
      "profile", {"AWS_PROFILE", "AWS_DEFAULT_PROFILE"}, "default", false};
  m_settings[Setting::CREDENTIALS_FILE] = {"credentials_file",
                                           {"AWS_SHARED_CREDENTIALS_FILE"},
                                           config_file_path("credentials"),
                                           false};
  m_settings[Setting::CONFIG_FILE] = {
      "config_file", {"AWS_CONFIG_FILE"}, config_file_path("config"), false};

  m_settings[Setting::REGION] = {
      "region", {"AWS_REGION", "AWS_DEFAULT_REGION"}, "us-east-1"};

  m_settings[Setting::METADATA_SERVICE_TIMEOUT] = {
      "metadata_service_timeout", {"AWS_METADATA_SERVICE_TIMEOUT"}, "1"};
  m_settings[Setting::METADATA_SERVICE_NUM_ATTEMPTS] = {
      "metadata_service_num_attempts",
      {"AWS_METADATA_SERVICE_NUM_ATTEMPTS"},
      "1"};

  m_settings[Setting::EC2_METADATA_SERVICE_ENDPOINT] = {
      "ec2_metadata_service_endpoint", {"AWS_EC2_METADATA_SERVICE_ENDPOINT"}};
  m_settings[Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE] = {
      "ec2_metadata_service_endpoint_mode",
      {"AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE"},
      "ipv4"};
  m_settings[Setting::EC2_METADATA_V1_DISABLED] = {
      "ec2_metadata_v1_disabled", {"AWS_EC2_METADATA_V1_DISABLED"}, "false"};

  assert(m_settings.size() == static_cast<std::size_t>(Setting::LAST));
}

void Settings::add_user_option(Setting s, std::string value) {
  if (m_is_initialized) {
    throw std::runtime_error("Cannot add a user option, already initialized");
  }

  add_to_cache(s, std::move(value));
}

void Settings::add_to_cache(Setting s, std::string value) {
  m_cache.set(name(s), std::move(value));
}

void Settings::initialize() {
  m_is_initialized = true;
  m_explicit_profile = m_cache.has(info(Setting::PROFILE).name);

  load_profiles();

  m_config_profile = config_profile(get_string(Setting::PROFILE));
}

void Settings::load_profiles() {
  for (auto s : {Setting::CREDENTIALS_FILE, Setting::CONFIG_FILE}) {
    const auto is_config_file = s == Setting::CONFIG_FILE;
    const auto context = is_config_file ? "config" : "credentials";
    const auto &path = get_string(s);

    auto loaded_profiles = Aws_config_file{path}.load();

    if (!loaded_profiles.has_value()) {
      log_warning("Could not open the %s file at '%s': %s.", context,
                  path.c_str(), shcore::errno_to_string(errno).c_str());
      continue;
    }

    if (is_config_file) {
      m_config_profiles = std::move(*loaded_profiles);
    } else {
      m_credentials_profiles = std::move(*loaded_profiles);
    }
  }
}

const std::string *Settings::get(Setting s) const {
  if (!m_is_initialized) {
    throw std::runtime_error(
        "Cannot get a value of a setting, not initialized");
  }

  const auto &i = info(s);
  const std::string name(i.name);

  if (const auto value = m_cache.get(name)) {
    return value;
  }

  const auto store = [s, &name,
                      self = const_cast<Settings *>(this)](std::string v) {
    self->add_to_cache(s, std::move(v));
    return self->m_cache.get(name);
  };

  for (const auto env : i.env_vars) {
    if (const auto value = shcore::get_env(env); value.has_value()) {
      return store(*value);
    }
  }

  if (i.in_config && m_config_profile) {
    if (const auto value = m_config_profile->get(name);
        value && !value->empty()) {
      return store(*value);
    }
  }

  if (i.default_value.has_value()) {
    return store(*i.default_value);
  }

  return nullptr;
}

const std::string &Settings::get_string(Setting s) const {
  const auto v = get(s);

  if (!v) {
    throw std::runtime_error("Missing value of setting: " +
                             std::string{name(s)});
  }

  return *v;
}

int Settings::get_int(Setting s) const {
  const auto &value = get_string(s);

  try {
    return as_int(value);
  } catch (const std::exception &e) {
    throw std::runtime_error("Could not convert value '" + value +
                             "' of setting '" + std::string(name(s)) +
                             "' to int: " + e.what());
  }
}

bool Settings::get_bool(Setting s) const { return as_bool(get_string(s)); }

const Profile *Settings::config_profile(const std::string &name) const {
  return m_config_profiles.get(name);
}

const Profile *Settings::credentials_profile(const std::string &name) const {
  return m_credentials_profiles.get(name);
}

bool Settings::has_static_credentials(const std::string &name) const {
  if (const auto profile = credentials_profile(name)) {
    if (profile->has_credentials()) {
      return true;
    }
  }

  if (const auto profile = config_profile(name)) {
    if (profile->has_credentials()) {
      return true;
    }
  }

  return false;
}

bool Settings::as_bool(std::string_view b) {
  return shcore::str_caseeq(b, "true");
}

int Settings::as_int(std::string_view i) {
  return shcore::lexical_cast<int>(i);
}

}  // namespace aws
}  // namespace mysqlshdk
