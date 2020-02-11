/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/oci/oci.h"

#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_python.h"
#include "mysqlshdk/libs/oci/oci_setup.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

namespace {

constexpr const char kPassphrase[] = "pass_phrase";
constexpr const char kDefaultProfile[] = "DEFAULT";

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

    if (profile.empty()) profile = kDefaultProfile;

    mysqlshdk::oci::Oci_setup setup;

    // Setups or load the corresponding profile
    if (!setup.has_profile(profile)) {
      setup.create_profile(profile);
    } else {
      console->print_info("Loading the OCI configuration for the profile '" +
                          profile + "'...");
      setup.load_profile(profile);
    }

    if (!setup.is_cancelled()) {
      std::string oci_cfg_path = setup.get_oci_cfg_path();
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

}  // namespace oci
}  // namespace mysqlsh
