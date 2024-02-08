/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_PAR_H_
#define MYSQLSHDK_LIBS_OCI_OCI_PAR_H_

#include <memory>
#include <string>
#include <utility>

#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/utils/masked_value.h"
#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlshdk {
namespace oci {

enum class PAR_access_type {
  OBJECT_READ,
  OBJECT_WRITE,
  OBJECT_READ_WRITE,
  ANY_OBJECT_READ,
  ANY_OBJECT_WRITE,
  ANY_OBJECT_READ_WRITE
};

enum class PAR_list_action { DENY, LIST_OBJECTS };

struct PAR {
  std::string id;
  std::string name;
  std::string access_uri;
  std::string access_type;
  std::string object_name;
  std::string time_created;
  std::string time_expires;
  std::string list_action;
  std::size_t size;
};

enum class PAR_type { MANIFEST, PREFIX, GENERAL, NONE };

struct PAR_structure {
  inline PAR_type type() const noexcept { return m_type; }

  inline const std::string &endpoint() const noexcept { return m_endpoint; }

  inline const std::string &object_prefix() const noexcept {
    return m_object_prefix;
  }

  inline const std::string &object_name() const noexcept {
    return m_object_name;
  }

  inline const std::string &object_path() const noexcept {
    return m_object_path;
  }

  inline const std::string &par_url() const noexcept { return m_par_url; }

  inline const std::string &full_url() const noexcept { return m_full_url; }

 private:
  friend PAR_type parse_par(const std::string &, PAR_structure *);

  PAR_type m_type = PAR_type::NONE;

  std::string m_endpoint;
  std::string m_object_prefix;
  std::string m_object_name;

  std::string m_object_path;
  std::string m_par_url;
  std::string m_full_url;
};

std::string to_string(PAR_access_type access_type);

std::string to_string(PAR_list_action list_action);

std::string to_string(PAR_type type);

std::string hide_par_secret(const std::string &par, std::size_t start_at = 0);

template <typename T>
Masked_string anonymize_par(T &&par) {
  return {std::forward<T>(par), hide_par_secret(par)};
}

// Parses full object PAR, including REST end point.
PAR_type parse_par(const std::string &url, PAR_structure *data = nullptr);

class IPAR_config : public storage::Config {
 public:
  IPAR_config() = default;

  explicit IPAR_config(const std::string &url) { parse_par(url, &m_par); }

  IPAR_config(const IPAR_config &) = delete;
  IPAR_config(IPAR_config &&) = default;

  IPAR_config &operator=(const IPAR_config &) = delete;
  IPAR_config &operator=(IPAR_config &&) = default;

  ~IPAR_config() override {
    if (!m_temp_folder.empty()) {
      shcore::remove_directory(m_temp_folder);
    }
  }

  const PAR_structure &par() const { return m_par; }

  const std::string &temp_folder() const { return m_temp_folder; }

  void set_temp_folder(const std::string &path) { m_temp_folder = path; }

 protected:
  PAR_structure m_par;

 private:
  std::string describe_self() const override;

  std::string describe_url(const std::string &url) const override;

  std::string m_temp_folder;
};

template <PAR_type Type>
class PAR_config : public IPAR_config {
 public:
  using IPAR_config::IPAR_config;

  PAR_config() = delete;

  explicit PAR_config(const PAR_structure &par) {
    if (Type == par.type()) {
      m_par = par;
      if (Type == PAR_type::PREFIX) {
        set_temp_folder(shcore::create_temporary_folder());
      }
    }
  }

  bool valid() const override { return Type == m_par.type(); }

 protected:
  void validate_url(const std::string &url) const {
    bool invalid = !valid();

    if (!invalid) {
      PAR_structure par;
      parse_par(url, &par);
      invalid = invalid || m_par.full_url() != par.full_url();
    }

    if (invalid) {
      // Any other name is not supported.
      throw std::invalid_argument("Invalid " + to_string(m_par.type()) +
                                  " PAR: " + url);
    }
  }
};

class General_par_config : public PAR_config<PAR_type::GENERAL> {
 public:
  using PAR_config::PAR_config;

 private:
  std::unique_ptr<storage::IFile> file(const std::string &path) const override;

  std::unique_ptr<storage::IDirectory> directory(
      const std::string &path) const override;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_PAR_H_
