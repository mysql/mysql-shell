/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/rest/error_codes.h"
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

Masked_string get_uri_base(const Masked_string &uri) {
  return {get_uri_base(uri.real()), get_uri_base(uri.masked())};
}

std::string get_uri_path(const std::string &uri) {
  const auto pos = span_uri_base(uri);

  if (pos >= uri.length()) {
    return {};
  } else {
    return uri.substr(pos + 1);
  }
}

std::string get_uri_path(const Masked_string &uri) {
  return get_uri_path(uri.real());
}

void throw_if_error(const rest::String_response &response,
                    const Masked_string &uri) {
  if (const auto error = response.get_error(); error.has_value()) {
    throw rest::to_exception(*error,
                             "Could not access '" + uri.masked() + "': ");
  }
}

}  // namespace

Http_request::Http_request(Masked_string path, bool use_retry,
                           rest::Headers headers)
    : Request(std::move(path), std::move(headers)) {
  if (use_retry) {
    m_retry_strategy = mysqlshdk::rest::default_retry_strategy();
    retry_strategy = m_retry_strategy.get();
  }
}

Http_object::Http_object(const std::string &full_path, bool use_retry)
    : Http_object(get_uri_base(full_path), get_uri_path(full_path), use_retry) {
}

Http_object::Http_object(const Masked_string &full_path, bool use_retry)
    : Http_object(get_uri_base(full_path), get_uri_path(full_path), use_retry) {
}

Http_object::Http_object(const Masked_string &base, const std::string &path,
                         bool use_retry)
    : m_base(base), m_path(path), m_use_retry(use_retry) {}

void Http_object::open(Mode m) {
  if (Mode::APPEND == m) {
    throw std::logic_error("Http_object::open(): append mode is not supported");
  }

  if (Mode::READ == m) {
    exists();
  }

  m_offset = 0;
  m_open_mode = m;
}

bool Http_object::is_open() const { return m_open_mode.has_value(); }

std::string Http_object::filename() const {
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
  return shcore::pctdecode(fname);
}

bool Http_object::exists() const {
  bool result = true;

  if (const auto error = fetch_file_size(); error.has_value()) {
    if (rest::Response::Status_code::NOT_FOUND == error->status_code()) {
      result = false;
    } else {
      throw_if_error(error, "Failed to access object");
    }
  }

  return result;
}

std::unique_ptr<IDirectory> Http_object::parent() const {
  return make_directory(m_base.real(), m_parent_config);
}

void Http_object::set_write_data(Http_request *request) {
  request->body = m_buffer.data();
  request->size = m_buffer.size();
}

void Http_object::close() {
  assert(is_open());

  if (Mode::WRITE == *m_open_mode) {
    auto request = Http_request(m_path, m_use_retry,
                                {{"content-type", "application/octet-stream"}});

    set_write_data(&request);

    throw_if_error(get_rest_service(m_base)->put(&request).get_error(),
                   "Failed to put object");
    m_buffer.clear();
  }

  m_open_mode.reset();
  m_exists = false;
}

size_t Http_object::file_size() const {
  throw_if_error(fetch_file_size(), "Failed to fetch size of object");

  return m_file_size;
}

Masked_string Http_object::full_path() const {
  return {m_base.real() + m_path, m_base.masked() + m_path};
}

off64_t Http_object::seek(off64_t offset) {
  assert(is_open());

  if (Mode::READ == *m_open_mode) {
    m_offset = std::min<off64_t>(offset, file_size());
  }

  return m_offset;
}

ssize_t Http_object::read(void *buffer, size_t length) {
  assert(is_open() && Mode::READ == *m_open_mode);

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

  auto request = Http_request(m_path, m_use_retry, std::move(h));
  auto response = get_rest_service(m_base)->get(&request);

  if (Response::Status_code::PARTIAL_CONTENT == response.status) {
    const auto &content = response.buffer;
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

ssize_t Http_object::write(const void *buffer, size_t length) {
  assert(is_open() && Mode::WRITE == *m_open_mode);

  const auto incoming = reinterpret_cast<char *>(const_cast<void *>(buffer));
  m_buffer.append(incoming, length);
  m_file_size += length;

  return length;
}

void Http_object::remove() {
  auto request = Http_request(m_path, m_use_retry,
                              {{"content-type", "application/octet-stream"}});

  // truncate the file
  char body = 0;
  request.body = &body;
  request.size = 0;

  throw_if_error(get_rest_service(m_base)->put(&request).get_error(),
                 "Failed to truncate object");

  m_exists = false;
  m_file_size = 0;
}

void Http_object::throw_if_error(
    const std::optional<rest::Response_error> &error,
    const std::string &context) const {
  if (error.has_value()) {
    throw rest::to_exception(*error,
                             context + " '" + full_path().masked() + "': ");
  }
}

std::optional<rest::Response_error> Http_object::fetch_file_size() const {
  if (!m_exists) {
    auto request = Http_request(m_path, m_use_retry);
    auto response = get_rest_service(m_base)->head(&request);
    auto error = response.get_error();

    if (error.has_value()) {
      return error;
    } else {
      m_file_size = response.content_length();

      if (response.headers["Accept-Ranges"] != "bytes") {
        throw std::runtime_error(
            "Target server does not support partial requests.");
      }

      m_exists = true;
    }
  }

  return {};
}

void Http_object::set_full_path(const Masked_string &full_path) {
  m_base = get_uri_base(full_path);
  m_path = get_uri_path(full_path);
}

void Http_directory::init_rest(const Masked_string &url) { m_url = url; }

bool Http_directory::exists() const {
  throw std::logic_error("Http_directory::exists() - not implemented");
}

void Http_directory::create() {
  throw std::logic_error("Http_directory::create() - not implemented");
}

Masked_string Http_directory::full_path() const { return m_url; }

std::unordered_set<IDirectory::File_info> Http_directory::get_file_list(
    const std::string &context, const std::string &pattern) const {
  std::unordered_set<IDirectory::File_info> file_info;
  const auto rest = get_rest_service(m_url);

  do {
    // This may throw but we just let it bubble up
    auto request = Http_request(get_list_url(), m_use_retry);
    auto response = rest->get(&request);

    throw_if_error(response, m_url);

    try {
      auto list = parse_file_list(response.buffer.raw(), pattern);
      file_info.reserve(list.size() + file_info.size());
      std::move(list.begin(), list.end(),
                std::inserter(file_info, file_info.begin()));
    } catch (const shcore::Exception &error) {
      std::string msg = "Error " + context;
      msg.append(": ").append(error.what());

      log_debug2("%s\n%s", msg.c_str(), response.buffer.data());

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
  // file may outlive the directory, need to copy the values
  const auto full = full_path();
  auto real = full.real();
  auto masked = full.masked();
  Masked_string copy = {std::move(real), std::move(masked)};
  auto file = std::make_unique<Http_object>(copy, db::uri::pctencode_path(name),
                                            m_use_retry);
  file->set_parent_config(m_config);
  return file;
}

bool Http_directory::is_local() const { return false; }

std::string Http_directory::join_path(const std::string &a,
                                      const std::string &b) const {
  return a + "/" + b;
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
