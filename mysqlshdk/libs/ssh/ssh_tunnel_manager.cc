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

#include "mysqlshdk/libs/ssh/ssh_tunnel_manager.h"
#ifndef _MSC_VER
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif
#endif
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/utils/logger.h"

#ifdef _WIN32
using ssize_t = __int64;
#endif  // _WIN32

namespace mysqlshdk {
namespace ssh {
namespace {
inline int ssh_poll(pollfd *data, size_t size) {
#if _MSC_VER
  return WSAPoll(data, static_cast<ULONG>(size), -1);
#else
  return poll(data, static_cast<nfds_t>(size), -1);
#endif
}
}  // namespace

Ssh_tunnel_manager::Ssh_tunnel_manager()
    : m_wakeup_socket_port(0), m_wakeup_socket(-1) {
#if _MSC_VER
  WSADATA wsa_data;
  int i_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (i_result != NO_ERROR) {
    throw Ssh_tunnel_exception("Error at WSAStartup()");
  }
#endif
  init_libssh();
  auto ret_val = create_socket();
  log_info("SSH: tunnel manager: Wakeup socket port created: %d", ret_val.port);
  m_wakeup_socket_port = ret_val.port;
  m_wakeup_socket = ret_val.socket_handle;
}

Ssh_tunnel_manager::~Ssh_tunnel_manager() {
  m_stop = true;

  shutdown(m_wakeup_socket, SHUT_RDWR);
  // first, let's shutdown all sockets
  for (auto &it : m_socket_list) {
    shutdown(it.first, SHUT_RDWR);
  }

  stop();  // wait for thread to finish
  auto sock_lock = lock_socket_list();
  for (auto &it : m_socket_list) {
    it.second->stop();
    it.second.release();
  }
#if _MSC_VER
  WSACleanup();
#endif
}

std::unique_lock<std::recursive_mutex> Ssh_tunnel_manager::lock_socket_list() {
  std::unique_lock<std::recursive_mutex> mtx(m_socket_mtx);
  return mtx;
}

void Ssh_tunnel_manager::run() { local_socket_handler(); }

Ssh_tunnel_manager::Sock_info Ssh_tunnel_manager::create_socket(int backlog) {
  Sock_info return_val;
  errno = 0;
  return_val.socket_handle = socket(AF_INET, SOCK_STREAM, 0);
  if (return_val.socket_handle == -1) {
    throw Ssh_tunnel_exception("unable to create socket: " + get_error());
  }

  int val = 1;
  errno = 0;
  if (setsockopt(return_val.socket_handle, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<char *>(&val), sizeof(val)) == -1) {
    ssh_close_socket(return_val.socket_handle);
    throw Ssh_tunnel_exception("unable to set socket option: " + get_error());
  }

  set_socket_non_blocking(return_val.socket_handle);

  struct sockaddr_in addr, info;
  socklen_t len = sizeof(struct sockaddr_in);
  memset(&addr, 0, len);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // OS will pick random port for us, we will get it later.
  addr.sin_port = htons(0);

  if (bind(return_val.socket_handle, (struct sockaddr *)&addr, len) == -1) {
    ssh_close_socket(return_val.socket_handle);
    throw Ssh_tunnel_exception("unable to bind: " + get_error());
  }

  getsockname(return_val.socket_handle, (struct sockaddr *)&info, &len);
  return_val.port = htons(info.sin_port);

  if (listen(return_val.socket_handle, backlog) == -1) {
    ssh_close_socket(return_val.socket_handle);
    throw Ssh_tunnel_exception("can't listen on socket: " + get_error());
  }

  return return_val;
}

std::tuple<Ssh_return_type, uint16_t> Ssh_tunnel_manager::create_tunnel(
    std::unique_ptr<Ssh_session> session) {
  log_debug3("SSH: tunnel manager: About to create ssh tunnel.");
  auto ret = create_socket(32);
  log_debug2("SSH: tunnel manager: Tunnel port created on socket: %d",
             ret.port);
  session->update_local_port(ret.port);
  std::unique_ptr<Ssh_tunnel_handler> handler(
      new Ssh_tunnel_handler(ret.port, ret.socket_handle, std::move(session)));
  handler->start();
  {
    const auto lock = lock_socket_list();
    m_socket_list.insert(std::make_pair(ret.socket_handle, std::move(handler)));
  }
  poke_wakeup_socket();  // If we're connected, we should notify manager that it
                         // shoud reload connection list.
  return std::make_tuple(Ssh_return_type::CONNECTED, ret.port);
}

int Ssh_tunnel_manager::lookup_tunnel(const Ssh_connection_options &config) {
  auto sock_lock = lock_socket_list();

  for (auto &it : m_socket_list) {
    if (it.second->config().compare_connection(config)) {
      if (!it.second->is_running()) {
        disconnect(config);
        log_warning("SSH: tunnel manager: Dead tunnel found, clearing it up.");
        return 0;
      }
      return it.second->local_port();
    }
  }

  return 0;
}

// We need to handle wakeupsocket connection, this should be enough.
static void accept_and_close(int socket) {
  struct sockaddr_in client;
  socklen_t addrlen = sizeof(client);
  errno = 0;
  int client_sock = accept(socket, (struct sockaddr *)&client, &addrlen);
  ssh_close_socket(client_sock);
}

std::vector<pollfd> Ssh_tunnel_manager::get_socket_list() {
  std::vector<pollfd> socket_list;
  {
    auto sock_lock = lock_socket_list();
    for (auto &it : m_socket_list) {
      pollfd p;
      p.fd = it.second->local_socket();
      p.events = POLLIN;
      socket_list.push_back(p);
    }
  }

  {
    pollfd p;
    p.fd = m_wakeup_socket;
    p.events = POLLIN;
    socket_list.push_back(p);
  }
  return socket_list;
}

void Ssh_tunnel_manager::local_socket_handler() {
  auto socket_list = get_socket_list();
  int rc = 0;
  do {
    // We need to duplicate this as we will be changing it
    // later, so we could loose other socket data.
    auto poll_socket_list = socket_list;
    rc = ssh_poll(poll_socket_list.data(), poll_socket_list.size());
    if (rc < 0) {
      log_error("SSH: tunnel manager: poll() error: %s.", get_error().c_str());
      break;
    }

    if (rc == 0) {
      log_error("SSH: tunnel manager: poll() timeout.");
      break;
    }

    for (auto &poll_it : poll_socket_list) {
      if (poll_it.revents == 0) continue;
      if (poll_it.revents == POLLERR) {
        log_error("SSH: tunnel manager: Error revents: %d.", poll_it.revents);
        m_stop = true;
        break;
      }

      if (poll_it.fd == m_wakeup_socket) {
        // This is special case we reload fds and continue.
        log_debug2(
            "SSH: tunnel manager: Wakeup socket got connection, reloading "
            "socket list.");
        socket_list.clear();
        accept_and_close(poll_it.fd);
        if (m_stop) break;

        socket_list = get_socket_list();
        continue;
      } else {  // This is a new connection, we need to handle it.
        auto sock_lock = lock_socket_list();
        auto it = m_socket_list.find(poll_it.fd);
        if (it != m_socket_list.end()) {
          while (it->second->handle_new_connection(poll_it.fd)) {
          }
        } else {
          // Let's check if this is something that wasn't removed from the sock
          // list, then just close it.
          bool found = false;
          for (const auto &s : poll_socket_list) {
            if (s.fd == poll_it.fd && s.fd != m_wakeup_socket) {
              shutdown(poll_it.fd, SHUT_RDWR);
              found = true;
              break;
            }
          }

          if (found) {  // We have to reload the socket list here.
            socket_list = get_socket_list();
          } else {
            log_error(
                "SSH: tunnel manager: Something went wrong, incoming socket "
                "connection "
                "wasn't found in the socketList, abort.");
            m_stop = true;
            break;
          }
        }
      }
    }
  } while (!m_stop);

  auto sock_lock = lock_socket_list();
  for (auto &s_it : m_socket_list) {
    s_it.second.release();
    shutdown(s_it.first, SHUT_RDWR);
  }

  // This means wakeup socket is also cleared.
  m_wakeup_socket = 0;
  m_socket_list.clear();
}

void Ssh_tunnel_manager::poke_wakeup_socket() {
  if (m_wakeup_socket_port == 0) {
    log_error("SSH: tunnel manager: Somehow wakeup socket isn't set yet.");
    return;
  }

  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr *)&server;
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    log_error("SSH: tunnel manager: Error occurred opening wakeup socket");
    return;
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(m_wakeup_socket_port);
  if (connect(sock, serverptr, sizeof(server)) < 0) {
    log_debug2(
        "SSH: tunnel manager: We've connected. Now we wait for socket to catch "
        "up and "
        "disconnect us.");
    ssize_t readlen = 0;
    std::vector<char> buff(1, '\0');
    errno = 0;
    readlen = recv(sock, buff.data(), buff.size(), 0);
    if (readlen == 0)
      log_debug2(
          "SSH: tunnel manager: Wakeup socket sent nothing, that's fine.");
    else
      log_error("SSH: tunnel manager: Wakeup socket error: %s.",
                get_error().c_str());
  }

