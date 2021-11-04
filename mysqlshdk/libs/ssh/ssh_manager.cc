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

#include "mysqlshdk/libs/ssh/ssh_manager.h"
#include <string>
#include <vector>
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/libs/ssh/ssh_session.h"
#include "mysqlshdk/libs/ssh/ssh_tunnel_manager.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/shellcore/credential_manager.h"

namespace mysqlshdk {
namespace ssh {

Ssh_manager::Ssh_manager() {}

void ssh::Ssh_manager::start() {
  m_manager = std::make_unique<ssh::Ssh_tunnel_manager>();

  log_info("SSH: manager: Starting tunnel");
  m_manager->start();
}

void Ssh_manager::shutdown() {
  m_manager->set_stop();
  m_manager->poke_wakeup_socket();
}

void Ssh_manager::port_usage_increment(const Ssh_connection_options &config) {
  if (m_manager) m_manager->use_tunnel(config);
}

void Ssh_manager::port_usage_decrement(const Ssh_connection_options &config) {
  if (m_manager) m_manager->release_tunnel(config);
}

void Ssh_manager::create_tunnel(Ssh_connection_options *ssh_config) {
  // This function can only be called if there's SSH data
  assert(ssh_config->has_data());

  // There can be only one Ssh_manager per application and it cannot be started
  // twice. Access to it can be done only through
  // mysqlshdk::ssh::current_ssh_manager();
  static std::once_flag manager_start_once;

  std::call_once(manager_start_once, [this] { start(); });

  int tunnel_port = m_manager->lookup_tunnel(*ssh_config);
  if (tunnel_port > 0) {
    mysqlsh::current_console()->print_info(
        "Existing SSH tunnel found, connecting...");
    if (ssh_config->has_local_port()) {
      if (ssh_config->get_local_port() != tunnel_port) {
        throw std::runtime_error(
            "Trying to overwrite existing tunnel connection");
      }
    }
    ssh_config->clear_local_port();
    ssh_config->set_local_port(tunnel_port);
  } else {
    auto session = std::make_unique<ssh::Ssh_session>();
    bool connected = false;
    while (!connected) {
      mysqlsh::current_console()->print_info("Opening SSH tunnel to " +
                                             ssh_config->get_server() + "...");

      auto ret_val = session->connect(*ssh_config);
      switch (std::get<0>(ret_val)) {
        case ssh::Ssh_return_type::CONNECTION_FAILURE: {
          std::string error_msg = std::get<1>(ret_val);
          throw std::runtime_error(std::string("Cannot open SSH Tunnel: ")
                                       .append(error_msg.c_str()));
        }
        case ssh::Ssh_return_type::CONNECTED: {
          auto tunnel_info = m_manager->create_tunnel(std::move(session));
          uint16_t port = std::get<1>(tunnel_info);
          log_debug("SSH: manager: SSH tunnel opened on port: %d",
                    static_cast<int>(port));
          ssh_config->set_local_port(port);
          connected = true;
          break;
        }
        case ssh::Ssh_return_type::INVALID_AUTH_DATA: {
          std::string error_msg = std::get<1>(ret_val);

          if (ssh_config->interactive()) {
            mysqlsh::current_console()->print_error(
                "Authentication error opening SSH tunnel: " + error_msg);
            switch (mysqlsh::current_console()->confirm(
                error_msg, mysqlsh::Prompt_answer::NO, "&Retry", "&Cancel")) {
              case mysqlsh::Prompt_answer::YES:
                shcore::Credential_manager::get().remove_password(*ssh_config);
                ssh_config->clear_password();
                session->disconnect();
                break;
              case mysqlsh::Prompt_answer::NO:
              default:
                throw shcore::cancelled("Tunnel connection cancelled");
            }
          } else {
            throw std::runtime_error(
                "Authentication error opening SSH tunnel: " + error_msg);
          }
          break;
        }
        case ssh::Ssh_return_type::FINGERPRINT_CHANGED:
        case ssh::Ssh_return_type::FINGERPRINT_MISMATCH: {
          std::string fingerprint = std::get<1>(ret_val);
          mysqlsh::current_console()->print_warning(
              "Server public key has changed.\nIt means either you're under "
              "attack or the administrator has changed the key.\nNew public "
              "fingerprint is: " +
              fingerprint);
          throw std::runtime_error("Invalid fingerprint detected.");
        }
        case ssh::Ssh_return_type::FINGERPRINT_UNKNOWN:  // The server is
                                                         // unknown. The public
                                                         // key fingerprint is:
        case ssh::Ssh_return_type::FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING: {
          std::string fingerprint = std::get<1>(ret_val);
          std::string msg =
              "The authenticity of host '" + ssh_config->get_host() +
              "' can't be established.\nServer key fingerprint is " +
              fingerprint;
          if (ssh_config->interactive()) {
            msg.append("\nAre you sure you want to continue connecting?");
            if (mysqlsh::current_console()->confirm(
                    msg, mysqlsh::Prompt_answer::NO, "&Yes", "&No") !=
                mysqlsh::Prompt_answer::YES)
              throw shcore::cancelled("Tunnel connection cancelled");
            ssh_config->set_fingerprint(fingerprint);
            session->disconnect();
          } else {
            // Not being able to confirm the connection is safe, we cancel it
            msg.append("\nTunnel connection cancelled");
            throw shcore::cancelled(msg.c_str());
          }
        }
      }
    }
  }
}

std::vector<Ssh_session_info> Ssh_manager::list_active_tunnels() {
  if (m_manager) {
    return m_manager->list_tunnels();
  }
  return std::vector<Ssh_session_info>();
}

Ssh_manager::~Ssh_manager() {
  if (m_manager) {
    shutdown();
    if (m_manager->is_running()) m_manager->join();
  }
}

}  // namespace ssh
}  // namespace mysqlshdk
