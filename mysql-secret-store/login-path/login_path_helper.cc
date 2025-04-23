/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
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

#include "mysql-secret-store/login-path/login_path_helper.h"

#include <my_alloc.h>
#include <my_default.h>
#include <mysql.h>

#include <cassert>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mysql-secret-store/login-path/entry.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/file_uri.h"
#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

using mysql::secret_store::common::get_helper_exception;
using mysql::secret_store::common::Helper_exception;
using mysql::secret_store::common::Helper_exception_code;
using mysql::secret_store::common::k_scheme_name_file;
using mysql::secret_store::common::k_scheme_name_ssh;

namespace mysql {
namespace secret_store {
namespace login_path {

namespace {

constexpr std::string_view k_secret_type_password = "password";
constexpr std::string_view k_scheme_delimiter = ":/";
constexpr std::string_view k_mysqlsh_scheme = "mysqlsh+";

/**
 * Parses the INI file outputted by mysql_config_editor.
 */
std::vector<Entry> parse_ini(const std::string &ini) {
  if (ini.empty()) {
    return {};
  }

  // mysql_config_editor has very structured output, so parsing is simplified
  std::vector<Entry> result;

  for (const auto &line : shcore::str_split(ini, "\r\n", -1, true)) {
    if (line[0] == '[') {
      result.emplace_back();
      auto name = line.substr(1, line.length() - 2);

      if (const auto pos = name.find(k_scheme_delimiter);
          std::string_view::npos != pos) {
        result.back().scheme = name.substr(0, pos);
      }

      result.back().name = std::move(name);
    } else {
      const auto pos = line.find(" = ");
      const auto option = line.substr(0, pos);
      auto value = shcore::unquote_string(line.substr(pos + 3), '"');

      if (option == "user") {
        result.back().user = std::move(value);
      } else if (option == "host") {
        result.back().host = std::move(value);
      } else if (option == "socket") {
        result.back().socket = std::move(value);
      } else if (option == "port") {
        result.back().port = std::move(value);
      } else if (option == "password") {
        result.back().has_password = true;
      } else {
        throw Helper_exception{"Failed to parse INI output, unknown option: " +
                               option};
      }
    }
  }

  return result;
}

/**
 * Cleans the name, so that in can be stored in an INI file.
 */
std::string clean_name(const std::string &name) {
  // name is used as a section name in an INI file, cannot contain [] characters
  return shcore::str_replace(name, "[", "%5B", "]", "%5D");
}

/**
 * Normalizes a URL which is not used to connect to a MySQL server (has a file
 * or an ssh scheme).
 */
std::string normalize_non_mysql_url(const std::string &scheme,
                                    const std::string &url) {
  assert(!scheme.empty());

  std::unique_ptr<mysqlshdk::db::uri::IUri_parsable> parsable;

  if (k_scheme_name_file == scheme) {
    parsable = std::make_unique<mysqlshdk::db::uri::File_uri>(url);
  } else if (k_scheme_name_ssh == scheme) {
    parsable = std::make_unique<mysqlshdk::ssh::Ssh_connection_options>(url);
  } else {
    throw Helper_exception{"Unsupported scheme: " + scheme};
  }

  assert(parsable);

  return parsable->as_uri(mysqlshdk::db::uri::formats::scheme_user_transport());
}

/**
 * Normalizes a URL which is used to connect to a MySQL server (doesn't have a
 * scheme).
 */
std::string normalize_mysql_url(const Entry &entry) {
  assert(entry.scheme.empty());

  // URLs which are not encoded are used as an internal detail and help to
  // identify entries which were not created by us. Such entries can have both
  // socket and port set, which means Connection_options cannot be used.
  std::string normalized;

  normalized.reserve(64);

  if (!entry.user.empty()) {
    normalized += entry.user;
    normalized += '@';
  }

  if (entry.socket.empty()) {
    normalized += entry.host;

    if (!entry.port.empty()) {
      normalized += ':';
      normalized += entry.port;
    }
  } else {
    normalized += entry.socket;
  }

  return normalized;
}

/**
 * Initializes entry read from the INI file.
 */
void initialize_entry(Entry *entry) {
  entry->secret_type = k_secret_type_password;

  if (entry->scheme.empty()) {
    // entry was created by helper if its name matches the name that we would
    // write
    entry->entry_type = clean_name(normalize_mysql_url(*entry)) == entry->name
                            ? Entry_type::SET_BY_HELPER
                            : Entry_type::EXTERNAL;
  } else {
    if (k_scheme_name_file == entry->scheme ||
        k_scheme_name_ssh == entry->scheme) {
      // we assume that all entries with file/ssh scheme are created by helper
      entry->entry_type = Entry_type::SET_BY_HELPER;
    } else if (entry->scheme.starts_with(k_mysqlsh_scheme)) {
      entry->entry_type = Entry_type::SET_BY_HELPER;
      entry->secret_type = entry->scheme.substr(k_mysqlsh_scheme.length());
    } else {
      entry->entry_type = Entry_type::EXTERNAL;
    }
  }
}

/**
 * ID of an entry with 'password' secret type.
 */
std::string get_url_as_id(const Entry &entry) {
  assert(k_secret_type_password == entry.secret_type);

  if (!entry.scheme.empty()) {
    try {
      return normalize_non_mysql_url(entry.scheme, entry.name);
    } catch (...) {
      // no op we do nothing here, just fallback to the default handling as it
      // looks like name is something we can't handle
    }
  }

  mysqlshdk::db::uri::Generic_uri uri_opts;
  mysqlshdk::db::uri::File_uri file_opts;
  mysqlshdk::db::Connection_options conn_opts;
  mysqlshdk::ssh::Ssh_connection_options ssh_opts;

  mysqlshdk::db::uri::IUri_parsable *options;

  if (!entry.scheme.empty()) {
    if (conn_opts.is_allowed_scheme(entry.scheme))
      options = &conn_opts;
    else if (ssh_opts.is_allowed_scheme(entry.scheme))
      options = &ssh_opts;
    else if (file_opts.is_allowed_scheme(entry.scheme))
      options = &file_opts;
    else
      options = &uri_opts;
  } else {
    if (!entry.socket.empty())
      options = &conn_opts;
    else
      options = &uri_opts;
  }

  if (!entry.user.empty()) {
    options->set(mysqlshdk::db::kUser, entry.user);
  }
  if (!entry.host.empty()) {
    options->set(mysqlshdk::db::kHost, entry.host);
  }
  if (!entry.port.empty()) {
    options->set(mysqlshdk::db::kPort, std::stoi(entry.port));
  }
  if (!entry.socket.empty()) {
    options->set(mysqlshdk::db::kSocket, entry.socket);
  }

  return options->as_uri(
      entry.scheme.empty()
          ? mysqlshdk::db::uri::formats::user_transport()
          : mysqlshdk::db::uri::formats::scheme_user_transport());
}

/**
 * Gets the ID of the given entry.
 */
std::string get_id(const Entry &entry) {
  assert(!entry.secret_type.empty());

  if (k_secret_type_password == entry.secret_type) {
    return get_url_as_id(entry);
  } else {
    // we're guaranteed to find the delimiter, because this is a non-password
    // entry, whose type is extracted from a scheme
    const auto pos = entry.name.find(k_scheme_delimiter);
    assert(std::string::npos != pos);
    return shcore::pctdecode(
        std::string_view{entry.name}.substr(pos + k_scheme_delimiter.length()));
  }
}

/**
 * Checks if secret contains only valid characters.
 */
bool validate_secret(const std::string &secret) {
  return std::string::npos ==
         secret.find_first_of(
             "\x00\x03\x04\x0A\x0D\x0F\x11\x12\x13\x15\x16\x17\x19\x1A\x1C\x7F",
             0, 16);
}

}  // namespace

Login_path_helper::Login_path_helper()
    : common::Helper("login-path", shcore::get_long_version(),
                     MYSH_HELPER_COPYRIGHT) {}

void Login_path_helper::check_requirements() { m_invoker.validate(); }

void Login_path_helper::store(const common::Secret &secret) {
  using mysqlshdk::utils::Version;

  // my_load_defaults will replace \* combinations with *, need to quote it
  // in 8.0.24, mysql_config_editor will quote the string, but it still does not
  // escape the backslash characters
  const auto s = Version(m_invoker.version()) < Version(8, 0, 24)
                     ? shcore::quote_string(secret.secret, '"')
                     : shcore::str_replace(secret.secret, "\\", "\\\\");

  // mysql_config_editor reads the password to a fixed-size buffer of 80
  // characters. This buffer has to hold the password and a terminating
  // null character, hence length is limited to 79 characters. Due to a bug,
  // passwords longer than 78 characters are not null-terminated, so we're
  // setting 78 as the upper limit.
  if (s.length() > 78) {
    throw Helper_exception{
        "The login-path helper cannot store secrets longer than 78 "
        "characters. Please keep in mind that all '\\' characters are "
        "prepended with '\\', decreasing available space."};
  }

  if (!validate_secret(secret.secret)) {
    throw get_helper_exception(Helper_exception_code::INVALID_SECRET);
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
      mysqlshdk::db::uri::Generic_uri options{id.id};

      if (options.has_value(mysqlshdk::db::kPort)) {
        m_invoker.erase_port(entry);
      } else {
        m_invoker.erase_socket(entry);
      }
    }
  } else {
    m_invoker.erase(entry);
  }
}

void Login_path_helper::list(std::vector<common::Secret_id> *secrets,
                             std::optional<std::string> secret_type) {
  for (auto &entry : list()) {
    if (!secret_type.has_value() || entry.secret_type == *secret_type) {
      auto id = get_id(entry);
      secrets->emplace_back(
          common::Secret_id{std::move(entry.secret_type), std::move(id)});
    }
  }
}

std::vector<Entry> Login_path_helper::list() {
  auto intermediate_result = parse_ini(m_invoker.list());

  std::vector<Entry> result;

  for (auto &entry : intermediate_result) {
    // we're not interested in entries without passwords (i.e. client section)
    if (entry.has_password) {
      initialize_entry(&entry);

      if (Entry_type::SET_BY_HELPER == entry.entry_type) {
        // entry was created by helper
        result.emplace_back(std::move(entry));
      } else {
        // unknown entry, validate if it's suitable

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
  auto entries = list();
  std::vector<Entry> result;
  const auto secret_entry = to_entry(secret);
  const auto id = get_id(secret_entry);

  std::copy_if(
      std::make_move_iterator(entries.begin()),
      std::make_move_iterator(entries.end()), std::back_inserter(result),
      [secret_type = secret_entry.secret_type, &id](const Entry &entry) {
        return get_id(entry) == id && entry.secret_type == secret_type;
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

    default: {
      std::vector<std::string> data;

      for (const auto &entry : result) {
        data.push_back(get_id(entry));
      }

      throw Helper_exception{shcore::str_format(
          "Multiple entries match the secret, unable to deduce which one to "
          "return: [%s], [%s], [%s]",
          secret.id.c_str(), id.c_str(), shcore::str_join(data, ", ").c_str())};
    }
  }
}

Entry Login_path_helper::to_entry(const common::Secret_id &secret) const {
  Entry entry;
  entry.secret_type = secret.secret_type;
  entry.has_password = true;
  entry.entry_type = Entry_type::SET_BY_HELPER;

  if (k_secret_type_password == entry.secret_type) {
    mysqlshdk::db::uri::Generic_uri options{secret.id};

    if (options.has_value(mysqlshdk::db::kScheme)) {
      entry.scheme = options.get(mysqlshdk::db::kScheme);
    }
    if (options.has_value(mysqlshdk::db::kUser)) {
      entry.user = options.get(mysqlshdk::db::kUser);
    }
    if (options.has_value(mysqlshdk::db::kHost)) {
      entry.host = options.get(mysqlshdk::db::kHost);
    }
    if (options.has_value(mysqlshdk::db::kPort)) {
      entry.port = std::to_string(options.get_numeric(mysqlshdk::db::kPort));
    }
    if (options.has_value(mysqlshdk::db::kSocket)) {
      entry.socket = options.get(mysqlshdk::db::kSocket);
    }

    if (k_scheme_name_file == entry.scheme ||
        k_scheme_name_ssh == entry.scheme) {
      entry.name = normalize_non_mysql_url(entry.scheme, secret.id);
    } else {
      entry.name = normalize_mysql_url(entry);
    }

    entry.name = clean_name(entry.name);
  } else {
    entry.scheme = k_mysqlsh_scheme;
    entry.scheme += entry.secret_type;

    entry.name = entry.scheme;
    entry.name += k_scheme_delimiter;
    entry.name += shcore::pctencode(secret.id);
  }

  return entry;
}

std::string Login_path_helper::load(const Entry &entry) {
  static constexpr auto password = "--password=";
  // call to mysql_init() is required to initialize mutexes
  std::unique_ptr<MYSQL, decltype(&mysql_close)> deleter{mysql_init(nullptr),
                                                         mysql_close};

  MEM_ROOT argv_alloc{};  // uses: PSI_NOT_INSTRUMENTED, 512
  shcore::Scoped_callback release_argv_alloc(
      [&argv_alloc]() { argv_alloc.Clear(); });
  const auto argv_name = name();
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
