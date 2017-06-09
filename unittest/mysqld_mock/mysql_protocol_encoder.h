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

#ifndef MYSQLD_MOCK_MYSQL_PROTOCOL_ENCODER_INCLUDED
#define MYSQLD_MOCK_MYSQL_PROTOCOL_ENCODER_INCLUDED

#include <string>
#include <vector>
#include <stdint.h>

#include "json_statement_reader.h"

namespace server_mock {

using byte = uint8_t;

/** @enum MySQLColumnType
 *
 * Supported MySQL Coumn types.
 *
 **/
enum class MySQLColumnType: uint8_t {
  TINY =  0x01,
  LONG = 0x03,
  LONGLONG = 0x08,
  STRING = 0xfe
};

const uint16_t MYSQL_PARSE_ERROR = 1064;

class MySQLProtocolEncoder {
public:
  using column_info_type = QueriesJsonReader::column_info_type;
  using row_values_type = QueriesJsonReader::row_values_type;

  /** @enum MySQLCapabilities
   *
   * Values for MySQL capabilities bitmask.
   *
   **/
  enum class MySQLCapabilities {
    LONG_PASSWORD = 1 << 0,
    FOUND_ROWS = 1 << 1,
    LONG_FLAG = 1 << 2,
    CONNECT_WITH_DB = 1 << 3,

    NO_SCHEMA = 1 << 4,
    COMPRESS = 1 << 5,
    ODBC = 1 << 6,
    LOCAL_FILE = 1 << 7,

    IGNORE_SPACE = 1 << 8,
    PROTOCOL_41 = 1 << 9,
    INTERACTIVE = 1 << 10,
    SSL = 1 << 11,

    SIG_PIPE = 1 << 12,
    TRANSACTIONS = 1 << 13,
    RESERVED_14 = 1 << 14,
    SECURE_CONNECTION = 1 << 15,

    MULTI_STATEMENTS = 1 << 16,
    MULTI_RESULTS = 1 << 17,
    MULTI_PS_MULTO_RESULTS = 1 << 18,
    PLUGIN_AUTH = 1 << 19,

    CONNECT_ATTRS = 1 << 20,
    PLUGIN_AUTH_LENENC_CLIENT_DATA = 1 << 21,
    EXPIRED_PASSWORDS = 1 << 22,
    SESSION_TRACK = 1 << 23,

    WONKY_EOF = 1 << 24,
  };

  using msg_buffer = std::vector<byte>;

  /** @brief Encodes MySQL OK message
   *
   * @param seq_no          protocol packet sequence number to use
   * @param affected_rows   number of the rows affected by the statment
   *                        this OK replies to
   * @param last_insert_id  id of the last row inserted by the statement
   *                        this OK replies to (if any)
   * @param status          status of the statement this OK replies to
   * @param warnings        number of the warning for the statement this OK replies to
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_ok_message(uint8_t seq_no,
                               uint64_t affected_rows = 0,
                               uint64_t last_insert_id = 0,
                               uint16_t status = 0,
                               uint16_t warnings = 0);

  /** @brief Encodes MySQL error message
   *
   * @param seq_no      protocol packet sequence number to use
   * @param error code  code of the reported error
   * @param sql_state   SQL state to report
   * @param error_msg   error message
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_error_message(uint8_t seq_no,
                                  uint16_t error_code,
                                  const std::string &sql_state,
                                  const std::string &error_msg);

  /** @brief Encodes MySQL greetings message sent from the server when
   *         the client connects.
   *
   * @param seq_no          protocol packet sequence number to use
   * @param mysql_version   MySQL server version string
   * @param connection_id   is of the client connection
   * @param nonce           authentication plugin data
   * @param capabilities    bitmask with MySQL Server capabilities
   * @param character_set   id of the connection character set
   * @param status_flags    bitmask with MySQL Server status flags
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_greetings_message(uint8_t seq_no,
                                      const std::string &mysql_version = "mysql-foo",
                                      uint32_t connection_id = 1,
                                      const std::string &nonce = "012345678901234567890",
                                      MySQLCapabilities capabilities = MySQLCapabilities::PROTOCOL_41,
                                      uint8_t character_set = 0,
                                      uint16_t status_flags = 0);

  /** @brief Encodes message containing number of the columns
   *        (used while sending resultset for the QUERY).
   *
   * @param seq_no  protocol packet sequence number to use
   * @param number  number of the columns to encode
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_columns_number_message(uint8_t seq_no, uint64_t number);


  /** @brief Encodes message containing single column metadata.
   *
   * @param seq_no  protocol packet sequence number to use
   * @param number  map containing parameters names and values pairs for the column
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_column_meta_message(uint8_t seq_no,
                                        const column_info_type &column_info);

  /** @brief Encodes message containing single row in the resultset.
   *
   * @param seq_no        protocol packet sequence number to use
   * @param columns_info  vector with column metadata for consecutive row fields
   * @param row_values    vector with values (as string) for the consecutive row fields
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_row_message(uint8_t seq_no,
                                const std::vector<column_info_type> &columns_info,
                                const row_values_type &row_values);

  /** @brief Encodes EOF message used to mark the end of columns metadata and rows
   *         when sending the resultset to the client.
   *
   * @param seq_no    protocol packet sequence number to use
   * @param status    status mask for the ongoing operation
   * @param warnings  number of the warnings for ongoing operation
   *
   * @returns buffer with the encoded message
   **/
  msg_buffer encode_eof_message(uint8_t seq_no,
                                uint16_t status = 0,
                                uint16_t warnings = 0);

 protected:
  void encode_msg_begin(msg_buffer &out_buffer);
  void encode_msg_end(msg_buffer &out_buffer, uint8_t seq_no);
  void append_byte(msg_buffer& buffer, byte value);

  template<class T, typename = std::enable_if<std::is_integral<T>::value>>
  void append_int(msg_buffer& buffer, T value, size_t len = sizeof(T)) {
    buffer.reserve(buffer.size() + len);
    while(len-- > 0) {
      byte b = static_cast<byte>(value);
      buffer.push_back(b);
      value = static_cast<T>(value >> 8);
    }
  }

  void append_str(msg_buffer &buffer, const std::string &value);
  void append_buffer(msg_buffer &buffer, const msg_buffer &value);
  void append_lenenc_int(msg_buffer &buffer, uint64_t val);
  void append_lenenc_str(msg_buffer &buffer, const std::string &value);
  uint8_t column_type_from_string(const std::string& type);

  msg_buffer encode_column_meta_message(uint8_t seq_no, uint8_t type,
                                    const std::string &name = "",
                                    const std::string &orig_name = "",
                                    const std::string &table = "",
                                    const std::string &orig_table = "",
                                    const std::string &schema = "",
                                    const std::string &catalog = "def",
                                    uint16_t flags = 0,
                                    uint8_t decimals = 0,
                                    uint32_t length = 0,
                                    uint16_t character_set = 0x3f);
};

} // namespace

#endif // MYSQLD_MOCK_MYSQL_PROTOCOL_ENCODER_INCLUDED
