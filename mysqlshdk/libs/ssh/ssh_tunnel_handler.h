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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_TUNNEL_HANDLER_H_
#define MYSQLSHDK_LIBS_SSH_SSH_TUNNEL_HANDLER_H_

#include <errno.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <poll.h>
#endif
#include <string.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "mysqlshdk/libs/ssh/ssh_common.h"
#include "mysqlshdk/libs/ssh/ssh_session.h"

namespace mysqlshdk {
namespace ssh {

/**
 * @brief Handle SSH data transfer between local port and remote port using
 * ssh::Channel.
 *
 */
class Ssh_tunnel_handler : public Ssh_thread {
 public:
  Ssh_tunnel_handler(uint16_t local_port, int local_socket,
                     std::unique_ptr<ssh::Ssh_session> session);
  ~Ssh_tunnel_handler();
  int local_socket() const;
  int local_port() const;
  const Ssh_connection_options &config() const;

  /**
   * @brief takes care of the incoming connection, accepts it and set socket
   * option.
   *
   * @param incoming_socket the socket which got the connection
   */
  bool handle_new_connection(int incoming_socket);

  void use() { ++m_usage; }
  int release() {
    assert(m_usage > 0);
    return --m_usage;
  }
  Ssh_session_info get_tunnel_info() const {
    return m_session->get_session_info();
  }

 protected:
  void run() override;

  std::unique_ptr<Ssh_session> m_session;
  uint16_t m_local_port;
  int m_local_socket;
  std::map<int, std::unique_ptr<::ssh::Channel>> m_client_socket_list;
  ssh_event m_event;

 private:
  void handle_connection();
  void transfer_data_from_client(int sock, ::ssh::Channel *chan);
  void transfer_data_to_client(int sock, ::ssh::Channel *chan);
  std::unique_ptr<::ssh::Channel> open_tunnel();
  void prepare_tunnel(int client_socket);

  std::recursive_mutex m_new_connection_mtx;
  std::queue<int> m_new_connection;
  std::atomic_int m_usage = 0;
};

}  // namespace ssh
}  // namespace mysqlshdk
#endif  //  MYSQLSHDK_LIBS_SSH_SSH_TUNNEL_HANDLER_H_
