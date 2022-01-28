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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_BUCKET_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_BUCKET_H_

#include <optional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/rest/signed_rest_service.h"
#include "mysqlshdk/libs/utils/enumset.h"

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

struct Multipart_object {
  std::string name;
  std::string upload_id;
};

struct Multipart_object_part {
  std::size_t part_num = 0;
  std::string etag;
  std::size_t size = 0;
};

struct Object_details {
  enum class Fields { NAME, SIZE, ETAG, TIME_CREATED };

  using Fields_mask = mysqlshdk::utils::Enum_set<Fields, Fields::TIME_CREATED>;

  static constexpr auto NAME = Fields_mask(Fields::NAME);
  static constexpr auto NAME_SIZE = NAME | Fields::SIZE;
  static constexpr auto ALL_FIELDS = Fields_mask::all();

  std::string name;
  std::size_t size = 0;
  std::string etag;
  std::string time_created;
};

class Bucket {
 public:
  Bucket() = delete;

  Bucket(const Bucket &) = delete;
  Bucket(Bucket &&) = default;

  Bucket &operator=(const Bucket &) = delete;
  Bucket &operator=(Bucket &&) = delete;

  virtual ~Bucket() = default;

  /**
   * Lists all objects in the bucket.
   *
   * @param prefix: List only objects with the specified prefix.
   * @param limit: Return up to the specified number of objects (0 - unlimited).
   * @param recursive: Recurse into subdirectories.
   * @param fields: Fields to fetch.
   * @param out_prefixes: If not recursive, names of the subdirectories will
   *                      be stored here.
   *
   * @returns A list of objects.
   */
  std::vector<Object_details> list_objects(
      const std::string &prefix = "", size_t limit = 0, bool recursive = true,
      const Object_details::Fields_mask &fields = Object_details::NAME_SIZE,
      std::unordered_set<std::string> *out_prefixes = nullptr);

  /**
   * Retrieves basic information from an object in the bucket.
   *
   * @param object_name: the name of the object for which to retrieve the
   * information.
   *
   * @returns Object size;
   *
   * @throws Response_error if the object does not exist.
   */
  size_t head_object(const std::string &object_name);

  /**
   * Deletes an object from the bucket.
   *
   * @param: object_name: the name of the object to be deleted.
   *
   * @throws Response_error if the given object does not exist.
   */
  void delete_object(const std::string &object_name);

  /**
   * Renames an object.
   *
   * @param src_name: Current name.
   * @param new_name: New name.
   */
  void rename_object(const std::string &src_name, const std::string &new_name);

  /**
   * Creates an object on the bucket.
   *
   * @param object_name: The name of the object to be created.
   * @param data: Buffer containing the information to be stored on the object.
   * @param size: The length of the data contained on the buffer.
   */
  void put_object(const std::string &object_name, const char *data,
                  size_t size);

  /**
   * Retrieves content data from an object.
   *
   * @param object_name: The object from which the data is to be retrieved.
   * @param buffer: A buffer where the data will be stored.
   * @param from_byte: First byte to be retrieved from the object.
   * @param to_byte: Last byte to be retrieved from the object.
   *
   * @returns The number of retrieved bytes.
   *
   * The from_byte and to_byte parameters are used to create a range of data to
   * be retrieved. The range definition has different meanings depending on the
   * presense of the two parameters:
   *
   * from-to: Retrieves the indicated range of data (inclusive-inclusive).
   * from-: Retrieves all the data starting at from.
   * -to: Retrieves the last 'to' bytes. Use optional<> overload.
   */
  size_t get_object(const std::string &object_name,
                    mysqlshdk::rest::Base_response_buffer *buffer,
                    const std::optional<size_t> &from_byte,
                    const std::optional<size_t> &to_byte);
  size_t get_object(const std::string &object_name,
                    mysqlshdk::rest::Base_response_buffer *buffer,
                    size_t from_byte, size_t to_byte);
  size_t get_object(const std::string &object_name,
                    mysqlshdk::rest::Base_response_buffer *buffer,
                    size_t from_byte);
  size_t get_object(const std::string &object_name,
                    mysqlshdk::rest::Base_response_buffer *buffer);

  // Multipart Handling

