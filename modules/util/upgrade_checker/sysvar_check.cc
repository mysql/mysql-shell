/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/upgrade_checker/sysvar_check.h"

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

namespace mysqlsh {
namespace upgrade_checker {
namespace {

const char *k_sysvar_group_removed = "removed";
const char *k_sysvar_group_removed_log = "removedLogging";
const char *k_sysvar_group_allowed = "allowedValues";
const char *k_sysvar_group_forbidden = "forbiddenValues";
const char *k_sysvar_group_defaults = "newDefaults";
const char *k_sysvar_group_deprecated = "deprecated";

const std::vector<std::string> k_removed_sys_log_vars = {
    "log_syslog_facility", "log_syslog_include_pid", "log_syslog_tag",
    "log_syslog"};

// ensures that sysvar checks registry is only loaded once
std::once_flag registry_load_flag;

template <class cont_type, class value_type>
bool cont_contains(const cont_type &cont, const value_type &value) {
  return std::ranges::find(cont, value) != std::end(cont);
}

char normalize_platform(const std::string &os_platform) {
  if (std::string("WLM").find(os_platform[0]) != std::string::npos)
    return os_platform[0];

  if (os_platform.starts_with("OSX")) return 'M';

  return 'L';
}
}  // namespace

Sysvar_registry Sysvar_check::s_registry;

std::optional<std::string> Sysvar_platform_value::get_default(
    size_t bits) const {
  assert(bits == 32 || bits == 64);

  auto it = m_defaults.find(bits);
  if (it != m_defaults.end()) {
    return it->second;
  }

  it = m_defaults.find(0);
  if (it != m_defaults.end()) {
    return it->second;
  }

  return {};
}

std::optional<std::vector<std::string>> Sysvar_platform_value::get_allowed(
    size_t bits) const {
  assert(bits == 32 || bits == 64);

  auto it = m_allowed.find(bits);
  if (it != m_allowed.end()) {
    return it->second;
  }

  it = m_allowed.find(0);
  if (it != m_allowed.end()) {
    return it->second;
  }

  return {};
}

std::optional<std::vector<std::string>> Sysvar_platform_value::get_forbidden(
    size_t bits) const {
  assert(bits == 32 || bits == 64);

  auto it = m_forbidden.find(bits);
  if (it != m_forbidden.end()) {
    return it->second;
  }

  it = m_forbidden.find(0);
  if (it != m_forbidden.end()) {
    return it->second;
  }

  return {};
}

void Sysvar_platform_value::set_default(std::string value, size_t bits) {
  assert(bits == 0 || bits == 32 || bits == 64);

  m_defaults[bits] = std::move(value);
}

void Sysvar_platform_value::set_allowed(std::vector<std::string> allowed,
                                        size_t bits) {
  assert(bits == 0 || bits == 32 || bits == 64);

  m_allowed[bits] = std::move(allowed);
}

void Sysvar_platform_value::set_forbidden(std::vector<std::string> forbidden,
                                          size_t bits) {
  assert(bits == 0 || bits == 32 || bits == 64);

  m_forbidden[bits] = std::move(forbidden);
}

void Sysvar_configuration::set_default(std::string value, size_t bits,
                                       char platform) {
  assert(std::string("AWULM").find(platform) != std::string::npos);
  assert(bits == 0 || bits == 32 || bits == 64);

  m_platform_values[platform].set_default(std::move(value), bits);
}

void Sysvar_configuration::set_allowed(std::vector<std::string> allowed,
                                       size_t bits, char platform) {
  assert(std::string("AWULM").find(platform) != std::string::npos);
  assert(bits == 0 || bits == 32 || bits == 64);

  m_platform_values[platform].set_allowed(std::move(allowed), bits);
}

void Sysvar_configuration::set_forbidden(std::vector<std::string> forbidden,
                                         size_t bits, char platform) {
  assert(std::string("AWULM").find(platform) != std::string::npos);
  assert(bits == 0 || bits == 32 || bits == 64);

  m_platform_values[platform].set_forbidden(std::move(forbidden), bits);
}

std::optional<std::string> Sysvar_configuration::get_default(
    size_t bits, char platform) const {
  assert(std::string("WLM").find(platform) != std::string::npos);
  assert(bits == 32 || bits == 64);

  std::optional<std::string> value;

  // Searches for a value defined on the specific platform
  auto it = m_platform_values.find(platform);
  if (it != m_platform_values.end()) {
    value = it->second.get_default(bits);
  }

  // Searches for a value defined for Unix (non-windows)
  if (!value.has_value() && platform != 'W') {
    it = m_platform_values.find('U');
    if (it != m_platform_values.end()) {
      value = it->second.get_default(bits);
    }
  }

  // Searches for a value defined for All
  if (!value.has_value()) {
    it = m_platform_values.find('A');
    if (it != m_platform_values.end()) {
      value = it->second.get_default(bits);
    }
  }

  return value;
}
std::optional<std::vector<std::string>> Sysvar_configuration::get_allowed(
    size_t bits, char platform) const {
  assert(std::string("WLM").find(platform) != std::string::npos);
  assert(bits == 32 || bits == 64);

  std::optional<std::vector<std::string>> allowed;

  // Searches for a value defined on the specific platform
  auto it = m_platform_values.find(platform);
  if (it != m_platform_values.end()) {
    allowed = it->second.get_allowed(bits);
  }

  // Searches for a value defined for Unix (non-windows)
  if (!allowed.has_value() && platform != 'W') {
    it = m_platform_values.find('U');
    if (it != m_platform_values.end()) {
      allowed = it->second.get_allowed(bits);
    }
  }

  // Searches for a value defined for All
  if (!allowed.has_value()) {
    it = m_platform_values.find('A');
    if (it != m_platform_values.end()) {
      allowed = it->second.get_allowed(bits);
    }
  }

  return allowed;
};

std::optional<std::vector<std::string>> Sysvar_configuration::get_forbidden(
    size_t bits, char platform) const {
  assert(std::string("WLM").find(platform) != std::string::npos);
  assert(bits == 32 || bits == 64);

  std::optional<std::vector<std::string>> forbidden;

  // Searches for a value defined on the specific platform
  auto it = m_platform_values.find(platform);
  if (it != m_platform_values.end()) {
    forbidden = it->second.get_forbidden(bits);
  }

  // Searches for a value defined for Unix (non-windows)
  if (!forbidden.has_value() && platform != 'W') {
    it = m_platform_values.find('U');
    if (it != m_platform_values.end()) {
      forbidden = it->second.get_forbidden(bits);
    }
  }

  // Searches for a value defined for All
  if (!forbidden.has_value()) {
    it = m_platform_values.find('A');
    if (it != m_platform_values.end()) {
      forbidden = it->second.get_forbidden(bits);
    }
  }

  return forbidden;
};

void Sysvar_definition::set_default(std::string value,
                                    const std::optional<Version> &version,
                                    size_t bits, char platform) {
  if (version.has_value()) {
    changes[*version].set_default(std::move(value), bits, platform);
  } else {
    initials.set_default(std::move(value), bits, platform);
  }
}

void Sysvar_definition::set_allowed(std::vector<std::string> allowed,
                                    const std::optional<Version> &version,
                                    size_t bits, char platform) {
  if (version.has_value()) {
    changes[*version].set_allowed(std::move(allowed), bits, platform);
  } else {
    initials.set_allowed(std::move(allowed), bits, platform);
  }
}

void Sysvar_definition::set_forbidden(std::vector<std::string> forbidden,
                                      const std::optional<Version> &version,
                                      size_t bits, char platform) {
  if (version.has_value()) {
    changes[*version].set_forbidden(std::move(forbidden), bits, platform);
  } else {
    initials.set_forbidden(std::move(forbidden), bits, platform);
  }
}

void Sysvar_registry::register_sysvar(Sysvar_definition &&def) {
  m_registry[def.name] = std::move(def);
}

std::vector<Sysvar_version_check> Sysvar_registry::get_checks(
    const Upgrade_info &upgrade_info) {
  std::vector<Sysvar_version_check> result;

  for (const auto &item : m_registry) {
    auto check = item.second.get_check(upgrade_info);
    if (check.has_value()) {
      result.push_back(std::move(*check));
    }
  }

  return result;
}

namespace {
constexpr const char *k_all = "all";
constexpr const char *k_windows = "windows";
constexpr const char *k_unix = "unix";
constexpr const char *k_linux = "linux";
constexpr const char *k_macos = "macos";

constexpr const char k_all_id = 'A';
constexpr const char k_windows_id = 'W';
constexpr const char k_unix_id = 'U';
constexpr const char k_linux_id = 'L';
constexpr const char k_macos_id = 'M';

constexpr const char *k_all_bits = "all";
constexpr const char *k_32_bits = "32";
constexpr const char *k_64_bits = "64";

constexpr const char *k_default = "default";
constexpr const char *k_allowed = "allowed";
constexpr const char *k_forbidden = "forbidden";

std::vector<std::string> parse_string_list(const rapidjson::Value &source) {
  std::vector<std::string> result;

  if (source.IsArray()) {
    auto array = source.GetArray();
    for (auto iterator = array.Begin(); iterator != array.End(); iterator++) {
      if (iterator->IsString()) {
        result.push_back(iterator->GetString());
      }
    }
  }

  return result;
}

std::string get_string(const rapidjson::Value &source) {
  return std::string(source.GetString(), source.GetStringLength());
}

void parse_default(const rapidjson::Value &source, Sysvar_configuration *target,
                   char platform_id) {
  if (source.HasMember(k_all_bits)) {
    if (source[k_all_bits].IsString()) {
      target->set_default(get_string(source[k_all_bits]), 0, platform_id);
    } else {
      // TODO(rennox): Weird case, value of '' in optimizer_switch
    }
  }

  if (source.HasMember(k_32_bits)) {
    target->set_default(get_string(source[k_32_bits]), 32, platform_id);
  }

  if (source.HasMember(k_64_bits)) {
    target->set_default(get_string(source[k_64_bits]), 64, platform_id);
  }
}

void parse_allowed(const rapidjson::Value &source, Sysvar_configuration *target,
                   char platform_id) {
  if (source.HasMember(k_all_bits)) {
    target->set_allowed(parse_string_list(source[k_all_bits]), 0, platform_id);
  }

  if (source.HasMember(k_32_bits)) {
    target->set_allowed(parse_string_list(source[k_32_bits]), 32, platform_id);
  }

  if (source.HasMember(k_64_bits)) {
    target->set_allowed(parse_string_list(source[k_64_bits]), 64, platform_id);
  }
}

void parse_forbidden(const rapidjson::Value &source,
                     Sysvar_configuration *target, char platform_id) {
  if (source.HasMember(k_all_bits)) {
    target->set_forbidden(parse_string_list(source[k_all_bits]), 0,
                          platform_id);
  }

  if (source.HasMember(k_32_bits)) {
    target->set_forbidden(parse_string_list(source[k_32_bits]), 32,
                          platform_id);
  }

  if (source.HasMember(k_64_bits)) {
    target->set_forbidden(parse_string_list(source[k_64_bits]), 64,
                          platform_id);
  }
}

void parse_platform_values(const rapidjson::Value &source,
                           Sysvar_configuration *target, char platform_id) {
  if (source.HasMember(k_default)) {
    parse_default(source[k_default], target, platform_id);
  }

  if (source.HasMember(k_allowed)) {
    parse_allowed(source[k_allowed], target, platform_id);
  }

  if (source.HasMember(k_forbidden)) {
    parse_forbidden(source[k_forbidden], target, platform_id);
  }
}

void parse_configuration(const rapidjson::Value &source,
                         Sysvar_configuration *target) {
  if (source.HasMember(k_all)) {
    parse_platform_values(source[k_all], target, k_all_id);
  }

  if (source.HasMember(k_windows)) {
    parse_platform_values(source[k_windows], target, k_windows_id);
  }

  if (source.HasMember(k_unix)) {
    parse_platform_values(source[k_unix], target, k_unix_id);
  }

  if (source.HasMember(k_linux)) {
    parse_platform_values(source[k_linux], target, k_linux_id);
  }

  if (source.HasMember(k_macos)) {
    parse_platform_values(source[k_macos], target, k_macos_id);
  }
}

void parse_sysvar(const rapidjson::Value &source, Sysvar_definition *sysvar) {
  sysvar->name = get_string(source["name"]);
  if (source.HasMember("version")) {
    sysvar->introduction_version = Version(get_string(source["version"]));
  }

  if (source.HasMember("deprecation")) {
    sysvar->deprecation_version = Version(get_string(source["deprecation"]));
  }

  if (source.HasMember("removal")) {
    sysvar->removal_version = Version(get_string(source["removal"]));
  }

  if (source.HasMember("replacement")) {
    sysvar->replacement = get_string(source["replacement"]);
  }

  // Parses the initial values
  if (source.HasMember("initial")) {
    parse_configuration(source["initial"], &sysvar->initials);
  }

  // Now parses the configuration changes
  if (source.HasMember("changes")) {
    if (source["changes"].IsArray()) {
      const auto json_config = source["changes"].GetArray();
      for (auto iterator = json_config.Begin(); iterator != json_config.End();
           iterator++) {
        if (!iterator->HasMember("version")) {
          throw std::logic_error(
              shcore::str_format("Malformed system variable change, missing "
                                 "change version at variable '%s'",
                                 sysvar->name.c_str()));
        }

        auto &config =
            sysvar->changes[Version((*iterator)["version"].GetString())];

        parse_configuration(*iterator, &config);
      }
    }
  }
}
}  // namespace

void Sysvar_registry::load_configuration() {
  std::string path = shcore::get_share_folder();
  path = shcore::path::join_path(path, "sysvars.json");

  if (!shcore::path::exists(path))
    throw std::runtime_error(path +
                             ": not found, shell installation likely invalid");

  std::ifstream file(path, std::ifstream::binary);
  rapidjson::IStreamWrapper isw(file);

  rapidjson::Document configuration;
  configuration.ParseStream(isw);
  auto list = configuration.GetArray();
  int position = 0;
  for (auto iterator = list.Begin(); iterator != list.End(); ++iterator) {
    Sysvar_definition sysvar;
    try {
      parse_sysvar(*iterator, &sysvar);
      register_sysvar(std::move(sysvar));
      position++;
    } catch (const std::exception &err) {
      std::string message;
      if (sysvar.name.empty()) {
        message = shcore::str_format(
            "error loading definition for system variable at position '%d'",
            position);
      } else {
        message = shcore::str_format(
            "error loading definition for system variable '%s'",
            sysvar.name.c_str());
      }
      throw std::logic_error(shcore::str_format(
          "An error occurred loading the sysvars.json file, it might be "
          "corrupted: %s: %s",
          message.c_str(), err.what()));
    }
  }
}

const Sysvar_definition &Sysvar_registry::get_sysvar(
    const std::string &name) const {
  return m_registry.at(name);
}

std::optional<Sysvar_version_check> Sysvar_definition::get_check(
    const Upgrade_info &upgrade_info) const {
  // Variable did not exist on the source server, was either not yet created
  // or removed before
  if (introduction_version.has_value() &&
      introduction_version.value() > upgrade_info.server_version)
    return {};

  if (removal_version.has_value() &&
      removal_version.value() <= upgrade_info.server_version)
    return {};

  const auto os_platform = normalize_platform(upgrade_info.server_os);

  auto initial_default =
      initials.get_default(upgrade_info.server_bits, os_platform);
  auto allowed_values =
      initials.get_allowed(upgrade_info.server_bits, os_platform);
  auto forbidden_values =
      initials.get_forbidden(upgrade_info.server_bits, os_platform);

  std::optional<std::string> final_default;

  for (const auto &change : changes) {
    // Additional changes are after the target server version so we don't care
    if (change.first > upgrade_info.target_version) {
      break;
    }

    // We are interested in the last list of allowed values if any
    allowed_values =
        change.second.get_allowed(upgrade_info.server_bits, os_platform);

    forbidden_values =
        change.second.get_forbidden(upgrade_info.server_bits, os_platform);

    if (change.first <= upgrade_info.server_version) {
      initial_default =
          change.second.get_default(upgrade_info.server_bits, os_platform);
    } else if (change.first <= upgrade_info.target_version) {
      final_default =
          change.second.get_default(upgrade_info.server_bits, os_platform);
    }
  }

  bool is_deprecated = deprecation_version.has_value() &&
                       *deprecation_version <= upgrade_info.target_version;

  bool is_removed = removal_version.has_value() &&
                    *removal_version <= upgrade_info.target_version;

  if ((final_default.has_value() && initial_default != final_default) ||
      allowed_values.has_value() || forbidden_values.has_value() ||
      is_deprecated || is_removed) {
    Sysvar_version_check cnf;
    cnf.name = name;
    cnf.replacement = replacement;

    // NOTE these is_removed/is_deprecated are TRUE considering the context of
    // the upgrade opereation, this is:
    // if the variable versions are like:
    // introduced in v1
    // deprecated in v10
    // removed in v20
    // The variable will only ve considered deprecated if the upgrade ends in a
    // version >= V10, similarly, will only be considered removed if the upgrade
    // ends in a version >= 20
    if (is_removed) {
      cnf.removal_version = removal_version;
    }

    if (is_deprecated) {
      cnf.deprecation_version = deprecation_version;
    }

    if (initial_default.has_value()) {
      cnf.initial_defaults = initial_default.value();
      if (final_default.has_value()) {
        cnf.updated_defaults = final_default.value();
      } else {
        cnf.updated_defaults = cnf.initial_defaults;
      }
    }

    if (allowed_values.has_value()) {
      cnf.allowed_values = std::move(*allowed_values);
    }

    if (forbidden_values.has_value()) {
      cnf.forbidden_values = std::move(*forbidden_values);
    }

    return cnf;
  }

  return {};
}

Sysvar_check::Sysvar_check(const Upgrade_info &info)
    : Upgrade_check(ids::k_sys_vars_check) {
  std::call_once(registry_load_flag,
                 [&]() { s_registry.load_configuration(); });

  set_groups({k_sysvar_group_removed, k_sysvar_group_removed_log,
              k_sysvar_group_forbidden, k_sysvar_group_allowed,
              k_sysvar_group_defaults, k_sysvar_group_deprecated});

  m_checks = s_registry.get_checks(info);
}

std::vector<Upgrade_issue> Sysvar_check::run(
    const std::shared_ptr<mysqlshdk::db::ISession> &session,
    const Upgrade_info &server_info, Checker_cache *cache) {
  std::vector<Upgrade_issue> issues;

  cache->cache_sysvars(session.get(), server_info);
  for (const auto &sysvar_check : m_checks) {
    auto sysvar = cache->get_sysvar(sysvar_check.name);

    Upgrade_issue issue;
    if (sysvar && sysvar->source != "COMPILED") {
      if (sysvar_check.removal_version.has_value()) {
        if (cont_contains(k_removed_sys_log_vars, sysvar->name))
          issue = get_issue(k_sysvar_group_removed_log, sysvar, sysvar_check);
        else
          issue = get_issue(k_sysvar_group_removed, sysvar, sysvar_check);
      } else if (has_invalid_forbidden_values(sysvar, sysvar_check)) {
        issue = get_issue(k_sysvar_group_forbidden, sysvar, sysvar_check);
      } else if (has_invalid_allowed_values(sysvar, sysvar_check)) {
        issue = get_issue(k_sysvar_group_allowed, sysvar, sysvar_check);
      } else if (sysvar_check.deprecation_version.has_value()) {
        issue = get_issue(k_sysvar_group_deprecated, sysvar, sysvar_check);
      }
    } else if (sysvar_check.initial_defaults != sysvar_check.updated_defaults) {
      issue = get_issue(k_sysvar_group_defaults, nullptr, sysvar_check);
    }

    if (!issue.empty()) issues.push_back(std::move(issue));
  }

  return issues;
}

bool Sysvar_check::has_invalid_allowed_values(
    const Checker_cache::Sysvar_info *sysvar,
    const Sysvar_version_check &sysvar_check) const {
  if (!sysvar_check.allowed_values.empty()) {
    if (!cont_contains(sysvar_check.allowed_values, sysvar->value)) return true;
  }
  return false;
}

bool Sysvar_check::has_invalid_forbidden_values(
    const Checker_cache::Sysvar_info *sysvar,
    const Sysvar_version_check &sysvar_check) const {
  if (!sysvar_check.forbidden_values.empty()) {
    if (cont_contains(sysvar_check.forbidden_values, sysvar->value))
      return true;
  }
  return false;
}

Upgrade_issue Sysvar_check::get_issue(
    const std::string &group, const Checker_cache::Sysvar_info *sysvar,
    const Sysvar_version_check &sysvar_check) {
  Token_definitions tokens = {
      {"item", sysvar_check.name},
  };

  if (sysvar) {
    tokens["value"] = sysvar->value;
    tokens["source"] = sysvar->source;
  }

  const auto issue_field = "issue." + group;
  const auto &raw_description = get_text(issue_field.c_str());

  std::string description{raw_description};
  if (group == k_sysvar_group_removed || group == k_sysvar_group_deprecated) {
    tokens["deprecated_version"] =
        sysvar_check.deprecation_version.value_or(Version()).get_base();
    tokens["removed_version"] =
        sysvar_check.removal_version.value_or(Version()).get_base();

    if (sysvar_check.replacement.has_value()) {
      const auto &raw_replacement = get_text("replacement");
      description += ", " + raw_replacement;
      tokens["replacement"] = std::string(sysvar_check.replacement.value());
    }
  }
  if (group == k_sysvar_group_allowed)
    tokens["allowed"] = shcore::str_join(sysvar_check.allowed_values, ", ");
  if (group == k_sysvar_group_forbidden)
    tokens["forbidden"] = shcore::str_join(sysvar_check.forbidden_values, ", ");
  if (group == k_sysvar_group_defaults) {
    tokens["initial_default"] = sysvar_check.initial_defaults;
    tokens["updated_default"] = sysvar_check.updated_defaults;
  }

  description += ".";
  description = resolve_tokens(description, tokens);

  auto issue_level = Upgrade_issue::ERROR;
  if (group == k_sysvar_group_deprecated || group == k_sysvar_group_defaults)
    issue_level = Upgrade_issue::WARNING;

  Upgrade_issue issue;
  issue.schema = sysvar_check.name;
  issue.level = issue_level;
  issue.description = description.c_str();

  issue.object_type = Upgrade_issue::Object_type::SYSVAR;
  issue.group = group;

  return issue;
}
}  // namespace upgrade_checker
}  // namespace mysqlsh