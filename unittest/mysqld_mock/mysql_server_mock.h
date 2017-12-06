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

#ifndef MYSQLD_MOCK_MYSQL_SERVER_MOCK_INCLUDED
#define MYSQLD_MOCK_MYSQL_SERVER_MOCK_INCLUDED

#include "json_statement_reader.h"
#include "mysql_protocol_decoder.h"
#include "mysql_protocol_encoder.h"

namespace server_mock {

/** @class MySQLServerMock
 *
 * @brief Main class. Resposible for accepting and handling client's connections.
 *
 **/
class MySQLServerMock {
 public:

  /** @brief Constructor.
   *
   * @param expected_queries_file Path to the json file with definitins
   *                        of the expected SQL statements and responses
   * @param bind_port Number of the port on which the server accepts clients
   *                        connections
   */
  MySQLServerMock(const std::string &expected_queries_file,
                  unsigned bind_port);

  /** @brief Starts handling the clients connections in infinite loop.
   *         Will return only in case of an exception (error).
   */
  void run();

 private:
  void setup_service();

  void handle_connections();

  bool process_statements(socket_t client_socket);

  void handle_statement(socket_t client_socket, uint8_t seq_no,
                        const QueriesJsonReader::statement_info& statement);

  void send_error(socket_t client_socket, uint8_t seq_no,
                  uint16_t error_code,
                  const std::string &error_msg,
                  const std::string &sql_state = "HY000");

  void send_ok(socket_t client_socket, uint8_t seq_no);

  static int kListenQueueSize;
  unsigned bind_port_;
  socket_t listener_;
  QueriesJsonReader json_reader_;
  MySQLProtocolEncoder protocol_encoder_;
  MySQLProtocolDecoder protocol_decoder_;
};

} // namespace

#endif // MYSQLD_MOCK_MYSQL_SERVER_MOCK_INCLUDED
