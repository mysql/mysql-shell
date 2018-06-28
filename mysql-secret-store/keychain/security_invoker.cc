/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-secret-store/keychain/security_invoker.h"

#include "mysql-secret-store/include/helper.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysql {
namespace secret_store {
namespace keychain {

using mysql::secret_store::common::Helper_exception;

namespace {

constexpr auto k_keychain_name_env_variable =
    "MYSQLSH_CREDENTIAL_STORE_KEYCHAIN";
constexpr auto k_creator_code = "MYSH";

std::string quote_string(const std::string &input) {
  return shcore::quote_string(input, '\"');
}

char hexval(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  } else {
    throw Helper_exception{"Unknown hexadecimal digit: " + std::string{c}};
  }
}

std::string hex2string(const std::string &in) {
  std::string out;
  out.reserve(in.length() / 2);

  for (auto p = in.begin(); p != in.end(); ++p) {
    char c = hexval(*p);
    p++;
    if (p == in.end()) {
      throw Helper_exception{"Invalid hexadecimal string: " + in};
    }
    c = (c << 4) + hexval(*p);
    out.push_back(c);
  }

  return out;
}

}  // namespace

void Security_invoker::validate() { invoke({"help"}); }

void Security_invoker::store(const Entry &entry, const std::string &secret) {
  std::vector<std::string> args{"add-generic-password"};
  get_args(entry, &args, true);
  args.emplace_back("-U");
  args.emplace_back("-w");
  args.emplace_back(quote_string(secret));
  args.emplace_back(quote_string(get_keychain()));
  invoke({"-i"}, shcore::str_join(args, " ") + "\n");
}

std::string Security_invoker::get(const Entry &entry) {
  std::vector<std::string> args{"find-generic-password"};
  get_args(entry, &args);
  args.emplace_back("-g");
  args.emplace_back(get_keychain());
  auto output = invoke(args);

  if (!output.empty()) {
    static const std::string password_marker = "password: ";

    for (const auto &line : shcore::str_split(output, "\r\n", -1, true)) {
      if (line.find(password_marker) == 0) {
        auto password = line.substr(password_marker.length());

        if (password.empty()) {
          return "";
        } else if (password[0] == '"') {
          return password.substr(1, password.length() - 2);
        } else if (password[0] == '0' && password[1] == 'x') {
          return hex2string(
              password.substr(2, password.find_first_of(" ") - 2));
        }
      }
    }
  }

  throw Helper_exception{"Could not read the password"};
}

void Security_invoker::erase(const Entry &entry) {
  std::vector<std::string> args{"delete-generic-password"};
  get_args(entry, &args);
  args.emplace_back(get_keychain());
  invoke(args);
}

std::vector<Security_invoker::Entry> Security_invoker::list() {
  auto output = invoke({"dump-keychain", get_keychain()});
  std::vector<Entry> result;

  if (!output.empty()) {
    static const std::string new_entry_marker = "keychain:";
    static const std::string account_marker = "\"acct\"<blob>";
    static const std::string type_marker = "\"gena\"<blob>";
    static const std::string service_marker = "\"svce\"<blob>";
    static const std::string id_marker = "\"crtr\"<uint32>";
    static const std::string empty_marker = "<NULL>";

    bool should_add = false;
    Entry entry;

    for (const auto &line : shcore::str_split(output, "\r\n", -1, true)) {
      if (line.find(new_entry_marker) == 0) {
        if (should_add) {
          result.emplace_back(entry);
          should_add = false;
        }
      } else {
        const auto stripped_line = shcore::str_strip(line);
        const auto pos = stripped_line.find("=");

        if (std::string::npos != pos) {
          const auto attribute = stripped_line.substr(0, pos);
          auto value = stripped_line.substr(pos + 1);

          if (value != empty_marker) {
            value = shcore::str_strip(value, "\"");
          }

          if (attribute == id_marker) {
            if (value == k_creator_code) {
              should_add = true;
            }
          } else if (attribute == account_marker) {
            entry.account = value;
          } else if (attribute == type_marker) {
            entry.type = value;
          } else if (attribute == service_marker) {
            entry.service = value;
          }
        }
      }
    }

    if (should_add) {
      result.emplace_back(entry);
    }
  }

  return result;
}

std::string Security_invoker::invoke(const std::vector<std::string> &args,
                                     const std::string &input) const {
  std::vector<const char *> process_args;
  process_args.emplace_back("security");

  for (const auto &arg : args) {
    process_args.emplace_back(arg.c_str());
  }

  process_args.emplace_back(nullptr);

  shcore::Process_launcher app{&process_args[0]};

  app.start();

  if (!input.empty()) {
    app.write(input.c_str(), input.length());
    app.finish_writing();
  }

  std::string output = shcore::str_strip(app.read_all());
  const auto exit_code = app.wait();

  if (0 != exit_code) {
    throw Helper_exception{"Process \"" + std::string{process_args[0]} +
                           "\" finished with exit code " +
                           std::to_string(exit_code) + ": " + output};
  }

  return output;
}

void Security_invoker::get_args(const Entry &entry,
                                std::vector<std::string> *args, bool enquote) {
  auto process = [enquote](const std::string &s) {
    return enquote ? quote_string(s) : s;
  };
  args->emplace_back("-G");
  args->emplace_back(process(entry.type));
  args->emplace_back("-a");
  args->emplace_back(process(entry.account));
  args->emplace_back("-s");
  args->emplace_back(process(entry.service));
  args->emplace_back("-c");
  args->emplace_back(process(k_creator_code));
  args->emplace_back("-D");
  args->emplace_back(process("mysql " + entry.type));
}

std::string Security_invoker::get_keychain() {
  const auto env = getenv(k_keychain_name_env_variable);

  if (nullptr == env) {
    return shcore::str_strip(invoke({"default-keychain"}), "\"");
  } else {
    return env;
  }
}

}  // namespace keychain
}  // namespace secret_store
}  // namespace mysql
