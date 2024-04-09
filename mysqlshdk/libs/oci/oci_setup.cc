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

#include "mysqlshdk/libs/oci/oci_setup.h"

#include <algorithm>
#include <utility>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {

namespace {

using mysqlsh::current_console;

constexpr const char kKeyFile[] = "key_file";
constexpr const char kPassphrase[] = "pass_phrase";

}  // namespace

namespace oci {

Oci_setup::Oci_setup(const std::string &oci_config_path)
    : m_config(mysqlshdk::config::Case::SENSITIVE,
               mysqlshdk::config::Escape::NO) {
  m_config.read(oci_config_path);
}

std::string Oci_setup::load_private_key(const std::string &path) {
  struct Callback_data {
    int attempt = 0;
    bool cancelled = false;
  } data;

  // Password callback in case the key file is encrypted
  const auto pwd_cb = [](char *buf, int size, int, void *u) {
    auto d = reinterpret_cast<Callback_data *>(u);
    const auto msg = d->attempt++ ? "Wrong passphrase, please try again: "
                                  : "Please enter the API key passphrase: ";
    std::string passphrase;
    shcore::on_leave_scope clear_passphrase{
        [&passphrase]() { shcore::clear_buffer(&passphrase); }};

    if (current_console()->prompt_password(msg, &passphrase) !=
        shcore::Prompt_result::Ok) {
      d->cancelled = true;
      return 0;
    } else {
      const auto passphrase_size =
          std::min(size, static_cast<int>(passphrase.size()));
      memcpy(buf, passphrase.c_str(), passphrase_size);

      return passphrase_size;
    }
  };

  std::string error;

  try {
    bool success = false;

    do {
      try {
        shcore::ssl::Private_key_storage::instance().from_file(path, pwd_cb,
                                                               &data);
        success = true;
      } catch (const shcore::ssl::Decrypt_error &) {
        // Ignore, user gave wrong password
      }
    } while (!data.cancelled && !success);
  } catch (const std::runtime_error &err) {
    error = err.what();
  }

  return error;
}

bool Oci_setup::has_profile(const std::string &profile_name) {
  return m_config.has_group(profile_name);
}

void Oci_setup::load_profile(const std::string &profile_name) {
  const auto console = current_console();

  if (!m_config.has_group(profile_name)) {
    console->println("The OCI configuration for the profile '" + profile_name +
                     "' does not exist.");
    return;
  }

  if (!m_config.has_option(profile_name, kKeyFile)) {
    console->println("The OCI configuration is missing a private API key.");
    return;
  }

  struct Callback_data {
    std::string passphrase;
    bool requires_passphrase = false;
  } data;
  shcore::on_leave_scope clear_passphrase{
      [&data]() { shcore::clear_buffer(&data.passphrase); }};

  // If there's a pass phrase on the config file
  if (m_config.has_option(profile_name, kPassphrase)) {
    data.passphrase = *m_config.get(profile_name, kPassphrase);
  }

  bool wrong_pass_phrase = false;
  const auto key_file = *m_config.get(profile_name, kKeyFile);

  try {
    const auto pwd_cb = [](char *buf, int size, int, void *u) {
      auto d = reinterpret_cast<Callback_data *>(u);
      int result = 0;

      if (!d->passphrase.empty()) {
        result = std::min(size, static_cast<int>(d->passphrase.size()));
        memcpy(buf, d->passphrase.c_str(), result);
      }

      // Ensure the wizard knows a passphrase is required
      d->requires_passphrase = true;

      return result;
    };

    shcore::ssl::Private_key_storage::instance().from_file(key_file, pwd_cb,
                                                           &data);
  } catch (const shcore::ssl::Decrypt_error &) {
    wrong_pass_phrase = true;
  }

  bool prompt_pwd = false;

  if (wrong_pass_phrase) {
    if (data.requires_passphrase) {
      if (!data.passphrase.empty()) {
        console->print_warning(
            "The API key for '" + profile_name +
            "' requires a passphrase to be successfully used and the one in "
            "the configuration is incorrect.");
      }

      prompt_pwd = true;
    }
  } else {
    if (!data.requires_passphrase && !data.passphrase.empty()) {
      console->print_warning("The API key for '" + profile_name +
                             "' does not require a passphrase but one is "
                             "configured on the profile, ignoring it.");
    }
  }

  if (prompt_pwd) {
    if (const auto error = load_private_key(key_file); !error.empty()) {
      console->print_warning("Failed to load the API key for '" + profile_name +
                             "': " + error);
    }
  }
}

}  // namespace oci
}  // namespace mysqlshdk
