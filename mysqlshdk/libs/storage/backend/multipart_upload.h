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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_MULTIPART_UPLOAD_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_MULTIPART_UPLOAD_H_

#include <string>
#include <utility>

namespace mysqlshdk {
namespace storage {
namespace backend {

/**
 * An interface which performs multipart upload operations.
 */
class IMultipart_upload_helper {
 public:
  virtual ~IMultipart_upload_helper() = default;

  /**
   * Performs a non-multipart upload.
   */
  virtual void upload(const char *body, std::size_t size) = 0;

  /**
   * Creates a multipart upload.
   */
  virtual void create() = 0;

  /**
   * Checks if multipart upload is in progress.
   */
  virtual bool is_active() const = 0;

  /**
   * Uploads next part of a multipart upload.
   */
  virtual void upload_part(const char *body, std::size_t size) = 0;

  /**
   * Commits the multipart upload.
   */
  virtual void commit() = 0;

  /**
   * Aborts the multipart upload.
   */
  virtual void abort() = 0;

  /**
   * Cleans up the multipart upload.
   */
  virtual void cleanup() = 0;

  /**
   * Provides name of the object being uploaded.
   */
  virtual const std::string &name() const = 0;

  /**
   * Provides ID of the multipart upload.
   */
  virtual const std::string &upload_id() const = 0;
};

/**
 * Executes a multipart upload.
 */
class Multipart_uploader final {
 public:
  /**
   * Creates the uploader.
   *
   * @param helper Runs multipart upload commands.
   * @param part_size Size of a single part.
   */
  Multipart_uploader(IMultipart_upload_helper *helper, std::size_t part_size)
      : m_multipart_upload(helper), m_part_size(part_size) {}

  Multipart_uploader(const Multipart_uploader &) = delete;
  Multipart_uploader(Multipart_uploader &&) = default;

  Multipart_uploader &operator=(const Multipart_uploader &) = delete;
  Multipart_uploader &operator=(Multipart_uploader &&) = default;

  ~Multipart_uploader();

  /**
   * Appends more data to the upload.
   */
  void append(const void *data, std::size_t length);

  /**
   * Commits the upload, flushes all buffers. If not enough data for a single
   * part was provided, uploads the data in one (non-multipart) upload.
   */
  void commit();

 private:
  void maybe_abort(const char *context, const std::string &error = {});

  IMultipart_upload_helper *m_multipart_upload;
  std::size_t m_part_size;
  std::string m_buffer;
};

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_MULTIPART_UPLOAD_H_
