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

#include "mysqlshdk/libs/ssh/ssh_session_options.h"

#include "mysqlshdk/libs/ssh/ssh_common.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace {
std::string get_value(ssh_session sess, const enum ssh_options_e type) {
  char *value = nullptr;
  ssh_options_get(sess, type, &value);
  std::string ret_val;
  if (value != nullptr) {
    ret_val.assign(value);
    ssh_string_free_char(value);
  }
  // FIXME: there's a bug in libssh which makes this check unreliable
  // it can be enabled once ssh_options_get will be fixed
  // if (err == SSH_ERROR) throw
  // ::ssh::SshException(m_session->getCSession());
  return ret_val;
}
}  // namespace

namespace mysqlshdk {
namespace ssh {

Ssh_session_options::Ssh_session_options(const std::string &config,
                                         const std::string &host)
    : m_session{std::make_unique<::ssh::Session>()} {
  init_libssh();
  // The host is critical to read the stanza.
  if (host.empty()) {
    m_no_host = true;
    return;
  }
  m_session->setOption(SSH_OPTIONS_HOST, host.c_str());
  try {
    // On empty we pass nullptr to trigger loading the default configuration
    // files
    m_session->optionsParseConfig(config.empty() ? nullptr : config.c_str());
  } catch (::ssh::SshException &ex) {
    auto logger = shcore::current_logger(true);
    if (logger) {
      log_error("Error during SSH config parse: %s [%d], path was: %s",
                ex.getError().c_str(), ex.getCode(), config.c_str());
    } else {
      fprintf(stderr, "Error during SSH config parse: %s [%d], path was: %s",
              ex.getError().c_str(), ex.getCode(), config.c_str());
    }
    throw std::invalid_argument("Unable to parse specified SSH config file");
  }
}

std::string Ssh_session_options::get_host() const {
  return get_value(m_session->getCSession(), SSH_OPTIONS_HOST);
}

std::string Ssh_session_options::get_user() const {
  return get_value(m_session->getCSession(), SSH_OPTIONS_USER);
}

std::string Ssh_session_options::get_known_hosts() const {
  return get_value(m_session->getCSession(), SSH_OPTIONS_KNOWNHOSTS);
}

std::string Ssh_session_options::get_identity_file() const {
  if (m_no_host) return "";
  auto identity_file =
      get_value(m_session->getCSession(), SSH_OPTIONS_IDENTITY);
  identity_file =
      shcore::str_replace(identity_file, "%d", shcore::path::home());
  // libssh preloads session->options.identity with some predefined keys
  // if we are reading from the SSH config we can only check if the file exists
  // and if not, log error and return empty string so
  // Ssh_connection_config::set_key won't complain about missing file
  if (shcore::is_file(identity_file)) return identity_file;

  // There's a chance that the logger will try to log
  // before actually the logger will be ready.
  // and what's worse we can do nothing with it, so the only solution is to just
  // skip the log since there's no error log here, there should be no problem
  // with it.
  auto logger = shcore::current_logger(true);
  if (logger) {
    log_warning("SSH: The SSH identity file %s doesn't exist.",
                identity_file.c_str());
  }
  return "";
}

unsigned int Ssh_session_options::get_port() const {
  if (m_no_host) return 0;
  unsigned int port = 0;
  if (ssh_options_get_port(m_session->getCSession(), &port) == SSH_ERROR)
    log_error("SSH: Unable to obtain port");
  return port;
}
}  // namespace ssh
}  // namespace mysqlshdk
