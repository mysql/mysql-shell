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
#include "mysqlshdk/libs/storage/utils.h"
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

namespace {

Rest_service *get_rest_service(const Masked_string &base) {
  static thread_local std::unordered_map<std::string,
                                         std::unique_ptr<Rest_service>>
      services;

  auto &service = services[base.real()];

  if (!service) {
    // copy the values, Rest_service may outlive the original caller
    auto real = base.real();
    auto masked = base.masked();
    Masked_string copy = {std::move(real), std::move(masked)};
    service = std::make_unique<mysqlshdk::rest::Rest_service>(copy, true);

    // Timeout conditions:
    // - 30 seconds for HEAD and DELETE requests
    // - The rest will timeout if less than 1K is received in 60 seconds
    service->set_timeout(30000, 1024, 60);
  }

  return service.get();
}

std::size_t span_uri_base(const std::string &uri) {
  if (uri.empty()) {
    throw std::logic_error("URI is empty");
  }

  const auto uri_no_scheme = mysqlshdk::storage::utils::strip_scheme(uri);

  if (uri.length() == uri_no_scheme.length()) {
    throw std::logic_error("URI does not have a scheme");
  }

  const auto pos = uri_no_scheme.rfind('/');

  if (std::string::npos == pos) {
    return uri.size();
  } else {
    return uri.size() - uri_no_scheme.size() + pos;
  }
}

std::string get_uri_base(const std::string &uri) {
  auto base = uri.substr(0, span_uri_base(uri));

  if ('/' != base.back()) {
    base += '/';
  }

  return base;
}

std::string get_uri_path(const std::string &uri) {
  const auto pos = span_uri_base(uri);

  if (pos >= uri.length()) {
    return {};
  } else {
    return uri.substr(pos + 1);
  }
}

}  // namespace

Http_get::Http_get(const std::string &full_path, bool use_retry)
    : Http_get(get_uri_base(full_path), get_uri_path(full_path), use_retry) {}

Http_get::Http_get(const Masked_string &base, const std::string &path,
                   bool use_retry)
    : m_base(base), m_path(path), m_use_retry(use_retry) {}

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
  std::string fname = full_path().real();
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
    auto response = get_rest_service(m_base)->head(
        m_path, {}, m_use_retry ? &retry_strategy : nullptr);

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
  return make_directory(m_base.real());
}

void Http_get::close() { m_open = false; }

size_t Http_get::file_size() const {
  // Makes sure the file exists
  exists();

  return m_file_size;
}

Masked_string Http_get::full_path() const {
  return {m_base.real() + m_path, m_base.masked() + m_path};
}

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
  auto response = get_rest_service(m_base)->get(
      m_path, h, m_use_retry ? &retry_strategy : nullptr);
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

Http_directory::Http_directory(const std::string &url, bool use_retry)
    : Http_directory(use_retry) {
  init_rest(url);
}

void Http_directory::init_rest(const Masked_string &url) { m_url = url; }

bool Http_directory::exists() const {
  throw std::logic_error("Http_directory::exists() - not implemented");
}

void Http_directory::create() {
  throw std::logic_error("Http_directory::create() - not implemented");
}

void Http_directory::close() {
  // NO-OP
}

Masked_string Http_directory::full_path() const { return m_url; }

std::unordered_set<IDirectory::File_info> Http_directory::get_file_list(
    const std::string &context, const std::string &pattern) const {
  std::unordered_set<IDirectory::File_info> file_info;
  const auto rest = get_rest_service(m_url);

  do {
    // This may throw but we just let it bubble up
    auto retry_strategy = mysqlshdk::rest::default_retry_strategy();
    mysqlshdk::rest::Response response =
        rest->get(get_list_url(), {}, m_use_retry ? &retry_strategy : nullptr);

    response.check_and_throw();

    try {
      auto list = parse_file_list(response.body, pattern);
      file_info.reserve(list.size() + file_info.size());
      std::move(list.begin(), list.end(),
                std::inserter(file_info, file_info.begin()));
    } catch (const shcore::Exception &error) {
      std::string msg = "Error " + context;
      msg.append(": ").append(error.what());

      log_debug2("%s\n%s", msg.c_str(), response.body.c_str());

      throw shcore::Exception::runtime_error(msg);
    }
  } while (!is_list_files_complete());

  return file_info;
}

std::unordered_set<IDirectory::File_info> Http_directory::list_files(
    bool /*hidden_files*/) const {
  return get_file_list("listing files");
}

std::unordered_set<IDirectory::File_info> Http_directory::filter_files(
    const std::string &pattern) const {
  return get_file_list("listing files matching " + pattern, pattern);
}

std::unique_ptr<IFile> Http_directory::file(
    const std::string &name, const File_options & /*options*/) const {
  return std::make_unique<Http_get>(full_path(), name);
}

bool Http_directory::is_local() const { return false; }

std::string Http_directory::join_path(const std::string &a,
                                      const std::string &b) const {
  return a + "/" + b;
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