  /**
   * Retrieves information about multipart objects being uploaded.
   *
   * A multipart object is considered in uploading state after it is created and
   * before commit/abort is called.
   *
   * @param limit: Used to limit the number of object data retrieved.
   *
   * @returns A list of multipart objects.
   */
  std::vector<Multipart_object> list_multipart_uploads(size_t limit = 0);

  /**
   * Retrieves information about the parts of a multipart object being uploaded.
   *
   * @param object: The object for which the part information is to be
   * retrieved.
   * @param limit: To limit the number of parts retrieved, if not specified will
   * retrieve the information for all the uploaded parts.
   *
   * @returns the list of uploaded parts for the given object.
   */
  std::vector<Multipart_object_part> list_multipart_uploaded_parts(
      const Multipart_object &object, size_t limit = 0);

  /**
   * Initiates a multipart object upload.
   *
   * @param object_name: the name of the object to be uploaded.
   *
   * @returns a Multipart_object with the information of the object being
   * uploaded.
   */
  Multipart_object create_multipart_upload(const std::string &object_name);

  /**
   * Uploads a part for an object being uploaded.
   *    *
   * @param object: the multipart object data for which this part belongs.
   * @param part_num: an incremental identifier for the part, the object will be
   * assembled joining the parts in ascending order based on this identifier.
   *
   * @returns the part summary of the uploaded part.
   */
  Multipart_object_part upload_part(const Multipart_object &object,
                                    size_t part_num, const char *body,
                                    size_t size);

  /**
   * Finishes a multipart object upload.
   *
   * @param object: the multipart object to be completed
   * @param parts: the summary of the parts to be included on the object.
   */
  void commit_multipart_upload(const Multipart_object &object,
                               const std::vector<Multipart_object_part> &parts);

  /**
   * Aborts a multipart object upload.
   *
   * @param object_name: the name of the multipart object to be discarded.
   */
  void abort_multipart_upload(const Multipart_object &object);

  /**
   * Provides configuration of this bucket;
   *
   * @returns Bucket configuration.
   */
  const Config_ptr &config() const { return m_config; }

 protected:
  explicit Bucket(const Config_ptr &config);

  rest::Signed_rest_service *ensure_connection();

 private:
  virtual rest::Signed_request list_objects_request(
      const std::string &prefix, size_t limit, bool recursive,
      const Object_details::Fields_mask &fields,
      const std::string &start_from) = 0;

  virtual std::vector<Object_details> parse_list_objects(
      const rest::Base_response_buffer &buffer, std::string *next_start_from,
      std::unordered_set<std::string> *out_prefixes) = 0;

  virtual rest::Signed_request head_object_request(
      const std::string &object_name) = 0;

  virtual rest::Signed_request delete_object_request(
      const std::string &object_name) = 0;

  virtual rest::Signed_request put_object_request(
      const std::string &object_name, rest::Headers headers) = 0;

  virtual rest::Signed_request get_object_request(
      const std::string &object_name, rest::Headers headers) = 0;

  virtual void execute_rename_object(rest::Signed_rest_service *service,
                                     const std::string &src_name,
                                     const std::string &new_name) = 0;

  virtual rest::Signed_request list_multipart_uploads_request(size_t limit) = 0;

  virtual std::vector<Multipart_object> parse_list_multipart_uploads(
      const rest::Base_response_buffer &buffer) = 0;

  virtual rest::Signed_request list_multipart_uploaded_parts_request(
      const Multipart_object &object, size_t limit) = 0;

  virtual std::vector<Multipart_object_part>
  parse_list_multipart_uploaded_parts(
      const rest::Base_response_buffer &buffer) = 0;

  virtual rest::Signed_request create_multipart_upload_request(
      const std::string &object_name, std::string *request_body) = 0;

  virtual std::string parse_create_multipart_upload(
      const rest::Base_response_buffer &buffer) = 0;

  virtual rest::Signed_request upload_part_request(
      const Multipart_object &object, size_t part_num) = 0;

  virtual rest::Signed_request commit_multipart_upload_request(
      const Multipart_object &object,
      const std::vector<Multipart_object_part> &parts,
      std::string *request_body) = 0;

  virtual rest::Signed_request abort_multipart_upload_request(
      const Multipart_object &object) = 0;

  Config_ptr m_config;
  rest::Signed_rest_service *m_rest_service = nullptr;
  std::thread::id m_rest_service_thread;
};

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_BUCKET_H_
