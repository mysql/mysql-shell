/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/ssl_keygen.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#if OPENSSL_VERSION_NUMBER > 0x30000000L /* 3.0.x */
#include <openssl/encoder.h>
#else
#include <openssl/hmac.h>
#endif
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

struct SSLInit {
  std::once_flag flag;
  SSLInit() {
    std::call_once(flag, []() { SSL_library_init(); });
  }
} s_ssl_init;

using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
#if OPENSSL_VERSION_NUMBER < 0x30000000L /* 3.0.x */
using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
#endif
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using EVP_PKEY_CTX_ptr =
    std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;

// EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were renamed to EVP_MD_CTX_new()
// and EVP_MD_CTX_free() in OpenSSL 1.1.
#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

using EVP_MD_CTX_ptr =
    std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

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
    const auto digest =
        restricted::md5(decoded_key.c_str(), decoded_key.size());

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

void load_private_key(const std::string &path, Password_callback pwd_cb,
                      void *user_data) {
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
      Private_key_storage::get().put(path, std::move(pkey));
    } else {
      throw_last_error("loading private key");
    }
  }
}

std::string private_key_fingerprint(const std::string &path) {
  const auto key = Private_key_storage::get().contains(path);

  if (!key.second) {
    throw std::runtime_error("Private key was not loaded: " + path);
  }

  return get_fingerprint(key.first->second.get());
}

std::string create_key_pair(const std::string &path,
                            const std::string &key_name, unsigned int bitsize,
                            const std::string &passphrase) {
  std::string private_path =
      shcore::path::join_path(path, key_name + kPrivatePemSuffix);
  std::string public_path =
      shcore::path::join_path(path, key_name + kPublicPemSuffix);

  int rc = 0;
  unsigned long e = RSA_F4;

#if OPENSSL_VERSION_NUMBER > 0x30000000L /* 3.0.x */

  EVP_PKEY_CTX_ptr pctx(EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL),
                        ::EVP_PKEY_CTX_free);

  EVP_PKEY_keygen_init(pctx.get());

  OSSL_PARAM params[3];
  params[0] = OSSL_PARAM_construct_uint("bits", &bitsize);
  params[1] = OSSL_PARAM_construct_ulong("e", &e);
  params[2] = OSSL_PARAM_construct_end();

  EVP_PKEY *praw_key = NULL;

  rc = EVP_PKEY_CTX_set_params(pctx.get(), params);
  if (rc == 1) {
    rc = EVP_PKEY_generate(pctx.get(), &praw_key);
  }

  if (rc != 1) throw_last_error("generating private key");

  EVP_KEY_ptr pkey(praw_key, ::EVP_PKEY_free);

  BIO_ptr bio_public(BIO_new_file(public_path.c_str(), "w+"), ::BIO_free);
  rc = PEM_write_bio_PUBKEY(bio_public.get(), pkey.get());

  if (rc != 1) throw_last_error("saving public key file");

  rc = shcore::set_user_only_permissions(public_path);
  if (rc != 0) {
    throw std::runtime_error(
        "Error setting file permissions on public key file: " + public_path);
  }

  BIO_ptr bio_private(BIO_new_file(private_path.c_str(), "w+"), ::BIO_free);
  OSSL_ENCODER_CTX *ctx = OSSL_ENCODER_CTX_new_for_pkey(
      pkey.get(), OSSL_KEYMGMT_SELECT_ALL, "PEM", NULL, NULL);

  if (!passphrase.empty()) {
    OSSL_ENCODER_CTX_set_cipher(ctx, "aes256", NULL);
    unsigned char *passwd = reinterpret_cast<unsigned char *>(
        const_cast<char *>(passphrase.c_str()));
    OSSL_ENCODER_CTX_set_passphrase(ctx, passwd, passphrase.size());
  }

  OSSL_ENCODER_to_bio(ctx, bio_private.get());

#else
  BN_ptr bn(BN_new(), ::BN_free);

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
#endif

  if (rc != 1) throw_last_error("saving private key file");

  rc = shcore::set_user_only_permissions(private_path);
  if (rc != 0) {
    throw std::runtime_error(
        "Error setting file permissions on private key file: " + public_path);
  }

  return get_fingerprint(pkey.get());
}

