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

#include "mysqlshdk/libs/ssh/ssh_tunnel_handler.h"

#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/utils/logger.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef _WIN32
using ssize_t = __int64;
#endif  // _WIN32

namespace mysqlshdk {
namespace ssh {

namespace {
int on_socket_event(socket_t UNUSED(fd), int UNUSED(revents),
                    void *UNUSED(userdata)) {
  // the return should be:
  //  0 success
  // -1 the internal ssh_poll_handle was removed/freed and should be removed
  // from the context -2 an error happened and the ssh_event_dopoll() should
  // stop
  return 0;
}

ssh_event make_event(ssh_session s) {
  auto event = ssh_event_new();
  ssh_event_add_session(event, s);
  return event;
}

void cleanup_event(ssh_event e, ssh_session s) {
  ssh_event_remove_session(e, s);
  ssh_event_free(e);
}

void cleanup_socket(ssh_event e, int sock,
                    std::unique_ptr<::ssh::Channel> chan) {
  ssh_event_remove_fd(e, sock);
  chan->close();
  ssh_close_socket(sock);
  chan.reset(nullptr);
}
}  // namespace

Ssh_tunnel_handler::Ssh_tunnel_handler(uint16_t local_port, int local_socket,
                                       std::unique_ptr<Ssh_session> session)
    : m_session(std::move(session)),
      m_local_port(local_port),
      m_local_socket(local_socket) {
  m_event = make_event(m_session->get_csession());
}

Ssh_tunnel_handler::~Ssh_tunnel_handler() {
  stop();
  if (m_session) {
    cleanup_event(m_event, m_session->get_csession());
    m_session->disconnect();
    m_session.reset();
  }
}

int Ssh_tunnel_handler::local_socket() const { return m_local_socket; }

int Ssh_tunnel_handler::local_port() const { return m_local_port; }

const Ssh_connection_options &Ssh_tunnel_handler::config() const {
  return m_session->config();
}

void Ssh_tunnel_handler::run() { handle_connection(); }

// This is noop function so ssh_even_dopoll will exit once client socket will
// have new data

void Ssh_tunnel_handler::handle_connection() {
  log_debug3("SSH: tunnel handler: Start tunnel handler thread.");
  int rc = 0;

  do {
    std::unique_lock<std::recursive_mutex> lock(m_new_connection_mtx);
    if (!m_new_connection.empty()) {
      prepare_tunnel(m_new_connection.front());
      m_new_connection.pop();
    }
    lock.unlock();
    rc = ssh_event_dopoll(m_event, 100);

    if (rc == SSH_ERROR) {
      auto ssh_error = m_session->get_ssh_error();
      if (ssh_error)
        log_error(
            "SSH: tunnel handler: There was an error handling connection poll, "
            "retrying: %s",
            ssh_error);
      else
        log_error(
            "SSH: tunnel handler: There was an error handling connection poll, "
            "retrying");

      for (auto &s_it : m_client_socket_list) {
        cleanup_socket(m_event, s_it.first, std::move(s_it.second));
      }
      m_client_socket_list.clear();

      cleanup_event(m_event, m_session->get_csession());

      if (!m_session->is_connected()) m_session->clean_connect();
      if (!m_session->is_connected()) {
        log_error("SSH: tunnel handler: Unable to reconnect session.");
        break;
      }

      m_event = make_event(m_session->get_csession());
      continue;
    }

    for (auto it = m_client_socket_list.begin();
         it != m_client_socket_list.end() && !m_stop;) {
      try {
        transfer_data_from_client(it->first, it->second.get());
        transfer_data_to_client(it->first, it->second.get());
        ++it;
      } catch (const Ssh_tunnel_exception &exc) {
        cleanup_socket(m_event, it->first, std::move(it->second));
        it = m_client_socket_list.erase(it);
        log_error("SSH: tunnel handler: Error during data transfer: %s",
                  exc.what());
      }
    }
  } while (!m_stop);

  for (auto &s_it : m_client_socket_list) {
    cleanup_socket(m_event, s_it.first, std::move(s_it.second));
  }
  m_client_socket_list.clear();
  log_debug3("SSH: tunnel handler: Tunnel handler thread stopped.");
}

bool Ssh_tunnel_handler::handle_new_connection(int incoming_socket) {
  log_debug3("SSH: tunnel handler: About to handle new connection.");
  struct sockaddr_in client;
  socklen_t addrlen = sizeof(client);
  errno = 0;
  int client_sock =
      accept(incoming_socket, (struct sockaddr *)&client, &addrlen);
  if (client_sock < 0) {
    if (errno != EWOULDBLOCK || errno == EINTR)
      log_error("SSH: tunnel handler: accept() failed: %s.",
                get_error().c_str());

    log_debug3("SSH: tunnel handler: No new connections.");
    return false;
  }

  set_socket_non_blocking(client_sock);

#ifdef __APPLE__
  int no_sigpipe = 1;
  if (setsockopt(client_sock, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                 sizeof(no_sigpipe)) < 0)
    log_error("SSH: tunnel handler: Failed to set SO_NOSIGPIPE on socket");
#endif

  std::lock_guard<std::recursive_mutex> guard(m_new_connection_mtx);
  m_new_connection.push(client_sock);
  log_debug3("SSH: tunnel handler: Accepted new connection.");
  return true;
}

void Ssh_tunnel_handler::transfer_data_from_client(int sock,
                                                   ::ssh::Channel *chan) {
  ssize_t readlen = 0;
  std::vector<char> buff(m_session->config().get_buffer_size(), '\0');

  while (!m_stop && (readlen = recv(sock, buff.data(), buff.size(), 0)) > 0) {
    int b_written = 0;
    for (char *buff_ptr = buff.data(); readlen > 0 && !m_stop;
         buff_ptr += b_written, readlen -= b_written) {
      try {
        b_written = chan->write(buff_ptr, readlen);
        if (b_written <= 0) {
          throw Ssh_tunnel_exception(
              "unable to write, remote end disconnected");
        }
      } catch (::ssh::SshException &exc) {
        throw Ssh_tunnel_exception(exc.getError());
      }
    }
  }
}

namespace {
std::string err_code_to_string(int err) {
  std::string ret_val;
  if (err == SSH_OK)
    ret_val = "NO ERROR";
  else if (err == SSH_ERROR)
    ret_val = "ERROR OF SOME KIND";
  else if (err == SSH_AGAIN)
    ret_val = "REPEAT CALL";
  else if (err == SSH_EOF)
    ret_val = "END OF FILE";
  else
    ret_val = "UNKNOWN ERROR";
  ret_val.append(" (" + std::to_string(err) + ")");
  return ret_val;
}
}  // namespace

void Ssh_tunnel_handler::transfer_data_to_client(int sock,
                                                 ::ssh::Channel *chan) {
  ssize_t readlen = 0;
  std::vector<char> buff(m_session->config().get_buffer_size(), '\0');
  do {
    try {
      readlen = chan->readNonblocking(buff.data(), buff.size());
    } catch (::ssh::SshException &exc) {
      throw Ssh_tunnel_exception(exc.getError());
    }

    // do not handle SSH_EOF in separate return step,
    // this will cause infite loop.

    if (readlen < 0 && readlen != SSH_AGAIN)
      throw Ssh_tunnel_exception("unable to read, remote end disconnected: " +
                                 err_code_to_string(readlen));

    if (readlen == 0) {
      if (chan->isClosed()) throw Ssh_tunnel_exception("channel is closed");
      break;
    }

    ssize_t b_written = 0;
    for (char *buff_ptr = buff.data(); readlen > 0 && !m_stop;
         buff_ptr += b_written, readlen -= b_written) {
      do {
        b_written = send(sock, buff_ptr, readlen, MSG_NOSIGNAL);
        if (b_written <= 0 && (errno == EAGAIN || errno == EINTR)) {
          continue;
        } else {
          break;
        }
      } while (true);
      if (b_written <= 0) {
        log_debug("SSH Error: errno: %d,", errno);
        throw Ssh_tunnel_exception("unable to write, client disconnected");
      }
    }
  } while (!m_stop);
}

std::unique_ptr<::ssh::Channel> Ssh_tunnel_handler::open_tunnel() {
  const int connection_delay = 100;
  auto channel = m_session->create_channel();
  ssh_channel_set_blocking(channel->getCChannel(), false);

  int rc = SSH_ERROR;
  std::size_t i = 0;

  while ((m_session->config().get_connection_timeout() * 1000 -
          (i * connection_delay)) > 0) {
    rc = channel->openForward(m_session->config().get_remote_host().c_str(),
                              m_session->config().get_remote_port(),
                              m_session->config().get_source_host().c_str(),
                              m_session->config().get_local_port());
    if (rc == SSH_AGAIN) {
      log_debug3(
          "SSH: tunnel handler: Unable to open channel, wait a moment and "
          "retry.");
      i++;
      std::this_thread::sleep_for(std::chrono::milliseconds(connection_delay));
    } else {
      log_debug("SSH: tunnel handler: Channel successfully opened");
      break;
    }
  }

  // If we're here and it's still not ok, we throw exception as we can't open
  // the channel.
  if (rc != SSH_OK) throw Ssh_tunnel_exception("Unable to open channel");

  return channel;
}

void Ssh_tunnel_handler::prepare_tunnel(int client_socket) {
  std::unique_ptr<::ssh::Channel> channel;
  try {
    channel = open_tunnel();

    int16_t events = POLLIN;
    if (ssh_event_add_fd(m_event, client_socket, events, on_socket_event,
                         this) != SSH_OK) {
      log_error(
          "SSH: tunnel handler: Unable to open tunnel. Could not register "
          "event handler.");
      channel.reset();
      ssh_close_socket(client_socket);
    } else {
      log_debug("SSH: tunnel handler: Tunnel created.");
      m_client_socket_list.insert(
          std::make_pair(client_socket, std::move(channel)));
    }
  } catch (const ssh::Ssh_tunnel_exception &exc) {
    ssh_close_socket(client_socket);
    log_error(
        "SSH: tunnel handler: Unable to open tunnel. Exception when opening "
        "tunnel: %s",
        exc.what());
  } catch (::ssh::SshException &exc) {
    ssh_close_socket(client_socket);
    log_error(
        "SSH: tunnel handler: Unable to open tunnel. Exception when opening "
        "tunnel: %s",
        exc.getError().c_str());
  }
}

}  // namespace ssh
}  // namespace mysqlshdk
