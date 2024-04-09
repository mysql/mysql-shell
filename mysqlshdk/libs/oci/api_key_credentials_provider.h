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

#ifndef MYSQLSHDK_LIBS_OCI_API_KEY_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_API_KEY_CREDENTIALS_PROVIDER_H_

#include <string>

#include "mysqlshdk/libs/oci/config_credentials_provider.h"

namespace mysqlshdk {
namespace oci {

/**
 * Provides API key-based authentication, authentication details are read from
 * the config file.
 */
class Api_key_credentials_provider final : public Config_credentials_provider {
 public:
  Api_key_credentials_provider(const std::string &config_file,
                               const std::string &config_profile);

  Api_key_credentials_provider(const Api_key_credentials_provider &) = delete;
  Api_key_credentials_provider(Api_key_credentials_provider &&) = delete;

  Api_key_credentials_provider &operator=(
      const Api_key_credentials_provider &) = delete;
  Api_key_credentials_provider &operator=(Api_key_credentials_provider &&) =
      delete;

 private:
  Credentials fetch_credentials() override;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_API_KEY_CREDENTIALS_PROVIDER_H_
