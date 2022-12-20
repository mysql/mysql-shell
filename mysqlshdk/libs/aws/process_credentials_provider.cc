/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/process_credentials_provider.h"

#include <cassert>
#include <cstdio>
#include <optional>
#include <string_view>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_time.h"

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

void throw_error(const std::string &string) {
  throw std::runtime_error{"AWS credential process: " + string};
}

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
    const std::string &path, const Aws_config_file::Profile *profile)
    : Aws_credentials_provider(
          {"output of 'credential_process' (config file " + path + ")",
           "AccessKeyId", "SecretAccessKey"}),
      m_profile(profile) {}

bool Process_credentials_provider::available(
    const Aws_config_file::Profile &profile) {
  return profile.settings.count("credential_process") > 0;
}

Aws_credentials_provider::Credentials
Process_credentials_provider::fetch_credentials() {
  const auto credential_process =
      m_profile->settings.find("credential_process");

  if (m_profile->settings.end() == credential_process ||
      credential_process->second.empty()) {
    return {};
  }

  return parse_json(run_process(credential_process->second));
}

Aws_credentials_provider::Credentials Process_credentials_provider::parse_json(
    const std::string &json) const {
  rapidjson::Document doc;
  doc.Parse(json.c_str(), json.length());

  const auto handle_error = [this, &json](const std::string &string) {
    log_debug("Output of the AWS credential process:\n%s",
              hide_secret(json, secret_access_key()).c_str());
    throw_error("AWS credential process: " + string);
  };

  if (doc.HasParseError()) {
    handle_error(std::string{"failed to parse JSON: "} +
                 rapidjson::GetParseError_En(doc.GetParseError()));
  }

  if (!doc.IsObject()) {
    handle_error("output should be a JSON object");
  }

  const auto version = doc.FindMember("Version");

  if (doc.MemberEnd() == version || !version->value.IsInt()) {
    handle_error(
        "JSON object should contain a 'Version' key with an integer value");
  }

  if (1 != version->value.GetInt()) {
    handle_error("got unsupported version, expected: 1");
  }

  const auto required = [&doc, &handle_error](const char *name) {
    const auto it = doc.FindMember(name);

    if (doc.MemberEnd() == it || !it->value.IsString()) {
      handle_error(shcore::str_format(
          "JSON object should contain a '%s' key with a string value", name));
    }

    return it->value.GetString();
  };

  const auto optional =
      [&doc, &handle_error](const char *name) -> std::optional<std::string> {
    const auto it = doc.FindMember(name);

    if (doc.MemberEnd() == it) {
      return {};
    }

    if (!it->value.IsString()) {
      handle_error(shcore::str_format(
          "JSON object should contain a '%s' key with a string value", name));
    }

    return it->value.GetString();
  };

  Credentials creds;

  creds.access_key_id = required(access_key_id());
  creds.secret_access_key = required(secret_access_key());
  creds.session_token = optional("SessionToken");

  if (const auto expiration = optional("Expiration"); expiration.has_value()) {
    try {
      creds.expiration = shcore::rfc3339_to_time_point(*expiration);
    } catch (const std::exception &e) {
      handle_error(std::string{"failed to parse 'Expiration' value: "} +
                   e.what());
    }
  }

  return creds;
}

}  // namespace aws
}  // namespace mysqlshdk
