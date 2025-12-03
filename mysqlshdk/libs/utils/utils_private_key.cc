/*
 * Copyright (c) 2019, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_private_key.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_ssl.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace ssl {

namespace {

using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

void throw_last_error(const std::string &context) {
  const auto stack = openssl_error_stack();
  bool is_decrypt_error = false;

  for (const auto error : stack) {
    // EVP_R_BAD_DECRYPT is reported when decrypting the key fails, sometimes
    // decryption does not return an error even though password is wrong or
    // missing, then PKCS12_R_DECODE_ERROR is reported when decrypted data
    // cannot be converted into a key
    if (const auto reason = ERR_GET_REASON(error);
        EVP_R_BAD_DECRYPT == reason || PKCS12_R_DECODE_ERROR == reason) {
      is_decrypt_error = true;
      break;
    }
  }

  if (is_decrypt_error) {
    throw Decrypt_error(context);
  } else {
    std::string error_str = "Unexpected error";

    if (!context.empty()) error_str.append(1, ' ').append(context);

    error_str.append(": ").append(ERR_reason_error_string(stack.front()));

    throw std::runtime_error(error_str);
  }
}

struct Read_key_state final {
 public:
  Read_key_state() = delete;

  Read_key_state(const std::string &path, std::string_view passphrase_)
      : key_path(path), passphrase(passphrase_) {}

  Read_key_state(const Read_key_state &other) = delete;
  Read_key_state(Read_key_state &&other) = delete;

  Read_key_state &operator=(const Read_key_state &other) = delete;
  Read_key_state &operator=(Read_key_state &&other) = delete;

  ~Read_key_state() = default;

  bool again() const noexcept { return user_asked < 5 && !interrupted; }

  const std::string &key_path;
  const std::string_view passphrase;

  int attempt = 0;
  bool interrupted = false;
  int user_asked = 0;
};

std::string expand_path(std::string_view path) {
  return shcore::get_absolute_path(shcore::path::expand_user(path));
}

void validate_private_key_file(const std::string &path) {
  if (!shcore::path_exists(path)) {
    throw std::runtime_error("Private key '" + path + "' does not exist.");
  }

  if (!shcore::is_file(path)) {
    throw std::runtime_error("Private key '" + path + "' is not a file.");
  }
}

std::string bio_mem_to_string(BIO *bio) {
  char *data = nullptr;
  size_t size = BIO_get_mem_data(bio, &data);

  return std::string{data, size};
}

}  // namespace

std::string Public_key::full_path(const std::string &dir,
                                  const std::string &key_name) {
  return expand_path(shcore::path::join_path(dir, key_name + "_public.pem"));
}

std::string Public_key::fingerprint(Hash hash) const {
  const auto key = to_string();
  const auto key_view = shcore::str_strip_view(key);

  //--- Only keep the real data
  std::size_t start = 0;
  std::size_t length = key_view.length();

  if ('-' == key_view.back()) {
    // -----END PUBLIC KEY-----
    length = key_view.rfind('\n');
  }

  if ('-' == key_view.front()) {
    // -----BEGIN PUBLIC KEY-----
    const auto pos = key_view.find('\n');
    start += pos;
    length -= pos;
  }

  if (std::string decoded; decode_base64(
          shcore::str_replace(key_view.substr(start, length), "\n", ""),
          &decoded)) {
    auto fun = &sha256;

    switch (hash) {
      case Hash::RESTRICTED_MD5:
        fun = restricted::md5;
        break;

      case Hash::RESTRICTED_SHA1:
        fun = restricted::sha1;
        break;

      case Hash::SHA256:
        fun = sha256;
        break;
    }

    return to_fingerprint(fun(decoded));
  } else {
    throw std::runtime_error(
        "Unexpected error on fingerprint generation: unexpected length on "
        "decoded key.");
  }
}

std::string Public_key::to_string() const {
  BIO_ptr bio{BIO_new(BIO_s_mem()), ::BIO_free};

  if (1 != PEM_write_bio_PUBKEY(bio.get(), ptr())) {
    throw_last_error("writing public key");
  }

  return bio_mem_to_string(bio.get());
}

void Public_key::save(const std::string &path) const {
  BIO_ptr bio{BIO_new_file(path.c_str(), "w"), ::BIO_free};

  if (1 != PEM_write_bio_PUBKEY(bio.get(), ptr())) {
    throw_last_error("saving public key file");
  }

  if (0 != shcore::set_user_only_permissions(path)) {
    throw std::runtime_error(
        "Error setting file permissions on public key file: " + path);
  }
}

std::string Private_key::full_path(const std::string &dir,
                                   const std::string &key_name) {
  return expand_path(shcore::path::join_path(dir, key_name + ".pem"));
}

Private_key Private_key::generate(int bits) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L /* 3.0.x */
  const auto key_ptr = EVP_RSA_gen(bits);

  if (!key_ptr) {
    throw_last_error("generating private key");
  }

  return Private_key{{key_ptr, ::EVP_PKEY_free}};
