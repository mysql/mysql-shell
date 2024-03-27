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

#include "mysqlshdk/libs/aws/container_credentials_provider.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/aws/aws_settings.h"

namespace mysqlshdk {
namespace aws {

namespace {

// default Amazon ECS hostname
constexpr auto k_container_ip = "169.254.170.2";

void validate_uri(const std::string &uri) {
  try {
    static constexpr const char *k_allowed_hosts[] = {
        k_container_ip,
        "localhost",
        "127.0.0.1",
    };

    mysqlshdk::db::uri::Generic_uri parsed{uri};

    for (const auto allowed : k_allowed_hosts) {
      if (allowed == parsed.host) {
        return;
      }
    }

    throw std::invalid_argument{
        "Unsupported host '" + parsed.host + "', allowed values: " +
        shcore::str_join(std::begin(k_allowed_hosts), std::end(k_allowed_hosts),
                         ", ")};
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument{"Invalid URI for ECS credentials '" + uri +
                                "': " + e.what()};
  }
}

}  // namespace

Container_credentials_provider::Container_credentials_provider()
    : Aws_credentials_provider(
          {"ECS credentials", "AccessKeyId", "SecretAccessKey"}) {
  if (const auto rel =
          shcore::get_env("AWS_CONTAINER_CREDENTIALS_RELATIVE_URI");
      rel.has_value()) {
    m_full_uri = "http://";
    m_full_uri += k_container_ip;
    m_full_uri += *rel;
  } else if (auto full = shcore::get_env("AWS_CONTAINER_CREDENTIALS_FULL_URI");
             full.has_value()) {
    m_full_uri = std::move(*full);
  }

  if (m_full_uri.empty()) {
    log_info(
        "AWS ECS provider is not going to be used: environment variables are "
        "not set");
    return;
  }

  log_info("AWS ECS URI: '%s'", m_full_uri.c_str());

  if (auto value = shcore::get_env("AWS_CONTAINER_AUTHORIZATION_TOKEN");
      value.has_value()) {
    log_info("The ECS authorization token is set");
    m_authorization_token = std::move(*value);
  }
}

bool Container_credentials_provider::available() const noexcept {
  return !m_full_uri.empty();
}

Aws_credentials_provider::Credentials
Container_credentials_provider::fetch_credentials() {
  validate_uri(m_full_uri);

  if (!m_service) {
    m_service = std::make_unique<rest::Rest_service>(m_full_uri);
    // timeout is 2 seconds
    m_service->set_timeout(2, 1, 2);
    m_service->set_connect_timeout(2);
  }

  return parse_credentials(execute_request());
}

std::string Container_credentials_provider::execute_request() const {
  try {
    // sleep one second between retries
    const auto retry = rest::Retry_strategy_builder{1}
                           .set_max_attempts(3)
                           .retry_on_server_errors()
                           .retry_on(rest::Error_code::OPERATION_TIMEDOUT)
                           .build();

    rest::Headers headers{{"Accept", "application/json"}};

    if (!m_authorization_token.empty()) {
      headers.emplace("Authorization", m_authorization_token);
    }

    rest::Request request{"", std::move(headers)};
    request.retry_strategy = retry.get();

    const auto response = m_service->get(&request);
    response.throw_if_error();

    if (!response.body || !response.body->size()) {
      throw std::runtime_error("no response");
    }

    return std::string(response.body->data(), response.body->size());
  } catch (const std::exception &e) {
    throw std::runtime_error{"Failed to fetch ECS credentials from '" +
                             m_full_uri + "': " + e.what()};
  }
}

Aws_credentials_provider::Credentials
Container_credentials_provider::parse_credentials(
    const std::string &json) const {
  try {
    return parse_json_credentials(json, "Token");
  } catch (const std::exception &e) {
    throw std::runtime_error{"Failed to parse ECS credentials fetched from '" +
                             m_full_uri + "': " + e.what()};
  }
}

}  // namespace aws
}  // namespace mysqlshdk
