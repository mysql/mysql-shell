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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_OPTIONS_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_OPTIONS_H_

#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

class Object_storage_options {
 public:
  Object_storage_options() = default;

  Object_storage_options(const Object_storage_options &) = default;
  Object_storage_options(Object_storage_options &&) = default;

  Object_storage_options &operator=(const Object_storage_options &) = default;
  Object_storage_options &operator=(Object_storage_options &&) = default;

  virtual ~Object_storage_options() = default;

  explicit operator bool() const { return !m_container_name.empty(); }

  std::shared_ptr<Config> config() const {
    assert(*this);
    return create_config();
  }

  void throw_on_conflict(const Object_storage_options &other) const {
    if (*this && other) {
      throw std::invalid_argument(
          shcore::str_format("The option '%s' cannot be used when the value of "
                             "'%s' option is set.",
                             get_main_option(), other.get_main_option()));
    }
  }

  virtual const char *get_main_option() const = 0;

  virtual std::vector<const char *> get_secondary_options() const = 0;

 protected:
  virtual void on_unpacked_options() const {
    if (!has_value(get_main_option())) {
      for (const auto &option : get_secondary_options()) {
        if (has_value(option)) {
          throw std::invalid_argument(
              shcore::str_format(s_option_error, option, get_main_option()));
        }
      }
    }
  }

  static constexpr auto s_option_error =
      "The option '%s' cannot be used when the value of '%s' option is not "
      "set.";

  std::string m_container_name;

 private:
  friend class Config;

  virtual std::shared_ptr<Config> create_config() const = 0;

  virtual bool has_value(const char *option) const = 0;
};

/**
 * Calls Object_storage_options::throw_on_conflict() for all combinations of
 * pairs in `options`.
 */
template <std::size_t N>
void throw_on_conflict(
    const std::array<const Object_storage_options *, N> &options) {
  static_assert(N >= 2, "need at least two options");

  // options at indexes where values of mask are true are selected as pair
  std::array<bool, N> mask{false};
  // we start with the first two options
  mask[0] = mask[1] = true;
  // mask now consists of values: true, true, false, ...; it is the last
  // permutation (lexicographically) of the whole range; we then go through all
  // permutations of these Boolean values, at each step of the loop we get a
  // different pair of options, generating all pairs among the given options

  const Object_storage_options *left;
  const Object_storage_options *right = nullptr;

  do {
    left = nullptr;

    for (std::size_t i = 0; i < N; ++i) {
      if (mask[i]) {
        if (!left) {
          left = options[i];
        } else {
          right = options[i];
          break;
        }
      }
    }

    left->throw_on_conflict(*right);
  } while (std::prev_permutation(mask.begin(), mask.end()));
}

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_OPTIONS_H_
