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

#ifndef MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_CONTAINER_H_
#define MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_CONTAINER_H_

#include <optional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/rest/signed_rest_service.h"
#include "mysqlshdk/libs/utils/enumset.h"

#include "mysqlshdk/libs/azure/blob_storage_config.h"
#include "mysqlshdk/libs/storage/backend/object_storage_bucket.h"

namespace mysqlshdk {
namespace azure {

using mysqlshdk::storage::backend::object_storage::Container;
using mysqlshdk::storage::backend::object_storage::Multipart_object;
using mysqlshdk::storage::backend::object_storage::Multipart_object_part;
using mysqlshdk::storage::backend::object_storage::Object_details;
using rest::Base_response_buffer;
using rest::Headers;
using rest::Query;
using rest::Signed_request;

class Blob_container : public Container {
 public:
  explicit Blob_container(const Storage_config_ptr &config);

  Blob_container() = delete;

  Blob_container(const Blob_container &) = delete;
  Blob_container(Blob_container &&) = default;

  Blob_container &operator=(const Blob_container &) = delete;
  Blob_container &operator=(Blob_container &&) = delete;

  ~Blob_container() override = default;

  bool exists();

  void create();

  void delete_();

  void abort_multipart_upload(const Multipart_object &object) override;

  Signed_request abort_multipart_upload_request(
      const Multipart_object & /*object*/) override {
    return rest::Signed_request("");
  }

  bool has_object_rename() const override { return false; }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Azure_signer_test, azure_requests);
#endif
  std::string blob_container_path() const;
  std::string object_path(const std::string &name) const;

  void add_sas_token(mysqlshdk::rest::Query *query) const;

  Signed_request create_blob_container_request(Headers headers = {},
                                               Query query = {}) const;

  Signed_request create_blob_request(const std::string &name,
                                     Headers headers = {},
                                     Query query = {}) const;

  Signed_request list_objects_request(const std::string &prefix, size_t limit,
                                      bool recursive,
                                      const Object_details::Fields_mask &fields,
                                      const std::string &start_from) override;

  std::vector<Object_details> parse_list_objects(
      const Base_response_buffer &buffer, std::string *next_start_from,
      std::unordered_set<std::string> *out_prefixes) override;

  // NOTE: Listing of multipart uploads is only possible through the list blobs
  // call, including uncommitted blobs, so this helper function is needed to
  // define what should be returned from the listed objects
  std::vector<Object_details> parse_list_objects(
      const Base_response_buffer &buffer, std::string *next_start_from,
      std::unordered_set<std::string> *out_prefixes,
      bool only_multipart_uploads);

  Signed_request head_object_request(const std::string &object_name) override;

  Signed_request delete_object_request(const std::string &object_name) override;

  Signed_request put_object_request(const std::string &object_name,
                                    Headers headers) override;

  Signed_request get_object_request(const std::string &object_name,
                                    Headers headers) override;

  void execute_rename_object(rest::Signed_rest_service * /*service*/,
                             const std::string & /*src_name*/,
                             const std::string & /*new_name*/) override {
    throw std::runtime_error(
        "The rename_object operation is not supported in Azure.");
  }

  Signed_request list_multipart_uploads_request(size_t limit) override;

  std::vector<Multipart_object> parse_list_multipart_uploads(
      const Base_response_buffer &buffer) override;

  rest::Type multipart_request_method() const override {
    return rest::Type::PUT;
  }

  Signed_request list_multipart_uploaded_parts_request(
      const Multipart_object &object, size_t limit) override;

  std::vector<Multipart_object_part> parse_list_multipart_uploaded_parts(
      const Base_response_buffer &buffer) override;

  std::string get_create_multipart_upload_content() const override {
    return "-";
  }

  Signed_request create_multipart_upload_request(
      const std::string &object_name, std::string *request_body) override;

  std::string parse_create_multipart_upload(
      const rest::String_response &response) override;

  void on_multipart_upload_created(const std::string &object_name) override;

  Signed_request upload_part_request(const Multipart_object &object,
                                     size_t part_num, size_t size) override;

  std::string parse_multipart_upload(const rest::Response &response) override;

  Signed_request commit_multipart_upload_request(
      const Multipart_object &object,
      const std::vector<Multipart_object_part> &parts,
      std::string *request_body) override;

  void validate_range(
      const std::optional<size_t> &from_byte,
      const std::optional<size_t> & /*to_byte*/) const override {
    if (!from_byte.has_value()) {
      throw std::invalid_argument(
          "Retrieving partial object requires starting offset.");
    }
  }

  Storage_config_ptr m_storage_config;
  std::string m_blob_container_path;
};

}  // namespace azure
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_CONTAINER_H_
