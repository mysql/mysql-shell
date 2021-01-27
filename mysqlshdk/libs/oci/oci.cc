/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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
}  // namespace

namespace oci {

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
