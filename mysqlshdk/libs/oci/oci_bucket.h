/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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
#include <vector>

#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/oci/oci_rest_service.h"
#include "mysqlshdk/libs/utils/enumset.h"

namespace mysqlshdk {
namespace oci {
enum class Object_fields { NAME, SIZE, ETAG, MD5, TIME_CREATED };

using Object_fields_mask =
    mysqlshdk::utils::Enum_set<Object_fields, Object_fields::TIME_CREATED>;

namespace object_fields {
const Object_fields_mask kNameSize =
    Object_fields_mask(Object_fields::NAME).set(Object_fields::SIZE);

const Object_fields_mask kName = Object_fields_mask(Object_fields::NAME);

const Object_fields_mask kAll = Object_fields_mask::all();
}  // namespace object_fields

struct Multipart_object {
  std::string name;
  std::string upload_id;
};

struct Multipart_object_part {
  size_t part_num = 0;
  std::string etag;
  size_t size = 0;
};

struct Object_details {
  std::string name;
  size_t size = 0;
  std::string etag;
  std::string md5;
  std::string time_created;
};

class Object;
/**
 * C++ Implementation for Bucket operations through the OCI Object Store REST
 * API.
 *
 * This object would create an instance of the Oci_rest_service in the context
 * of the OCI configuration path and OCI profile available on the shell options.
 */
class Bucket : public std::enable_shared_from_this<Bucket> {
 public:
  Bucket() = delete;

  /**
   * The Bucket class represents an bucket accessible through the OCI
   * configuration for the user.
   *
   * @param bucketName: the name of the bucket to be represented through the
   * created instance.
   *
   * @throw Response_error in case the indicated bucket does not exist on the
   * indicated tenancy.
   */
  explicit Bucket(const Oci_options &options);
  Bucket(const Bucket &other) = delete;
  Bucket(Bucket &&other) = delete;
  Bucket &operator=(const Bucket &other) = delete;
  Bucket &operator=(Bucket &&other) = delete;
  /**
   * Accessor functions for the information retrieved throgh the GetBucket REST
   * API.
   *
   * NOTE: Only part of the information returned was made available, additional
   * information should be added as needed.
   */
  const std::string &get_namespace() const { return *m_options.os_namespace; }
  const std::string &get_name() const { return *m_options.os_bucket_name; }
  const std::string &get_compartment_id() const { return m_compartment_id; }
  const std::string &get_etag() const { return m_etag; }
  bool is_read_only() const { return m_is_read_only; }
  const std::string &get_access_type() const { return m_access_type; }

  /**
   * Retrieves information for the objects available on the bucket.
   *
   * For additional information regarding the parameters look at the ListObjects
   * REST API documentation.
   *
   * NOTE: The ListObjects REST API limits the number of returned records to
   * 1000, note that such limitation is removed on this implementation which
   * will only limit the returned data if it is indicated specifying a limit
   * greather than 0.
   */
  std::vector<Object_details> list_objects(
      const std::string &prefix = "", const std::string &start = "",
      const std::string &end = "", size_t limit = 0, bool recursive = true,
      const Object_fields_mask &fields = object_fields::kNameSize,
      std::vector<std::string> *out_prefixes = nullptr,
      std::string *out_nextStart = nullptr);

  /**
   * Creates an object on the bucket.
   *
   * @param objectName: The name of the object to be created.
   * @param data: Buffer containing the information to be stored on the object.
   * @param size: The length of the data contained on the buffer.
   * @param override: Flag to indicate the object should be overwritten if
   * already exists.
   */
  void put_object(const std::string &objectName, const char *data, size_t size,
                  bool override = true);

  /**
   * Deletes an object from the bucket.
   *
   * @param: objectName: the name of the object to be deleted.
   *
   * @throw Response_error if the given object does not exist.
   */
  void delete_object(const std::string &objectName);

  /**
   * Retrieves basic information from an object in the bucket.
   *
   * @param objectName: the name of the object for which to retrieve the
   * information.
   *
   * NOTE: This function is returning just the size, additional information is
   * available but should be added only if needed. For additional information
   * look at the HeadObject REST API.
   *
   * @throw Response_error if the object does not exist.
   */
  size_t head_object(const std::string &objectName);

  /**
   * Retrieves content data from an object.
   *
   * @param objectName: The object from which the data is to be retrieved.
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
   * -to: Retrieves the last 'to' bytes. Use nullable<> overload.
   */
  size_t get_object(const std::string &objectName,
                    mysqlshdk::rest::Base_response_buffer *buffer,
                    const mysqlshdk::utils::nullable<size_t> &from_byte,
                    const mysqlshdk::utils::nullable<size_t> &to_byte);
  size_t get_object(const std::string &objectName,
                    mysqlshdk::rest::Base_response_buffer *buffer,
                    size_t from_byte, size_t to_byte);
  size_t get_object(const std::string &objectName,
                    mysqlshdk::rest::Base_response_buffer *buffer,
                    size_t from_byte);
  size_t get_object(const std::string &objectName,
                    mysqlshdk::rest::Base_response_buffer *buffer);

  void rename_object(const std::string &srcName, const std::string &newName);

  // Multipart Handling
  /**
   * Retrieves information about multipart objects being uploaded.
   *
   * A multipart object is considered in uploading state after it is created and
   * before commit/abort is called.
   *
   * @param limit: Used to limit the number of object data retrieved.
   *
   * @returns A list with the name and etag for the objects being uoloaded.
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
  std::vector<Multipart_object_part> list_multipart_upload_parts(
      const Multipart_object &object, size_t limit = 0);

  /**
   * Initiates a multipart object upload.
   *
   * @param objectName: the name of the object to be uploaded.
   *
   * @returns a Multipart_object with the information of the object being
   * uploaded.
   */
  Multipart_object create_multipart_upload(const std::string &objectName);

  /**
   * Uploads a part for an object being uploaded.
   *    *
   * @param object: the multipart object data for which this part belongs.
   * @param partNum: an incremental identifier for the part, the object will be
   * assembled joining the parts in ascending order based on this identifier.
   *
   * @returns the part summary of the uploaded part.
   */
  Multipart_object_part upload_part(const Multipart_object &object,
                                    size_t partNum, const char *body,
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
   * @param objectName: the name of the multipart object to be discarded.
   */
  void abort_multipart_upload(const Multipart_object &object);

  std::shared_ptr<Oci_rest_service> get_rest_service() {
    return m_rest_service;
  }

  const Oci_options &get_options() { return m_options; }

 private:
  Oci_options m_options;
  std::shared_ptr<Oci_rest_service> m_rest_service;

  const std::string kBucketPath;
  const std::string kListObjectsPath;
  const std::string kObjectActionFormat;
  const std::string kRenameObjectPath;
  const std::string kMultipartActionPath;
  const std::string kUploadPartFormat;
  const std::string kMultipartActionFormat;

  // NOTE: Minimal list of fields is read and stored, for full list of available
  // fields look at
  // https://docs.cloud.oracle.com/en-us/iaas/api/#/en/objectstorage/20160918/Bucket/
  std::string m_compartment_id;
  std::string m_etag;
  bool m_is_read_only;
  std::string m_access_type;
};
}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_BUCKET_H_
