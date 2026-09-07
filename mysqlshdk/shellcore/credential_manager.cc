/*
 * Copyright (c) 2018, 2026, Oracle and/or its affiliates.
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

#include "mysqlshdk/shellcore/credential_manager.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mysql-secret-store/include/mysql-secret-store/api.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

using mysql::secret_store::api::get_available_helpers;
using mysql::secret_store::api::get_helper;
using mysql::secret_store::api::Helper_interface;
using mysql::secret_store::api::Helper_name;
using mysql::secret_store::api::Secret_spec;
using mysql::secret_store::api::Secret_type;
using mysql::secret_store::api::set_logger;
using mysql::secret_store::api::validate_url;

namespace {

constexpr auto k_default_helper = "default";
constexpr auto k_invalid_helper_name = "<invalid>";
constexpr auto k_disabled_helper_name = "<disabled>";

constexpr auto k_credential_helper_option = "credentialStore.helper";
constexpr auto k_save_passwords_option = "credentialStore.savePasswords";
constexpr auto k_exclude_filters_option = "credentialStore.excludeFilters";

constexpr auto k_credential_helper_cmdline = "--credential-store-helper=<h>";
constexpr auto k_save_passwords_cmdline = "--save-passwords=<value>";

constexpr auto k_credential_helper_env = "MYSQLSH_CREDENTIAL_STORE_HELPER";
constexpr auto k_save_passwords_env = "MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS";

constexpr auto k_save_passwords_always = "always";
constexpr auto k_save_passwords_never = "never";
constexpr auto k_save_passwords_prompt = "prompt";

constexpr auto k_no_such_secret_error = "Could not find the secret";
constexpr auto k_invalid_url_error = "Invalid URL";
constexpr auto k_invalid_secret_error =
    "Secret contains a disallowed character";
constexpr std::string_view k_internal_secret_id_prefix = "mysqlsh:";
constexpr std::string_view k_mfa_password_id_prefix = "mysqlsh:mfa-password:";

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

Secret_spec get_secret_spec(const std::string &url) {
  return {Secret_type::PASSWORD, url};
}

Secret_spec get_secret_spec(const mysqlshdk::IConnection &options) {
  return get_secret_spec(Credential_manager::normalize_credential_url(options));
}

Secret_spec get_mfa_secret_spec(const std::string &url, int factor) {
  assert(factor > 0 && factor < mysqlshdk::db::k_max_auth_factors);
  return {Secret_type::GENERIC, std::string{k_mfa_password_id_prefix} +
                                    std::to_string(factor + 1) + ":" + url};
}

bool is_mfa_secret(const Secret_spec &spec, std::string *url = nullptr) {
  if (spec.type != Secret_type::GENERIC ||
      spec.id.compare(0, k_mfa_password_id_prefix.size(),
                      k_mfa_password_id_prefix) != 0) {
    return false;
  }

  const auto separator = spec.id.find(':', k_mfa_password_id_prefix.size());
  if (separator == std::string::npos || separator + 1 == spec.id.size()) {
    return false;
  }

  const auto factor =
      spec.id.substr(k_mfa_password_id_prefix.size(),
                     separator - k_mfa_password_id_prefix.size());
  if (factor != "2" && factor != "3") return false;

  if (url) *url = spec.id.substr(separator + 1);
  return true;
}

void validate_generic_secret_id(const std::string &id) {
  if (id.compare(0, k_internal_secret_id_prefix.size(),
                 k_internal_secret_id_prefix) == 0) {
    throw Exception::argument_error("Secret ID is reserved for internal use.");
  }
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
        "The option " + std::string(k_save_passwords_option) +
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

const char *to_message(Secret_type type) {
  switch (type) {
    case Secret_type::PASSWORD:
      return "credential";

    case Secret_type::GENERIC:
      return "secret";
  }

  throw std::runtime_error{"Unknown Secret_type value"};
}

}  // namespace

Credential_manager::Credential_manager() {
  observe_notification(SN_SHELL_OPTION_CHANGED);
}

Credential_manager &Credential_manager::get() {
  static Credential_manager instance;
  return instance;
}

std::string Credential_manager::normalize_credential_url(
    const std::string &url) {
  return normalize_credential_url(mysqlshdk::db::Connection_options{url});
}

std::string Credential_manager::normalize_credential_url(
    const mysqlshdk::IConnection &options) {
  auto tokens = mysqlshdk::db::uri::formats::user_transport();
  if (options.has_scheme() &&
      (options.get_scheme() == "file" || options.get_scheme() == "ssh")) {
    tokens.set(mysqlshdk::db::uri::Tokens::Scheme);
  }
  return options.as_uri(tokens);
}

void Credential_manager::initialize() {
  if (!m_is_initialized) {
    set_logger([](std::string_view msg) {
      log_debug2("%.*s", static_cast<int>(msg.size()), msg.data());
    });

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
                                       std::string(k_exclude_filters_option) +
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

bool Credential_manager::get_password(mysqlshdk::IConnection *options) const {
  if (m_helper) {
    std::string password;
    auto secret_spec = get_secret_spec(*options);
    log_debug2("Retrieving password from credential manager for '%s'",
               secret_spec.id.c_str());
    bool ret = m_helper->get(secret_spec, &password);

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

Credential_manager::Factor_mask Credential_manager::get_mfa_passwords(
    const std::string &url, const Factor_mask &requested,
    Mfa_passwords *passwords) const {
  Factor_mask retrieved{};

  if (!m_helper) return retrieved;

  for (int factor = 0; factor < mysqlshdk::db::k_max_auth_factors; ++factor) {
    if (!requested[factor] || (*passwords)[factor].has_value()) continue;

    std::string password;
    const auto secret_spec =
        factor == 0 ? get_secret_spec(url) : get_mfa_secret_spec(url, factor);
    log_debug2("Retrieving password from credential manager for '%s'",
               secret_spec.id.c_str());
    if (m_helper->get(secret_spec, &password)) {
      (*passwords)[factor] = std::move(password);
      retrieved[factor] = true;
    } else if (const auto error = m_helper->get_last_error();
               k_no_such_secret_error != error) {
      mysqlsh::current_console()->print_error(
          "Failed to retrieve the password: " + error);
    }
  }

  return retrieved;
}

bool Credential_manager::save_password(const mysqlshdk::IConnection &options) {
  if (m_helper && should_save_password(normalize_credential_url(options))) {
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

bool Credential_manager::save_mfa_passwords(const std::string &url,
                                            const Mfa_passwords &passwords,
                                            const Factor_mask &factors) {
  if (!m_helper ||
      !should_save_password(
          url, std::count(factors.begin(), factors.end(), true) > 1)) {
    return false;
  }

  bool saved = false;
  for (int factor = 0; factor < mysqlshdk::db::k_max_auth_factors; ++factor) {
    if (!factors[factor] || !passwords[factor].has_value()) continue;

    const auto secret_spec =
        factor == 0 ? get_secret_spec(url) : get_mfa_secret_spec(url, factor);
    if (!m_helper->store(secret_spec, *passwords[factor])) {
      mysqlsh::current_console()->print_error("Failed to store the password: " +
                                              m_helper->get_last_error());
    } else {
      saved = true;
    }
  }

  return saved;
}

bool Credential_manager::remove_password(
    const mysqlshdk::IConnection &options) {
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

bool Credential_manager::remove_mfa_passwords(const std::string &url,
                                              const Factor_mask &factors) {
  if (!m_helper) return false;

  bool removed = false;
  for (int factor = 0; factor < mysqlshdk::db::k_max_auth_factors; ++factor) {
    if (!factors[factor]) continue;

    const auto secret_spec =
        factor == 0 ? get_secret_spec(url) : get_mfa_secret_spec(url, factor);
    if (m_helper->erase(secret_spec)) {
      removed = true;
    } else if (const auto error = m_helper->get_last_error();
               k_no_such_secret_error != error) {
      mysqlsh::current_console()->print_error("Failed to erase the password: " +
                                              error);
    }
  }

  return removed;
}

bool Credential_manager::get_credential(const std::string &url,
                                        std::string *credential) const {
  check_helper("get the credential");

  bool ret = m_helper->get({Secret_type::PASSWORD, url}, credential);
  if (!ret) {
    auto error = m_helper->get_last_error();
    if (k_no_such_secret_error != error) {
      mysqlsh::current_console()->print_error(
          "Failed to retrieve the password: " + error);
    }
  }
  return ret;
}

void Credential_manager::store_credential(const std::string &url,
                                          const Mfa_passwords &passwords) {
  check_helper("save the credential");

  std::string validated_url;
  try {
    validated_url = validate_url(url);
  } catch (const std::exception &e) {
    throw Exception::argument_error(e.what());
  }

  struct Credential_update {
    Secret_spec spec;
    std::string password;
    std::optional<std::string> previous_password;
  };

  std::vector<Credential_update> updates;
  for (int factor = 0; factor < mysqlshdk::db::k_max_auth_factors; ++factor) {
    if (!passwords[factor].has_value()) continue;

    const auto secret_spec = factor == 0
                                 ? get_secret_spec(validated_url)
                                 : get_mfa_secret_spec(validated_url, factor);
    Credential_update update{secret_spec, *passwords[factor], std::nullopt};
    std::string password;
    if (m_helper->get(secret_spec, &password)) {
      update.previous_password = std::move(password);
    } else if (const auto error = m_helper->get_last_error();
               k_no_such_secret_error != error) {
      if (std::string::npos != error.find(k_invalid_url_error)) {
        throw Exception::argument_error(error);
      }
      throw Exception::runtime_error(
          str_format("Failed to retrieve the credential: %s", error.c_str()));
    }
    updates.emplace_back(std::move(update));
  }

  for (std::size_t current = 0; current < updates.size(); ++current) {
    const auto &update = updates[current];
    if (m_helper->store(update.spec, update.password)) continue;

    const auto error = m_helper->get_last_error();
    std::vector<std::string> rollback_errors;
    for (std::size_t i = current; i-- > 0;) {
      const auto &previous = updates[i];
      const bool restored =
          previous.previous_password.has_value()
              ? m_helper->store(previous.spec, *previous.previous_password)
              : m_helper->erase(previous.spec);
      if (!restored) {
        rollback_errors.emplace_back("Failed to restore '" + previous.spec.id +
                                     "': " + m_helper->get_last_error());
      }
    }

    if (rollback_errors.empty() &&
        std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    }

    auto message =
        str_format("Failed to save the credential: %s", error.c_str());
    if (!rollback_errors.empty()) {
      message += "\nFailed to restore previous credentials:\n" +
                 str_join(rollback_errors, "\n");
    }
    throw Exception::runtime_error(message);
  }
}

void Credential_manager::store_credential(const std::string &url,
                                          const std::string &password) {
  check_helper("save the credential");

  if (!m_helper->store({Secret_type::PASSWORD, url}, password)) {
    const auto error = m_helper->get_last_error();
    if (std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    }
    throw Exception::runtime_error(
        str_format("Failed to save the credential: %s", error.c_str()));
  }
}

void Credential_manager::delete_credential(const std::string &url) {
  check_helper("delete the credential");

  std::string validated_url;
  try {
    validated_url = validate_url(url);
  } catch (const std::exception &e) {
    throw Exception::argument_error(e.what());
  }

  std::vector<Secret_spec> specs;
  if (!m_helper->list(&specs)) {
    throw Exception::runtime_error("Failed to obtain credentials to delete: " +
                                   m_helper->get_last_error());
  }

  bool removed = false;
  for (auto it = specs.cbegin(); it != specs.cend(); ++it) {
    const auto &spec = *it;
    std::string mfa_url;
    if ((spec.type != Secret_type::PASSWORD || spec.id != validated_url) &&
        (!is_mfa_secret(spec, &mfa_url) || mfa_url != validated_url)) {
      continue;
    }

    if (std::find(specs.cbegin(), it, spec) != it) continue;

    if (m_helper->erase(spec)) {
      removed = true;
    } else if (const auto error = m_helper->get_last_error();
               k_no_such_secret_error != error) {
      if (std::string::npos != error.find(k_invalid_url_error)) {
        throw Exception::argument_error(error);
      }
      throw Exception::runtime_error(
          str_format("Failed to delete the credential: %s", error.c_str()));
    }
  }

  if (!removed) {
    throw Exception::runtime_error("Failed to delete the credential: " +
                                   std::string{k_no_such_secret_error});
  }
}

void Credential_manager::delete_all_credentials() {
  check_helper("delete all credentials");

  std::vector<Secret_spec> specs;
  if (!m_helper->list(&specs)) {
    throw Exception::runtime_error(
        "Failed to obtain list of credentials to delete: " +
        m_helper->get_last_error());
  }

  std::vector<std::string> errors;
  for (const auto &spec : specs) {
    if (spec.type != Secret_type::PASSWORD && !is_mfa_secret(spec)) continue;

    if (!m_helper->erase(spec)) {
      errors.emplace_back("Failed to delete '" + spec.id +
                          "': " + m_helper->get_last_error());
    }
  }

  if (!errors.empty()) {
    throw Exception::runtime_error("Failed to delete all credentials:\n" +
                                   str_join(errors, "\n"));
  }
}

std::vector<std::string> Credential_manager::list_credentials() const {
  check_helper("list credentials");

  std::vector<Secret_spec> specs;
  if (!m_helper->list(&specs)) {
    throw Exception::runtime_error("Failed to list credentials: " +
                                   m_helper->get_last_error());
  }

  std::set<std::string> credentials;
  for (const auto &spec : specs) {
    if (spec.type == Secret_type::PASSWORD) {
      credentials.emplace(spec.id);
    } else {
      std::string url;
      if (is_mfa_secret(spec, &url)) credentials.emplace(std::move(url));
    }
  }

  return {credentials.begin(), credentials.end()};
}

void Credential_manager::store_secret(const std::string &id,
                                      const std::string &secret) {
  validate_generic_secret_id(id);

  // we prepend 'r' for a unmodified secret and 'b' for a base64-encoded secret
  // we encode the secret if helper reports an error when storing the raw secret
  try {
    store_secret(Secret_type::GENERIC, id, "r" + secret);
  } catch (const std::exception &e) {
    if (std::string::npos ==
        std::string_view{e.what()}.find(k_invalid_secret_error)) {
      throw;
    }

    std::string base64;

    if (!shcore::encode_base64(secret, &base64)) {
      // in case of encoding errors, just throw the original exception
      throw;
    }

    store_secret(Secret_type::GENERIC, id, "b" + base64);
  }
}

void Credential_manager::store_secret(Secret_type type, const std::string &id,
                                      const std::string &secret) {
  check_helper(str_format("save the %s", to_message(type)));

  if (!m_helper->store({type, id}, secret)) {
    const auto error = m_helper->get_last_error();

    if (std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    } else {
      throw Exception::runtime_error(str_format(
          "Failed to save the %s: %s", to_message(type), error.c_str()));
    }
  }
}

std::string Credential_manager::read_secret(const std::string &id) {
  validate_generic_secret_id(id);

  auto secret = read_secret(Secret_type::GENERIC, id);

  if (secret.empty()) {
    return secret;
  }

  switch (secret.front()) {
    case 'r':
      return secret.substr(1);

    case 'b': {
      std::string decoded;

      if (!shcore::decode_base64(secret.substr(1), &decoded)) {
        // if decoding fails, just return the whole secret
        return secret;
      }

      return decoded;
    }

    default:
      return secret;
  }
}

std::string Credential_manager::read_secret(Secret_type type,
                                            const std::string &id) {
  check_helper(str_format("read the %s", to_message(type)));

  std::string secret;

  if (!m_helper->get({type, id}, &secret)) {
    const auto error = m_helper->get_last_error();

    if (std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    } else {
      throw Exception::runtime_error(str_format(
          "Failed to read the %s: %s", to_message(type), error.c_str()));
    }
  }

  return secret;
}

void Credential_manager::delete_secret(const std::string &id) {
  validate_generic_secret_id(id);
  delete_secret(Secret_type::GENERIC, id);
}

void Credential_manager::delete_secret(Secret_type type,
                                       const std::string &id) {
  check_helper(str_format("delete the %s", to_message(type)));

  if (!m_helper->erase({type, id})) {
    const auto error = m_helper->get_last_error();

    if (std::string::npos != error.find(k_invalid_url_error)) {
      throw Exception::argument_error(error);
    } else {
      throw Exception::runtime_error(str_format(
          "Failed to delete the %s: %s", to_message(type), error.c_str()));
    }
  }
}

void Credential_manager::delete_all_secrets(Secret_type type) {
  check_helper(str_format("delete all %ss", to_message(type)));

  std::vector<Secret_spec> specs;

  if (!m_helper->list(&specs, type)) {
    throw Exception::runtime_error(
        str_format("Failed to obtain list of %ss to delete: %s",
                   to_message(type), m_helper->get_last_error().c_str()));
  }

  std::vector<std::string> errors;

  for (const auto &s : specs) {
    if (type == Secret_type::GENERIC && is_mfa_secret(s)) continue;

    if (type == s.type) {
      if (!m_helper->erase(s)) {
        errors.emplace_back("Failed to delete '" + s.id +
                            "': " + m_helper->get_last_error());
      }
    }
  }

  if (!errors.empty()) {
    throw Exception::runtime_error(
        str_format("Failed to delete all %ss:\n%s", to_message(type),
                   shcore::str_join(errors, "\n").c_str()));
  }
}

std::vector<std::string> Credential_manager::list_secrets(
    Secret_type type) const {
  check_helper(str_format("list %ss", to_message(type)));

  std::vector<Secret_spec> specs;

  if (!m_helper->list(&specs, type)) {
    throw Exception::runtime_error(
        str_format("Failed to list %ss: %s", to_message(type),
                   m_helper->get_last_error().c_str()));
  }

  std::vector<std::string> ret;

  for (auto &s : specs) {
    // type was added to the list() command by WL#16958, helpers may not support
    // it, double check the type
    if (type == s.type && !(type == Secret_type::GENERIC && is_mfa_secret(s))) {
      ret.emplace_back(std::move(s.id));
    }
  }

  return ret;
}

bool Credential_manager::should_save_password(const std::string &url,
                                              bool all_prompted_passwords) {
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
        const auto message =
            std::string{all_prompted_passwords ? "Save all prompted passwords"
                                               : "Save password"} +
            " for '" + url + "'?";
        auto answer = mysqlsh::current_console()->confirm(
            message, Prompt_answer::NO, "&Yes", "&No", "Ne&ver");

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

void Credential_manager::check_helper(const std::string &context) const {
  if (!m_helper) {
    throw Exception::runtime_error(str_format(
        "Cannot %s, current credential helper is invalid", context.c_str()));
  }
}

}  // namespace shcore
