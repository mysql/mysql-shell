/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/ssh/ssh_connection_options.h"

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/ssh/ssh_session_options.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/nullable_options.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace ssh {

Ssh_connection_options::Ssh_connection_options(
    const std::string &uri,
    mysqlshdk::utils::nullable_options::Comparison_mode mode)
    : Ssh_connection_options(mode) {
  try {
    mysqlshdk::db::uri::Uri_parser parser(false);
    *this = parser.parse_ssh_uri(uri, m_options.get_mode());
  } catch (const std::invalid_argument &error) {
    std::string msg = "Invalid URI: ";
    msg.append(error.what());
    throw std::invalid_argument(msg);
  }
}

Ssh_connection_options::Ssh_connection_options(
    mysqlshdk::utils::nullable_options::Comparison_mode mode)
    : IConnection("ssh_connection", mode), m_key_encrypted(false) {
  for (const auto &o :
       {mysqlshdk::db::kSshRemoteHost, mysqlshdk::db::kSshConfigFile,
        mysqlshdk::db::kSshIdentityFile})
    m_options.set(o, nullptr,
                  mysqlshdk::utils::nullable_options::Set_mode::CREATE);
  set_scheme("ssh");
}

std::string Ssh_connection_options::as_uri(
    mysqlshdk::db::uri::Tokens_mask format) const {
  mysqlshdk::db::uri::Uri_encoder encoder(false);
  return encoder.encode_uri(*this, format);
}

std::string Ssh_connection_options::key_file_uri() const {
  auto filename = get_key_file();
  if (filename[0] == '/') filename.erase(filename.begin());

  return "file:/" + filename;
}

void Ssh_connection_options::set_default_data() {
  // Default values
  if (!has_config_file()) {
    const auto &config_file =
        mysqlsh::current_shell_options(true)->get().ssh.config_file;
    if (!config_file.empty()) {
      set_config_file(config_file);
    }
  }

  preload_ssh_config();

  if (!has_user()) set_user(shcore::get_system_user());
  if (!has_port()) set_port(22);
}

void Ssh_connection_options::preload_ssh_config() {
  // we have nothing to do if host and config is missing
  if (!has_host()) return;

  // we need to create an ssh session so we can read data from the stanza
  Ssh_session_options ssh_config(has_config_file() ? get_config_file() : "",
                                 get_host());

  // If no user/port are provided in the connection options but they are
  // actually configured in the SSH connections file, we load them into the
  // connection options to make them visible to the user
  std::string user = ssh_config.get_user();
  if (!user.empty() && !has_user()) {
    set_user(user);
  }

  auto port = ssh_config.get_port();
  if (port > 0 && !has_port()) {
    set_port(port);
  }
}

bool Ssh_connection_options::interactive() {
  return mysqlsh::current_shell_options(true)->get().wizards;
}

void Ssh_connection_options::check_key_encryption(const std::string &path) {
  std::ifstream ifs;
#ifdef _WIN32
  const auto wide_filename = shcore::utf8_to_wide(path);
  ifs.open(wide_filename, std::ofstream::in);
#else
  ifs.open(path, std::ofstream::in);
#endif

  if (!ifs.good()) throw std::runtime_error("Unable to open file " + path);

  int i = 0;
  for (std::string line; getline(ifs, line); ++i) {
    // we care only about the second line
    if (i == 1) {
      if (shcore::str_casestr(line.c_str(), "encrypted") != nullptr) {
        m_key_encrypted = true;
        return;
      }
      break;
    }
  }
  m_key_encrypted = false;
}

void Ssh_connection_options::dump_config() const {
  log_debug2("SSH: Connection config info:");
  log_debug2("SSH: connectTimeout: %zu", m_connection_timeout);
  log_debug2("SSH: bufferSize: %zu", m_buffer_size);
  if (has_config_file())
    log_debug2("SSH: config file: %s", get_config_file().c_str());
  log_debug2("SSH: local host: %s", m_sourcehost.c_str());
  if (!m_local_port.is_null()) log_debug2("SSH local port: %d", *m_local_port);
  log_debug2("SSH: remote host: %s", get_remote_host().c_str());
  if (!m_remote_port.is_null())
    log_debug2("SSH remote port: %d", *m_remote_port);
  log_debug2("SSH: remote ssh host: %s", get_host().c_str());
  if (has_port()) {
    log_debug2("SSH: remote ssh port: %d", get_port());
  }
}

bool Ssh_connection_options::compare_connection(
    const Ssh_connection_options &other) const {
  return (m_sourcehost == other.m_sourcehost &&
          get_user() == other.get_user() && get_host() == other.get_host() &&
          get_port() == other.get_port() &&
          get_remote_host() == other.get_remote_host() &&
          get_remote_port() == other.get_remote_port());
}

namespace {
std::string verify_path(const std::string &path, bool allow_empty = false) {
  auto stripped_path = mysqlshdk::storage::utils::strip_scheme(path);
  auto full_path = shcore::path::expand_user(stripped_path);
  if (!shcore::path::is_absolute(full_path)) {
    throw std::invalid_argument("Only absolute paths are accepted, the path '" +
                                path + "' looks like relative one.");
  }

  if (!allow_empty && !shcore::path_exists(full_path))
    throw std::invalid_argument("The path '" + path + "' doesn't exist");

  if (!allow_empty) shcore::check_file_readable_or_throw(full_path);

  if (!allow_empty) shcore::check_file_access_rights_to_open(full_path);
  return full_path;
}
}  // namespace

void Ssh_connection_options::set_key_file(const std::string &path) {
  if (path.empty()) {
    clear_key_file();
  } else {
    try {
      auto full_path = verify_path(path);
      check_key_encryption(full_path);
      _set_fixed(mysqlshdk::db::kSshIdentityFile, full_path);
    } catch (const std::exception &err) {
      throw std::invalid_argument(
          shcore::str_format("Invalid SSH Identity file: %s", err.what()));
    }
  }
}

void Ssh_connection_options::set_config_file(const std::string &file) {
  if (file.empty()) {
    clear_config_file();
  } else {
    try {
      auto full_path = verify_path(file);
      _set_fixed(mysqlshdk::db::kSshConfigFile, full_path);
    } catch (const std::exception &err) {
      throw std::invalid_argument(
          shcore::str_format("Invalid SSH configuration file: %s", err.what()));
    }
  }
}

}  // namespace ssh
}  // namespace mysqlshdk
