/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "modules/util/common/storage_options.h"

#include <cassert>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/storage/backend/oci_par_directory_config.h"
#include "mysqlshdk/libs/storage/utils.h"

namespace mysqlsh {
namespace common {

namespace {

mysqlshdk::oci::PAR_structure parse_par(const std::string &url) {
  mysqlshdk::oci::PAR_structure par;
  mysqlshdk::oci::parse_par(url, &par);
  return par;
}

std::shared_ptr<mysqlshdk::oci::IPAR_config> get_par_config(
    const mysqlshdk::oci::PAR_structure &par) {
  switch (par.type()) {
    case mysqlshdk::oci::PAR_type::PREFIX:
      return std::make_shared<
          mysqlshdk::storage::backend::oci::Oci_par_directory_config>(par);

    case mysqlshdk::oci::PAR_type::GENERAL:
      return std::make_shared<mysqlshdk::oci::General_par_config>(par);

    case mysqlshdk::oci::PAR_type::NONE:
      break;
  }

  return {};
}

std::shared_ptr<mysqlshdk::oci::IPAR_config> get_par_config(
    const std::string &url) {
  return get_par_config(parse_par(url));
}

}  // namespace

const shcore::Option_pack_def<Storage_options> &Storage_options::options() {
  static const auto opts = shcore::Option_pack_def<Storage_options>()
                               .include(&Storage_options::m_oci_options)
                               .include(&Storage_options::m_s3_options)
                               .include(&Storage_options::m_azure_options)
                               .on_done(&Storage_options::on_unpacked_options);

  return opts;
}

Storage_options::Storage_options(bool reads_files)
    : m_azure_options(
          reads_files
              ? mysqlshdk::azure::Blob_storage_options::Operation::READ
              : mysqlshdk::azure::Blob_storage_options::Operation::WRITE) {}

void Storage_options::on_unpacked_options() {
  mysqlshdk::storage::backend::object_storage::throw_on_conflict(
      std::array<const mysqlshdk::storage::backend::object_storage::
                     Object_storage_options *,
                 3>{&m_s3_options, &m_azure_options, &m_oci_options});

  if (m_oci_options) {
    set_storage_config(m_oci_options.config(), Storage_type::Oci_os);
  }

  if (m_s3_options) {
    set_storage_config(m_s3_options.config(), Storage_type::S3);
  }

  if (m_azure_options) {
    set_storage_config(m_azure_options.config(), Storage_type::Azure);
  }
}

void Storage_options::set_storage_config(mysqlshdk::storage::Config_ptr storage,
                                         Storage_type type) {
  m_storage = std::move(storage);
  m_storage_type = type;
}

const mysqlshdk::storage::backend::object_storage::Object_storage_options *
Storage_options::storage_options() const noexcept {
  if (m_oci_options) {
    return &m_oci_options;
  } else if (m_s3_options) {
    return &m_s3_options;
  } else if (m_azure_options) {
    return &m_azure_options;
  }

  return nullptr;
}

std::string Storage_options::describe(const std::string &url) const {
  if (const auto storage = storage_config(url); storage && storage->valid()) {
    return storage->describe(url);
  } else {
    return "'" + url + "'";
  }
}

mysqlshdk::storage::Config_ptr Storage_options::storage_config(
    const std::string &url, Storage_type *type) const {
  Storage_type storage;
  auto config = config_for(url, &storage);

  if (type) {
    *type = storage;
  }

  return config;
}

std::unique_ptr<mysqlshdk::storage::IDirectory> Storage_options::make_directory(
    const std::string &url, Storage_type *type) const {
  Storage_type storage;
  const auto config = config_for(url, &storage);

  // if this is a general PAR, it cannot be used to create a directory, but we
  // don't check for that here, as make_directory will simply throw in such case
  auto dir = mysqlshdk::storage::make_directory(url, config);

  if (type) {
    *type = storage;
  }

  return dir;
}

std::unique_ptr<mysqlshdk::storage::IFile> Storage_options::make_file(
    const std::string &url, Storage_type *type) const {
  // automatically handle compressed files
  return make_file(make_file_impl(url, type));
}

std::unique_ptr<mysqlshdk::storage::IFile> Storage_options::make_file(
    const std::string &url, mysqlshdk::storage::Compression compression,
    const mysqlshdk::storage::Compression_options &options,
    Storage_type *type) const {
  return mysqlshdk::storage::make_file(make_file_impl(url, type), compression,
                                       options);
}

std::unique_ptr<mysqlshdk::storage::IFile> Storage_options::make_file(
    std::unique_ptr<mysqlshdk::storage::IFile> f) {
  const auto compression = mysqlshdk::storage::from_file_path(f->filename());
  return mysqlshdk::storage::make_file(std::move(f), compression);
}

mysqlshdk::storage::Config_ptr Storage_options::config_for(
    const std::string &url, Storage_type *type) const {
  assert(type);

  if (const auto scheme = mysqlshdk::storage::utils::get_scheme(url);
      !scheme.empty()) {
    // if URL has a scheme, it takes priority over configured storage
    if (mysqlshdk::storage::utils::scheme_matches(scheme, "https")) {
      // this could be a PAR
      if (auto config = get_par_config(url)) {
        switch (config->par().type()) {
          case mysqlshdk::oci::PAR_type::PREFIX:
            *type = Storage_type::Oci_prefix_par;
            break;

          case mysqlshdk::oci::PAR_type::GENERAL:
            *type = Storage_type::Oci_par;
            break;

          default:
            assert(false);
            break;
        }

        return config;
      }

      *type = Storage_type::Http;
    } else if (mysqlshdk::storage::utils::scheme_matches(scheme, "http")) {
      *type = Storage_type::Http;
    } else if (mysqlshdk::storage::utils::scheme_matches(scheme, "file")) {
      *type = Storage_type::Local;
    } else {
      *type = Storage_type::Unknown;
    }

    return {};
  } else {
    // use configured storage
    *type = m_storage_type;
    return m_storage;
  }
}

std::unique_ptr<mysqlshdk::storage::IFile> Storage_options::make_file_impl(
    const std::string &url, Storage_type *type) const {
  if (url.empty() || mysqlshdk::storage::utils::strip_scheme(url).empty()) {
    throw std::invalid_argument{"The URL to a file cannot be empty."};
  }

  Storage_type storage;
  const auto config = config_for(url, &storage);

  if (Storage_type::Oci_prefix_par == storage) {
    // this is not a PAR to a file
    throw std::invalid_argument{"The given URL is not a PAR to a file."};
  }

  auto f = mysqlshdk::storage::make_file(url, config);

  if (type) {
    *type = storage;
  }

  return f;
}

}  // namespace common
}  // namespace mysqlsh
