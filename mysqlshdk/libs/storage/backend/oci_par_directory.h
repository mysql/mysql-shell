/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_PAR_DIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_PAR_DIRECTORY_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/oci/oci_par.h"

#include "mysqlshdk/libs/storage/backend/http.h"
#include "mysqlshdk/libs/storage/backend/multipart_upload.h"
#include "mysqlshdk/libs/storage/backend/oci_par_directory_config.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

class Oci_par_file : public Http_object {
 public:
  Oci_par_file(const Oci_par_directory_config_ptr &config,
               const Masked_string &base, const std::string &path);

  Oci_par_file &operator=(const Oci_par_file &other) = delete;
  Oci_par_file &operator=(Oci_par_file &&other) = default;

  ~Oci_par_file() override;

  int error() const override { return 0; }

  off64_t tell() const override { return m_offset; }

  ssize_t write(const void *, size_t) override;

 private:
  class Multipart_upload_helper;

  void flush_on_close() override;

  std::unique_ptr<Multipart_upload_helper> m_helper;
  Multipart_uploader m_uploader;
};

class Oci_par_directory : public Http_directory {
 public:
  explicit Oci_par_directory(const Oci_par_directory_config_ptr &config);

  Oci_par_directory(const Oci_par_directory &other) = delete;
  Oci_par_directory(Oci_par_directory &&other) = default;

  Oci_par_directory &operator=(const Oci_par_directory &other) = delete;
  Oci_par_directory &operator=(Oci_par_directory &&other) = delete;

  ~Oci_par_directory() override {}

  Masked_string full_path() const override;

  bool exists() const override;

  void create() override;

  std::unique_ptr<IFile> file(const std::string &name,
                              const File_options &options = {}) const override;

 private:
  std::string get_list_url() const override;

  bool is_list_files_complete() const override;

  void init_rest();

  std::unordered_set<IDirectory::File_info> parse_file_list(
      const std::string &data, const std::string &pattern = "") const override;

  Oci_par_directory_config_ptr m_config;

  mutable std::string m_next_start_with;

  mutable std::optional<bool> m_exists;
};

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_PAR_DIRECTORY_H_
