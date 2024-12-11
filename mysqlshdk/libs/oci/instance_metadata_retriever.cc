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

#include "mysqlshdk/libs/oci/instance_metadata_retriever.h"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/oci/oci_regions.h"

namespace mysqlshdk {
namespace oci {

namespace {

using shcore::ssl::Private_key;
using shcore::ssl::X509_certificate;

}  // namespace

Instance_metadata_retriever::Instance_metadata_retriever() {
  std::string uri = "http://169.254.169.254/opc/v2";

  log_info("Instance metadata URI: %s", uri.c_str());

  m_service = std::make_unique<rest::Rest_service>(std::move(uri));
  m_service->set_connect_timeout(10);
  m_service->set_timeout(60, 1, 60);

  m_retry_strategy = rest::Retry_strategy_builder(1, 2, 30, true)
                         .set_max_attempts(5)
                         .set_max_elapsed_time(5 * 60)  // 5 minutes
                         .retry_on_server_errors()
                         .build();
}

void Instance_metadata_retriever::refresh() {
  // instead of retrying each individual request, we retry all of them, trying
  // to maintain consistency of fetched data
  m_retry_strategy->reset();

  while (true) {
    try {
      log_debug("Fetching instance metadata");

      get_leaf_certificate();
      get_private_key();
      get_intermediate_certificate();

      log_debug("Fetching instance metadata - done");
      return;
    } catch (const rest::Connection_error &error) {
      if (m_retry_strategy->should_retry(error)) {
        wait_for_retry(error.what());
      } else {
        throw;
      }
    } catch (const rest::Response_error &error) {
      if (rest::Response::Status_code::BAD_GATEWAY == error.status_code()) {
        // it was not possible to resolve the host (usually via proxy)
        throw;
      }

      if (m_retry_strategy->should_retry(error)) {
        wait_for_retry(error.format().c_str());
      } else {
        throw;
      }
    }
  }
}

const std::string &Instance_metadata_retriever::region() const {
  if (m_region.empty()) {
    m_region = regions::from_short_name(get_region());

    log_debug("Region fetched from instance metadata: %s", m_region.c_str());
  }

  return m_region;
}

const std::string &Instance_metadata_retriever::instance_id() const {
  if (m_instance_id.empty()) {
    m_instance_id = get_instance_id();

    log_debug("Instance ID fetched from instance metadata: %s",
              m_instance_id.c_str());
  }

  return m_instance_id;
}

std::string Instance_metadata_retriever::tenancy_id() const {
  for (const auto &attribute : m_leaf_certificate->subject()) {
    if (shcore::str_beginswith(attribute.value, "opc-tenant:")) {
      return attribute.value.substr(11);
    }

    if (shcore::str_beginswith(attribute.value, "opc-identity:")) {
      return attribute.value.substr(13);
    }
  }

  throw std::runtime_error(
      "The leaf certificate does not contain a tenancy OCID");
}

void Instance_metadata_retriever::wait_for_retry(const char *msg) const {
  log_info("Failed to fetch instance metadata: %s, retrying", msg);
  m_retry_strategy->wait_for_retry();
}

std::string Instance_metadata_retriever::get(const std::string &path) const {
  rest::Request request{path, {{"Authorization", "Bearer Oracle"}}};
  auto response = m_service->get(&request);

  response.throw_if_error();

  return std::move(response.buffer).steal_buffer();
}

void Instance_metadata_retriever::get_leaf_certificate() {
  log_debug("Fetching leaf certificate");

  m_leaf_certificate = std::make_unique<X509_certificate>(
      X509_certificate::from_string(get("/identity/cert.pem")));
}

void Instance_metadata_retriever::get_private_key() {
  log_debug("Fetching private key");

  m_private_key = std::make_unique<Private_key>(
      Private_key::from_string(get("/identity/key.pem")));
}

void Instance_metadata_retriever::get_intermediate_certificate() {
  log_debug("Fetching intermediate certificate");

  m_intermediate_certificate = std::make_unique<X509_certificate>(
      X509_certificate::from_string(get("/identity/intermediate.pem")));
}

std::string Instance_metadata_retriever::get_region() const {
  return shcore::str_lower(shcore::str_strip(get("/instance/region")));
}

std::string Instance_metadata_retriever::get_instance_id() const {
  return shcore::str_lower(shcore::str_strip(get("/instance/id")));
}

}  // namespace oci
}  // namespace mysqlshdk
