/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_SIGNER_H_
#define MYSQLSHDK_LIBS_AWS_AWS_SIGNER_H_

#include <optional>
#include <string>
#include <vector>

#include "mysqlshdk/libs/rest/signed_rest_service.h"

#include "mysqlshdk/libs/aws/aws_credentials.h"
#include "mysqlshdk/libs/aws/s3_bucket_config.h"

namespace mysqlshdk {
namespace aws {

/**
 * Handles signing of the AWS requests.
 *
 * NOTE: this is currently tuned for S3:
 *  - CanonicalURI is URI-encoded once
 *  - CanonicalHeaders include: host, Content-Type (if specified), all x-amz-*.
 *  - Payload is signed.
 *
 * Signer also assumes that query string parameters of the URI are listed
 * alphabetically and are already URI-encoded.
 */
class Aws_signer : public rest::Signer {
 public:
  explicit Aws_signer(const S3_bucket_config &config);

  Aws_signer(const Aws_signer &) = default;
  Aws_signer(Aws_signer &&) = default;

  Aws_signer &operator=(const Aws_signer &) = default;
  Aws_signer &operator=(Aws_signer &&) = default;

  ~Aws_signer() override = default;

  bool should_sign_request(const rest::Signed_request *) const override;

  rest::Headers sign_request(const rest::Signed_request *request,
                             time_t now) const override;

  bool refresh_auth_data() override;

  bool auth_data_expired(time_t now) const override;

  bool is_authorization_error(const rest::Response &response) const override;

 private:
#ifdef FRIEND_TEST
  friend class Aws_signer_test;
#endif  // FRIEND_TEST

  Aws_signer() = default;

  bool update_credentials();

  void set_credentials(std::shared_ptr<Aws_credentials> credentials);

  std::string m_host;
  std::string m_region;
  std::string m_service = "s3";
  bool m_sign_all_headers = false;
  Aws_credentials_provider *m_credentials_provider;
  std::shared_ptr<Aws_credentials> m_credentials;
  std::vector<unsigned char> m_secret_access_key;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_SIGNER_H_
