/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/http.h"

#include <algorithm>
#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using Rest_service = mysqlshdk::rest::Rest_service;
using Headers = mysqlshdk::rest::Headers;
using Response = mysqlshdk::rest::Response;
using Response_error = mysqlshdk::rest::Response_error;

namespace mysqlshdk {
namespace storage {
namespace backend {

Http_get::Http_get(const std::string &uri, bool use_retry, bool verify_ssl)
    : m_uri(uri), m_use_retry(use_retry) {
  m_rest = std::make_unique<Rest_service>(m_uri, verify_ssl);

  // Timeout conditions:
  // - 30 seconds for HEAD and DELETE requests
  // - The rest will timeout if less than 1K is received in 60 seconds
  m_rest->set_timeout(30000, 1024, 60);
}

void Http_get::open(Mode m) {
  if (Mode::READ != m) {
    throw std::logic_error("Http_get::open(): only read mode is supported");
  }

  exists();

  m_offset = 0;
  m_open = true;
}

bool Http_get::is_open() const { return m_open; }

std::string Http_get::filename() const {
  std::string fname(m_uri);
  {
    auto p = fname.find("#");
    if (p != std::string::npos) {
      fname = fname.substr(0, p);
    }
  }
  {
    auto p = fname.rfind("?");
    if (p != std::string::npos) {
      fname = fname.substr(0, p);
    }
  }
  {
    auto p = fname.rfind("/");
    if (p != std::string::npos) {
      fname = fname.substr(p + 1);
    }
  }
  return fname;
}

bool Http_get::exists() const {
  if (!m_exists.has_value()) {
    auto retry_strategy = mysqlshdk::rest::default_retry_strategy();
    auto response = m_rest->head(std::string{}, {},
                                 m_use_retry ? &retry_strategy : nullptr);

    response.check_and_throw();

    m_file_size = std::stoul(response.headers["content-length"]);
    if (response.headers["Accept-Ranges"] != "bytes") {
      throw std::runtime_error(
          "Target server does not support partial requests.");
    }

    m_exists = true;
  }

  return m_exists.value();
}

std::unique_ptr<IDirectory> Http_get::parent() const {
  return make_directory(shcore::path::dirname(m_uri) + "/");
}

void Http_get::close() { m_open = false; }

size_t Http_get::file_size() const {
  // Makes sure the file exists
  exists();

  return m_file_size;
}

std::string Http_get::full_path() const { return m_uri; }

off64_t Http_get::seek(off64_t offset) {
  assert(is_open());

  const off64_t fsize = file_size();
  m_offset = std::min(offset, fsize);
  return m_offset;
}

ssize_t Http_get::read(void *buffer, size_t length) {
  assert(is_open());

  if (!(length > 0)) return 0;
  const off64_t fsize = file_size();
  if (m_offset >= fsize) return 0;

  const size_t first = m_offset;
  const size_t last_unbounded = m_offset + length - 1;
  // http range request is both sides inclusive
  const size_t last = std::min(file_size() - 1, last_unbounded);
  const std::string range =
      "bytes=" + std::to_string(first) + "-" + std::to_string(last);
  Headers h{{"range", range}};

  auto retry_strategy = mysqlshdk::rest::default_retry_strategy();
  auto response =
      m_rest->get(std::string{}, h, m_use_retry ? &retry_strategy : nullptr);
  if (Response::Status_code::PARTIAL_CONTENT == response.status) {
    const auto &content = response.body;
    if (length < content.size()) {
      throw std::runtime_error("Got more data than expected");
    }
    std::copy(content.data(), content.data() + content.size(),
              reinterpret_cast<uint8_t *>(buffer));
    m_offset += content.size();
    return content.size();
  } else if (Response::Status_code::OK == response.status) {
    throw std::runtime_error("Range requests are not supported.");
  } else if (Response::Status_code::RANGE_NOT_SATISFIABLE == response.status) {
    throw std::runtime_error("Range request " + std::to_string(first) + "-" +
                             std::to_string(last) + " is out of bounds.");
  }
  return 0;
}

Http_directory::Http_directory(const std::string &url, bool use_retry,
                               bool verify_ssl)
    : Http_directory(use_retry) {
  init_rest(url, verify_ssl);
}

void Http_directory::init_rest(const std::string &url, bool verify_ssl) {
  m_url = url;
  m_rest = std::make_unique<Rest_service>(m_url, verify_ssl);

  // Timeout conditions:
  // - 30 seconds for HEAD and DELETE requests
  // - The rest will timeout if less than 1K is received in 60 seconds
  m_rest->set_timeout(30000, 1024, 60);
}

bool Http_directory::exists() const {
  return m_open_status_code == Response::Status_code::OK;
}

void Http_directory::create() {
  throw std::logic_error("Http_directory::create() - not implemented");
}

void Http_directory::close() {
  // NO-OP
}

std::string Http_directory::full_path() const { return m_url; }

std::vector<IDirectory::File_info> Http_directory::get_file_list(
    const std::string &context, const std::string &pattern) const {
  // This may throw but we just let it bubble up
  auto retry_strategy = mysqlshdk::rest::default_retry_strategy();
  mysqlshdk::rest::Response response =
      m_rest->get(get_list_url(), {}, m_use_retry ? &retry_strategy : nullptr);

  response.check_and_throw();

  try {
    return parse_file_list(response.body, pattern);
  } catch (const shcore::Exception &error) {
    std::string msg = "Error " + context;
    msg.append(": ").append(error.what());

    log_debug2("%s\n%s", msg.c_str(), response.body.c_str());

    throw shcore::Exception::runtime_error(msg);
  }
}

std::vector<IDirectory::File_info> Http_directory::list_files(
    bool /*hidden_files*/) const {
  return get_file_list("listing files");
}

std::vector<IDirectory::File_info> Http_directory::filter_files(
    const std::string &pattern) const {
  return get_file_list("listing files matching " + pattern, pattern);
}

std::unique_ptr<IFile> Http_directory::file(
    const std::string &name, const File_options & /*options*/) const {
  return std::make_unique<Http_get>(full_path() + name);
}

bool Http_directory::is_local() const { return false; }

std::string Http_directory::join_path(const std::string &a,
                                      const std::string &b) const {
  return a + "/" + b;
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
