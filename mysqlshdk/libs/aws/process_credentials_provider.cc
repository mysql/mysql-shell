/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/process_credentials_provider.h"

#include <cassert>
#include <cstdio>
#include <optional>
#include <string_view>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#ifdef _WIN32
#define pclose _pclose
#endif  // _WIN32

namespace mysqlshdk {
namespace aws {

namespace {

std::string hide_secret(const std::string &s, std::string_view secret) {
  static constexpr std::string_view k_quotes{"\"'"};
  assert(std::string::npos == secret.find_first_of(k_quotes));

  const auto pos = s.find(secret);

  if (std::string::npos == pos) {
    return s;
  }

  const auto handle_unquoted = [&s](auto first_char) {
    // find potential start of a value
    const auto start =
        s.find_first_not_of(std::string_view{": \t"}, first_char);

    if (std::string::npos == start) {
      // give up
      return s;
    }

    const auto end = s.find_first_of(" \t\n\r", start);

    if (std::string::npos == end) {
      // hide till the end
      return s.substr(0, start) + "****";
    }

    return s.substr(0, start) + "****" + s.substr(end);
  };

  const auto closing_quote = s.find_first_of(k_quotes, pos + secret.size());

  if (std::string::npos == closing_quote) {
    return handle_unquoted(pos + secret.size());
  }

  const auto start = s.find_first_of(k_quotes, closing_quote + 1);

  if (std::string::npos == start) {
    return handle_unquoted(closing_quote + 1);
  }

  const auto end = '"' == s[start]
                       ? mysqlshdk::utils::span_quoted_string_dq(s, start)
                       : mysqlshdk::utils::span_quoted_string_sq(s, start);

  if (std::string::npos == end) {
    // hide till the end
    return s.substr(0, start + 1) + "****";
  }

  return s.substr(0, start + 1) + "****" + s.substr(end - 1);
}

auto error(const std::string &string) {
  return std::runtime_error{"AWS credential process: " + string};
}

void throw_error(const std::string &string) { throw error(string); }

std::string run_process(const std::string &process) {
  log_debug("Running AWS credential process: %s", process.c_str());

  const auto pipe =
#ifdef _WIN32
      _wpopen(shcore::utf8_to_wide(process).c_str(), L"r");
#else   // !_WIN32
      popen(process.c_str(), "r");
#endif  // !_WIN32
  ;

  if (nullptr == pipe) {
    throw_error("failed to execute process: " + shcore::errno_to_string(errno));
  }

  std::string output;
  char buffer[512];

  while (fgets(buffer, sizeof(buffer), pipe)) {
    output.append(buffer);
  }

  const auto error_code = pclose(pipe);

  if (error_code < 0) {
    throw_error("failed to close the process: " +
                shcore::errno_to_string(errno));
  }

  if (error_code > 0) {
    throw_error("process returned an error code " + std::to_string(error_code) +
                ", output: " + output);
  }

  if (output.empty()) {
    throw_error("process did not write any output");
  }

  return output;
}

}  // namespace

Process_credentials_provider::Process_credentials_provider(
    const Settings &settings, const std::string &profile)
    : Aws_credentials_provider(
          {"output of 'credential_process' (config file: " +
               settings.get_string(Setting::CONFIG_FILE) +
               ", profile: " + profile + ")",
           "AccessKeyId", "SecretAccessKey"}) {
  if (const auto p = settings.config_profile(profile)) {
    if (const auto s = p->get("credential_process"); s && !s->empty()) {
      m_process = s;
    } else {
      log_info("Profile '%s' does not have the 'credential_process' setting.",
               profile.c_str());
    }
  } else {
    log_warning(
        "The config file at '%s' does not contain a profile named: '%s'.",
        settings.get_string(Setting::CONFIG_FILE).c_str(), profile.c_str());
  }
}

bool Process_credentials_provider::available() const noexcept {
  return m_process;
}

Aws_credentials_provider::Credentials
Process_credentials_provider::fetch_credentials() {
  return parse_json(run_process(*m_process));
}

Aws_credentials_provider::Credentials Process_credentials_provider::parse_json(
    const std::string &json) const {
  try {
    return parse_json_credentials(
        json, "SessionToken", 2, [](const shcore::json::JSON &doc) {
          if (1 != shcore::json::required_uint(doc, "Version")) {
            throw std::runtime_error{"got unsupported version, expected: 1"};
          }
        });
  } catch (const std::exception &e) {
    log_debug("Output of the AWS credential process:\n%s",
              hide_secret(json, secret_access_key()).c_str());
    throw error(e.what());
  }
}

}  // namespace aws
}  // namespace mysqlshdk
