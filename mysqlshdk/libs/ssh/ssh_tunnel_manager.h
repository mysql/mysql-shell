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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_TUNNEL_MANAGER_H_
#define MYSQLSHDK_LIBS_SSH_SSH_TUNNEL_MANAGER_H_

#include <errno.h>
#include <string.h>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/ssh/ssh_common.h"
#include "mysqlshdk/libs/ssh/ssh_session.h"
#include "mysqlshdk/libs/ssh/ssh_tunnel_handler.h"

namespace mysqlshdk {
namespace ssh {

/**
 * @brief Class is responsible for managing multiple tunnels. It is also
 * managing the local sockets which listen for incoming user connections.
 *
 */
class Ssh_tunnel_manager : public Ssh_thread {
 public:
  Ssh_tunnel_manager();

  /**
   * @brief prapre an SSH tunnel class and informs the thread to reload the
   * listen port list.
   *
   * @param session
   * @return
   */
  std::tuple<Ssh_return_type, uint16_t> create_tunnel(
      std::unique_ptr<Ssh_session> session);

  /**
   * @brief look for existing tunnel that can be reused based on the SSH
   * connection data.
   *
   * @param config Ssh_connection_config
   * @return existing tunnel port
   */
  int lookup_tunnel(const Ssh_connection_options &config);
  ~Ssh_tunnel_manager() override;

  /**
   * @brief trigger wakeup socket so it reload the listen port list
   *
   */
  void poke_wakeup_socket();
  void use_tunnel(const Ssh_connection_options &config);
  void release_tunnel(const Ssh_connection_options &config);
  void disconnect(const Ssh_connection_options &config);
  std::vector<Ssh_session_info> list_tunnels();

 private:
  typedef struct {
    uint16_t port;
    int socket_handle;
  } Sock_info;

  Sock_info create_socket(int backlog = 1);
  std::unique_lock<std::recursive_mutex> lock_socket_list();
  void run() override;
  void local_socket_handler();
  std::vector<pollfd> get_socket_list();

  mutable std::recursive_mutex m_socket_mtx;
  uint16_t m_wakeup_socket_port;
  int m_wakeup_socket;
  std::map<int, std::unique_ptr<Ssh_tunnel_handler>> m_socket_list;
};

}  // namespace ssh
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_SSH_SSH_TUNNEL_MANAGER_H_
