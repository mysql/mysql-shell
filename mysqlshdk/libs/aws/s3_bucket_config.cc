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
#include <utility>

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#include "mysqlshdk/libs/aws/assume_role_credentials_provider.h"
#include "mysqlshdk/libs/aws/aws_credentials_resolver.h"
#include "mysqlshdk/libs/aws/aws_signer.h"
#include "mysqlshdk/libs/aws/container_credentials_provider.h"
#include "mysqlshdk/libs/aws/env_credentials_provider.h"
#include "mysqlshdk/libs/aws/imds_credentials_provider.h"
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

}  // namespace

S3_bucket_config::S3_bucket_config(const S3_bucket_options &options)
    : Bucket_config(options),
      Config_file(options),
      Credentials_file(options),
      Config_profile(options) {
  m_label = "AWS-S3-OS";
  m_endpoint = options.m_endpoint_override;

  const auto add_nonempty = [this](Setting s, const std::string &v) {
    if (!v.empty()) {
      m_settings.add_user_option(s, v);
    }
  };

  add_nonempty(Setting::PROFILE, Config_profile::config_profile());
  add_nonempty(Setting::CONFIG_FILE, Config_file::config_file());
  add_nonempty(Setting::CREDENTIALS_FILE, Credentials_file::credentials_file());
  add_nonempty(Setting::REGION, options.m_region);
  m_settings.initialize();

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
    m_hash += config_file();
    m_hash += '-';
    m_hash += config_profile();
    m_hash += '-';
    m_hash += credentials_file();
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
    m_hash += region();
  }

  return m_hash;
}

std::string S3_bucket_config::describe_self() const {
  return "AWS S3 bucket=" + m_container_name;
}

void S3_bucket_config::use_path_style_access() { m_path_style_access = true; }

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

    m_host += "s3." + region() + ".amazonaws.com";
    m_endpoint = "https://" + m_host;
  }
}

void S3_bucket_config::setup_credentials_provider() {
  Aws_credentials_resolver resolver;

  if (m_settings.explicit_profile()) {
    // profile was set by an option, don't use credentials from environment
    // variables, see https://github.com/aws/aws-cli/issues/113
    log_info(
        "The environment variables are not going to be used to fetch AWS "
        "credentials, because the '%s' option is set.",
        S3_bucket_options::profile_option());
  } else {
    resolver.add(std::make_unique<Env_credentials_provider>());
  }

  resolver.add(std::make_unique<Assume_role_credentials_provider>(
      m_settings, config_profile()));
  resolver.add_profile_providers(m_settings, config_profile());
  resolver.add(std::make_unique<Container_credentials_provider>());
  resolver.add(std::make_unique<Imds_credentials_provider>(m_settings));

  m_credentials_provider = resolver.resolve();
}

}  // namespace aws
}  // namespace mysqlshdk
