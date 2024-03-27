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

#include "mysqlshdk/libs/aws/sts_client.h"

#include <tinyxml2.h>

#include <utility>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/rest/rest_utils.h"
#include "mysqlshdk/libs/rest/signed/signed_request.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

namespace {

Assume_role_response parse_assume_role(const rest::String_response &response) {
  const auto xml = rest::parse_xml(response.buffer);
  const auto root = xml->FirstChildElement("AssumeRoleResponse");

  if (!root) {
    throw shcore::Exception::runtime_error(
        "Expected root element called 'AssumeRoleResponse'");
  }

  const auto result = root->FirstChildElement("AssumeRoleResult");

  if (!result) {
    throw shcore::Exception::runtime_error(
        "'AssumeRoleResponse' does not have a 'AssumeRoleResult' element");
  }

  const auto credentials = result->FirstChildElement("Credentials");

  if (!credentials) {
    throw shcore::Exception::runtime_error(
        "'AssumeRoleResponse'.'AssumeRoleResult' does not have a 'Credentials' "
        "element");
  }

  const auto get_value = [credentials](const char *name, std::string *tgt) {
    if (const auto value = credentials->FirstChildElement(name)) {
      *tgt = value->GetText();
    } else {
      throw shcore::Exception::runtime_error(
          shcore::str_format("'AssumeRoleResponse'.'AssumeRoleResult'.'"
                             "Credentials' does not have a '%s' element",
                             name));
    }
  };

  Assume_role_response r;

  get_value("AccessKeyId", &r.access_key_id);
  get_value("SecretAccessKey", &r.secret_access_key);
  get_value("SessionToken", &r.session_token);
  get_value("Expiration", &r.expiration);

  return r;
}

}  // namespace

Sts_client::Sts_client(const std::string &region,
                       Aws_credentials_provider *provider)
    : m_region(region), m_provider(provider) {
  m_host += "sts." + this->region() + ".amazonaws.com";
  m_endpoint = "https://" + m_host;
}

std::unique_ptr<rest::ISigner> Sts_client::signer() const {
  return std::make_unique<Aws_signer>(*this);
}

rest::Signed_rest_service *Sts_client::http() {
  if (!m_http) {
    m_http = std::make_unique<rest::Signed_rest_service>(*this);
  }

  return m_http.get();
}

Assume_role_response Sts_client::assume_role(const Assume_role_request &r) {
  rest::Query query;

  query.emplace("RoleArn", r.arn);
  query.emplace("RoleSessionName", r.session_name);

  if (r.duration_seconds.has_value()) {
    query.emplace("DurationSeconds", std::to_string(*r.duration_seconds));
  }

  if (!r.external_id.empty()) {
    query.emplace("ExternalId", r.external_id);
  }

  return parse_assume_role(post("AssumeRole", std::move(query)));
}

rest::String_response Sts_client::post(std::string method, rest::Query query) {
  query.emplace("Action", std::move(method));
  query.emplace("Version", "2011-06-15");

  const auto body = rest::www_form_urlencoded(query);
  rest::Signed_request request{
      "/", {{"Content-Type", "application/x-www-form-urlencoded"}}};

  request.body = body.c_str();
  request.size = body.length();

  rest::String_response response;

  http()->post(&request, &response);

  return response;
}

}  // namespace aws
}  // namespace mysqlshdk
