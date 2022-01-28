/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/s3_bucket_config.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "mysqlshdk/libs/aws/aws_config_file.h"
#include "mysqlshdk/libs/aws/aws_signer.h"
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
    : Config(options), m_credentials_file(options.m_credentials_file) {
  const auto config_file_path = [](const std::string &filename) {
    static const auto s_home_dir = shcore::get_home_dir();
    return shcore::path::join_path(s_home_dir, ".aws", filename);
  };

  if (m_credentials_file.empty()) {
    m_credentials_file = config_file_path("credentials");
  }

  if (m_config_file.empty()) {
    m_config_file = config_file_path("config");
  }

  if (m_config_profile.empty()) {
    m_config_profile = "default";
  }

  load_profile(m_config_file, true);
  load_profile(m_credentials_file, false);
  validate_profile();

  if (m_region.empty()) {
    m_region = "us-east-1";
  }

  if (!options.m_endpoint_override.empty()) {
    m_endpoint = options.m_endpoint_override;

    while ('/' == m_endpoint.back()) {
      m_endpoint.pop_back();
    }

    m_host = storage::utils::strip_scheme(m_endpoint);
    // if endpoint is overridden, path-style access is used
    use_path_style_access();
  } else {
    // if bucket can be used as a virtual path, the following URI is used:
    // <bucket>.s3.<region>.amazonaws.com
    if (valid_as_virtual_path(m_bucket_name)) {
      m_host = m_bucket_name + ".";
    } else {
      use_path_style_access();
    }

    m_host += "s3." + m_region + ".amazonaws.com";
    m_endpoint = "https://" + m_host;
  }
}

std::unique_ptr<rest::Signer> S3_bucket_config::signer() const {
  return std::make_unique<Aws_signer>(*this);
}

std::unique_ptr<storage::backend::object_storage::Bucket>
S3_bucket_config::bucket() const {
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
    m_hash += m_bucket_name;
    m_hash += '-';
    m_hash += m_config_file;
    m_hash += '-';
    m_hash += m_config_profile;
    m_hash += '-';
    m_hash += m_credentials_file;
    m_hash += '-';
    m_hash += m_access_key_id.value_or("");
    m_hash += '-';
    m_hash += m_region;
  }

  return m_hash;
}

std::string S3_bucket_config::describe_self() const {
  return "AWS S3 bucket=" + m_bucket_name;
}

void S3_bucket_config::load_profile(const std::string &path,
                                    bool is_config_file) {
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

  if (profile->access_key_id) {
    m_access_key_id = profile->access_key_id;
  }

  if (profile->secret_access_key) {
    m_secret_access_key = profile->secret_access_key;
  }

  if (profile->session_token && !profile->session_token->empty()) {
    m_session_token = profile->session_token;
  }

  if (is_config_file) {
    const auto region = profile->settings.find("region");

    if (profile->settings.end() != region) {
      m_region = region->second;
    }
  }
}

void S3_bucket_config::validate_profile() const {
  const auto fail_if_missing = [this](const std::string &setting,
                                      const std::optional<std::string> &value) {
    if (!value) {
      throw std::runtime_error(
          "The '" + setting + "' setting for the profile '" + m_config_profile +
          "' was not found in neither '" + m_config_file + "' nor '" +
          m_credentials_file + "' files.");
    }
  };

  fail_if_missing("aws_access_key_id", m_access_key_id);
  fail_if_missing("aws_secret_access_key", m_secret_access_key);
}

void S3_bucket_config::use_path_style_access() { m_path_style_access = true; }

}  // namespace aws
}  // namespace mysqlshdk
