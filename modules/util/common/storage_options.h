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

#ifndef MODULES_UTIL_COMMON_STORAGE_OPTIONS_H_
#define MODULES_UTIL_COMMON_STORAGE_OPTIONS_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"

#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/storage/compressed_file.h"
#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlsh {
namespace common {

class Storage_options {
 public:
  enum class Storage_type {
    Local,
    Http,
    Oci_os,
    Oci_par,
    Oci_prefix_par,
    S3,
    Azure,
    Memory,
  };

  explicit Storage_options(bool reads_files);

  Storage_options(const Storage_options &other) = default;
  Storage_options(Storage_options &&other) = default;

  Storage_options &operator=(const Storage_options &other) = default;
  Storage_options &operator=(Storage_options &&other) = default;

  virtual ~Storage_options() = default;

  static const shcore::Option_pack_def<Storage_options> &options();

  /**
   * Fetches storage configuration for the given URL.
   *
   * @param url URL to a directory.
   * @param type Resultant storage type.
   *
   * @return Storage configuration.
   */
  mysqlshdk::storage::Config_ptr storage_config(
      const std::string &url, Storage_type *type = nullptr) const;

  /**
   * Creates a directory handle for a given URL. If URL has a scheme, it takes
   * priority over the configured storage.
   *
   * @param url URL to a directory.
   * @param type Resultant storage type.
   *
   * @return Created directory handle.
   */
  std::unique_ptr<mysqlshdk::storage::IDirectory> make_directory(
      const std::string &url, Storage_type *type = nullptr) const;

  /**
   * Creates a file handle for a given URL. If URL has a scheme, it takes
   * priority over the configured storage. Automatically wraps files which
   * have an extension that corresponds to one of the supported compression
   * algorithms.
   *
   * @param url URL to a file.
   * @param type Resultant storage type.
   *
   * @return Created file handle.
   */
  std::unique_ptr<mysqlshdk::storage::IFile> make_file(
      const std::string &url, Storage_type *type = nullptr) const;

  /**
   * Creates a file handle for a given URL. If URL has a scheme, it takes
   * priority over the configured storage. Uses the given compression.
   *
   * @param url URL to a file.
   * @param compression Compression to be used.
   * @param options Compression options.
   * @param type Resultant storage type.
   *
   * @return Created file handle.
   */
  std::unique_ptr<mysqlshdk::storage::IFile> make_file(
      const std::string &url, mysqlshdk::storage::Compression compression,
      const mysqlshdk::storage::Compression_options &options = {},
      Storage_type *type = nullptr) const;

  /**
   * Automatically wraps files which have an extension that corresponds to one
   * of the supported compression algorithms.
   *
   * @param f File handle.
   *
   * @return Wrapped file handle.
   */
  static std::unique_ptr<mysqlshdk::storage::IFile> make_file(
      std::unique_ptr<mysqlshdk::storage::IFile> f);

 protected:
  void set_storage_config(mysqlshdk::storage::Config_ptr storage,
                          Storage_type type);

  void throw_on_url_and_storage_conflict(const std::string &url,
                                         const char *context) const;

  const mysqlshdk::oci::Oci_bucket_options &oci_options() const noexcept {
    return m_oci_options;
  }

  const mysqlshdk::aws::S3_bucket_options &s3_options() const noexcept {
    return m_s3_options;
  }

  const mysqlshdk::azure::Blob_storage_options &azure_options() const noexcept {
    return m_azure_options;
  }

 private:
  void on_unpacked_options();

  const mysqlshdk::storage::backend::object_storage::Object_storage_options *
  storage_options() const noexcept;

  mysqlshdk::storage::Config_ptr config_for(const std::string &url,
                                            Storage_type *type) const;

  std::unique_ptr<mysqlshdk::storage::IFile> make_file_impl(
      const std::string &url, Storage_type *type) const;

  mysqlshdk::oci::Oci_bucket_options m_oci_options;
  mysqlshdk::aws::S3_bucket_options m_s3_options;
  mysqlshdk::azure::Blob_storage_options m_azure_options;

  mysqlshdk::storage::Config_ptr m_storage;
  Storage_type m_storage_type = Storage_type::Local;
};

}  // namespace common
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_STORAGE_OPTIONS_H_
