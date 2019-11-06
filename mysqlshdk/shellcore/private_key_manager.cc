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

#include "mysqlshdk/shellcore/private_key_manager.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <algorithm>
#include <memory>
#include <utility>
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace {
void throw_last_error(const std::string &context) {
  auto rc = ERR_get_error();
  auto reason = ERR_GET_REASON(rc);
  ERR_clear_error();

  if (reason == EVP_R_BAD_DECRYPT) {
    throw std::runtime_error("Decrypt error: " + context);
  } else {
    std::string error_str = "Unexpected error";
    if (!context.empty()) error_str.append(" ").append(context);
    error_str.append(": ");
    std::string reason_str = ERR_reason_error_string(rc);

    throw std::runtime_error(error_str + reason_str);
  }
}

struct Read_key_state final {
 public:
  Read_key_state() = delete;
  Read_key_state(const std::string &path, const std::string &passphrase)
      : key_path(path), passphrase(passphrase) {}
  Read_key_state(const Read_key_state &other) = delete;
  Read_key_state(Read_key_state &&other) = delete;

  Read_key_state &operator=(const Read_key_state &other) = delete;
  Read_key_state &operator=(Read_key_state &&other) = delete;

  ~Read_key_state() = default;

  bool again() const noexcept { return user_asked < 5 && !interrupted; }

  const std::string &key_path;
  const std::string &passphrase;
  int attempt = 0;
  bool interrupted = false;
  int user_asked = 0;
};

std::shared_ptr<EVP_PKEY> load_private_key(const std::string &path,
                                           const std::string &passphrase) {
  if (!shcore::path_exists(path)) {
    throw std::runtime_error("'" + path + "' does not exist.");
  }

  if (!shcore::is_file(path)) {
    throw std::runtime_error("'" + path + "' is not a file.");
  }

  auto get_password = [](char *buf, int size, int /* rwflag */,
                         void *u) -> int {
    auto state = reinterpret_cast<Read_key_state *>(u);
    state->attempt++;
    auto console = mysqlsh::current_console();
    const std::string msg = [&state]() -> std::string {
      if (state->user_asked > 0) {
        return "Wrong passphrase, please try again: ";
      }
      return "Please enter the passphrase for private key \"" +
             state->key_path + "\": ";
    }();

    // use passphrase provided as function arg
    if (state->attempt == 1) {
      if (!state->passphrase.empty()) {
        const auto passphrase_size =
            std::min(static_cast<size_t>(size), state->passphrase.size());
        memcpy(buf, state->passphrase.c_str(), passphrase_size);
        return passphrase_size;
      }
    }

    // ask for passphrase
    std::string new_passphrase;
    auto result = console->prompt_password(msg, &new_passphrase);
    state->user_asked++;
    if (result != shcore::Prompt_result::Ok) {
      shcore::clear_buffer(&new_passphrase);
      state->interrupted = true;
      return -1;
    }

    const auto passphrase_size =
        std::min(static_cast<size_t>(size), new_passphrase.size());
    memcpy(buf, new_passphrase.c_str(), passphrase_size);
    shcore::clear_buffer(&new_passphrase);
    return passphrase_size;
  };

  Read_key_state state(path, passphrase);
  using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

  EVP_PKEY *key_ptr = nullptr;
  // When private key is broken or malformed and get_password callback is never
  // called, then we try to open private key at most 10 times (tries), to avoid
  // infinite-loop.
  for (int tries = 0; !key_ptr && state.again() && tries < 10; tries++) {
    BIO_ptr bio_private(BIO_new_file(path.c_str(), "r"), ::BIO_free);
    key_ptr = PEM_read_bio_PrivateKey(bio_private.get(), nullptr, get_password,
                                      &state);
  }

  if (key_ptr) {
    return std::shared_ptr<EVP_PKEY>(key_ptr, ::EVP_PKEY_free);
  }
  throw_last_error("loading private key");
  return nullptr;
}
}  // namespace

namespace shcore {
Private_key_storage::~Private_key_storage() {
  for (auto &item : m_store) {
    clear_buffer(const_cast<char *>(&(item.first)[0]), item.first.capacity());
  }
}

Private_key_storage &Private_key_storage::get() {
  static Private_key_storage store;
  return store;
}

void Private_key_storage::put(
    const std::string &path,
    std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)> &&key) {
  std::lock_guard<std::mutex> lock(m_store_mutex);
  if (key) {
    m_store[path] = std::shared_ptr<EVP_PKEY>(std::move(key));
  }
}

std::pair<Private_key_storage::Key_library::iterator, bool>
Private_key_storage::contains(const std::string &path) {
  auto item = m_store.find(path);
  return std::make_pair(item, item != m_store.end());
}

std::shared_ptr<EVP_PKEY> Private_key_storage::from_file(
    const std::string &path, const std::string &passphrase) {
  std::lock_guard<std::mutex> lock(m_store_mutex);
  // todo(kg): normalize path(?)
  auto key = m_store[path];
  if (!key) {
    m_store[path] = key = load_private_key(path, passphrase);
  }
  return key;
}

std::shared_ptr<EVP_PKEY> Private_key_storage::from_file(
    const std::string &path) {
  return this->from_file(path, std::string{});
}
}  // namespace shcore