#else
  using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
  using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;

  BN_ptr bn(BN_new(), ::BN_free);
  BN_set_word(bn.get(), RSA_F4);

  RSA_ptr rsa(RSA_new(), ::RSA_free);

  if (1 != RSA_generate_key_ex(rsa.get(), bits, bn.get(), nullptr)) {
    throw_last_error("generating private key");
  }

  Native_key key{EVP_PKEY_new(), ::EVP_PKEY_free};

  if (1 != EVP_PKEY_set1_RSA(key.get(), rsa.get())) {
    throw_last_error("converting private key");
  }

  return Private_key{std::move(key)};
#endif
}

Private_key Private_key::from_file(std::string_view path,
                                   std::string_view passphrase) {
  const auto full = expand_path(path);
  validate_private_key_file(full);

  const auto get_password = [](char *buf, int size, int /* rwflag */,
                               void *u) -> int {
    auto state = reinterpret_cast<Read_key_state *>(u);

    const auto use_pass = [buf, size](std::string_view pass) {
      const auto pass_size = std::min(static_cast<size_t>(size), pass.size());
      memcpy(buf, pass.data(), pass_size);
      return pass_size;
    };

    // use passphrase provided as function arg
    if (0 == state->attempt++ && !state->passphrase.empty()) {
      return use_pass(state->passphrase);
    }

    // ask for passphrase
    const std::string msg = [&state]() -> std::string {
      if (state->user_asked > 0) {
        return "Wrong passphrase, please try again: ";
      }

      return "Please enter the passphrase for private key '" + state->key_path +
             "': ";
    }();
    std::string pass;
    shcore::on_leave_scope clear_passphrase{
        [&pass]() { shcore::clear_buffer(pass); }};

    ++state->user_asked;

    if (shcore::Prompt_result::Ok !=
        mysqlsh::current_console()->prompt_password(msg, &pass)) {
      state->interrupted = true;
      return -1;
    }

    return use_pass(pass);
  };

  Read_key_state state{full, passphrase};
  EVP_PKEY *key_ptr = nullptr;

  while (!key_ptr && state.again()) {
    ERR_clear_error();
    BIO_ptr bio{BIO_new_file(full.c_str(), "r"), ::BIO_free};
    key_ptr = PEM_read_bio_PrivateKey(bio.get(), nullptr, get_password, &state);
  }

  if (!key_ptr) {
    throw_last_error("loading private key");
  }

  return Private_key{{key_ptr, ::EVP_PKEY_free}};
}

Private_key Private_key::from_file(std::string_view path,
                                   Password_callback callback,
                                   void *user_data) {
  assert(callback);

  const auto full = expand_path(path);
  validate_private_key_file(full);

  BIO_ptr bio{BIO_new_file(full.c_str(), "r"), ::BIO_free};
  const auto key_ptr =
      PEM_read_bio_PrivateKey(bio.get(), nullptr, callback, user_data);

  if (!key_ptr) {
    throw_last_error("loading private key");
  }

  return Private_key{{key_ptr, ::EVP_PKEY_free}};
}

Private_key Private_key::from_string(std::string_view contents,
                                     Password_callback callback,
                                     void *user_data) {
  BIO_ptr bio{BIO_new_mem_buf(contents.data(), contents.length()), ::BIO_free};

  if (!bio) {
    throw_last_error("acquiring memory");
  }

  const auto key_ptr =
      PEM_read_bio_PrivateKey(bio.get(), nullptr, callback, user_data);

  if (!key_ptr) {
    throw_last_error("loading private key");
  }

  return Private_key{{key_ptr, ::EVP_PKEY_free}};
}

