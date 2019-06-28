/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/shellcore/credential_manager.h"

#include <algorithm>

#include "mysql-secret-store/include/mysql-secret-store/api.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

using mysqlshdk::db::Connection_options;

using mysql::secret_store::api::get_available_helpers;
using mysql::secret_store::api::get_helper;
using mysql::secret_store::api::Helper_interface;
using mysql::secret_store::api::Helper_name;
using mysql::secret_store::api::Secret_spec;
using mysql::secret_store::api::Secret_type;
using mysql::secret_store::api::set_logger;

namespace {

constexpr auto k_default_helper = "default";
constexpr auto k_invalid_helper_name = "<invalid>";
constexpr auto k_disabled_helper_name = "<disabled>";

constexpr auto k_credential_helper_option = "credentialStore.helper";
constexpr auto k_save_passwords_option = "credentialStore.savePasswords";
constexpr auto k_exclude_filters_option = "credentialStore.excludeFilters";

constexpr auto k_credential_helper_cmdline = "--credential-store-helper=val";
constexpr auto k_save_passwords_cmdline = "--save-passwords=value";

constexpr auto k_credential_helper_env = "MYSQLSH_CREDENTIAL_STORE_HELPER";
constexpr auto k_save_passwords_env = "MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS";

constexpr auto k_save_passwords_always = "always";
constexpr auto k_save_passwords_never = "never";
constexpr auto k_save_passwords_prompt = "prompt";

constexpr auto k_no_such_secret_error = "Could not find the secret";
constexpr auto k_invalid_url_error = "Invalid URL";

Helper_name get_helper_by_name(const std::string &name) {
  auto helpers = get_available_helpers();
  auto helper =
      std::find_if(helpers.begin(), helpers.end(),
                   [name](const Helper_name &n) { return n.get() == name; });

  if (helper == helpers.end()) {
    throw std::runtime_error{"Credential helper named \"" + name +
                             "\" could not be found or is invalid. "
                             "See logs for more information."};
  } else {
    return *helper;
  }
}

std::string get_default_helper_name() {
  static constexpr auto default_helper =
#ifdef _WIN32
      "windows-credential";
#else  // ! _WIN32
#ifdef __APPLE__
      "keychain";
#else   // ! __APPLE__
      "login-path";
#endif  // ! __APPLE__
#endif  // ! _WIN32
  return default_helper;
}

std::string get_helper_name(const std::string &helper) {
  return helper == k_default_helper ? get_default_helper_name() : helper;
}

std::unique_ptr<Helper_interface> get_helper(const std::string &helper) {
  return get_helper(get_helper_by_name(get_helper_name(helper)));
}

std::string get_url(const Connection_options &options) {
  return options.as_uri(mysqlshdk::db::uri::formats::user_transport());
}

Secret_spec get_secret_spec(const Connection_options &options) {
  return {Secret_type::PASSWORD, get_url(options)};
}

Credential_manager::Save_passwords to_save_passwords(const std::string &str) {
  if (str == k_save_passwords_always) {
    return Credential_manager::Save_passwords::ALWAYS;
  } else if (str == k_save_passwords_never) {
    return Credential_manager::Save_passwords::NEVER;
  } else if (str == k_save_passwords_prompt) {
    return Credential_manager::Save_passwords::PROMPT;
  } else {
    throw Exception::value_error(
        "The option " + std::string{k_save_passwords_option} +
        " must be one of: " +
        str_join(std::vector<std::string>{k_save_passwords_always,
                                          k_save_passwords_prompt,
                                          k_save_passwords_never},
                 ", ") +
        ".");
  }
}

std::string to_string(Credential_manager::Save_passwords val) {
  switch (val) {
    case Credential_manager::Save_passwords::ALWAYS:
      return k_save_passwords_always;

    case Credential_manager::Save_passwords::NEVER:
      return k_save_passwords_never;

    case Credential_manager::Save_passwords::PROMPT:
      return k_save_passwords_prompt;
  }

  throw std::runtime_error{"Unknown Save_passwords value"};
}

}  // namespace

Credential_manager::Credential_manager() {
  observe_notification(SN_SHELL_OPTION_CHANGED);
}

Credential_manager &Credential_manager::get() {
  static Credential_manager instance;
  return instance;
}