std::vector<unsigned char> sha256(const char *data, size_t size) {
  EVP_MD_CTX_ptr mctx(EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
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

namespace restricted {

std::vector<unsigned char> md5(const char *data, size_t size) {
  EVP_MD_CTX_ptr mdctx(EVP_MD_CTX_new(), ::EVP_MD_CTX_free);

  // allow MD5 in FIPS mode
#if OPENSSL_VERSION_NUMBER >= 0x30000000L /* 3.0.x */
  std::unique_ptr<EVP_MD, decltype(&::EVP_MD_free)> md_ptr(
      EVP_MD_fetch(nullptr, "MD5", "fips=no"), ::EVP_MD_free);
  const auto md = md_ptr.get();
#else  // < 3.0.x
  const auto md = EVP_md5();
#ifdef EVP_MD_CTX_FLAG_NON_FIPS_ALLOW
  EVP_MD_CTX_set_flags(mdctx.get(), EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
#endif  // EVP_MD_CTX_FLAG_NON_FIPS_ALLOW
#endif  // < 3.0.x

  EVP_DigestInit_ex(mdctx.get(), md, nullptr);
  EVP_DigestUpdate(mdctx.get(), data, size);

  unsigned int final_size;
  std::vector<unsigned char> md_value;
  md_value.resize(16);

  EVP_DigestFinal_ex(mdctx.get(), md_value.data(), &final_size);

  return md_value;
}

}  // namespace restricted

#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */

HMAC_CTX *HMAC_CTX_new() {
  HMAC_CTX *ctx = new HMAC_CTX();
  HMAC_CTX_init(ctx);
  return ctx;
}

void HMAC_CTX_free(HMAC_CTX *ctx) {
  HMAC_CTX_cleanup(ctx);
  delete ctx;
}

#endif  // OPENSSL_VERSION_NUMBER < 0x10100000L

std::vector<unsigned char> hmac_sha256(const std::vector<unsigned char> &key,
                                       const std::string &data) {
  std::vector<unsigned char> hash;

#if OPENSSL_VERSION_NUMBER > 0x30000000L /* 3.0.x */
  EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
  std::unique_ptr<EVP_MAC_CTX, decltype(&EVP_MAC_CTX_free)> ctx(
      EVP_MAC_CTX_new(mac), EVP_MAC_CTX_free);

  OSSL_PARAM params[2], *p = params;

  char digest[] = "SHA256";

  *p++ = OSSL_PARAM_construct_utf8_string("digest", digest, 0);
  *p = OSSL_PARAM_construct_end();

  if (!EVP_MAC_init(ctx.get(), key.data(), key.size(), params)) {
    throw std::runtime_error("Cannot initialize HMAC context.");
  }

  if (!EVP_MAC_update(ctx.get(),
                      reinterpret_cast<const unsigned char *>(data.data()),
                      data.size())) {
    throw std::runtime_error("Cannot update HMAC signature.");
  }

  size_t hash_len = EVP_MAX_MD_SIZE;
  hash.resize(hash_len);

  if (!EVP_MAC_final(ctx.get(), hash.data(), &hash_len, hash_len)) {
    throw std::runtime_error("Cannot finalize HMAC signature.");
  }
#else
  std::unique_ptr<HMAC_CTX, decltype(&HMAC_CTX_free)> ctx(HMAC_CTX_new(),
                                                          HMAC_CTX_free);
  const auto md = EVP_sha256();

  if (!HMAC_Init_ex(ctx.get(), key.data(), key.size(), md, nullptr)) {
    throw std::runtime_error("Cannot initialize HMAC context.");
  }

  if (!HMAC_Update(ctx.get(),
                   reinterpret_cast<const unsigned char *>(data.data()),
                   data.size())) {
    throw std::runtime_error("Cannot update HMAC signature.");
  }

  unsigned int hash_len = EVP_MAX_MD_SIZE;
  hash.resize(hash_len);

  if (!HMAC_Final(ctx.get(), hash.data(), &hash_len)) {
    throw std::runtime_error("Cannot finalize HMAC signature.");
  }
#endif

  hash.resize(hash_len);

  return hash;
}
}  // namespace ssl
}  // namespace shcore
