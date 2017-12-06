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

#ifndef MYSQLD_MOCK_MYSQL_PROTOCOL_DECODER_INCLUDED
#define MYSQLD_MOCK_MYSQL_PROTOCOL_DECODER_INCLUDED

#include <functional>
#include <string>
#include <vector>
#include <stdint.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
using socket_t = SOCKET;
#else
using socket_t = int;
#endif

namespace server_mock {

using byte = uint8_t;

/** @enum MySQLCommand
 *
 * Types of the supported commands from the client.
 *
 **/
enum MySQLCommand {
  QUIT = 0x01,
  QUERY = 0x03
};

/** @class MySQLProtocolDecoder
 *
 * @brief Responsible for decoding classic MySQL protocol packets
 *
 **/
class MySQLProtocolDecoder {
 public:

  /** @brief Callback used to read more data from the socket
   **/
  using read_callback = std::function<void(int, uint8_t *data, size_t size, int)>;

  /** @brief Constructor
   *
   * @param read_callback Callback to use to read more data from the socket
   **/
  MySQLProtocolDecoder(const read_callback& read_clb);

  /** @brief Single protocol packet data.
   **/
  struct protocol_packet_type {
    // packet sequence number
    uint8_t packet_seq{0};
    // raw packet data
    std::vector<byte> packet_buffer;
  };

  /** @brief Reads single packet from the network socket.
   *
   * @returns Read packet data.
   **/
  protocol_packet_type read_message(socket_t client_socket, int flags=0);

  /** @brief Retrieves command type from the packet sent by the client.
   *
   * @param packet  protocol packet to inspect
   * @returns command type
   **/
  MySQLCommand get_command_type(const protocol_packet_type& packet);

  /** @brief Retrieves SQL statement from the packet sent by the client.
   *
   * The method assumes that the packet is MySQL QUERY command.
   *
   * @param packet  packet to inspect
   * @returns SQL statement
   **/
  std::string get_statement(const protocol_packet_type& packet);

 private:
  const read_callback read_callback_;
};

} // namespace

#endif // MYSQLD_MOCK_MYSQL_PROTOCOL_DECODER_INCLUDED
