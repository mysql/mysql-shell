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

// This include is needed before including <map> on the header file
// because python defines _XOPEN_SOURCE and _POSIX_C_SOURCE without
// verifying if they were already defined, resulting in a compilation
// issue in some platforms
#include "mysqlshdk/include/scripting/python_context.h"

#include "modules/util/oci_setup.h"

#include <algorithm>
#include <functional>
#include <regex>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/include/shellcore/shell_python.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {
constexpr const char kUser[] = "user";
constexpr const char kTenancy[] = "tenancy";
constexpr const char kRegion[] = "region";
constexpr const char kDefaultOciKeyName[] = "oci_api_key";
constexpr const char kRsaMethod[] = "rsa_method";
constexpr const char kRsaMethodDefault[] =
    "Use the default API key [oci_api_key]";
constexpr const char kRsaMethodCreate[] = "Create new API key";
constexpr const char kRsaMethodExisting[] = "Use an existing API key";
constexpr const char kKeyName[] = "key_name";
constexpr const char kKeyFile[] = "key_file";
constexpr const char kPublicKeyFile[] = "public_key_file";
constexpr const char kFingerprint[] = "fingerprint";
constexpr const char kPassphrase[] = "pass_phrase";
constexpr const char kPassphrasePrompt[] = "pass_phrase_prompt";
constexpr const char kUsePassphrase[] = "use_pass_phrase";
constexpr const char kConfirmPassphrase[] = "confirm_pass_phrase";
constexpr const char kStorePassphrase[] = "store_pass_phrase";

constexpr const char kCaToronto1[] = "ca-toronto-1";
constexpr const char kEuFrankfurt1[] = "eu-frankfurt-1";
constexpr const char kUkLondon1[] = "uk-london-1";
constexpr const char kUsAshburn1[] = "us-ashburn-1";
constexpr const char kUsPhoenix1[] = "us-phoenix-1";

constexpr const char kDefaultProfile[] = "DEFAULT";

std::string validate_ocid(const std::string &value) {
  if (!std::regex_match(value, std::regex("^([0-9a-zA-Z-_]+[.:])([0-9a-zA-Z-_]*"
                                          "[.:]){3,}([0-9a-zA-Z-_]+)$")))
    return "Invalid OCID Format.";
  return "";
}

std::shared_ptr<shcore::Python_context> get_python_context(
    const std::shared_ptr<shcore::Shell_core> &shell) {
  std::shared_ptr<shcore::Python_context> python;

  auto language = shell->language_object(shcore::IShell_core::Mode::Python);
  if (language) {
    auto python_language = dynamic_cast<shcore::Shell_python *>(language);

    if (python_language) python = python_language->python_context();
  }

  if (!python)
    throw std::runtime_error(
        "Python language is required for OCI and it is unavailable.");

  return python;
}

void register_oci_sdk(const std::shared_ptr<shcore::Python_context> &python) {
  std::string oci_sdk_path;
  oci_sdk_path = shcore::path::join_path(shcore::get_mysqlx_home_path(),
                                         "share", "mysqlsh", "oci_sdk");
  if (!shcore::is_folder(oci_sdk_path))
    throw std::runtime_error("SDK not found at " + oci_sdk_path +
                             ", shell installation likely invalid.");

  std::vector<std::string> code;
  code.push_back("import sys");

#ifdef _WIN32
  auto path = shcore::str_replace(oci_sdk_path, "\\", "\\\\");
#else
  auto path = oci_sdk_path;
#endif
  code.push_back("sys.path.insert(0, '" + path + "')");

  auto eggs = shcore::listdir(oci_sdk_path);
  for (const auto &egg : eggs) {
    if (shcore::str_iendswith(egg.c_str(), ".egg")) {
      path = shcore::path::join_path(oci_sdk_path, egg);
#ifdef _WIN32
      path = shcore::str_replace(path, "\\", "\\\\");
#endif
      code.push_back("sys.path.insert(0, '" + path + "')");
    }
  }

  if (!python->raw_execute(code))
    throw std::runtime_error(
        "Error enabling the OCI SDK: Failed setting up dependencies.");
}

}  // namespace

