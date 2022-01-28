/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/ssl_keygen.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <memory>
#include <regex>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/private_key_manager.h"

namespace shcore {
namespace ssl {
namespace {
using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

void throw_last_error(const std::string &context) {
  auto rc = ERR_get_error();
  auto reason = ERR_GET_REASON(rc);
  ERR_clear_error();

  if (reason == EVP_R_BAD_DECRYPT) {
    throw Decrypt_error(context);
  } else {
    std::string error_str = "Unexpected error";
    if (!context.empty()) error_str.append(" ").append(context);
    error_str.append(": ");
    std::string reason_str = ERR_reason_error_string(rc);

    throw std::runtime_error(error_str + reason_str);
  }
}

void get_public_key(EVP_PKEY *key_ptr, std::string *public_key) {
  BIO_ptr bio_public(BIO_new(BIO_s_mem()), ::BIO_free);
  int rc = PEM_write_bio_PUBKEY(bio_public.get(), key_ptr);

  if (rc == 1) {
    //--- Gets the Public Key Data
    char *data = nullptr;
    size_t size = BIO_get_mem_data(bio_public.get(), &data);

    std::string key(data, size);

    *public_key = key;
  } else {
    throw_last_error("getting public key from private key");
  }
}

std::string get_fingerprint(EVP_PKEY *key_ptr) {
  std::string ret_val;

  //--- Gets the Public Key from the Private Key
  std::string public_key;
  get_public_key(key_ptr, &public_key);

  //--- Only keep the real data
  public_key =
      shcore::str_replace(public_key, "-----BEGIN PUBLIC KEY-----", "");
  public_key = shcore::str_replace(public_key, "-----END PUBLIC KEY-----", "");
  public_key = shcore::str_replace(public_key, "\n", "");

  std::string decoded_key;
  if (decode_base64(public_key, &decoded_key)) {
    const auto digest = md5(decoded_key.c_str(), decoded_key.size());

    std::vector<std::string> fp;
    for (size_t index = 0; index < 16; index++) {
      char buffer[3];
      snprintf(buffer, sizeof(buffer), "%02x", digest[index]);
      fp.push_back(buffer);
    }

    ret_val = shcore::str_join(fp, ":");
  } else {
    throw std::runtime_error(
        "Unexpected error on fingerprint generation: unexpected length on "
        "decoded key.");
  }

  return ret_val;
}

}  // namespace

void init() { SSL_library_init(); }

std::string load_private_key(const std::string &path, Password_callback pwd_cb,
                             void *user_data) {
  std::string fingerprint;
  if (!shcore::path_exists(path)) {
    throw std::runtime_error("The indicated path does not exist.");
  } else if (!shcore::is_file(path)) {
    throw std::runtime_error("The indicated path is not a file.");
  } else {
    BIO_ptr bio_private(BIO_new_file(path.c_str(), "r"), ::BIO_free);
    EVP_PKEY *key_ptr =
        PEM_read_bio_PrivateKey(bio_private.get(), nullptr, pwd_cb, user_data);
    if (key_ptr) {
      EVP_KEY_ptr pkey(key_ptr, ::EVP_PKEY_free);
      fingerprint = get_fingerprint(pkey.get());
      Private_key_storage::get().put(path, std::move(pkey));
    } else {
      throw_last_error("loading private key");
    }
  }

  return fingerprint;
}

std::string create_key_pair(const std::string &path,
                            const std::string &key_name, size_t bitsize,
                            const std::string &passphrase) {
  std::string private_path =
      shcore::path::join_path(path, key_name + kPrivatePemSuffix);
  std::string public_path =
      shcore::path::join_path(path, key_name + kPublicPemSuffix);

  BN_ptr bn(BN_new(), ::BN_free);

  int rc = 0;
  unsigned long e = RSA_F4;

  EVP_KEY_ptr pkey(EVP_PKEY_new(), ::EVP_PKEY_free);
  RSA_ptr rsa(RSA_new(), ::RSA_free);

  rc = BN_set_word(bn.get(), e);
  if (rc == 1) {
    rc = RSA_generate_key_ex(rsa.get(), bitsize, bn.get(), nullptr);
    if (rc == 1) {
      rc = EVP_PKEY_set1_RSA(pkey.get(), rsa.get());
    }
  }

  if (rc != 1) throw_last_error("generating private key");

  BIO_ptr bio_public(BIO_new_file(public_path.c_str(), "w+"), ::BIO_free);
  rc = PEM_write_bio_PUBKEY(bio_public.get(), pkey.get());

  if (rc != 1) throw_last_error("saving public key file");

  rc = shcore::set_user_only_permissions(public_path);
  if (rc != 0) {
    throw std::runtime_error(
        "Error setting file permissions on public key file: " + public_path);
  }

  BIO_ptr bio_private(BIO_new_file(private_path.c_str(), "w+"), ::BIO_free);
  if (passphrase.empty()) {
    rc = PEM_write_bio_RSAPrivateKey(bio_private.get(), rsa.get(), nullptr,
                                     nullptr, 0, nullptr, nullptr);
  } else {
    auto *cipher = EVP_aes_256_cbc();
    unsigned char *passwd = reinterpret_cast<unsigned char *>(
        const_cast<char *>(passphrase.c_str()));
    rc = PEM_write_bio_RSAPrivateKey(bio_private.get(), rsa.get(), cipher,
                                     passwd, passphrase.size(), nullptr,
                                     nullptr);
  }

  if (rc != 1) throw_last_error("saving private key file");

  rc = shcore::set_user_only_permissions(private_path);
  if (rc != 0) {
    throw std::runtime_error(
        "Error setting file permissions on private key file: " + public_path);
  }

  return get_fingerprint(pkey.get());
}

std::vector<unsigned char> sha256(const char *data, size_t size) {
// EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were renamed to EVP_MD_CTX_new()
// and EVP_MD_CTX_free() in OpenSSL 1.1.
#if OPENSSL_VERSION_NUMBER >= 0x10100000L /* 1.1.x */
  std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)> mctx(
      EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
#else
  std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_destroy)> mctx(
      EVP_MD_CTX_create(), ::EVP_MD_CTX_destroy);
#endif
  const EVP_MD *md = EVP_sha256();
  int r = EVP_DigestInit_ex(mctx.get(), md, nullptr);
  if (r != 1) {
    throw std::runtime_error("SHA256: error initializing encoder.");
  }

  r = EVP_DigestUpdate(mctx.get(), data, size);
  if (r != 1) {
    throw std::runtime_error("SHA256: error while encoding data.");
  }

  std::vector<unsigned char> md_value;
  unsigned int md_len = EVP_MAX_MD_SIZE;
  md_value.resize(md_len);

  r = EVP_DigestFinal_ex(mctx.get(), md_value.data(), &md_len);
  if (r != 1) {
    throw std::runtime_error("SHA256: error completing encode operation.");
  }

  md_value.resize(md_len);

  return md_value;
}

std::vector<unsigned char> md5(const char *data, size_t size) {
  std::vector<unsigned char> md_value;
  md_value.resize(16);

  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, data, size);
  MD5_Final(md_value.data(), &ctx);

  return md_value;
}

}  // namespace ssl
}  // namespace shcore
