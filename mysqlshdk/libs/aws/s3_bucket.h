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

#ifndef MYSQLSHDK_LIBS_AWS_S3_BUCKET_H_
#define MYSQLSHDK_LIBS_AWS_S3_BUCKET_H_

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/rest/signed_rest_service.h"
#include "mysqlshdk/libs/storage/backend/object_storage_bucket.h"

#include "mysqlshdk/libs/aws/s3_bucket_config.h"

namespace mysqlshdk {
namespace aws {

using storage::backend::object_storage::Multipart_object;
using storage::backend::object_storage::Multipart_object_part;
using storage::backend::object_storage::Object_details;

class S3_bucket : public storage::backend::object_storage::Container {
 public:
  S3_bucket() = delete;

  explicit S3_bucket(const S3_bucket_config_ptr &config);

  S3_bucket(const S3_bucket &other) = delete;
  S3_bucket(S3_bucket &&other) = delete;

  S3_bucket &operator=(const S3_bucket &other) = delete;
  S3_bucket &operator=(S3_bucket &&other) = delete;

  bool exists();

  void create();

  void delete_();

  void copy_object(const std::string &src_name, const std::string &new_name);

  void copy_object_multipart(const std::string &src_name,
                             const std::string &new_name,
                             std::size_t total_size, std::size_t part_size);

  void delete_objects(const std::vector<Object_details> &list);

  void delete_objects(const std::vector<std::string> &list);

 private:
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
      const rest::String_response &response) override;

  rest::Signed_request upload_part_request(const Multipart_object &object,
                                           size_t part_num,
                                           size_t size) override;

  rest::Signed_request commit_multipart_upload_request(
      const Multipart_object &object,
      const std::vector<Multipart_object_part> &parts,
      std::string *request_body) override;

  rest::Signed_request abort_multipart_upload_request(
      const Multipart_object &object) override;

  rest::Signed_request create_bucket_request(
      const rest::Query &query = {}) const;

  rest::Signed_request create_object_request(const std::string &name,
                                             rest::Headers headers = {}) const;

  rest::Signed_request create_object_request(const std::string &name,
                                             const rest::Query &query) const;

  std::string add_query_parameters(const std::string &path,
                                   const rest::Query &query) const;

  std::string object_path(const std::string &name) const;

  std::string copy_source_header(const std::string &name) const;

  template <typename T>
  void delete_objects(
      const std::vector<T> &list,
      const std::function<const std::string &(const T &)> &get_name);

  S3_bucket_config_ptr m_config;

  const std::string m_bucket_path;
  const std::string m_object_path_prefix;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_S3_BUCKET_H_
