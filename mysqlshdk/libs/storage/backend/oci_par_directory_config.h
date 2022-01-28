/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_PAR_DIRECTORY_CONFIG_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_PAR_DIRECTORY_CONFIG_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/storage/config.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

class Oci_par_directory_config
    : public mysqlshdk::oci::PAR_config<mysqlshdk::oci::PAR_type::PREFIX> {
 public:
  using PAR_config::PAR_config;

 private:
  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &path) const override;

  std::unique_ptr<mysqlshdk::storage::IDirectory> directory(
      const std::string &path) const override;
};

using Oci_par_directory_config_ptr =
    std::shared_ptr<const Oci_par_directory_config>;

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_PAR_DIRECTORY_CONFIG_H_
