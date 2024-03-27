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

#ifndef MYSQLSHDK_LIBS_AWS_STS_CLIENT_H_
#define MYSQLSHDK_LIBS_AWS_STS_CLIENT_H_

#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/rest/signed/signed_rest_service.h"
#include "mysqlshdk/libs/rest/signed/signed_rest_service_config.h"

#include "mysqlshdk/libs/aws/aws_signer.h"

namespace mysqlshdk {
namespace aws {

struct Assume_role_request {
  std::string arn;
  std::string session_name;
  std::optional<int> duration_seconds;
  std::string external_id;
};

struct Assume_role_response {
  std::string access_key_id;
  std::string secret_access_key;
  std::string session_token;
  std::string expiration;
};

/**
 * Client which implements AWS Security Token Service (STS) API.
 */
class Sts_client final : public Aws_signer_config,
                         public rest::Signed_rest_service_config {
 public:
  Sts_client(const std::string &region, Aws_credentials_provider *provider);

  Sts_client(const Sts_client &) = delete;
  Sts_client(Sts_client &&) = default;

  Sts_client &operator=(const Sts_client &) = delete;
  Sts_client &operator=(Sts_client &&) = default;

  ~Sts_client() = default;

  const std::string &host() const override { return m_host; }

  const std::string &region() const override { return m_region; }

  const std::string &service() const override { return m_service; }

  Aws_credentials_provider *credentials_provider() const override {
    return m_provider;
  }

  const std::string &service_endpoint() const override { return m_endpoint; }

  const std::string &service_label() const override { return m_label; }

  std::unique_ptr<rest::ISigner> signer() const override;

  Assume_role_response assume_role(const Assume_role_request &r);

 private:
  rest::Signed_rest_service *http();

  rest::String_response post(std::string method, rest::Query query);

  std::string m_label = "AWS-STS";
  std::string m_service = "sts";

  std::string m_endpoint;
  std::string m_host;
  std::string m_region;
  Aws_credentials_provider *m_provider;

  std::unique_ptr<rest::Signed_rest_service> m_http;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_STS_CLIENT_H_
