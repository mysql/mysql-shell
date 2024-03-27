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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REST_SERVICE_CONFIG_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REST_SERVICE_CONFIG_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/rest/signed/signer.h"

namespace mysqlshdk {
namespace rest {

class Signed_rest_service_config {
 public:
  Signed_rest_service_config() = default;

  Signed_rest_service_config(const Signed_rest_service_config &) = default;
  Signed_rest_service_config(Signed_rest_service_config &&) = default;

  Signed_rest_service_config &operator=(const Signed_rest_service_config &) =
      default;
  Signed_rest_service_config &operator=(Signed_rest_service_config &&) =
      default;

  virtual ~Signed_rest_service_config() = default;

  virtual const std::string &service_endpoint() const = 0;

  virtual const std::string &service_label() const = 0;

  virtual std::unique_ptr<ISigner> signer() const = 0;

  virtual bool signature_caching_enabled() const { return true; }

  virtual std::unique_ptr<IRetry_strategy> retry_strategy() const {
    return rest::default_retry_strategy();
  }
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REST_SERVICE_CONFIG_H_
