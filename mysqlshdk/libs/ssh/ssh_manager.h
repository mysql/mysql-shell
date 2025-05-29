/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_MANAGER_H_
#define MYSQLSHDK_LIBS_SSH_SSH_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/ssh/ssh_common.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"

namespace mysqlshdk {
namespace ssh {
class Ssh_tunnel_manager;

/**
 * Holds a reference to a tunnel, automatically releases it when goes out of
 * scope.
 */
class Ssh_tunnel final {
 public:
  Ssh_tunnel() : Ssh_tunnel(nullptr, 0) {}

  Ssh_tunnel(const Ssh_tunnel &) = delete;
  Ssh_tunnel(Ssh_tunnel &&other);

  Ssh_tunnel &operator=(const Ssh_tunnel &) = delete;
  Ssh_tunnel &operator=(Ssh_tunnel &&other);

  ~Ssh_tunnel() { unref(); }

  /**
   * Manually increases the reference count. The `unref()` needs to be called
   * afterwards, or tunnel will never be deleted.
   */
  void ref();

  /**
   * Manually decreases the reference count.
   */
  void unref();

 private:
  friend class Ssh_manager;

  Ssh_tunnel(Ssh_tunnel_manager *manager, int port)
      : m_manager(manager), m_port(port) {
    ref();
  }

  Ssh_tunnel_manager *m_manager;
  int m_port;
};

/**
 * @brief This is the main entrypoint for SSH connections. Ssh_manager takes
 * care of tunnel connection and tracks it's usage so then the tunnel is
 * automatically closed if not needed.
 */
class Ssh_manager final {
 public:
  Ssh_manager();
  ~Ssh_manager();
  Ssh_manager(const Ssh_manager &other) = delete;
  Ssh_manager(Ssh_manager &&other) = delete;
  Ssh_manager &operator=(const Ssh_manager &other) = delete;
  Ssh_manager &operator=(Ssh_manager &&other) = delete;

  /**
   * @brief creates an SSH tunnel connection to the remote machine and open
   * local port to forward data. It will reuse existing tunnel if
   * Ssh_connection_config matches already existing open tunnel. The tunnel port
   * can be fetched by calling Ssh_connection_config::get_local_port()
   *
   * @param ssh_config Ssh_connection_config The SSH connection data
   *
   * This function will succeed if tunnel creation succeed or there's existing
   * tunnel that can be used.
   */
  [[nodiscard]] Ssh_tunnel create_tunnel(Ssh_connection_options *ssh_config);

  /**
   * Looks up tunnel for the given SSH config, increases its reference count if
   * found.
   */
  [[nodiscard]] Ssh_tunnel get_tunnel(
      const Ssh_connection_options &config) const;

  /**
   * @brief create a list of active SSH tunnel connections
   *
   * @return string list of ssh uri active connections
   */
  std::vector<Ssh_session_info> list_active_tunnels();

 private:
  void start();
  void shutdown();

  std::mutex m_create_tunnel_mutex;
  std::unique_ptr<Ssh_tunnel_manager> m_manager;
};

std::shared_ptr<Ssh_manager> current_ssh_manager(bool allow_empty = false);

}  // namespace ssh
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_SSH_SSH_MANAGER_H_
