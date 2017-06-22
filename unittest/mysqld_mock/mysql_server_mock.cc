/*
  Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "mysql_server_mock.h"

#include <cstring>
#include <functional>
#include <iostream>

#ifndef _WIN32
#  include <netdb.h>
#  include <netinet/in.h>
#  include <fcntl.h>
#  include <sys/un.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#include <regex.h>
#else
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
typedef long ssize_t;
#include <regex>
#endif

using namespace std::placeholders;

namespace {

int get_socket_errno() {
#ifndef _WIN32
  return errno;
#else
  return WSAGetLastError();
#endif
}

std::string get_socket_errno_str() {
  return std::to_string(get_socket_errno());
}

void send_packet(socket_t client_socket, const uint8_t *data, size_t size, int flags = 0) {
  ssize_t sent = 0;
  size_t buffer_offset = 0;
  while (buffer_offset < size) {
    if ((sent = send(client_socket, reinterpret_cast<const char*>(data) + buffer_offset,
                     size-buffer_offset, flags)) < 0) {
      throw std::runtime_error("Error calling send(); errno= "
                               + get_socket_errno_str());
    }
    buffer_offset += static_cast<size_t>(sent);
  }
}

void send_packet(socket_t client_socket,
                 const server_mock::MySQLProtocolEncoder::msg_buffer &buffer,
                 int flags = 0) {
  send_packet(client_socket, buffer.data(), buffer.size(), flags);
}

void read_packet(socket_t client_socket, uint8_t *data, size_t size, int flags = 0) {
  ssize_t received = 0;
  size_t buffer_offset = 0;
  while (buffer_offset < size) {
    received = recv(client_socket, reinterpret_cast<char*>(data)+buffer_offset,
                    size-buffer_offset, flags);
    if (received <= 0) {
      throw(std::runtime_error("error calling recv (ret=" + std::to_string(received)  + ") errno: "
                             + get_socket_errno_str()));
    }
    buffer_offset += static_cast<size_t>(received);
  }
}

int close_socket(socket_t sock) {
#ifndef _WIN32
  return close(sock);
#else
  return closesocket(sock);
#endif
}

bool pattern_matching(const std::string &s,
                      const std::string &pattern) {
#ifndef _WIN32
  regex_t regex;
  auto r = regcomp(&regex, pattern.c_str(), 0);
  if (r) {
    throw std::runtime_error("Error compiling regex pattern: " + pattern);
  }
  r = regexec(&regex, s.c_str(), 0, NULL, 0);
  regfree(&regex);
  return (r == 0);
#else
  std::regex regex(pattern);
  return std::regex_match(s, regex);
#endif
}

}

namespace server_mock {
int MySQLServerMock::kListenQueueSize = 5;

MySQLServerMock::MySQLServerMock(const std::string &expected_queries_file,
                                 unsigned bind_port):
  bind_port_(bind_port),
  json_reader_(expected_queries_file),
  protocol_decoder_(&read_packet) {
}

void MySQLServerMock::run() {
  setup_service();
  handle_connections();
  close(listener_);
}

void MySQLServerMock::setup_service() {
  int err;
  struct addrinfo hints, *ainfo;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  err = getaddrinfo(nullptr, std::to_string(bind_port_).c_str(), &hints, &ainfo);
  if (err != 0) {
    throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(err));
  }

  listener_ = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
  if (listener_ < 0) {
    freeaddrinfo(ainfo);
    throw std::runtime_error("socket() failed: " + get_socket_errno_str());
  }

  int option_value = 1;
  if (setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&option_value),
        static_cast<socklen_t>(sizeof(int))) == -1) {
    freeaddrinfo(ainfo);
    throw std::runtime_error("socket() failed: " + get_socket_errno_str());
  }

  err = bind(listener_, ainfo->ai_addr, ainfo->ai_addrlen);
  if (err < 0) {
    freeaddrinfo(ainfo);
    throw std::runtime_error("bind() failed: " + get_socket_errno_str());
  }

  err = listen(listener_, kListenQueueSize);
  if (err < 0) {
    freeaddrinfo(ainfo);
    throw std::runtime_error("listen() failed: " + get_socket_errno_str());
  }

  freeaddrinfo(ainfo);
}

void MySQLServerMock::handle_connections() {
  socket_t client_socket;
  struct sockaddr_storage client_addr;
  socklen_t addr_size = sizeof(client_addr);

  std::cout << "Starting to handle connections" << std::endl;

  while(true) {
    client_socket = accept(listener_, (struct sockaddr*)&client_addr, &addr_size);
    if (client_socket < 0) {
      throw std::runtime_error("accept() failed: " + get_socket_errno_str());
    }

    std::cout << "Accepted client " << client_socket << std::endl;

    // int flags = fcntl(client_socket, F_GETFL, 0);
    // flags = flags & (~O_NONBLOCK);
    // fcntl(client_socket, F_SETFL, flags);

    try {
      auto buf = protocol_encoder_.encode_greetings_message(0);
      send_packet(client_socket, buf);

      auto packet = protocol_decoder_.read_message(client_socket);
      auto packet_seq =  static_cast<uint8_t>(packet.packet_seq + 1);
      send_ok(client_socket, packet_seq);
      bool res = process_statements(client_socket);
      if (!res) {
        std::cout << "Leaving accept loop" << std::endl;
        break;
      }
    }
    catch (const std::exception &e) {
      std::cerr << "Exception caught in connection loop: "
                <<  e.what() << std::endl;
      close_socket(client_socket);
    }
  }
  close_socket(client_socket);
}

bool MySQLServerMock::process_statements(socket_t client_socket) {

  while (true) {
    auto packet = protocol_decoder_.read_message(client_socket);
    auto cmd = protocol_decoder_.get_command_type(packet);
    switch (cmd) {
    case MySQLCommand::QUERY: {
      std::string statement_received = protocol_decoder_.get_statement(packet);
      const auto &next_statement = json_reader_.get_next_statement();

      bool statement_matching{false};
      if (!next_statement.statement_is_regex) { // not regex
        statement_matching = (statement_received == next_statement.statement);
      } else { // regex
        statement_matching = pattern_matching(statement_received,
                                              next_statement.statement);
      }

      if (!statement_matching) {
        auto packet_seq = static_cast<uint8_t>(packet.packet_seq + 1);
        send_error(client_socket, packet_seq, MYSQL_PARSE_ERROR,
            std::string("Unexpected stmt, got: \"") + statement_received +
            "\"; expected: \"" + next_statement.statement + "\"");
      } else {
        handle_statement(client_socket, packet.packet_seq, next_statement);
      }
    }
    break;
    case MySQLCommand::QUIT:
      std::cout << "received QUIT command from the client" << std::endl;
      return false;
    default:
      std::cerr << "received unsupported command from the client: "
                << static_cast<int>(cmd) << "\n";
      auto packet_seq = static_cast<uint8_t>(packet.packet_seq + 1);
      send_error(client_socket, packet_seq, 1064, "Unsupported command: " + std::to_string(cmd));
    }
  }

  return true;
}

void MySQLServerMock::handle_statement(socket_t client_socket, uint8_t seq_no,
                    const QueriesJsonReader::statement_info& statement) {
  using statement_result_type = QueriesJsonReader::statement_result_type;

  switch (statement.result_type) {
  case statement_result_type::STMT_RES_OK:
    send_ok(client_socket, static_cast<uint8_t>(seq_no+1));
  break;
  case statement_result_type::STMT_RES_RESULT: {
    const auto& resultset = statement.resultset;
    seq_no = static_cast<uint8_t>(seq_no + 1);
    auto buf = protocol_encoder_.encode_columns_number_message(seq_no++, resultset.columns.size());
    send_packet(client_socket, buf);
    for (const auto& column: resultset.columns) {
      auto col_buf = protocol_encoder_.encode_column_meta_message(seq_no++, column);
      send_packet(client_socket, col_buf);
    }
    buf = protocol_encoder_.encode_eof_message(seq_no++);
    send_packet(client_socket, buf);

    for (size_t i = 0; i < resultset.rows.size(); ++i) {
      auto res_buf = protocol_encoder_.encode_row_message(seq_no++, resultset.columns, resultset.rows[i]);
      send_packet(client_socket, res_buf);
    }
    buf = protocol_encoder_.encode_eof_message(seq_no++);
    send_packet(client_socket, buf);
  }
  break;
  case statement_result_type::STMT_RES_ERROR:
    // TODO: handle if needed
  default:;
    throw std::runtime_error("Unsupported command in handle_statement(): " +
      std::to_string((int)statement.result_type));
  }
}

void MySQLServerMock::send_error(socket_t client_socket, uint8_t seq_no,
                                 uint16_t error_code,
                                 const std::string &error_msg,
                                 const std::string &sql_state) {
  auto buf = protocol_encoder_.encode_error_message(seq_no, error_code,
                                                    sql_state, error_msg);
  send_packet(client_socket, buf);
}

void MySQLServerMock::send_ok(socket_t client_socket, uint8_t seq_no) {
  // TODO: for now hardcoding affected_rows and last_insert_id to 1
  // as this can be verified (MySQLRouter does that sometimes) by the
  // client when it does the INSERT statement
  // need to add a way to enable defining that in json file
  auto buf = protocol_encoder_.encode_ok_message(seq_no, 1, 1);
  send_packet(client_socket, buf);
}

} // namespace