Public_key Private_key::public_key() const {
  if (!m_public_key) {
    BIO_ptr bio{BIO_new(BIO_s_mem()), ::BIO_free};

    if (1 != PEM_write_bio_PUBKEY(bio.get(), ptr())) {
      throw_last_error("writing public key");
    }

    const auto key_ptr =
        PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr);

    if (!key_ptr) {
      throw_last_error("loading public key");
    }

    m_public_key = {key_ptr, ::EVP_PKEY_free};
  }

  return Public_key{m_public_key};
}

std::string Private_key::fingerprint(Hash hash) const {
  return public_key().fingerprint(hash);
}

std::string Private_key::to_string() const {
  BIO_ptr bio{BIO_new(BIO_s_mem()), ::BIO_free};

  if (1 != PEM_write_bio_PrivateKey(bio.get(), ptr(), nullptr, nullptr, 0,
                                    nullptr, nullptr)) {
    throw_last_error("writing private key");
  }

  return bio_mem_to_string(bio.get());
}

void Private_key::save(const std::string &path,
                       std::string_view password) const {
  BIO_ptr bio{BIO_new_file(path.c_str(), "w"), ::BIO_free};

  {
    int rc = 0;

    if (password.empty()) {
      rc = PEM_write_bio_PrivateKey(bio.get(), ptr(), nullptr, nullptr, 0,
                                    nullptr, nullptr);
    } else {
      const auto pwd = reinterpret_cast<unsigned char *>(
          const_cast<char *>(password.data()));
      rc = PEM_write_bio_PrivateKey(bio.get(), ptr(), EVP_aes_256_cbc(), pwd,
                                    password.length(), nullptr, nullptr);
    }

    if (1 != rc) {
      throw_last_error("saving private key file");
    }
  }

  if (0 != shcore::set_user_only_permissions(path)) {
    throw std::runtime_error(
        "Error setting file permissions on private key file: " + path);
  }
}

Private_key_id::~Private_key_id() { clear_buffer(m_id); }

Private_key_id Private_key_id::from_path(std::string_view path) {
  return Private_key_id("file://" + expand_path(path));
}

Private_key_storage &Private_key_storage::instance() {
  static Private_key_storage s_store;
  return s_store;
}

void Private_key_storage::put(const Private_key_id &id,
                              const Private_key &key) {
  std::lock_guard lock{m_store_mutex};
  m_store[id] = key.m_private_key;
}

void Private_key_storage::remove(const Private_key_id &id) {
  std::lock_guard lock{m_store_mutex};
  m_store.erase(id);
}

bool Private_key_storage::contains(const Private_key_id &id) {
  std::lock_guard lock{m_store_mutex};
  return m_store.count(id);
}

Private_key Private_key_storage::get(const Private_key_id &id) {
  std::lock_guard lock{m_store_mutex};

  if (const auto it = m_store.find(id); m_store.end() == it) {
    throw std::runtime_error("Cannot find private key with ID: " + id.id());
  } else {
    return Private_key{it->second};
  }
}

Private_key Private_key_storage::from_file(std::string_view path,
                                           std::string_view passphrase) {
  return from_file_impl(path, passphrase);
}

Private_key Private_key_storage::from_file(
    std::string_view path, Private_key::Password_callback callback,
    void *user_data) {
  return from_file_impl(path, callback, user_data);
}

template <typename... Args>
Private_key Private_key_storage::from_file_impl(std::string_view path,
                                                Args &&...args) {
  const auto id = Private_key_id::from_path(path);

  {
    std::lock_guard lock{m_store_mutex};

    if (const auto it = m_store.find(id); m_store.end() != it) {
      return Private_key{it->second};
    }
  }

  auto key = Private_key::from_file(path, std::forward<Args>(args)...);

  put(id, key);

  return key;
}

Keychain::Keychain() : m_id(std::string{}) {}

Keychain::~Keychain() { remove(); }

void Keychain::use(const Private_key &key) {
  remove();

  m_id = Private_key_id{key.fingerprint(Hash::SHA256)};

  Private_key_storage::instance().put(m_id, key);
}

void Keychain::remove() {
  if (!m_id.id().empty()) {
    Private_key_storage::instance().remove(m_id);
  }
}

}  // namespace ssl
}  // namespace shcore