void Credential_manager::initialize() {
  if (!m_is_initialized) {
    set_logger([](const std::string &msg) { log_debug2("%s", msg.c_str()); });

    if (k_disabled_helper_name == m_helper_string) {
      log_info("Credential store mechanism has been disabled by the user.");
    } else {
      try {
        set_helper(m_helper_string);
        log_info("Using credential store helper: %s",
                 m_helper->name().path().c_str());
      } catch (const std::exception &ex) {
        static constexpr auto disable_message =
            "Credential store mechanism is going to be disabled.";
        if (k_default_helper == m_helper_string) {
          // initialization of the default helper has failed
          const auto name = get_default_helper_name();
          log_error("Failed to initialize the default helper \"%s\": %s",
                    name.c_str(), ex.what());
          log_info(disable_message);
        } else {
          // stored option is invalid, inform the user
          mysqlsh::current_console()->print_error(
              "Failed to initialize the user-specified helper \"" +
              m_helper_string + "\": " + ex.what());
          mysqlsh::current_console()->print_info(disable_message);
        }
      }
    }

    m_is_initialized = true;
  }
}

void Credential_manager::register_options(Options *options) {
  using opts::cmdline;
  options->add_named_options()(
      &m_helper_string, k_default_helper, k_credential_helper_option,
      k_credential_helper_env, cmdline(k_credential_helper_cmdline),
      "Specifies the helper which is going to be used to store/retrieve the "
      "passwords.",
      [this](const std::string &val, opts::Source) {
        if (m_is_initialized) {
          try {
            set_helper(val);
          } catch (const std::exception &ex) {
            throw Exception::value_error(ex.what());
          }
        }

        return val;
      },
      [this](const std::string &) -> std::string {
        if (nullptr == m_helper) {
          return m_helper_string == k_disabled_helper_name
                     ? k_disabled_helper_name
                     : k_invalid_helper_name;
        } else {
          return m_helper_string;
        }
      })(
      &m_save_passwords, Save_passwords::PROMPT, k_save_passwords_option,
      k_save_passwords_env, cmdline(k_save_passwords_cmdline),
      "Controls automatic storage of passwords. Allowed values are: "
      "always, prompt, never.",
      [](const std::string &val, opts::Source) {
        return to_save_passwords(val);
      },
      [](const Save_passwords &val) { return to_string(val); })(
      &m_ignore_filters, std::vector<std::string>{}, k_exclude_filters_option,
      "A list of strings specifying which server URLs should be excluded "
      "from automatic storage of passwords.",
      [](const std::string &val, opts::Source) {
        try {
          auto list = Value::parse(val).as_array();
          std::vector<std::string> filters;

          for (const auto &filter : *list) {
            filters.emplace_back(filter.get_string());
          }

          return filters;
        } catch (const std::exception &ex) {
          throw Exception::value_error("The option " +
                                       std::string{k_exclude_filters_option} +
                                       " must be an array of strings");
        }
      },
      [](const std::vector<std::string> &val) {
        auto ret_val = Value::new_array();
        auto list = ret_val.as_array();

        for (const auto &filter : val) {
          list->emplace_back(filter);
        }

        return ret_val.json(false);
      });
}

void Credential_manager::handle_notification(const std::string &name,
                                             const Object_bridge_ref &,
                                             Value::Map_type_ref data) {
  if (name == SN_SHELL_OPTION_CHANGED) {
    if (data->get_string("option") == k_credential_helper_option) {
      if (!m_helper ||
          get_helper_name(m_helper_string) != m_helper->name().get()) {
        set_helper(m_helper_string);
      }
      log_info(
          "Credential store helper changed to: %s",
          m_helper ? m_helper->name().path().c_str() : k_disabled_helper_name);
    }
  }
}

void Credential_manager::set_helper(const std::string &helper) {
  if (k_disabled_helper_name == helper) {
    m_helper.reset(nullptr);
  } else {
    m_helper = get_helper(helper);
  }
}

std::vector<std::string> Credential_manager::list_credential_helpers() const {
  std::vector<std::string> helpers;

  for (const auto &h : get_available_helpers()) {
    helpers.emplace_back(h.get());
  }

  return helpers;
}

bool Credential_manager::get_password(Connection_options *options) const {
  if (m_helper) {
    std::string password;
    bool ret = m_helper->get(get_secret_spec(*options), &password);

    if (ret) {
      options->set_password(password);
    } else {
      auto error = m_helper->get_last_error();
      if (k_no_such_secret_error != error) {
        mysqlsh::current_console()->print_error(
            "Failed to retrieve the password: " + error);
      }
    }

    return ret;
  } else {
    return false;
  }
}