namespace oci {
void init(const std::shared_ptr<shcore::Shell_core> &shell) {
  try {
    // Ensure Python context is initialized
    shell->init_py();

    // Gets the python context to execute all the operations
    auto python = get_python_context(shell);

    register_oci_sdk(python);
  } catch (const std::runtime_error &err) {
    mysqlsh::current_console()->print_error(err.what());
  }
}

void load_profile(const std::string &user_profile,
                  const std::shared_ptr<shcore::Shell_core> &shell) {
  auto console = mysqlsh::current_console();

  try {
    // Gets the python context to execute all the operations
    auto python = get_python_context(shell);

    // Imports the oci module
    console->println();
    if (!python->raw_execute("import oci"))
      throw std::runtime_error(
          "Error enabling the OCI SDK: Error importing the oci module");

    // If needed, automatically loads the default objects
    std::string profile(user_profile);

    if (profile.empty()) profile = "DEFAULT";

    mysqlsh::oci::Oci_setup setup;

    // Setups or load the corresponding profile
    if (!setup.has_profile(profile))
      setup.create_profile(profile);
    else {
      console->print_info("Loading the OCI configuration for the profile '" +
                          profile + "'...");
      setup.load_profile(profile);
    }

    if (!setup.is_cancelled()) {
      std::string oci_cfg_path = setup.get_cfg_path();
#ifdef _WIN32
      oci_cfg_path = shcore::str_replace(oci_cfg_path, "\\", "\\\\");
#endif

      console->println();
      std::vector<std::string> code = {"config = oci.config.from_file('" +
                                       oci_cfg_path + "', '" + profile + "')"};

      if (setup.has(kPassphrase))
        code.push_back("config['pass_phrase'] = '" + setup[kPassphrase] + "'");

      code.push_back("identity = oci.identity.IdentityClient(config)");
      code.push_back("compute = oci.core.ComputeClient(config)");

      if (!python->raw_execute(code))
        throw std::runtime_error("Error loading the configuration.");

      console->print_info(
          "The config, identity and compute objects have been initialized.");
      console->print_info("Type '\\? oci' for help.");
    } else {
      console->print_info("Loading the OCI configuration has been cancelled.");
    }
  } catch (const std::runtime_error &err) {
    console->print_warning(err.what());
  }
}

Oci_setup::Oci_setup()
    : shcore::wizard::Wizard(),
      m_config(mysqlshdk::config::Case::SENSITIVE),
      m_oci_path(shcore::path::join_path(shcore::path::home(), ".oci")),
      m_oci_cfg_path(shcore::path::join_path(m_oci_path, "config")) {
  if (shcore::file_exists(m_oci_cfg_path)) {
    m_config.read(m_oci_cfg_path);
  }
}

void Oci_setup::init_create_profile_wizard() {
  m_steps.clear();
  reset();

  std::string oci_api_name(kDefaultOciKeyName);
  oci_api_name.append(shcore::ssl::kPrivatePemSuffix);
  std::string oci_key_path = shcore::path::join_path(m_oci_path, oci_api_name);

  bool default_key_exists = shcore::file_exists(oci_key_path);

  // Step : Retrieve the user OCID
  add_prompt(kUser)
      .description({"For details about how to get the user OCID, tenancy OCID "
                    "and region required for the OCI configuration visit:\n"
                    "  https://docs.cloud.oracle.com/iaas/Content/API/Concepts/"
                    "apisigningkey.htm#How3\n"})
      .prompt("Please enter your USER OCID: ")
      .link(shcore::wizard::K_NEXT, kTenancy)
      .validator(validate_ocid);

  // Step : Retrieve the tenancy OCID
  add_prompt(kTenancy)
      .prompt("Please enter your TENANCY OCID: ")
      .link(shcore::wizard::K_NEXT, kRegion)
      .validator(validate_ocid);

  // Step : Retrieve the region
  add_select(kRegion)
      .description({"The region of the tenancy needs to be specified, "
                    "available options include:"})
      .items({kCaToronto1, kEuFrankfurt1, kUkLondon1, kUsAshburn1, kUsPhoenix1})
      .allow_custom(true)
      .prompt(
          "Please enter the number of a REGION listed above or type a custom "
          "region name: ")
      .link(shcore::wizard::K_NEXT, kRsaMethod)
      .validator([](const std::string &data) -> std::string {
        auto stripped = shcore::str_strip(data);
        if (stripped.empty())
          return "The region name can not be empty or blank characters.";
        return "";
      });

  // Step : Determine how the API keys wil be defined
  std::vector<std::string> key_options;

  // Using the default one only is available if it exists
  if (default_key_exists) key_options.push_back(kRsaMethodDefault);

  key_options.push_back(kRsaMethodCreate);
  key_options.push_back(kRsaMethodExisting);

  add_select(kRsaMethod)
      .description({"To access Oracle Cloud Resources an API Key is "
                    "required, available options include:"})
      .items(key_options)
      .prompt("Please enter the number of an option listed above: ")
      .link(kRsaMethodCreate, kKeyName)
      .link(kRsaMethodExisting, kKeyFile)
      .link(kRsaMethodDefault, shcore::wizard::K_DONE)
      .validator([this, oci_key_path](const std::string &value) -> std::string {
        std::string error;

        if (value == kRsaMethodDefault) {
          try {
            load_private_key(oci_key_path,
                             "The selected API key requires a passphrase: ");
          } catch (const std::runtime_error &err) {
            error = err.what();
          }
        }

        return error;
      })
      .on_leave([default_key_exists, oci_key_path](
                    const std::string &value, shcore::wizard::Wizard *wizard) {
        // The next step will depend on the method selected to define the API
        // key
        // - The path is known when the default is selected
        // - If passphrase was required then option should be offered to store
        // it
        // - When creating a new API key but no default key exist, the default
        // name is used
        if (value == kRsaMethodDefault) {
          if (wizard->has(kPassphrase))
            wizard->relink(kRsaMethod, kRsaMethodDefault, kStorePassphrase);
        } else if (value == kRsaMethodCreate && !default_key_exists) {
          (*wizard)[kKeyName] = kDefaultOciKeyName;
          wizard->relink(kRsaMethod, kRsaMethodCreate, kUsePassphrase);
        }
      });

  add_prompt(kKeyName)
      .prompt("Enter the name of the new API key: ")
      .link(shcore::wizard::K_NEXT, kUsePassphrase)
      .validator([this](const std::string &name) -> std::string {
        std::string error;

        auto stripped = shcore::str_strip(name);
        if (stripped.empty()) {
          error = "The API key name can not be empty or blank characters.";
        } else if (!shcore::is_valid_identifier(stripped)) {
          error =
              "The key name must be a valid identifier: only letters, numbers "
              "and underscores are allowed.";
        } else {
          std::string pri = shcore::path::join_path(
              m_oci_path, stripped + shcore::ssl::kPrivatePemSuffix);
          std::string pub = shcore::path::join_path(
              m_oci_path, stripped + shcore::ssl::kPublicPemSuffix);

          if (shcore::file_exists(pri)) {
            error =
                "A private API key with the indicated name already exists, use "
                "a "
                "different name.";
          } else if (shcore::file_exists(pub)) {
            error =
                "A public API key with the indicated name already exists, use "
                "a "
                "different name.";
          }
        }

        return error;
      });

  add_confirm(kUsePassphrase)
      .prompt("Do you want to protect the API key with a passphrase?")
      .default_answer(mysqlsh::Prompt_answer::YES)
      .link(shcore::wizard::K_YES, kPassphrase)
      .link(shcore::wizard::K_NO, shcore::wizard::K_DONE);

  add_prompt(kPassphrase)
      .prompt("Enter a passphrase for the API key: ")
      .hide_input(true)
      .link(shcore::wizard::K_NEXT, kConfirmPassphrase)
      .validator([](const std::string &pwd) -> std::string {
        if (pwd.empty()) return "The passphrase can not be empty.";
        return "";
      });

  add_prompt(kConfirmPassphrase)
      .prompt("Enter the passphrase again for confirmation: ")
      .hide_input(true)
      .link(shcore::wizard::K_NEXT, kStorePassphrase)
      .validator([this](const std::string &data) -> std::string {
        std::string passphrase = m_data[kPassphrase];
        if (passphrase != data)
          return "The confirmation passphrase does not match the initial "
                 "passphrase.";
        return "";
      });

  add_prompt(kKeyFile)
      .prompt("Enter the location of the existing API key: ")
      .link(shcore::wizard::K_NEXT, shcore::wizard::K_DONE)
      .validator([this](const std::string &path) -> std::string {
        auto stripped = shcore::str_strip(path);
        if (stripped.empty())
          return "The path can not be empty or blank characters.";
        return load_private_key(stripped,
                                "The selected API key requires a passphrase: ");
      })
      .on_leave([](const std::string &value, shcore::wizard::Wizard *wizard) {
        if (wizard->has(kPassphrase))
          wizard->relink(kKeyFile, shcore::wizard::K_NEXT, kStorePassphrase);
      });

  add_confirm(kStorePassphrase)
      .prompt("Do you want to write your passphrase to the config file?")
      .default_answer(mysqlsh::Prompt_answer::NO)
      .link(shcore::wizard::K_NEXT, shcore::wizard::K_DONE);
}

std::string Oci_setup::load_private_key(const std::string &path,
                                        const std::string &inital_prompt) {
  // Password callback in case the key file is encrypted
  auto pwd_cb = [](char *buf, int size, int rwflag, void *u) -> int {
    std::string pass_phrase;

    auto wizard = static_cast<shcore::wizard::Wizard *>(u);

    auto console = current_console();

    std::string msg = wizard->has(kPassphrase)
                          ? "Wrong passphrase, please try again: "
                          : (*wizard)[kPassphrasePrompt];

    auto result = console->prompt_password(msg, &pass_phrase);

    if (result != shcore::Prompt_result::Ok) {
      wizard->cancel();
      return -1;
    } else {
      wizard->set(kPassphrase, pass_phrase);

      memcpy(buf, pass_phrase.c_str(), pass_phrase.size());

      return pass_phrase.size();
    }
  };

  std::string error;
  try {
    m_data[kPassphrasePrompt] = inital_prompt;
    std::string fingerprint;
    do {
      try {
        fingerprint = shcore::ssl::load_private_key(path, pwd_cb, this);
      } catch (const shcore::ssl::Decrypt_error &err) {
        // Ignore, user gave wrong password
      }
    } while (!is_cancelled() && fingerprint.empty());

    m_data[kKeyFile] = path;
    m_data[kFingerprint] = fingerprint;
  } catch (std::runtime_error &err) {
    error = err.what();
  }

  return error;
}

bool Oci_setup::has_profile(const std::string &profile_name) {
  return m_config.has_group(profile_name);
}

void Oci_setup::create_profile(const std::string &profile_name) {
  std::string profile(profile_name);

  if (profile.empty()) profile = kDefaultProfile;

  bool new_config = false;
  if (!shcore::file_exists(m_oci_cfg_path)) {
    new_config = true;
    shcore::create_directory(m_oci_path);
  }

  auto console = current_console();

  if (!m_config.has_group(profile)) {
    console->print_info(
        "This MySQL Shell Wizard will help you to configure a new OCI "
        "profile.");

    console->println("Configuring OCI for the profile '" + profile +
                     "'. Press Ctrl+C to cancel the wizard at any time.");
    console->println();

    init_create_profile_wizard();

    if (execute(kUser)) {
      try {
        if (m_data[kRsaMethod] == kRsaMethodCreate) {
          std::string key_name = m_data[kKeyName];

          std::string fingerprint = shcore::ssl::create_key_pair(
              m_oci_path, key_name, 2048, m_data[kPassphrase]);

          m_data[kKeyFile] = shcore::path::join_path(
              m_oci_path, key_name + shcore::ssl::kPrivatePemSuffix);
          m_data[kPublicKeyFile] = shcore::path::join_path(
              m_oci_path, key_name + shcore::ssl::kPublicPemSuffix);
          m_data[kFingerprint] = fingerprint;
        }

        m_config.add_group(profile);
        m_config.set(profile, kUser, m_data[kUser]);
        m_config.set(profile, kTenancy, m_data[kTenancy]);
        m_config.set(profile, kRegion, m_data[kRegion]);
        m_config.set(profile, kKeyFile, m_data[kKeyFile]);
        m_config.set(profile, kFingerprint, m_data[kFingerprint]);

        if (m_data[kStorePassphrase] == shcore::wizard::K_YES)
          m_config.set(profile, kPassphrase, m_data[kPassphrase]);

        m_config.write(m_oci_cfg_path);

        console->println();
        console->println("A new OCI profile named '" + profile +
                         "' has been created "
                         "*successfully*.");
        console->println("");

        if (m_data[kRsaMethod] == kRsaMethodCreate) {
          if (new_config) {
            console->println(
                "The configuration and keys have been stored in the following "
                "directory:");
          } else {
            console->println(
                "The keys have been stored in the following directory:");
          }

          console->println("  " + m_oci_path);
          console->println("");
        }

        console->println(
            "Please ensure your *public* API key is uploaded to your OCI User "
            "Settings page.");

        if (has(kPublicKeyFile))
          console->println("It can be found at " + m_data[kPublicKeyFile]);

      } catch (const std::runtime_error &err) {
        console->print_error(err.what());
      }
    } else {
      console->println(
          "The wizard has been canceled. No changes to the system were made.");
    }
  }
}

void Oci_setup::load_profile(const std::string &profile_name) {
  auto console = current_console();

  if (m_config.has_group(profile_name)) {
    if (m_config.has_option(profile_name, kKeyFile)) {
      std::string key_file = *m_config.get(profile_name, kKeyFile);

      auto pwd_cb = [](char *buf, int size, int rwflag, void *u) -> int {
        auto wizard = static_cast<shcore::wizard::Wizard *>(u);

        std::string passphrase;
        if (wizard->has(kPassphrase)) {
          passphrase = (*wizard)[kPassphrase];
          memcpy(buf, passphrase.c_str(), passphrase.size());
        }

        // Ensure the wizard knows a passphrase is required
        (*wizard)[kUsePassphrase] = shcore::wizard::K_YES;

        return passphrase.size();
      };

      // If there's a pass phrase on the config file
      if (m_config.has_option(profile_name, kPassphrase)) {
        m_data[kPassphrase] = *m_config.get(profile_name, kPassphrase);
      }

      bool wrong_pass_phrase = false;
      bool prompt_pwd = false;
      try {
        shcore::ssl::load_private_key(key_file, pwd_cb, this);
      } catch (const std::runtime_error &err) {
        wrong_pass_phrase = true;
      }

      if (wrong_pass_phrase) {
        if (has(kUsePassphrase)) {
          if (has(kPassphrase)) {
            console->print_warning("The API key for '" + profile_name +
                                   "' requires a passphrase to be successfully "
                                   "used and the one in the configuration is "
                                   "incorrect.");
            remove(kPassphrase);
          }

          prompt_pwd = true;
        }
      } else {
        if (!has(kUsePassphrase)) {
          if (has(kPassphrase)) {
            console->print_warning("The API key for '" + profile_name +
                                   "' does not require a passphrase but one is "
                                   "configured on the profile, ignoring it.");
          }
        }
      }

      if (prompt_pwd) {
        while (!is_cancelled() && !has(kPassphrase)) {
          load_private_key(key_file, "Please enter the API key passphrase: ");
        }
      }
    } else {
      console->println("The OCI configuration is missing a private API key.");
    }
  } else {
    console->println("The OCI configuration for the profile '" + profile_name +
                     "' does not exist.");
  }
}

}  // namespace oci
}  // namespace mysqlsh
