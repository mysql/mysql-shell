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

#include "mysqlshdk/libs/utils/ssl_keygen.h"

#include <algorithm>
#include <functional>
#include <regex>
#include <vector>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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
    unsigned char digest[17];
    digest[16] = '\0';
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, decoded_key.c_str(), decoded_key.size());
    MD5_Final(digest, &ctx);

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
    } else {
      throw_last_error("loading private key");
    }
  }

  return fingerprint;
}

bool decode_base64(const std::string &source, std::string *target) {
  //--- Unencodes the Public Key Data
  size_t size = source.size();
  size_t padding =
      source[size - 2] == '=' ? 2 : source[size - 1] == '=' ? 1 : 0;

  const size_t expected_size = (size * 3) / 4 - padding;

  std::unique_ptr<char> buffer(new char[expected_size]);

  BIO_ptr bio_source(BIO_new_mem_buf(source.c_str(), source.size()),
                     ::BIO_free);
  BIO_ptr bio_base64(BIO_new(BIO_f_base64()), ::BIO_free);
  BIO_push(bio_base64.get(), bio_source.get());
  BIO_set_flags(bio_base64.get(), BIO_FLAGS_BASE64_NO_NL);
  size_t final_size = BIO_read(bio_base64.get(), buffer.get(), size);

  target->assign(buffer.get(), expected_size);

  return final_size == expected_size;
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

  return get_fingerprint(pkey.get());
}
}  // namespace ssl
}  // namespace shcore
