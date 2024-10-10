/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/oci_par_directory.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

using mysqlshdk::db::uri::pctencode_query_value;

class Oci_par_file::Multipart_upload_helper : public IMultipart_upload_helper {
 public:
  Multipart_upload_helper(Oci_par_file *parent, std::string endpoint)
      : m_parent(parent), m_endpoint(std::move(endpoint)) {}

  void upload(const char *body, std::size_t size) override {
    m_parent->put(body, size);
  }

  void create() override {
    auto request =
        m_parent->http_request(m_parent->m_path, {{"opc-multipart", "true"}});
    const auto response = m_parent->rest_service()->put(&request);

    m_parent->throw_if_error(response.get_error(),
                             "Failed to create a multipart upload");

    try {
      const auto json = response.json().as_map();
      m_upload_id = json->at("uploadId").as_string();
      m_service = std::make_unique<mysqlshdk::rest::Rest_service>(
          m_endpoint + json->at("accessUri").as_string(), true, "m-PAR");
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error(shcore::str_format(
          "Failed to create a PAR multipart upload of '%s': %s", name().c_str(),
          e.what()));
    }
  }

  bool is_active() const override { return !!m_service; }

  void upload_part(const char *body, std::size_t size) override {
    auto request =
        m_parent->http_request(std::to_string(++m_parts),
                               {{"content-type", "application/octet-stream"}});

    request.body = body;
    request.size = size;

    m_parent->throw_if_error(m_service->put(&request).get_error(),
                             "Failed to put part of a PAR multipart upload");
  }

  void commit() override {
    auto request = m_parent->http_request({});

    m_parent->throw_if_error(m_service->post(&request).get_error(),
                             "Failed to commit a PAR multipart upload");
  }

  void abort() override {
    auto request = m_parent->http_request({});

    m_parent->throw_if_error(m_service->delete_(&request).get_error(),
                             "Failed to abort a PAR multipart upload");
  }

  const std::string &name() const override { return m_parent->m_path; }

  const std::string &upload_id() const override { return m_upload_id; }

 private:
  void cleanup() override {
    m_parts = 0;
    m_upload_id.clear();
    m_service.reset();
  }

  Oci_par_file *m_parent;
  std::string m_endpoint;

  std::size_t m_parts = 0;
  std::string m_upload_id;
  std::unique_ptr<mysqlshdk::rest::Rest_service> m_service;
};

Oci_par_file::Oci_par_file(const Oci_par_directory_config_ptr &config,
                           const Masked_string &base, const std::string &path)
    : Http_object(base, db::uri::pctencode_path(path), true),
      m_helper(std::make_unique<Multipart_upload_helper>(
          this, config->par().endpoint())),
      m_uploader(m_helper.get(), config->part_size()) {
  set_parent_config(config);
}

Oci_par_file::~Oci_par_file() = default;

ssize_t Oci_par_file::write(const void *buffer, size_t length) {
  assert(is_open() && Mode::WRITE == *m_open_mode);

  m_uploader.append(buffer, length);
  m_file_size += length;

  return length;
}

void Oci_par_file::flush_on_close() { m_uploader.commit(); }

std::unique_ptr<IFile> Oci_par_directory::file(
    const std::string &name, const File_options & /*options*/) const {
  return make_file(name, m_config);
}

std::unique_ptr<IDirectory> Oci_par_directory::directory(
    const std::string &name) const {
  return std::make_unique<Oci_par_directory>(
      std::make_shared<Oci_par_directory_config>(full_path().real() + name +
                                                 "/"));
}

Oci_par_directory::Oci_par_directory(const Oci_par_directory_config_ptr &config)
    : Http_directory(config, true), m_config(config) {
  init_rest();
}

Masked_string Oci_par_directory::full_path() const {
  return m_config->anonymized_full_url();
}

bool Oci_par_directory::exists() const {
  // If list files succeeds, we can say the target exists even if it's a prefix
  // PAR and there are no objects with that prefix as the result will be an
  // empty list, otherwise, an exception describing the issue would be raised
  if (!m_exists.has_value()) {
    is_empty();
    m_exists = true;
  }

  return m_exists.value_or(false);
}

void Oci_par_directory::create() {
  // NOOP
}

void Oci_par_directory::remove() {
  // NOOP
}

class Oci_par_directory::Par_listing_context : public Listing_context {
 public:
  Par_listing_context(std::size_t limit, const std::string &prefix)
      : Listing_context(limit), m_prefix(prefix), m_remaining(limit) {}

  std::string get_url() const override {
    // use delimiter to limit the number of results, we're just interested in
    // the files in the current directory
    std::string url = "?fields=name,size&delimiter=/";

    if (!m_prefix.empty()) {
      url += "&prefix=" + pctencode_query_value(m_prefix);
    }

    if (!m_next_start_with.empty()) {
      url += "&start=" + pctencode_query_value(m_next_start_with);
    }

    if (const auto l = m_remaining < k_list_limit ? m_remaining : 0) {
      url += "&limit=" + std::to_string(l);
    }

    return url;
  }

  Directory_listing parse(const std::string &data) const override {
    Directory_listing list;

    const auto response =
        shcore::Value::parse({data.data(), data.size()}).as_map();
    const auto objects = response->get_array("objects");
    const auto prefix_length = m_prefix.length();

    list.files.reserve(objects->size());

    for (const auto &object : *objects) {
      const auto file = object.as_map();
      auto name = file->get_string("name").substr(prefix_length);

      // The OCI Console UI allows creating an empty folder in a bucket. A
      // prefix PAR for such folder should be valid as long as the folder does
      // not contain files. On a scenario like this, using the prefix PAR to
      // list the objects will return 1 object named as <prefix>/. Since we are
      // taking as name, everything after the <prefix>/, that means in a case
      // like this, an empty named object will be returned. Such object should
      // be ignored from the listing.
      if (!name.empty()) {
        list.files.emplace(std::move(name),
                           static_cast<size_t>(file->get_uint("size")));
      }
    }

    if (const auto prefixes = response->get_array("prefixes")) {
      for (const auto &prefix : *prefixes) {
        auto name = prefix.as_string().substr(prefix_length);
        // last character is '/'
        name.pop_back();
        list.directories.emplace(std::move(name));
      }
    }

    const auto total = list.files.size() + list.directories.size();
    m_total += total;

    if (m_remaining) {
      if (total >= m_remaining) {
        m_remaining = 0;
      } else {
        m_remaining -= total;
      }
    }

    m_next_start_with = response->get_string("nextStartWith", "");

    return list;
  }

  bool is_complete() const override {
    return m_next_start_with.empty() || (limit() && m_total >= limit());
  }

 private:
  static constexpr size_t k_list_limit = 1000;
  const std::string &m_prefix;
  mutable std::string m_next_start_with;
  mutable std::size_t m_remaining;
  mutable std::size_t m_total = 0;
};

std::unique_ptr<Oci_par_directory::Listing_context>
Oci_par_directory::listing_context(size_t limit) const {
  return std::make_unique<Par_listing_context>(limit,
                                               m_config->par().object_prefix());
}

void Oci_par_directory::init_rest() {
  Http_directory::init_rest(
      ::mysqlshdk::oci::anonymize_par(m_config->par().par_url()));
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
