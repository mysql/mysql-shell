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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_PRIVATE_KEY_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_PRIVATE_KEY_H_

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/utils/utils_ssl.h"

struct evp_pkey_st;
using EVP_PKEY = evp_pkey_st;

namespace shcore {
namespace ssl {

/**
 * Native representation of a private/public key.
 */
using Native_key = std::shared_ptr<EVP_PKEY>;

/**
 * Error thrown when a private key cannot be decrypted due to wrong password.
 */
class Decrypt_error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/**
 * A public key.
 */
class Public_key {
 public:
  Public_key(const Public_key &) = default;
  Public_key(Public_key &&) = default;

  Public_key &operator=(const Public_key &) = default;
  Public_key &operator=(Public_key &&) = default;

  ~Public_key() = default;

  /**
   * Provides a full path to a public key.
   *
   * @param dir Directory where the public key is stored.
   * @param key_name Name of the public key.
   *
   * @returns Full path to the public key.
   */
  static std::string full_path(const std::string &dir,
                               const std::string &key_name);

  /**
   * Native handle to the key.
   */
  [[nodiscard]] EVP_PKEY *ptr() const noexcept { return m_public_key.get(); }

  /**
   * Provides fingerprint of this key.
   *
   * @param hash Hash to use
   */
  std::string fingerprint(Hash hash = Hash::RESTRICTED_MD5) const;

  /**
   * Provides a string representation of this key.
   */
  std::string to_string() const;

  /**
   * Writes this key at the given path.
   *
   * @param path Full path to the saved key.
   */
  void save(const std::string &path) const;

 private:
  friend class Private_key;

  explicit Public_key(Native_key public_key)
      : m_public_key(std::move(public_key)) {}

  Native_key m_public_key;
};

/**
 * A private key.
 */
class Private_key {
 public:
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
   * The callback must return the number of characters in the passphrase or -1
   * if an error occurred.
   */
  using Password_callback = int (*)(char *buf, int size, int rwflag, void *u);

  Private_key(const Private_key &) = default;
  Private_key(Private_key &&) = default;

  Private_key &operator=(const Private_key &) = default;
  Private_key &operator=(Private_key &&) = default;

  ~Private_key() = default;

  /**
   * Provides a full path to a private key.
   *
   * @param dir Directory where the private key is stored.
   * @param key_name Name of the private key.
   *
   * @returns Full path to the private key.
   */
  static std::string full_path(const std::string &dir,
                               const std::string &key_name);

  /**
   * Generates a new private key.
   *
   * @param bits The modulus size.
   */
  static Private_key generate(int bits = 2048);

  /**
   * Loads a private key from the given path.
   *
   * @param path Full path to the private key.
   * @param passphrase Pass phrase used to decrypt the key.
   */
  static Private_key from_file(const std::string &path,
                               std::string_view passphrase = {});

  /**
   * Loads a private key from the given path.
   *
   * @param path Full path to the private key.
   * @param callback Callback called when user needs to provide the key's pass
   *                 phrase.
   * @param user_data User data passed to the callback when it's called.
   */
  static Private_key from_file(const std::string &path,
                               Password_callback callback,
                               void *user_data = nullptr);

  /**
   * Loads a private key from a string representation.
   *
   * @param contents A string representation of a private key.
   * @param callback Callback called when user needs to provide the key's pass
   *                 phrase.
   * @param user_data User data passed to the callback when it's called.
   */
  static Private_key from_string(std::string_view contents,
                                 Password_callback callback = nullptr,
                                 void *user_data = nullptr);

  /**
   * Native handle to the key.
   */
  [[nodiscard]] EVP_PKEY *ptr() const noexcept { return m_private_key.get(); }

  /**
   * The corresponding public key.
   */
  Public_key public_key() const;

  /**
   * Provides fingerprint of the corresponding public key.
   *
   * @param hash Hash to use
   */
  std::string fingerprint(Hash hash = Hash::RESTRICTED_MD5) const;

  /**
   * Provides a string representation of this key.
   */
  std::string to_string() const;

