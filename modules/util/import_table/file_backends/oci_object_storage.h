/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_OCI_OBJECT_STORAGE_H_
#define MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_OCI_OBJECT_STORAGE_H_

#include <openssl/evp.h>
#include <memory>
#include <string>
#include "modules/util/import_table/file_backends/ifile.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/rest/rest_service.h"

namespace mysqlsh {
namespace import_table {

class Oci_object_storage : public IFile {
 public:
  Oci_object_storage() = delete;
  explicit Oci_object_storage(const std::string &uri);
  Oci_object_storage(const Oci_object_storage &other) = delete;
  Oci_object_storage(Oci_object_storage &&other) = delete;

  Oci_object_storage &operator=(const Oci_object_storage &other) = delete;
  Oci_object_storage &operator=(Oci_object_storage &&other) = delete;

  ~Oci_object_storage() override;

  void open() override;
  bool is_open() override;
  void close() override;

  size_t file_size() override;
  std::string file_name() override;
  off64_t seek(off64_t offset) override;
  ssize_t read(void *buffer, size_t length) override;

 private:
  void parse_uri(const std::string &uri);

  /**
   * Create, sign and cache OCI specific REST API headers.
   *
   * @param is_get_request true if we sign HTTP GET request, false if we sign
   * HTTP HEAD request.
   * @return Headers
   */
  mysqlshdk::rest::Headers make_signed_header(bool is_get_request = true);

  /**
   * Return empty or signed header depending on is_public_bucket flag.
   *
   * @param is_get_request true for GET request, false for HEAD request.
   * @return Headers object
   */
  mysqlshdk::rest::Headers make_header(bool is_get_request = true);
  time_t m_signed_header_cache_time[2] = {0, 0};
  mysqlshdk::rest::Headers m_cached_header[2];

  std::unique_ptr<mysqlshdk::rest::Rest_service> m_rest;
  off64_t m_offset = 0;
  mysqlshdk::rest::Response::Status_code m_open_status_code;
  size_t m_file_size = 0;
  std::string m_oci_uri;
  bool is_public_bucket = true;

  struct Uri {
    std::string region;
    std::string tenancy;  //< other name namespace
    std::string bucket;
    std::string object;
    std::string hostname;
    std::string path;
  };
  Uri m_uri;

  std::string m_tenancy_id;
  std::string m_user_id;
  std::string m_fingerprint;
  std::string m_auth_keyId;
  std::string m_key_file;
  std::shared_ptr<EVP_PKEY> m_private_key;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_OCI_OBJECT_STORAGE_H_
