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

#ifndef MYSQLSHDK_LIBS_AZURE_SIGNER_H_
#define MYSQLSHDK_LIBS_AZURE_SIGNER_H_

#include <optional>
#include <string>
#include <vector>

#include "mysqlshdk/libs/rest/signed_rest_service.h"

#include "mysqlshdk/libs/azure/blob_storage_config.h"
#include "mysqlshdk/libs/db/generic_uri.h"

namespace mysqlshdk {
namespace azure {

enum class Allowed_services { BLOBS, QUEUE, FILES };

using Services =
    mysqlshdk::utils::Enum_set<Allowed_services, Allowed_services::FILES>;

enum class Allowed_resource_types { SERVICE, CONTAINER, OBJECT };

using Resource_types =
    mysqlshdk::utils::Enum_set<Allowed_resource_types,
                               Allowed_resource_types::OBJECT>;

constexpr Resource_types k_container_object =
    Resource_types(Allowed_resource_types::CONTAINER) |
    Allowed_resource_types::OBJECT;

enum class Allowed_permissions {
  READ,
  WRITE,
  DELETE,
  LIST,
  ADD,
  CREATE,
  UPDATE,
  PROCESS,
  TAG,
  FILTER,
  IMMUTABLE_STORAGE,
  DELETE_VERSION,
  PERMANENT_DELETE
};

using Permissions =
    mysqlshdk::utils::Enum_set<Allowed_permissions,
                               Allowed_permissions::PERMANENT_DELETE>;

constexpr auto k_read_write_list = Permissions(Allowed_permissions::READ) |
                                   Allowed_permissions::WRITE |
                                   Allowed_permissions::LIST;

constexpr auto k_read_list =
    Permissions(Allowed_permissions::READ) | Allowed_permissions::LIST;

class Signer : public rest::Signer {
 public:
  explicit Signer(const Blob_storage_config &config);

  Signer(const Signer &) = default;
  Signer(Signer &&) = default;

  Signer &operator=(const Signer &) = default;
  Signer &operator=(Signer &&) = default;

  ~Signer() override = default;

  bool should_sign_request(const rest::Signed_request *) const override;

  rest::Headers sign_request(const rest::Signed_request *request,
                             time_t now) const override;

  bool refresh_auth_data() override { return false; }

  bool auth_data_expired(time_t) const override { return false; }

  void set_secret_access_key(const std::string &key);

  std::string create_account_sas_token(
      const Permissions &permissions = k_read_write_list,
      const Resource_types &resource_types = k_container_object,
      const Services &services = Services(Allowed_services::BLOBS),
      int expiry_hours = 6) const;

 private:
#ifdef FRIEND_TEST
  friend class Azure_signer_test;
#endif  // FRIEND_TEST

  Signer() = default;
  rest::Headers get_required_headers(const rest::Signed_request *request,
                                     time_t now) const;
  std::string get_canonical_headers(const rest::Headers &headers) const;
  std::string get_canonical_resource(const rest::Signed_request *request) const;
  std::string get_string_to_sign(const rest::Signed_request *request,
                                 const rest::Headers &headers) const;

  std::string m_account_name;
  std::string m_account_key;
  bool m_using_sas_token;
  std::vector<unsigned char> m_secret_access_key;
};

}  // namespace azure
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AZURE_SIGNER_H_
