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

#include "mysqlshdk/libs/aws/imds_credentials_provider.h"

#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

namespace {

class Too_many_retries : public std::exception {
 public:
  using exception::exception;

  const char *what() const noexcept override {
    return "Exceeded number of retries";
  }
};

class Wrong_json : public std::exception {
 public:
  using exception::exception;

  const char *what() const noexcept override {
    return "JSON response contains an error message";
  }
};

void validate_endpoint(const std::string &endpoint) {
  try {
    mysqlshdk::db::uri::Generic_uri parsed{endpoint};

    if (parsed.scheme.empty()) {
      throw std::invalid_argument{"Scheme is missing"};
    }

    if (!shcore::is_valid_hostname(parsed.host)) {
      throw std::invalid_argument{"Hostname is not valid"};
    }
  } catch (const std::exception &e) {
    throw std::invalid_argument{"Invalid IMDS endpoint '" + endpoint +
                                "': " + e.what()};
  }
}

}  // namespace

Imds_credentials_provider::Imds_credentials_provider(const Settings &settings)
    : Aws_credentials_provider(
          {"IMDS credentials", "AccessKeyId", "SecretAccessKey"}) {
  if (const auto disabled = shcore::get_env("AWS_EC2_METADATA_DISABLED");
      disabled.has_value()) {
    if (settings.as_bool(*disabled)) {
      log_info("The IMDS credentials provider is disabled.");
      return;
    }
  }

  if (const auto endpoint =
          settings.get(Setting::EC2_METADATA_SERVICE_ENDPOINT)) {
    m_endpoint = *endpoint;
    validate_endpoint(m_endpoint);

    if (m_endpoint.back() != '/') {
      m_endpoint.push_back('/');
    }
  } else {
    bool ipv6 = false;
    const auto mode = shcore::str_lower(
        settings.get_string(Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE));

    if ("ipv4" == mode) {
      ipv6 = false;
    } else if ("ipv6" == mode) {
      ipv6 = true;
    } else {
      throw std::invalid_argument("Invalid IMDS endpoint mode: " + mode);
    }

    m_endpoint = ipv6 ? "http://[fd00:ec2::254]/" : "http://169.254.169.254/";
  }

  log_info("The IMDS credentials provider is using endpoint: %s",
           m_endpoint.c_str());

  const auto positive = [&settings](Setting s) {
    try {
      auto v = settings.get_int(s);

      if (v < 1) {
        throw std::invalid_argument(
            "must be greater than or equal to 1, got: " + std::to_string(v));
      }

      return v;
    } catch (const std::exception &e) {
      throw std::invalid_argument("Invalid value of '" +
                                  std::string(settings.name(s)) +
                                  "': " + e.what());
    }
  };

  m_timeout_s = positive(Setting::METADATA_SERVICE_TIMEOUT);
  m_attempts = positive(Setting::METADATA_SERVICE_NUM_ATTEMPTS);
}

bool Imds_credentials_provider::available() const noexcept {
  return !m_endpoint.empty();
}

Aws_credentials_provider::Credentials
Imds_credentials_provider::fetch_credentials() {
  if (!m_service) {
    m_service = std::make_unique<rest::Rest_service>(m_endpoint);
    m_service->set_timeout(m_timeout_s, 1, m_timeout_s);
    m_service->set_connect_timeout(m_timeout_s);
  }

  try {
    const auto token = fetch_token();
    const auto role = fetch_iam_role(token);

    return parse_credentials(fetch_iam_credentials(token, role));
  } catch (const Too_many_retries &) {
    log_info("Exceeded number of attempts when fetching IMDS credentials.");
    return {};
  } catch (const Wrong_json &) {
    // server replied with a JSON error, do not use this provider
    return {};
  }
}

std::string Imds_credentials_provider::fetch_token() const {
  try {
    rest::Request request{"latest/api/token",
                          {{"x-aws-ec2-metadata-token-ttl-seconds", "21600"}}};

    for (int i = 0; i < m_attempts; ++i) {
      rest::String_response response;

      try {
        response = m_service->put(&request);
      } catch (const rest::Connection_error &e) {
        if (rest::Error_code::OPERATION_TIMEDOUT == e.code()) {
          // retry in case of a timeout
          continue;
        } else if (rest::Error_code::COULDNT_CONNECT == e.code()) {
          // network is unreachable
          break;
        } else {
          throw;
        }
      }

      if (rest::Response::Status_code::OK == response.status && response.body) {
        return std::string(response.body->data(), response.body->size());
      }

      if (rest::Response::Status_code::FORBIDDEN == response.status ||
          rest::Response::Status_code::NOT_FOUND == response.status ||
          rest::Response::Status_code::METHOD_NOT_ALLOWED == response.status) {
        break;
      }

      response.throw_if_error();
    }

    // requests will not use a token
    return {};
  } catch (const std::exception &e) {
    throw std::runtime_error{"Failed to fetch IMDS token from '" + m_endpoint +
                             "': " + e.what()};
  }
}

std::string Imds_credentials_provider::fetch_iam_role(
    const std::string &token) const {
  try {
    return execute_request(token);
  } catch (const Too_many_retries &) {
    throw;
  } catch (const std::exception &e) {
    throw std::runtime_error{"Failed to fetch IAM role from '" + m_endpoint +
                             "': " + e.what()};
  }
}

std::string Imds_credentials_provider::fetch_iam_credentials(
    const std::string &token, const std::string &role) const {
  try {
    return execute_request(token, role);
  } catch (const Too_many_retries &) {
    throw;
  } catch (const std::exception &e) {
    throw std::runtime_error{"Failed to fetch IAM credentials from '" +
                             m_endpoint + "': " + e.what()};
  }
}

Aws_credentials_provider::Credentials
Imds_credentials_provider::parse_credentials(const std::string &json) const {
  try {
    return parse_json_credentials(
        json, "Token", 4, [json](const shcore::json::JSON &doc) {
          if (doc.HasMember("Code") && doc.HasMember("Message")) {
            // IMDS can return a HTTP OK response with a JSON error, log it
            log_info("Error when fetching IAM credentials: %s", json.c_str());
            throw Wrong_json{};
          }
        });
  } catch (const Wrong_json &) {
    throw;
  } catch (const std::exception &e) {
    throw std::runtime_error{"Failed to parse IAM credentials fetched from '" +
                             m_endpoint + "': " + e.what()};
  }
}

std::string Imds_credentials_provider::execute_request(
    const std::string &token, const std::string &role) const {
  rest::Headers headers;

  if (!token.empty()) {
    headers.emplace("x-aws-ec2-metadata-token", token);
  }

  rest::Request request{"latest/meta-data/iam/security-credentials/" + role,
                        std::move(headers)};

  for (int i = 0; i < m_attempts; ++i) {
    rest::String_response response;

    try {
      response = m_service->get(&request);
    } catch (const rest::Connection_error &e) {
      if (rest::Error_code::OPERATION_TIMEDOUT == e.code()) {
        // retry in case of a timeout
        continue;
      } else if (rest::Error_code::COULDNT_CONNECT == e.code()) {
        // network is unreachable
        break;
      } else {
        throw;
      }
    }

    if (rest::Response::Status_code::OK == response.status && response.body &&
        response.body->size()) {
      if (!role.empty()) {
        // if role is set, we're fetching credentials and response should be
        // JSON
        if (const auto doc = shcore::json::parse(
                {response.body->data(), response.body->size()});
            doc.HasParseError()) {
          continue;
        }
      }

      return std::string(response.body->data(), response.body->size());
    }
  }

  throw Too_many_retries{};
}

}  // namespace aws
}  // namespace mysqlshdk
