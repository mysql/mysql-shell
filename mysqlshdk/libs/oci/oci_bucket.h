/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_BUCKET_H_
#define MYSQLSHDK_LIBS_OCI_OCI_BUCKET_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/rest/signed_rest_service.h"
#include "mysqlshdk/libs/storage/backend/object_storage_bucket.h"

#include "mysqlshdk/libs/oci/oci_bucket_config.h"

namespace mysqlshdk {
namespace oci {

using storage::backend::object_storage::Multipart_object;
using storage::backend::object_storage::Multipart_object_part;
using storage::backend::object_storage::Object_details;

/**
 * C++ Implementation for Bucket operations through the OCI Object Store REST
 * API.
 */
class Oci_bucket : public storage::backend::object_storage::Bucket {
 public:
  Oci_bucket() = delete;

  /**
   * The Bucket class represents an bucket accessible through the OCI
   * configuration for the user.
   *
   * @param config: the configuration for the bucket.
   *
   * @throws Response_error in case the indicated bucket does not exist on the
   * indicated tenancy.
   */
  explicit Oci_bucket(const Oci_bucket_config_ptr &config);

  Oci_bucket(const Oci_bucket &other) = delete;
  Oci_bucket(Oci_bucket &&other) = delete;

  Oci_bucket &operator=(const Oci_bucket &other) = delete;
  Oci_bucket &operator=(Oci_bucket &&other) = delete;

  PAR create_pre_authenticated_request(
      PAR_access_type access_type, const std::string &expiration_time,
      const std::string &par_name, const std::string &object_name = "",
      std::optional<PAR_list_action> list_action = {});

  void delete_pre_authenticated_request(const std::string &id);

  std::vector<PAR> list_preauthenticated_requests(
      const std::string &prefix = "", size_t limit = 0,
      const std::string &page = "");

  bool exists();

  void create(const std::string &compartment_id);

  void delete_();

 private:
  rest::Signed_request create_request(const std::string &object_name,
                                      rest::Headers headers = {}) const;

  rest::Signed_request list_objects_request(
      const std::string &prefix, size_t limit, bool recursive,
      const Object_details::Fields_mask &fields,
      const std::string &start_from) override;

  std::vector<Object_details> parse_list_objects(
      const rest::Base_response_buffer &buffer, std::string *next_start_from,
      std::unordered_set<std::string> *out_prefixes) override;

  rest::Signed_request head_object_request(
      const std::string &object_name) override;

  rest::Signed_request delete_object_request(
      const std::string &object_name) override;

  rest::Signed_request put_object_request(const std::string &object_name,
                                          rest::Headers headers) override;

  rest::Signed_request get_object_request(const std::string &object_name,
                                          rest::Headers headers) override;

  void execute_rename_object(rest::Signed_rest_service *service,
                             const std::string &src_name,
                             const std::string &new_name) override;

  rest::Signed_request list_multipart_uploads_request(size_t limit) override;

  std::vector<Multipart_object> parse_list_multipart_uploads(
      const rest::Base_response_buffer &buffer) override;

  rest::Signed_request list_multipart_uploaded_parts_request(
      const Multipart_object &object, size_t limit) override;

  std::vector<Multipart_object_part> parse_list_multipart_uploaded_parts(
      const rest::Base_response_buffer &buffer) override;

  rest::Signed_request create_multipart_upload_request(
      const std::string &object_name, std::string *request_body) override;

  std::string parse_create_multipart_upload(
      const rest::Base_response_buffer &buffer) override;

  rest::Signed_request upload_part_request(const Multipart_object &object,
                                           size_t part_num) override;

  rest::Signed_request commit_multipart_upload_request(
      const Multipart_object &object,
      const std::vector<Multipart_object_part> &parts,
      std::string *request_body) override;

  rest::Signed_request abort_multipart_upload_request(
      const Multipart_object &object) override;

  Oci_bucket_config_ptr m_config;

  const std::string kNamespacePath;
  const std::string kBucketPath;
  const std::string kListObjectsPath;
  const std::string kObjectActionFormat;
  const std::string kRenameObjectPath;
  const std::string kMultipartActionPath;
  const std::string kUploadPartFormat;
  const std::string kMultipartActionFormat;
  const std::string kParActionPath;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_BUCKET_H_
