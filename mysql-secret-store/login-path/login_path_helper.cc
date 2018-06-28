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

#include "mysql-secret-store/login-path/login_path_helper.h"

#include <my_alloc.h>
#include <my_default.h>
#include <mysql.h>

#include <memory>
#include <utility>

#include "mysql-secret-store/login-path/entry.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

using mysql::secret_store::common::Helper_exception;
using mysql::secret_store::common::Helper_exception_code;
using mysql::secret_store::common::get_helper_exception;

namespace mysql {
namespace secret_store {
namespace login_path {

namespace {

constexpr auto k_secret_type_password = "password";

std::vector<Entry> parse_ini(const std::string &ini) {
  // mysql_config_editor has very structured output, so parsing is simplified
  std::vector<Entry> result;

  if (!ini.empty()) {
    for (const auto &line : shcore::str_split(ini, "\r\n", -1, true)) {
      if (line[0] == '[') {
        result.emplace_back();
        result.back().name = line.substr(1, line.length() - 2);
      } else {
        const auto pos = line.find(" = ");
        const auto option = line.substr(0, pos);
        const auto value = line.substr(pos + 3);

        if (option == "user") {
          result.back().user = value;
        } else if (option == "host") {
          result.back().host = value;
        } else if (option == "socket") {
          result.back().socket = value;
        } else if (option == "port") {
          result.back().port = value;
        } else if (option == "password") {
          result.back().has_password = true;
        } else {
          throw Helper_exception{
              "Failed to parse INI output, unknown option: " + option};
        }
      }
    }
  }

  return result;
}

void validate_secret_type(const common::Secret_id &id) {
  if (id.secret_type != k_secret_type_password) {
    throw Helper_exception{
        "The login-path helper is only able to store passwords."};
  }
}

std::string get_url(const Entry &entry, bool encode) {
  mysqlshdk::db::uri::Uri_encoder encoder;
  return (entry.user.empty()
              ? ""
              : (encode ? encoder.encode_userinfo(entry.user) : entry.user) +
                    "@") +
         (encode ? encoder.encode_host(entry.host) : entry.host) +
         (entry.port.empty()
              ? entry.socket.empty()
                    ? ""
                    : (encode ? encoder.encode_socket(entry.socket)
                              : entry.socket)
              : ":" + (encode ? encoder.encode_port(entry.port) : entry.port));
}

std::string get_encoded_url(const Entry &entry) { return get_url(entry, true); }

std::string get_name(const Entry &entry) {
  // name is used as a section name in an INI file, cannot contain [] characters
  return shcore::str_replace(
      shcore::str_replace(get_url(entry, false), "[", "%5B"), "]", "%5D");
}

}  // namespace

Login_path_helper::Login_path_helper()
    : common::Helper("login-path", MYSH_VERSION, MYSH_HELPER_COPYRIGHT) {}

void Login_path_helper::check_requirements() { m_invoker.validate(); }

void Login_path_helper::store(const common::Secret &secret) {
  // my_load_defaults will replace \* combinations with *, need to quote it
  const auto s = shcore::quote_string(secret.secret, '"');

  // mysql_config_editor reads the password to a fixed-size buffer of 80
  // characters. This buffer has to hold the password and a terminating
  // null character, hence length is limited to 79 characters. Due to a bug,
  // passwords longer than 78 characters are not null-terminated, so we're
  // setting 78 as the upper limit. Secret is quoted (always adding at least
  // two characters) leaving 76 characters for user input.
  if (s.length() > 78) {
    throw Helper_exception{
        "The login-path helper cannot store secrets longer than 76 "
        "characters. Please keep in mind that all '\\' and '\"' characters"
        "are prepended with '\\', decreasing available space."};
  }

  m_invoker.store(to_entry(secret.id), s);
}

void Login_path_helper::get(const common::Secret_id &id, std::string *secret) {
  *secret = load(find(id));
}

void Login_path_helper::erase(const common::Secret_id &id) {
  auto entry = find(id);

  if (Entry_type::EXTERNAL == entry.entry_type) {
    // check if there's more than one external entry with the same name
    auto entries = list();
    std::vector<Entry> result;

    std::copy_if(entries.begin(), entries.end(), std::back_inserter(result),
                 [&entry](const Entry &e) {
                   return e.name == entry.name &&
                          e.entry_type == Entry_type::EXTERNAL;
                 });

    if (result.size() == 1) {
      // only one entry, safe to erase
      m_invoker.erase(entry);
    } else {
      // two entries mean that original login-path section had both port and
      // socket, we need to remove only port or only socket
      mysqlshdk::db::Connection_options options{id.url};

      if (options.has_port()) {
        m_invoker.erase_port(entry);
      } else {
        m_invoker.erase_socket(entry);
      }
    }
  } else {
    m_invoker.erase(entry);
  }
}

void Login_path_helper::list(std::vector<common::Secret_id> *secrets) {
  for (const auto &entry : list()) {
    secrets->emplace_back(
        common::Secret_id{entry.secret_type, get_encoded_url(entry)});
  }
}

std::vector<Entry> Login_path_helper::list() {
  auto intermediate_result = parse_ini(m_invoker.list());

  std::vector<Entry> result;

  for (auto &entry : intermediate_result) {
    // we're not interested in entries without passwords (i.e. client section)
    if (entry.has_password) {
      entry.secret_type = k_secret_type_password;

      if (entry.name == get_name(entry)) {
        // entry was created by helper
        entry.entry_type = Entry_type::SET_BY_HELPER;

        result.emplace_back(std::move(entry));
      } else {
        // unknown entry, validate if it's suitable
        entry.entry_type = Entry_type::EXTERNAL;

        if (!entry.socket.empty() && !entry.host.empty()) {
          if (!mysqlshdk::utils::Net::is_local_address(entry.host)) {
            throw Helper_exception{"Invalid entry: " + entry.name +
                                   ". Entry has a socket and a host which "
                                   "resolves to non-local address."};
          }
        }

        if (!entry.port.empty() && entry.host.empty()) {
          throw Helper_exception{
              "Invalid entry: " + entry.name +
              ". Entry has a port but does not have a host."};
        }

        if (!entry.port.empty() && !entry.socket.empty()) {
          // entry has both port and socket, create two entries, one for port
          // and one for socket
          result.emplace_back(entry);
          result.back().socket.erase();

          result.emplace_back(entry);
          result.back().port.erase();
          result.back().host.erase();
        } else {
          if (!entry.socket.empty() && !entry.host.empty()) {
            // if entry has a socket, host needs to be empty
            entry.host.erase();
          }

          result.emplace_back(std::move(entry));
        }
      }
    }
  }

  return result;
}

Entry Login_path_helper::find(const common::Secret_id &secret) {
  validate_secret_type(secret);

  auto entries = list();
  std::vector<Entry> result;
  const auto secret_entry = to_entry(secret);
  const auto encoded_entry = get_encoded_url(secret_entry);

  std::copy_if(entries.begin(), entries.end(), std::back_inserter(result),
               [&secret_entry, &encoded_entry](const Entry &entry) {
                 return get_encoded_url(entry) == encoded_entry &&
                        entry.secret_type == secret_entry.secret_type;
               });

  switch (result.size()) {
    case 0:
      throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);

    case 1:
      return result[0];

    case 2:
      if (result[0].entry_type != result[1].entry_type) {
        // two entries match, return the one created by helper
        return result[0].entry_type == Entry_type::SET_BY_HELPER ? result[0]
                                                                 : result[1];
      }

      // fall through

    default:
      throw Helper_exception{
          "Multiple entries match the secret, unable to deduce which one to "
          "return."};
  }
}

Entry Login_path_helper::to_entry(const common::Secret_id &secret) const {
  validate_secret_type(secret);

  Entry entry;
  entry.secret_type = secret.secret_type;
  entry.has_password = true;
  entry.entry_type = Entry_type::SET_BY_HELPER;

  mysqlshdk::db::Connection_options options{secret.url};

  if (options.has_user()) {
    entry.user = options.get_user();
  }
  if (options.has_host()) {
    entry.host = options.get_host();
  }
  if (options.has_port()) {
    entry.port = std::to_string(options.get_port());
  }
  if (options.has_socket()) {
    entry.socket = options.get_socket();
  }

  entry.name = get_name(entry);

  return entry;
}

std::string Login_path_helper::load(const Entry &entry) {
  static constexpr auto password = "--password=";
  // call to mysql_init() is required to initialize mutexes
  std::unique_ptr<MYSQL, decltype(&mysql_close)> deleter{mysql_init(nullptr),
                                                         mysql_close};

  MEM_ROOT argv_alloc{};  // uses: PSI_NOT_INSTRUMENTED, 512
  shcore::Scoped_callback release_argv_alloc(
      [&argv_alloc]() { free_root(&argv_alloc, MYF(0)); });
  std::string argv_name = name();
  char *argv_name_ptr = const_cast<char *>(argv_name.c_str());
  // my_load_defaults() requires argv[0] is not null
  char **new_argv = &argv_name_ptr;
  int new_argc = 1;
  const char *load_default_groups[] = {entry.name.c_str(), nullptr};
#ifdef _WIN32
  const char *null_file = "nul";  // or "\Device\Null" or temporary file
#else
  const char *null_file = "/dev/null";
#endif

  if (my_load_defaults(null_file, load_default_groups, &new_argc, &new_argv,
                       &argv_alloc, nullptr)) {
    throw Helper_exception{"Failed to read the secret"};
  } else {
    for (int i = 0; i < new_argc; ++i) {
      if (shcore::str_beginswith(new_argv[i], password)) {
        return new_argv[i] + strlen(password);
      }
    }
  }

  throw Helper_exception{"Failed to read the secret"};
}

}  // namespace login_path
}  // namespace secret_store
}  // namespace mysql
