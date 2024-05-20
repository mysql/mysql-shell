/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/s3_bucket_config.h"

#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "mysqlshdk/libs/aws/aws_credentials_resolver.h"
#include "mysqlshdk/libs/aws/aws_signer.h"
#include "mysqlshdk/libs/aws/config_credentials_provider.h"
#include "mysqlshdk/libs/aws/env_credentials_provider.h"
#include "mysqlshdk/libs/aws/process_credentials_provider.h"
#include "mysqlshdk/libs/aws/s3_bucket.h"

namespace mysqlshdk {
namespace aws {

namespace {

bool is_lower_or_digit(char c) {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

bool valid_as_virtual_path(const std::string &bucket_name) {
  // Bucket name can be used as a virtual path if it is a valid DNS label:
  //  - 0 - 63 characters
  //  - consists of A-Z, a-z, 0-9 and hyphen (-)
  //  - cannot start or end with a hyphen
  // Bucket names have some additional requirements, i.e. only lower-case
  // letters are allowed.
  if (bucket_name.empty() || bucket_name.length() > 63) {
    return false;
  }

  if (!is_lower_or_digit(bucket_name.front()) ||
      !is_lower_or_digit(bucket_name.back())) {
    return false;
  }

  for (const auto &c : bucket_name) {
    if (!is_lower_or_digit(c) && '-' != c) {
      return false;
    }
  }

  return true;
}

std::string config_file_path(const std::string &filename) {
  static const auto s_home_dir = shcore::get_home_dir();
  return shcore::path::join_path(s_home_dir, ".aws", filename);
}

}  // namespace

S3_bucket_config::S3_bucket_config(const S3_bucket_options &options)
    : Bucket_config(options),
      Config_file(options),
      Credentials_file(options),
      Config_profile(options),
      m_region(options.m_region) {
  m_label = "AWS-S3-OS";
  m_endpoint = options.m_endpoint_override;

  setup_profile_name();

  setup_credentials_file();
  load_profile(&m_profile_from_credentials_file);

  setup_config_file();
  load_profile(&m_profile_from_config_file);

  setup_region_name();

  setup_endpoint_uri();

  setup_credentials_provider();
}

std::unique_ptr<rest::ISigner> S3_bucket_config::signer() const {
  return std::make_unique<Aws_signer>(*this);
}

std::unique_ptr<storage::backend::object_storage::Container>
S3_bucket_config::container() const {
  return s3_bucket();
}

std::unique_ptr<S3_bucket> S3_bucket_config::s3_bucket() const {
  return std::make_unique<S3_bucket>(shared_ptr<S3_bucket_config>());
}

const std::string &S3_bucket_config::hash() const {
  if (m_hash.empty()) {
    m_hash.reserve(512);

    m_hash += m_label;
    m_hash += '-';
    m_hash += m_endpoint;
    m_hash += '-';
    m_hash += m_container_name;
    m_hash += '-';
    m_hash += m_config_file;
    m_hash += '-';
    m_hash += m_config_profile;
    m_hash += '-';
    m_hash += m_credentials_file;
    m_hash += '-';
    m_hash += m_credentials_provider->name();
    m_hash += '-';
    m_hash += m_credentials_provider->credentials()->access_key_id();
    m_hash += '-';
    m_hash += std::to_string(m_credentials_provider->credentials()
                                 ->expiration()
                                 .time_since_epoch()
                                 .count());
    m_hash += '-';
    m_hash += m_region;
  }

  return m_hash;
}

std::string S3_bucket_config::describe_self() const {
  return "AWS S3 bucket=" + m_container_name;
}

void S3_bucket_config::load_profile(
    std::optional<Aws_config_file::Profile> *target) {
  const auto is_config_file = target == &m_profile_from_config_file;
  const auto &path = is_config_file ? m_config_file : m_credentials_file;
  const auto context = is_config_file ? "config" : "credentials";
  Aws_config_file config{path};

  if (!config.load()) {
    log_warning("Could not open the %s file at '%s': %s.", context,
                path.c_str(), shcore::errno_to_string(errno).c_str());
    return;
  }

  const auto profile = config.get_profile(m_config_profile);

  if (!profile) {
    log_warning("The %s file at '%s' does not contain a profile named: '%s'.",
                context, path.c_str(), m_config_profile.c_str());
    return;
  }

  *target = *profile;
}

void S3_bucket_config::use_path_style_access() { m_path_style_access = true; }

void S3_bucket_config::setup_profile_name() {
  if (!m_config_profile.empty()) {
    m_explicit_profile = true;
    return;
  }

  for (const auto name : {"AWS_PROFILE", "AWS_DEFAULT_PROFILE"}) {
    if (const auto value = shcore::get_env(name); value.has_value()) {
      m_config_profile = *value;
      return;
    }
  }

  m_config_profile = "default";
}

void S3_bucket_config::setup_credentials_file() {
  setup_config_file(&m_credentials_file);
}

void S3_bucket_config::setup_config_file() {
  setup_config_file(&m_config_file);
}

void S3_bucket_config::setup_config_file(std::string *target) {
  if (!target->empty()) {
    return;
  }

  const auto is_config_file = target == &m_config_file;

  if (const auto value = shcore::get_env(
          is_config_file ? "AWS_CONFIG_FILE" : "AWS_SHARED_CREDENTIALS_FILE");
      value.has_value()) {
    *target = *value;
    return;
  }

  *target = config_file_path(is_config_file ? "config" : "credentials");
}

void S3_bucket_config::setup_region_name() {
  if (!m_region.empty()) {
    return;
  }

  for (const auto name : {"AWS_REGION", "AWS_DEFAULT_REGION"}) {
    if (const auto value = shcore::get_env(name); value.has_value()) {
      m_region = *value;
      return;
    }
  }

  if (m_profile_from_config_file.has_value()) {
    const auto region = m_profile_from_config_file->settings.find("region");

    if (m_profile_from_config_file->settings.end() != region &&
        !region->second.empty()) {
      m_region = region->second;
      return;
    }
  }

  m_region = "us-east-1";
}

void S3_bucket_config::setup_endpoint_uri() {
  while (!m_endpoint.empty() && '/' == m_endpoint.back()) {
    m_endpoint.pop_back();
  }

  if (!m_endpoint.empty()) {
    m_host = storage::utils::strip_scheme(m_endpoint);
    // if endpoint is overridden, path-style access is used
    use_path_style_access();
  } else {
    // if bucket can be used as a virtual path, the following URI is used:
    // <bucket>.s3.<region>.amazonaws.com
    if (valid_as_virtual_path(m_container_name)) {
      m_host = m_container_name + ".";
    } else {
      use_path_style_access();
    }

    m_host += "s3." + m_region + ".amazonaws.com";
    m_endpoint = "https://" + m_host;
  }
}

void S3_bucket_config::setup_credentials_provider() {
  Aws_credentials_resolver resolver;

  if (m_explicit_profile) {
    // profile was set by an option, don't use credentials from environment
    // variables, see https://github.com/aws/aws-cli/issues/113
    log_info(
        "The environment variables are not going to be used to fetch AWS "
        "credentials, because the '%s' option is set.",
        S3_bucket_options::profile_option());
  } else {
    resolver.add(std::make_unique<Env_credentials_provider>());
  }

  if (m_profile_from_credentials_file.has_value()) {
    resolver.add(std::make_unique<Config_credentials_provider>(
        m_credentials_file, "credentials file",
        &*m_profile_from_credentials_file));
  }

  if (m_profile_from_config_file.has_value()) {
    const auto &profile = *m_profile_from_config_file;

    resolver.add(std::make_unique<Process_credentials_provider>(m_config_file,
                                                                &profile));

    resolver.add(std::make_unique<Config_credentials_provider>(
        m_config_file, "config file", &profile));
  }

  m_credentials_provider = resolver.resolve();
}

}  // namespace aws
}  // namespace mysqlshdk
