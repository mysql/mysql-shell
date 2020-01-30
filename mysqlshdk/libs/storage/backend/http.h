/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

class Http_get : public IFile {
 public:
  Http_get() = delete;
  explicit Http_get(const std::string &uri);
  Http_get(const Http_get &other) = delete;
  Http_get(Http_get &&other) = default;

  Http_get &operator=(const Http_get &other) = delete;
  Http_get &operator=(Http_get &&other) = default;

  ~Http_get() = default;

  void open(Mode m) override;
  bool is_open() const override;
  int error() const override {
    throw std::logic_error("Http_get::error() - not implemented");
  }
  void close() override;

  size_t file_size() const override;
  std::string full_path() const override;

  std::string filename() const override {
    throw std::logic_error("Http_get::filename() - not implemented");
  }

  bool exists() const override {
    throw std::logic_error("Http_get::exists() - not implemented");
  }

  std::unique_ptr<IDirectory> parent() const override {
    throw std::logic_error("Http_get::parent() - not implemented");
  }

  off64_t seek(off64_t offset) override;
  off64_t tell() const override {
    throw std::logic_error("Http_get::tell() - not implemented");
  }
  ssize_t read(void *buffer, size_t length) override;

  ssize_t write(const void *, size_t) override {
    throw std::logic_error("Http_get::write() - not implemented");
  }

  bool flush() override {
    throw std::logic_error("Http_get::flush() - not implemented");
  }

  void rename(const std::string &) override {
    throw std::logic_error("Http_get::rename() - not implemented");
  }

  void remove() override {
    throw std::logic_error("Http_get::remove() - not implemented");
  }

 private:
  std::unique_ptr<mysqlshdk::rest::Rest_service> m_rest;
  off64_t m_offset = 0;
  mysqlshdk::rest::Response::Status_code m_open_status_code;
  size_t m_file_size = 0;
  std::string m_uri;
};

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_