  shutdown(sock, SHUT_RDWR);
}

void Ssh_tunnel_manager::use_tunnel(const Ssh_connection_options &config) {
  auto sock_lock = lock_socket_list();
  for (auto &it : m_socket_list) {
    if (it.second->config().compare_connection(config)) {
      it.second->use();
    }
  }
}

void Ssh_tunnel_manager::release_tunnel(const Ssh_connection_options &config) {
  auto sock_lock = lock_socket_list();
  for (auto &it : m_socket_list) {
    if (it.second->config().compare_connection(config)) {
      auto usage = it.second->release();

      if (usage == 0) {
        disconnect(config);
        break;
      }
    }
  }
}

void Ssh_tunnel_manager::disconnect(const Ssh_connection_options &config) {
  auto sock_lock = lock_socket_list();
  for (auto &it : m_socket_list) {
    if (it.second->config().compare_connection(config)) {
      // Here we need to perform disconnect
      it.second->stop();
      it.second.release();
      shutdown(it.first, SHUT_RDWR);
      m_socket_list.erase(it.first);
      log_debug2("SSH: tunnel manager: Shutdown port: %d",
                 config.get_local_port());
      break;
    }
  }
}

std::vector<Ssh_session_info> Ssh_tunnel_manager::list_tunnels() {
  std::vector<Ssh_session_info> ret_val;

  auto sock_lock = lock_socket_list();
  for (const auto &it : m_socket_list) {
    ret_val.push_back(it.second->get_tunnel_info());
  }

  return ret_val;
}

}  // namespace ssh
}  // namespace mysqlshdk
