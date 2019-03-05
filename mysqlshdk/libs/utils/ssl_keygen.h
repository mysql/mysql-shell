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

#ifndef MYSQLSHDK_LIBS_UTILS_OPEN_SSL_H_
#define MYSQLSHDK_LIBS_UTILS_OPEN_SSL_H_

#include <stdexcept>
#include <string>

namespace shcore {
namespace ssl {
/**
 * This parameter callback is used by the PEM_read_bio_PrivateKey
 * open ssl function to define a custom method of retrieving the
 * passphrase when a Key file being read requires it, taken from the
 * original documentation at
 * https://www.openssl.org/docs/man1.1.0/man3/PEM_read_bio_PrivateKey.html
 *
 * @param buf is the buffer to write the passphrase to.
 * @param size is the maximum length of the passphrase (i.e. the size of buf).
 * @param rwflag is a flag which is set to 0 when reading and 1 when writing.
 * @param u has the same value as the u parameter passed to the PEM routine.
 * It allows arbitrary data to be passed to the callback by the application
 *
 * The callback must return the number of characters in the passphrase or -1 if
 * an error occurred.
 */
using Password_callback = int (*)(char *buf, int size, int rwflag, void *u);

constexpr const char kPrivatePemSuffix[] = ".pem";
constexpr const char kPublicPemSuffix[] = "_public.pem";

class Decrypt_error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};
/**
 * Initializes the SSL library, must be called once before the rest
 * of the functions on this file are called.
 */
void init();
/**
 * Creates a private and public key pair.
 *
 * @param path The location where the files will be created.
 * @param key_name the prefix of the key file names.
 * @param bitsize the key size in bits.
 * @param passphrase the pass phrase to be used on the key file.
 * @returns the fingerprint for the public key.
 *
 * The following naming convention will be used:
 * - For the private key: <key_name>.pem
 * - For the public key:  <key_name>_public.pem
 */
std::string create_key_pair(const std::string &path,
                            const std::string &key_name, size_t bitsize = 2048,
                            const std::string &passphrase = "");

/**
 * Attempts loading a private key.
 *
 * @param path the path to the private key to be loaded.
 * @param callback function pointer to gather the passphrase in case the key is
 * encrypted.
 * @param userdata data passed to the callback.
 * @returns the fingerprint for the key.
 */
std::string load_private_key(const std::string &path,
                             Password_callback callback = nullptr,
                             void *user_data = nullptr);

/**
 * Decodes a base64 encoded string.
 *
 * @param source the base64 string to be decoded
 * @param target a string pointer where the decoded string will be stored.
 * @returns true on success decode
 */
bool decode_base64(const std::string &source, std::string *target);
}  // namespace ssl
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_OPEN_SSL_H_
