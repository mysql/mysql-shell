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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_OCI_CREDENTIALS_PROVIDER_H_

#include <cassert>
#include <ctime>
#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/rest/signed/credentials_provider.h"

#include "mysqlshdk/libs/oci/oci_credentials.h"

namespace mysqlshdk {
namespace oci {

class Oci_credentials_provider;

struct Oci_credentials_provider_traits {
  using Provider_t = Oci_credentials_provider;
  using Credentials_t = Oci_credentials;

  struct Intermediate_credentials {
    std::string auth_key_id;
    std::string private_key_id;
    std::optional<std::time_t> expiration{};
  };

  static std::shared_ptr<Credentials_t> convert(
      const Provider_t &self, const Intermediate_credentials &credentials);
};

class Oci_credentials_provider
    : public rest::Credentials_provider<Oci_credentials_provider_traits> {
 public:
  static constexpr auto s_storage_name = "OCI";

  Oci_credentials_provider(const Oci_credentials_provider &) = delete;
  Oci_credentials_provider(Oci_credentials_provider &&) = delete;

  Oci_credentials_provider &operator=(const Oci_credentials_provider &) =
      delete;
  Oci_credentials_provider &operator=(Oci_credentials_provider &&) = delete;

  inline const std::string &region() const noexcept {
    assert(!m_region.empty());
    return m_region;
  }

  inline const std::string &tenancy_id() const noexcept {
    assert(!m_tenancy_id.empty());
    return m_tenancy_id;
  }

  bool available() const noexcept override { return true; }

 protected:
  explicit Oci_credentials_provider(std::string name);

  static Credentials_ptr_t get_current_credentials(
      const Oci_credentials_provider &provider) {
    return provider.current_credentials();
  }

  void set_region(std::string region);

  void set_tenancy_id(std::string tenancy_id);

 private:
  std::string m_region;
  std::string m_tenancy_id;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_CREDENTIALS_PROVIDER_H_
