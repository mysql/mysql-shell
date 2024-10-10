/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
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
  std::unique_ptr<rest::IRetry_strategy> m_retry_strategy;
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
    // this throws exceptions
    return 0;
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

 protected:
#ifdef FRIEND_TEST
  FRIEND_TEST(Http_object_test, full_path_constructor);
#endif  // FRIEND_TEST

  void throw_if_error(const std::optional<rest::Response_error> &error,
                      const std::string &context) const;

  virtual void flush_on_close();

  mysqlshdk::rest::Rest_service *rest_service() const;

  Http_request http_request(Masked_string path,
                            rest::Headers headers = {}) const;

  void put(const char *data, std::size_t length);

  std::optional<rest::Response_error> fetch_file_size() const;

  void set_full_path(const Masked_string &full_path);

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

  void remove() override;

  bool is_empty() const override;

  Masked_string full_path() const override;

  Directory_listing list(bool hidden_files = false) const override;

  std::unique_ptr<IFile> file(const std::string &name,
                              const File_options &options = {}) const override;

  bool is_local() const override;

  std::string join_path(const std::string &a,
                        const std::string &b) const override;

 protected:
  class Listing_context {
   public:
    explicit Listing_context(size_t limit) : m_limit(limit) {}

    virtual ~Listing_context() = default;

    /**
     * Returns the URL to be used to pull the directory listing.
     */
    virtual std::string get_url() const = 0;

    /**
     * Parses the response from the list GET request to generate the entry list
     */
    virtual Directory_listing parse(const std::string &data) const = 0;

    /**
     * Checks if listing is complete.
     */
    virtual bool is_complete() const = 0;

   protected:
    std::size_t limit() const noexcept { return m_limit; }

   private:
    std::size_t m_limit;
  };

  explicit Http_directory(const Config_ptr &config, bool use_retry = false)
      : m_config(config), m_use_retry(use_retry) {}

  void init_rest(const Masked_string &url);

  virtual std::unique_ptr<Listing_context> listing_context(
      size_t limit) const = 0;

 protected:
  Directory_listing get_listing(size_t limit) const;

  Masked_string m_url;
  Config_ptr m_config;
  bool m_use_retry = false;
};

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_HTTP_H_