bool Credential_manager::save_password(const Connection_options &options) {
  if (m_helper && should_save_password(get_url(options))) {
    bool ret =
        m_helper->store(get_secret_spec(options), options.get_password());

    if (!ret) {
      mysqlsh::current_console()->print_error("Failed to store the password: " +
                                              m_helper->get_last_error());
    }

    return ret;
  } else {
    return false;
  }
}

bool Credential_manager::remove_password(const Connection_options &options) {
  if (m_helper) {
    bool ret = m_helper->erase(get_secret_spec(options));

    if (!ret) {
      mysqlsh::current_console()->print_error("Failed to erase the password: " +
                                              m_helper->get_last_error());
    }

    return ret;
  } else {
    return false;
  }
}

void Credential_manager::store_credential(const std::string &url,
                                          const std::string &credential) {
  if (!m_helper) {
    throw Exception::runtime_error(
        "Cannot save the credential, current credential helper is invalid");
  }

  if (!m_helper->store({Secret_type::PASSWORD, url}, credential)) {
    auto error = m_helper->get_last_error();

    if (std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    } else {
      throw Exception::runtime_error("Failed to save the credential: " + error);
    }
  }
}

void Credential_manager::delete_credential(const std::string &url) {
  if (!m_helper) {
    throw Exception::runtime_error(
        "Cannot delete the credential, current credential helper is invalid");
  }

  if (!m_helper->erase({Secret_type::PASSWORD, url})) {
    auto error = m_helper->get_last_error();

    if (std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    } else {
      throw Exception::runtime_error("Failed to delete the credential: " +
                                     error);
    }
  }
}

void Credential_manager::delete_all_credentials() {
  if (!m_helper) {
    throw Exception::runtime_error(
        "Cannot delete all credentials, current credential helper is invalid");
  }

  std::vector<Secret_spec> specs;

  if (!m_helper->list(&specs)) {
    throw Exception::runtime_error(
        "Failed to obtain list of credentials to delete: " +
        m_helper->get_last_error());
  }

  std::vector<std::string> errors;

  for (const auto &s : specs) {
    if (!m_helper->erase(s)) {
      errors.emplace_back("Failed to delete '" + s.url +
                          "': " + m_helper->get_last_error());
    }
  }

  if (!errors.empty()) {
    throw Exception::runtime_error("Failed to delete all credentials:\n" +
                                   shcore::str_join(errors, "\n"));
  }
}

std::vector<std::string> Credential_manager::list_credentials() const {
  if (!m_helper) {
    throw Exception::runtime_error(
        "Cannot list credentials, current credential helper is invalid");
  }

  std::vector<std::string> ret;
  std::vector<Secret_spec> specs;

  if (!m_helper->list(&specs)) {
    throw Exception::runtime_error("Failed to list credentials: " +
                                   m_helper->get_last_error());
  }

  for (const auto &s : specs) {
    if (s.type == Secret_type::PASSWORD) {
      ret.emplace_back(s.url);
    }
  }

  return ret;
}

bool Credential_manager::should_save_password(const std::string &url) {
  if (is_ignored_url(url)) {
    return false;
  }

  switch (m_save_passwords) {
    case Save_passwords::ALWAYS:
      return true;

    case Save_passwords::NEVER:
      return false;

    case Save_passwords::PROMPT:
      if (mysqlsh::current_shell_options()->get().wizards) {
        using mysqlsh::Prompt_answer;
        auto answer = mysqlsh::current_console()->confirm(
            "Save password for '" + url + "'?", Prompt_answer::NO, "&Yes",
            "&No", "Ne&ver");

        switch (answer) {
          case Prompt_answer::YES:
            return true;

          case Prompt_answer::NO:
          case Prompt_answer::NONE:
            return false;

          case Prompt_answer::ALT:
            add_ignore_filter(url);
            return false;
        }
      } else {
        return false;
      }
  }

  throw std::runtime_error{"Unknown Save_passwords value"};
}

void Credential_manager::add_ignore_filter(const std::string &filter) {
  m_ignore_filters.emplace_back(filter);
}

bool Credential_manager::is_ignored_url(const std::string &url) const {
  for (const auto &filter : m_ignore_filters) {
    if (match_glob(filter, url)) {
      return true;
    }
  }

  return false;
}

}  // namespace shcore
