/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/oci/config_credentials_provider.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

namespace {

using mysqlsh::current_console;

using shcore::ssl::Private_key;
using Loader = Private_key (*)(std::string_view, Private_key::Password_callback,
                               void *);

// loads the private key prompting for password
Private_key load_private_key(std::string_view pk, Loader loader) {
  struct Callback_data {
    int attempt = 0;
    bool cancelled = false;
  } data;

  const auto pwd_cb = [](char *buf, int size, int, void *u) {
    auto d = reinterpret_cast<Callback_data *>(u);
    const auto msg = d->attempt++ ? "Wrong passphrase, please try again: "
                                  : "Please enter the API key passphrase: ";
    std::string passphrase;
    shcore::on_leave_scope clear_passphrase{
        [&passphrase]() { shcore::clear_buffer(passphrase); }};

    if (current_console()->prompt_password(msg, &passphrase) !=
        shcore::Prompt_result::Ok) {
      d->cancelled = true;
      return 0;
    } else {
      const auto passphrase_size = std::min<int>(size, passphrase.size());
      memcpy(buf, passphrase.c_str(), passphrase_size);

      return passphrase_size;
    }
  };

  do {
    try {
      return loader(pk, pwd_cb, &data);
    } catch (const shcore::ssl::Decrypt_error &) {
      // Ignore, user gave wrong password
    }
  } while (!data.cancelled);

  // user did not provide a valid passphrase
  throw std::runtime_error{"Invalid passphrase"};
}

// loads the private key with the given passphrase
Private_key load_private_key(std::string_view pk, std::string_view pass,
                             Loader loader) {
  struct Callback_data {
    std::string_view passphrase;
    bool requires_passphrase = false;
  } data;
  data.passphrase = pass;

  try {
    // Password callback in case the key file is encrypted
    const auto pwd_cb = [](char *buf, int size, int, void *u) {
      auto d = reinterpret_cast<Callback_data *>(u);
      int result = 0;

      if (!d->passphrase.empty()) {
        result = std::min<int>(size, d->passphrase.size());
        memcpy(buf, d->passphrase.data(), result);
      }

      // we now know that a passphrase is required
      d->requires_passphrase = true;

      return result;
    };

    auto private_key = loader(pk, pwd_cb, &data);

    if (!data.requires_passphrase && !data.passphrase.empty()) {
      current_console()->print_warning(
          "The API key does not require a passphrase but one is configured on "
          "the profile, ignoring it.");
    }

    return private_key;
  } catch (const shcore::ssl::Decrypt_error &) {
    // wrong passphrase?
    if (!data.requires_passphrase) {
      // passphrase callback was not called, some other error
      throw;
    }
  }

  // wrong passphrase
  if (!data.passphrase.empty()) {
    current_console()->print_warning(
        "The API key requires a passphrase and the configured one is "
        "incorrect.");
  }

  return load_private_key(pk, loader);
}

}  // namespace

Config_credentials_provider::Config_credentials_provider(
    const std::string &config_file, const std::string &config_profile,
    std::string name, bool allow_key_content_env_var)
    : Oci_credentials_provider(std::move(name)),
      m_config(config_file, config_profile) {
  set_region(config_option(Entry::REGION));
  set_tenancy_id(config_option(Entry::TENANCY));

  load_key(allow_key_content_env_var);
}

void Config_credentials_provider::load_key(bool allow_key_content_env_var) {
  static constexpr auto k_key_content_env_var = "OCI_CLI_KEY_CONTENT";
  shcore::ssl::Private_key_id key_id{""};
  std::string pk;
  Loader loader;

  if (const auto env_var = shcore::get_env(k_key_content_env_var);
      allow_key_content_env_var && env_var.has_value()) {
    log_info("The API key contents set via '%s' environment variable",
             k_key_content_env_var);
    pk = *env_var;
    key_id = shcore::ssl::Private_key_id{k_key_content_env_var};
    loader = shcore::ssl::Private_key::from_string;
  } else {
    pk = config_option(Entry::KEY_FILE);
    key_id = shcore::ssl::Private_key_id::from_path(pk);
    loader = shcore::ssl::Private_key::from_file;
  }

  assert(!key_id.id().empty());

  if (!shcore::ssl::Private_key_storage::instance().contains(key_id)) {
    try {
      auto passphrase = config_option(Entry::PASS_PHRASE);
      shcore::on_leave_scope clear_passphrase{
          [&passphrase]() { shcore::clear_buffer(passphrase); }};

      shcore::ssl::Private_key_storage::instance().put(
          key_id, load_private_key(pk, passphrase, loader));
    } catch (const std::exception &e) {
      throw std::runtime_error(
          shcore::str_format("Cannot load API key associated with OCI "
                             "configuration profile named '%s': %s",
                             config_profile().c_str(), e.what()));
    }
  }

  m_credentials.private_key_id = key_id.id();
}

}  // namespace oci
}  // namespace mysqlshdk
