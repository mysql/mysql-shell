/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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
#include <optional>
#include <string>
#include <vector>

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

struct Http_request : public rest::Request {
 public:
  Http_request(Masked_string path, bool use_retry, rest::Headers headers = {});

 private:
  std::unique_ptr<rest::Retry_strategy> m_retry_strategy;
};

class Http_object : public IFile {
 public:
  Http_object() = delete;
  explicit Http_object(const std::string &full_path, bool use_retry = false);
  explicit Http_object(const Masked_string &full_path, bool use_retry = false);
  Http_object(const Masked_string &base, const std::string &path,
              bool use_retry = false);
  Http_object(const Http_object &other) = delete;
  Http_object(Http_object &&other) = default;

  Http_object &operator=(const Http_object &other) = delete;
  Http_object &operator=(Http_object &&other) = default;

  ~Http_object() = default;

  void open(Mode m) override;
  bool is_open() const override;

  int error() const override {
    throw std::logic_error("Http_object::error() - not implemented");
  }

  void close() override;

  size_t file_size() const override;

  Masked_string full_path() const override;

  std::string filename() const override;

  bool exists() const override;

  std::unique_ptr<IDirectory> parent() const override;

  off64_t seek(off64_t offset) override;

  off64_t tell() const override {
    throw std::logic_error("Http_object::tell() - not implemented");
  }

  ssize_t read(void *buffer, size_t length) override;

  ssize_t write(const void *, size_t) override;

  /**
   * Does NOTHING
   */
  bool flush() override { return true; }

  void rename(const std::string &) override {
    throw std::logic_error("Http_object::rename() - not implemented");
  }

  void remove() override;

  bool is_local() const override { return false; }

  void set_parent_config(const Config_ptr &config) { m_parent_config = config; }

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Http_object_test, full_path_constructor);
#endif  // FRIEND_TEST

  void throw_if_error(const std::optional<rest::Response_error> &error,
                      const std::string &context) const;

  std::optional<rest::Response_error> fetch_file_size() const;

  off64_t m_offset = 0;
  Masked_string m_base;
  std::string m_path;
  mutable bool m_exists = false;
  mutable size_t m_file_size = 0;
  std::optional<Mode> m_open_mode;
  bool m_use_retry = false;
  std::string m_buffer;
  Config_ptr m_parent_config;
};

class Http_directory : public IDirectory {
 public:
  Http_directory(const Http_directory &other) = delete;
  Http_directory(Http_directory &&other) = default;

  Http_directory &operator=(const Http_directory &other) = delete;
  Http_directory &operator=(Http_directory &&other) = default;

  ~Http_directory() override = default;

  bool exists() const override;

  void create() override;

  void close() override;

  Masked_string full_path() const override;

  std::unordered_set<IDirectory::File_info> list_files(
      bool hidden_files = false) const override;

  std::unordered_set<IDirectory::File_info> filter_files(
      const std::string &pattern) const override;

  std::unique_ptr<IFile> file(const std::string &name,
                              const File_options &options = {}) const override;

  bool is_local() const override;

  std::string join_path(const std::string &a,
                        const std::string &b) const override;

 protected:
  explicit Http_directory(const Config_ptr &config, bool use_retry = false)
      : m_config(config), m_use_retry(use_retry) {}
  void init_rest(const Masked_string &url);

  /**
   * Returns the URL to be used to pull the file list.
   *
   * The URL might either be exactly m_url or some modified version of it.
   */
  virtual std::string get_list_url() const { return ""; }

  /**
   * Parses the response from the list GET request to generate the file list
   */
  virtual std::unordered_set<IDirectory::File_info> parse_file_list(
      const std::string &data, const std::string &pattern = "") const = 0;

  virtual bool is_list_files_complete() const = 0;

 protected:
  std::unordered_set<IDirectory::File_info> get_file_list(
      const std::string &context, const std::string &pattern = "") const;

  Masked_string m_url;
  Config_ptr m_config;
  bool m_use_retry = false;
};

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_
