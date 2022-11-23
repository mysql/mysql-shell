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

#ifndef MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_OPTIONS_H_
#define MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_OPTIONS_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/storage/backend/object_storage_options.h"

namespace mysqlshdk {
namespace azure {

class Blob_storage_config;

class Blob_storage_options
    : public storage::backend::object_storage::Object_storage_options {
 public:
  enum class Operation { READ, WRITE };
  explicit Blob_storage_options(Operation operation)
      : m_operation{operation} {};

  Blob_storage_options(const Blob_storage_options &) = default;
  Blob_storage_options(Blob_storage_options &&) = default;

  Blob_storage_options &operator=(const Blob_storage_options &) = default;
  Blob_storage_options &operator=(Blob_storage_options &&) = default;

  ~Blob_storage_options() override = default;

  static constexpr const char *container_name_option() {
    return "azureContainerName";
  }

  static constexpr const char *config_file_option() {
    return "azureConfigFile";
  }

  static constexpr const char *storage_account_option() {
    return "azureStorageAccount";
  }

  static constexpr const char *storage_sas_token_option() {
    return "azureStorageSasToken";
  }

  static const shcore::Option_pack_def<Blob_storage_options> &options();

 private:
  friend class Blob_storage_config;

  const char *get_main_option() const override {
    return container_name_option();
  }
  std::vector<const char *> get_secondary_options() const override;
  bool has_value(const char *option) const override;

  std::shared_ptr<Blob_storage_config> azure_config() const;

  std::shared_ptr<storage::backend::object_storage::Config> create_config()
      const override;

  Operation m_operation;
  std::string m_storage_account;
  std::string m_storage_sas_token;
};

}  // namespace azure
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_OPTIONS_H_
