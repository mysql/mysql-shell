/*
  Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms, as
  designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.
  This program is distributed in the hope that it will be useful,  but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
  the GNU General Public License, version 2.0, for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "mysql_protocol_decoder.h"

#ifndef _WIN32
#  include <netdb.h>
#  include <netinet/in.h>
#  include <fcntl.h>
#  include <sys/un.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#else
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

#include <iostream>

namespace server_mock {

MySQLProtocolDecoder::MySQLProtocolDecoder(const read_callback& read_clb):
  read_callback_(read_clb)
{}

MySQLProtocolDecoder::protocol_packet_type
MySQLProtocolDecoder::read_message(socket_t client_socket, int flags) {
  protocol_packet_type result;
  uint8_t header_buf[4];
  uint32_t header{0};

  read_callback_(client_socket, &header_buf[0], sizeof(header_buf), flags);

  for (size_t i = 1; i <= 4; ++i) {
    header <<= 8;
    header |= header_buf[4-i];
  }

  uint32_t pkt_len = header & 0xffffff;

  if (pkt_len == 0xffffff) {
    // this means more data comming, which we don't need/support atm
    throw std::runtime_error("Protocol messages split into several packets not supported!");
  }

  result.packet_seq = static_cast<uint8_t>(header >> 24);

  if (pkt_len > 0) {
    result.packet_buffer.resize(pkt_len);
    read_callback_(client_socket, &result.packet_buffer[0], pkt_len, flags);
  }

  return result;
}

MySQLCommand MySQLProtocolDecoder::get_command_type(const protocol_packet_type& packet) {
  return static_cast<MySQLCommand>(packet.packet_buffer[0]);
}

std::string MySQLProtocolDecoder::get_statement(const protocol_packet_type& packet) {
  size_t buf_len = packet.packet_buffer.size() - 1;
  if (buf_len == 0) return "";

  std::vector<char> statement(buf_len+1);
  const char* buf = reinterpret_cast<const char*>(&packet.packet_buffer[1]);
  std::copy(buf, buf+buf_len, &statement[0]);
  statement[buf_len] = '\0';

  return std::string(statement.data());
}


} // namespace
