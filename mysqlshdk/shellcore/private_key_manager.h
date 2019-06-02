/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_SHELLCORE_PRIVATE_KEY_MANAGER_H_
#define MYSQLSHDK_SHELLCORE_PRIVATE_KEY_MANAGER_H_

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

struct evp_pkey_st;
using EVP_PKEY = evp_pkey_st;
extern "C" EVP_PKEY *EVP_PKEY_new(void);
extern "C" void EVP_PKEY_free(EVP_PKEY *pkey);

namespace shcore {
class Private_key_storage {
 public:
  using Path = std::string;
  using Private_key = std::shared_ptr<EVP_PKEY>;
  using Key_library = std::unordered_map<Path, Private_key>;

  Private_key_storage(const Private_key_storage &other) = delete;
  Private_key_storage(Private_key_storage &&other) = delete;

  Private_key_storage &operator=(const Private_key_storage &other) = delete;
  Private_key_storage &operator=(Private_key_storage &&other) = delete;

  ~Private_key_storage();

  static Private_key_storage &get();

  void put(const std::string &path,
           std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)> &&key);
  std::pair<Key_library::iterator, bool> contains(const std::string &path);
  std::shared_ptr<EVP_PKEY> from_file(const std::string &path);
  std::shared_ptr<EVP_PKEY> from_file(const std::string &path,
                                      const std::string &passphrase);

 private:
  Private_key_storage() = default;

  Key_library m_store;
  mutable std::mutex m_store_mutex;
};
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_PRIVATE_KEY_MANAGER_H_
