/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_BUCKET_OPTIONS_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_BUCKET_OPTIONS_H_

#include <memory>
#include <stdexcept>
#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

class Config;

template <class T>
class Bucket_options {
 public:
  Bucket_options() = default;

  Bucket_options(const Bucket_options &) = default;
  Bucket_options(Bucket_options &&) = default;

  Bucket_options &operator=(const Bucket_options &) = default;
  Bucket_options &operator=(Bucket_options &&) = default;

  virtual ~Bucket_options() = default;

  explicit operator bool() const { return !m_bucket_name.empty(); }

  std::shared_ptr<Config> config() const {
    assert(*this);
    return create_config();
  }

  template <class U>
  void throw_on_conflict(const U &other) const {
    if (*this && other) {
      throw std::invalid_argument(
          shcore::str_format("The option '%s' cannot be used when the value of "
                             "'%s' option is set.",
                             T::bucket_name_option(), U::bucket_name_option()));
    }
  }

 protected:
  virtual void on_unpacked_options() const {
    if (m_bucket_name.empty()) {
      if (!m_config_file.empty()) {
        throw std::invalid_argument(shcore::str_format(
            s_option_error, T::config_file_option(), T::bucket_name_option()));
      }

      if (!m_config_profile.empty()) {
        throw std::invalid_argument(shcore::str_format(
            s_option_error, T::profile_option(), T::bucket_name_option()));
      }
    }
  }

  static constexpr auto s_option_error =
      "The option '%s' cannot be used when the value of '%s' option is not "
      "set.";

  std::string m_bucket_name;
  std::string m_config_file;
  std::string m_config_profile;

 private:
  friend class Config;

  virtual std::shared_ptr<Config> create_config() const = 0;
};

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_BUCKET_OPTIONS_H_
