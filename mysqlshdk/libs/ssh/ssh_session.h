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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_SESSION_H_
#define MYSQLSHDK_LIBS_SSH_SSH_SESSION_H_

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#else
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
#include <libssh/callbacks.h>
#include <chrono>
#include <libssh/libsshpp.hpp>
#include <memory>
#include <string>
#include <tuple>
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#else
#pragma warning(pop)
#endif

#include "mysqlshdk/libs/ssh/ssh_common.h"
#include "mysqlshdk/libs/ssh/ssh_config_reader.h"
#include "mysqlshdk/libs/utils/threads.h"

namespace mysqlshdk {
namespace ssh {

/**
 * @brief General SSH connection class that performs connection and user
 * authentication.
 */
class Ssh_session final {
 public:
  Ssh_session();
  Ssh_session(const Ssh_session &other) = delete;
  Ssh_session(Ssh_session &&other) = delete;

  Ssh_session &operator=(const Ssh_session &other) = delete;
  Ssh_session &operator=(Ssh_session &&other) = delete;

  ~Ssh_session();

  /**
   * @brief set SSH connection settings, make connection, authentication and
   * handle fingerprint matching.
   *
   * @param config Ssh_connection_config
   * @return tuple which holds return code and message assigned for the given
   * code.
   */
  std::tuple<Ssh_return_type, std::string> connect(
      const Ssh_connection_options &config);

  void disconnect();
  bool is_connected() const;
  const Ssh_connection_options &config() const;
  void update_local_port(int port);

  void clean_connect();
  const char *get_ssh_error();

  /**
   * @brief open SSH channel for data transfer
   *
   * @return ssh::Channel
   */
  std::unique_ptr<::ssh::Channel> create_channel();

  const Ssh_connection_options &get_options() { return m_options; }

  Ssh_session_info get_session_info() const {
    return {m_options, m_time_created};
  }

 private:
  friend class Ssh_tunnel_handler;
  int verify_known_host(const ssh::Ssh_connection_options &config,
                        std::string *fingerprint);

  void authenticate_user(const ssh::Ssh_connection_options &config);
  Ssh_auth_return auth_agent(const ssh::Ssh_connection_options &config);
  Ssh_auth_return auth_password(ssh::Ssh_connection_options *config);
  Ssh_auth_return auth_auto_pubkey();
  Ssh_auth_return auth_key(ssh::Ssh_connection_options *config);
  Ssh_auth_return auth_interactive();
  Ssh_auth_return handle_auth_return(int auth);
  bool open_channel(::ssh::Channel *chann);
  ssh_session get_csession();

  std::unique_ptr<::ssh::Session> m_session;
  Ssh_connection_options m_options;
  bool m_is_connected;
  ssh_callbacks_struct m_ssh_callbacks;
  bool m_interactive;

  Ssh_config_data m_config;
  std::chrono::system_clock::time_point m_time_created;
};

}  // namespace ssh
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_SSH_SSH_SESSION_H_
