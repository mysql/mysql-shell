/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_check.h"

#include <algorithm>
#include <exception>
#include <string>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/manual_check.h"
#include "modules/util/upgrade_checker/upgrade_check_registry.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;
namespace suggested_version {
std::optional<Version> get_next_suggested_lts_version(
    const Version &version, std::string *out_key = nullptr) {
  std::optional<Version> next_lts;
  decltype(k_latest_versions)::const_iterator item = k_latest_versions.end();

  if (version < Version(8, 0, 0)) {
    item = k_latest_versions.find("8.0");
  } else {
    // This would be the case for LTS releases
    item = k_latest_versions.find(version.get_short());
    if (item == k_latest_versions.end()) {
      // This would be the case for innovation releases
      auto first_lts = get_first_lts_version(version);
      item = k_latest_versions.find(first_lts.get_short());
    }
    // We might be already on the latest release in the series, so it jumps to
    // the LTS in the next series (If they exist)
    if (item != k_latest_versions.end() && item->second == version) {
      if (version.get_major() == 8 and version.get_minor() == 0) {
        // The next LTS for the 8.0 series is the latest LTS in the  8.4 series
        auto first_lts = get_first_lts_version(version);
        item = k_latest_versions.find(first_lts.get_short());
      } else {
        // In any other case, it is the latest LTS in the next major
        auto first_lts =
            get_first_lts_version(Version(version.get_major() + 1, 0));
        item = k_latest_versions.find(first_lts.get_short());
      }
    }
  }

  if (item != k_latest_versions.end()) {
    if (out_key) {
      *out_key = item->first;
      return item->second;
    }
  }

  return {};
}

std::string format_suggested_version_message(
    const Version &serverVersion, const Version &targetVersion,
    const std::string &suggestedTargetVersion) {
  return shcore::str_format(
      "Upgrading MySQL Server from version %s to %s is not supported. Please "
      "consider running the check using the following option: targetVersion=%s",
      serverVersion.get_base().c_str(), targetVersion.get_base().c_str(),
      suggestedTargetVersion.c_str());
}
}  // namespace suggested_version

std::string check_for_version_suggestion(const Version &serverVersion,
                                         const Version &targetVersion) {
  std::string suggested_target_version;
  auto suggested = suggested_version::get_next_suggested_lts_version(
      serverVersion, &suggested_target_version);
  if (suggested.has_value() && *suggested >= targetVersion) return {};

  return suggested_version::format_suggested_version_message(
      serverVersion, targetVersion, suggested_target_version);
}

bool run_checks_for_upgrade(const Upgrade_check_config &config,
                            Upgrade_check_output_formatter &print) {
  assert(config.session());

  print.check_info(
      config.session()->get_connection_options().uri_endpoint(),
      config.upgrade_info().server_version_long,
      config.upgrade_info().target_version.get_base(),
      config.upgrade_info().explicit_target_version,
      check_for_version_suggestion(config.upgrade_info().server_version,
                                   config.upgrade_info().target_version));
  config.upgrade_info().validate();

  Upgrade_check_registry::Upgrade_check_vec rejected;
  const auto checklist = Upgrade_check_registry::create_checklist(
      config, false, config.warn_on_excludes() ? &rejected : nullptr);

  int errors = 0, warnings = 0, notices = 0;
  const auto update_counts = [&errors, &warnings,
                              &notices](Upgrade_issue::Level level) {
    switch (level) {
      case Upgrade_issue::ERROR:
        errors++;
        break;
      case Upgrade_issue::WARNING:
        warnings++;
        break;
      default:
        notices++;
        break;
    }
  };

  Checker_cache cache;

  // Workaround for 5.7 "No database selected/Corrupted" UPGRADE bug present
  // up to 5.7.39
  config.session()->execute("USE mysql;");

  for (const auto &check : checklist)
    if (check->is_runnable()) {
      try {
        print.check_title(*check);

        const auto issues = config.filter_issues(
            check->run(config.session(), config.upgrade_info(), &cache));
        for (const auto &issue : issues) update_counts(issue.level);
        print.check_results(*check, issues);
      } catch (const Check_configuration_error &e) {
        print.check_error(*check, e.what(), false);
      } catch (const std::exception &e) {
        print.check_error(*check, e.what());
      }
    } else {
      update_counts(dynamic_cast<Manual_check *>(check.get())->get_level());
      print.manual_check(*check);
    }

  std::string summary;
  if (errors > 0) {
    summary = shcore::str_format(
        "%i errors were found. Please correct these issues before upgrading "
        "to avoid compatibility issues.",
        errors);
  } else if ((warnings > 0) || (notices > 0)) {
    summary =
        "No fatal errors were found that would prevent an upgrade, "
        "but some potential issues were detected. Please ensure that the "
        "reported issues are not significant before upgrading.";
  } else {
    summary = "No known compatibility errors or issues were found.";
  }

  std::map<std::string, std::string> excluded;
  if (config.warn_on_excludes() && !config.exclude_list().empty()) {
    for (const auto &check : rejected) {
      if (!config.exclude_list().contains(check->get_name())) continue;

      excluded[check->get_name()] = check->get_title();
    }
  }

  print.summarize(errors, warnings, notices, summary, excluded);

  return 0 == errors;
}

bool list_checks_for_upgrade(const Upgrade_check_config &config,
                             Upgrade_check_output_formatter &print) {
  if (config.session()) {
    print.list_info(config.session()->get_connection_options().uri_endpoint(),
                    config.upgrade_info().server_version_long,
                    config.upgrade_info().target_version.get_base(),
                    config.upgrade_info().explicit_target_version);
  } else {
    print.list_info();
  }

  config.upgrade_info().validate(true);

  Upgrade_check_registry::Upgrade_check_vec rejected;

  const auto accepted = Upgrade_check_registry::create_checklist(
      config, config.session() == nullptr, &rejected);

  print.list_item_infos("Included", accepted);
  print.list_item_infos("Excluded", rejected);

  print.list_summarize(accepted.size(), rejected.size());

  return true;
}

bool check_for_upgrade(const Upgrade_check_config &config) {
  if (!config.list_checks()) {
    if (config.user_privileges()) {
      if (config.user_privileges()
              ->validate({"PROCESS", "SELECT"})
              .has_missing_privileges()) {
        throw std::runtime_error(
            "The upgrade check needs to be performed by user with PROCESS, and "
            "SELECT privileges.");
      }
    } else {
      log_warning(
          "User privileges were not validated, upgrade check may fail.");
    }
  }

  const auto print = config.formatter();
  assert(print);

  if (config.list_checks()) {
    return list_checks_for_upgrade(config, *print);
  }
  return run_checks_for_upgrade(config, *print);
}

}  // namespace upgrade_checker
}  // namespace mysqlsh