  /**
   * Writes this key at the given path.
   *
   * @param path Full path to the saved key.
   * @param password Password used to encrypt the key.
   */
  void save(const std::string &path, std::string_view password = {}) const;

 private:
  friend class Private_key_storage;

  explicit Private_key(Native_key private_key)
      : m_private_key(std::move(private_key)) {}

  Native_key m_private_key;
  mutable Native_key m_public_key;
};

/**
 * Unique identifier of a private key in the storage.
 */
class Private_key_id {
 public:
  Private_key_id() = delete;

  explicit Private_key_id(std::string id) : m_id(std::move(id)) {}

  Private_key_id(const Private_key_id &) = default;
  Private_key_id(Private_key_id &&) = default;

  Private_key_id &operator=(const Private_key_id &) = default;
  Private_key_id &operator=(Private_key_id &&) = default;

  ~Private_key_id();

  bool operator==(const Private_key_id &other) const noexcept {
    return m_id == other.m_id;
  }

  [[nodiscard]] inline const std::string &id() const noexcept { return m_id; }

  /**
   * Generates an ID from the given path.
   */
  static Private_key_id from_path(const std::string &path);

 private:
  std::string m_id;
};

/**
 * Stores the private keys.
 */
class Private_key_storage {
 public:
  Private_key_storage(const Private_key_storage &other) = delete;
  Private_key_storage(Private_key_storage &&other) = delete;

  Private_key_storage &operator=(const Private_key_storage &other) = delete;
  Private_key_storage &operator=(Private_key_storage &&other) = delete;

  ~Private_key_storage() = default;

  /**
   * Provides the single instance of this class.
   */
  static Private_key_storage &instance();

  /**
   * Places the private key in the storage.
   *
   * @param id Unique identifier of the key.
   * @param key Key to be stored.
   */
  void put(const Private_key_id &id, const Private_key &key);

  /**
   * Removes a private key from the storage.
   *
   * @param id Unique identifier of the key.
   */
  void remove(const Private_key_id &id);

  /**
   * Checks if the private key is in the storage.
   *
   * @param id Unique identifier of the key.
   *
   * @returns true If key is in storage.
   */
  bool contains(const Private_key_id &id);

  /**
   * Fetches the private key from the storage.
   *
   * @param id Unique identifier of the key.
   *
   * @returns The requested private key.
   *
   * @throws std::runtime_error If key was not found.
   */
  Private_key get(const Private_key_id &id);

  /**
   * Loads a private key from the given path. If key was loaded previously,
   * simply returns it.
   *
   * @param path Full path to the private key.
   * @param passphrase Pass phrase used to decrypt the key.
   */
  Private_key from_file(const std::string &path,
                        std::string_view passphrase = {});

  /**
   * Loads a private key from the given path. If key was loaded previously,
   * simply returns it.
   *
   * @param path Full path to the private key.
   * @param callback Callback called when user needs to provide the key's pass
   *                 phrase.
   * @param user_data User data passed to the callback when it's called.
   */
  Private_key from_file(const std::string &path,
                        Private_key::Password_callback callback,
                        void *user_data = nullptr);

 private:
  struct Private_key_id_hash {
    [[nodiscard]] size_t operator()(const Private_key_id &id) const {
      return std::hash<std::string>{}(id.id());
    }
  };

  using Key_library =
      std::unordered_map<Private_key_id, Native_key, Private_key_id_hash>;

  Private_key_storage() = default;

  template <typename... Args>
  Private_key from_file_impl(const std::string &path, Args &&... args);

  Key_library m_store;
  mutable std::mutex m_store_mutex;
};

/**
 * Automatically keeps a single private key in the storage.
 */
class Keychain {
 public:
  Keychain();

  Keychain(const Keychain &other) = delete;
  Keychain(Keychain &&other) = default;

  Keychain &operator=(const Keychain &other) = delete;
  Keychain &operator=(Keychain &&other) = default;

  ~Keychain();

  void use(const Private_key &key);

  inline const Private_key_id &id() const noexcept { return m_id; }

 private:
  void remove();

  Private_key_id m_id;
};

}  // namespace ssl
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_PRIVATE_KEY_H_
