/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/ssh/ssh_config_reader.h"

#include <iostream>
#include <optional>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace ssh {

namespace {
/**
 * Criteria for matching hosts is as follows:
 * - If current host is * matches ALL
 * - If host_match is false, matches if current and target are equal
 * - If host_match is true, matches based on the rules for the Matches entry in
 *   the SSH config (PENDING)
 **/
bool matches_host(const std::string &current, const std::string &target,
                  bool host_match = false) {
  // TODO(rennox): Implement proper logic for Host and Match matching
  if (current == "*" || (!host_match && shcore::str_caseeq(current, target))) {
    return true;
  }
  return false;
}
}  // namespace

Ssh_config_data::Ssh_config_data() {
  // These auth methods are enabled by default
  auth_methods.set(Ssh_auth_methods::PASSWORD_AUTH)
      .set(Ssh_auth_methods::PUBKEY_AUTH)
      .set(Ssh_auth_methods::KBDINT_AUTH);
}

void Ssh_config_reader::read(Ssh_config_data *out_config,
                             const std::string &target_host,
                             const std::string &path) {
  assert(out_config != nullptr);

  std::vector<std::string> hosts{target_host};

  std::string config_path(path);
  if (config_path.empty()) {
    config_path =
        shcore::path::join_path(shcore::get_home_dir(), ".ssh", "config");
  }

  read(out_config, target_host, config_path, &hosts);
}

void Ssh_config_reader::read(Ssh_config_data *out_config,
                             const std::string &target_host,
                             const std::string &path,
                             std::vector<std::string> *hosts) {
  std::string next_host;
  std::ifstream ifs;
#ifdef _WIN32
  const auto wide_filename = shcore::utf8_to_wide(path);
  ifs.open(wide_filename, std::ofstream::in);
#else
  ifs.open(path, std::ofstream::in);
#endif

  if (ifs.good()) {
    std::string current_host;
    while (!ifs.eof() && !ifs.fail()) {
      std::string line;
      std::getline(ifs, line);

      if (!ifs.eof() && !ifs.fail()) {
        // Removes whitespaces
        line = shcore::str_strip(line);

        // Skips emptylines and comments
        if (line.empty() || line[0] == '#') continue;

        // TODO(Bug#34144479): refactor the parser
        auto tokens = shcore::str_split(line, " \t=", -1, true);
        // Lines are expected to have attr[=]value, extra values are ignored
        if (tokens.size() < 2) continue;

        // Values may be double quoted
        std::string value{tokens[1]};
        if (!value.empty() && value[0] == '"') {
          if (auto end = mysqlshdk::utils::span_quoted_string_dq(value, 0);
              end == std::string_view::npos)
            value.clear();
          else
            value = value.substr(1, end - 2);  // end is after the last "
        }

        if (shcore::str_caseeq("host", tokens[0])) {
          current_host = tokens[1];
        } else if (matches_host(current_host, target_host)) {
          std::optional<Ssh_auth_methods> method;
          if (shcore::str_caseeq(tokens[0], "GSSAPIAuthentication")) {
            method = Ssh_auth_methods::GSSAPI_AUTH;
          } else if (shcore::str_caseeq(tokens[0],
                                        "KbdInteractiveAuthentication")) {
            method = Ssh_auth_methods::KBDINT_AUTH;
          } else if (shcore::str_caseeq(tokens[0], "PasswordAuthentication")) {
            method = Ssh_auth_methods::PASSWORD_AUTH;
          } else if (shcore::str_caseeq(tokens[0], "PubkeyAuthentication")) {
            method = Ssh_auth_methods::PUBKEY_AUTH;
          } else if (shcore::str_caseeq(tokens[0], "HostName")) {
            next_host = value;
          }

          if (method.has_value()) {
            if (shcore::str_caseeq(value, "yes")) {
              out_config->auth_methods.set(*method);
            } else {
              out_config->auth_methods.unset(*method);
            }
          }
        }
      }
    }
    ifs.close();
  }

  if (!next_host.empty() &&
      std::find(hosts->begin(), hosts->end(), next_host) == hosts->end()) {
    hosts->push_back(next_host);
    read(out_config, next_host, path, hosts);
  }
}

}  // namespace ssh
}  // namespace mysqlshdk
