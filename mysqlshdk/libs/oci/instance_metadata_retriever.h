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

#ifndef MYSQLSHDK_LIBS_OCI_INSTANCE_METADATA_RETRIEVER_H_
#define MYSQLSHDK_LIBS_OCI_INSTANCE_METADATA_RETRIEVER_H_

#include <cinttypes>
#include <memory>
#include <string>

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_x509.h"

namespace mysqlshdk {
namespace oci {

/**
 * Retrieves data from instance metadata service (IMDS).
 */
class Instance_metadata_retriever final {
 public:
  /**
   * Creates the retriever.
   */
  Instance_metadata_retriever();

  Instance_metadata_retriever(const Instance_metadata_retriever &) = delete;
  Instance_metadata_retriever(Instance_metadata_retriever &&) = delete;

  Instance_metadata_retriever &operator=(const Instance_metadata_retriever &) =
      delete;
  Instance_metadata_retriever &operator=(Instance_metadata_retriever &&) =
      delete;

  /**
   * Refreshes the private key and the certificates.
   */
  void refresh();

  inline const shcore::ssl::X509_certificate &leaf_certificate()
      const noexcept {
    return *m_leaf_certificate;
  }

  inline const shcore::ssl::Private_key &private_key() const noexcept {
    return *m_private_key;
  }

  inline const shcore::ssl::X509_certificate &intermediate_certificate()
      const noexcept {
    return *m_intermediate_certificate;
  }

  const std::string &region() const;

  const std::string &instance_id() const;

  std::string tenancy_id() const;

 private:
  void wait_for_retry(const char *msg) const;

  std::string get(const std::string &path) const;

  void get_leaf_certificate();

  void get_private_key();

  void get_intermediate_certificate();

  std::string get_region() const;

  std::string get_instance_id() const;

  std::unique_ptr<rest::Rest_service> m_service;
  std::unique_ptr<rest::IRetry_strategy> m_retry_strategy;

  std::unique_ptr<shcore::ssl::X509_certificate> m_leaf_certificate;
  std::unique_ptr<shcore::ssl::Private_key> m_private_key;
  std::unique_ptr<shcore::ssl::X509_certificate> m_intermediate_certificate;

  mutable std::string m_region;
  mutable std::string m_instance_id;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_INSTANCE_METADATA_RETRIEVER_H_
