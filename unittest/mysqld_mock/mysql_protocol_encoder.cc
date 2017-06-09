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

#include "mysql_protocol_encoder.h"

#include <cassert>
#include <iostream>

namespace server_mock {

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_ok_message(uint8_t seq_no,
                               uint64_t affected_rows,
                               uint64_t last_insert_id,
                               uint16_t status,
                               uint16_t warnings) {
  msg_buffer out_buffer;

  encode_msg_begin(out_buffer);

  append_byte(out_buffer, 0x0);
  append_lenenc_int(out_buffer, affected_rows);
  append_lenenc_int(out_buffer, last_insert_id);
  append_int(out_buffer, status);
  append_int(out_buffer, warnings);

  encode_msg_end(out_buffer, seq_no);

  return out_buffer;
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_error_message(uint8_t seq_no,
                                  uint16_t error_code,
                                  const std::string &sql_state,
                                  const std::string &error_msg) {
  msg_buffer out_buffer;

  encode_msg_begin(out_buffer);

  append_byte(out_buffer, 0xff);
  append_int(out_buffer, error_code);
  append_byte(out_buffer, 0x23); // "#"
  append_str(out_buffer, sql_state);
  append_str(out_buffer, error_msg);

  encode_msg_end(out_buffer, seq_no);

  return out_buffer;
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_greetings_message(uint8_t seq_no,
                                              const std::string &mysql_version,
                                              uint32_t connection_id,
                                              const std::string &nonce,
                                              MySQLCapabilities capabilities,
                                              uint8_t character_set,
                                              uint16_t status_flags) {
  msg_buffer out_buffer;

  encode_msg_begin(out_buffer);

  append_byte(out_buffer, 0x0a);
  append_str(out_buffer, mysql_version);
  append_byte(out_buffer, 0x0); // NULL
  append_int(out_buffer, connection_id);
  append_str(out_buffer, nonce.substr(0,8));
  append_byte(out_buffer, 0x0); // filler
  append_int(out_buffer, static_cast<uint16_t>(static_cast<uint32_t>(capabilities) & 0xffff)); // cap_1
  append_byte(out_buffer, character_set);
  append_int(out_buffer, status_flags);
  append_int(out_buffer, static_cast<uint16_t>(static_cast<uint32_t>(capabilities) >> 16)); // cap_2
  append_byte(out_buffer, 0x0); // auth-plugin-len = 0
  append_str(out_buffer, std::string(10, '\0')); // reserved
  append_str(out_buffer, nonce.substr(8));

  encode_msg_end(out_buffer, seq_no);

  return out_buffer;
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_columns_number_message(uint8_t seq_no, uint64_t number) {
  msg_buffer out_buffer;
  encode_msg_begin(out_buffer);

  append_lenenc_int(out_buffer, number);

  encode_msg_end(out_buffer, seq_no);
  return out_buffer;
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_column_meta_message(uint8_t seq_no,
                                                const column_info_type &column_info) {
  auto get_default_from_map = [](const column_info_type& col_info,
                                 const std::string &key,
                                 const std::string def) -> std::string {
    if (col_info.count(key) == 0) {
      return def;
    }
    return col_info.at(key);
  };

  return encode_column_meta_message(seq_no,
                                    column_type_from_string(column_info.at("type")),
                                    column_info.at("name"),
                                    get_default_from_map(column_info, "orig_name", ""),
                                    get_default_from_map(column_info, "table", ""),
                                    get_default_from_map(column_info, "orig_table", ""),
                                    get_default_from_map(column_info, "schema", ""),
                                    get_default_from_map(column_info, "catalog", "def"),
                                    static_cast<uint16_t>(std::stoul(get_default_from_map(column_info, "flags", "0"))),
                                    static_cast<uint8_t>(std::stoul(get_default_from_map(column_info, "decimals", "0"))),
                                    static_cast<uint32_t>(std::stoul(get_default_from_map(column_info, "length", "0"))),
                                    static_cast<uint16_t>(std::stoul(get_default_from_map(column_info, "character_set", "63")))
  );
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_column_meta_message(uint8_t seq_no, uint8_t type,
                                                 const std::string &name,
                                                 const std::string &orig_name,
                                                 const std::string &table,
                                                 const std::string &orig_table,
                                                 const std::string &schema,
                                                 const std::string &catalog,
                                                 uint16_t flags,
                                                 uint8_t decimals,
                                                 uint32_t length,
                                                 uint16_t character_set) {
  msg_buffer out_buffer;
  encode_msg_begin(out_buffer);

  append_lenenc_str(out_buffer, catalog);
  append_lenenc_str(out_buffer, schema);
  append_lenenc_str(out_buffer, table);
  append_lenenc_str(out_buffer, orig_table);
  append_lenenc_str(out_buffer, name);
  append_lenenc_str(out_buffer, orig_name);

  msg_buffer meta_buffer;
  append_int(meta_buffer, character_set);
  append_int(meta_buffer, length);
  append_byte(meta_buffer, type);
  append_int(meta_buffer, flags);
  append_byte(meta_buffer, decimals);
  append_int(meta_buffer, static_cast<uint16_t>(0));

  append_lenenc_int(out_buffer, meta_buffer.size());
  append_buffer(out_buffer, meta_buffer);

  encode_msg_end(out_buffer, seq_no);
  return out_buffer;
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_row_message(uint8_t seq_no,
                                         const std::vector<column_info_type> &columns_info,
                                         const row_values_type &row_values) {
  msg_buffer out_buffer;
  encode_msg_begin(out_buffer);

  if (columns_info.size() != row_values.size()) {
    throw std::runtime_error(std::string("columns_info.size() != row_values.size() ")
              + std::to_string(columns_info.size())
              + std::string("!=") +  std::to_string(row_values.size()));
  }

  for (size_t i = 0; i < row_values.size(); ++i) {
    auto field_type = static_cast<MySQLColumnType>(column_type_from_string(columns_info[i].at("type")));
    switch (field_type) {
      case MySQLColumnType::TINY:
      case MySQLColumnType::LONG:
      case MySQLColumnType::LONGLONG:
      case MySQLColumnType::STRING:
        append_lenenc_str(out_buffer, row_values[i]);
        break;
      default:
        throw std::runtime_error("Unsupported MySQL field type "
                                 + std::to_string(static_cast<int>(field_type)));
    }
  }

  encode_msg_end(out_buffer, seq_no);
  return out_buffer;
}

MySQLProtocolEncoder::msg_buffer
MySQLProtocolEncoder::encode_eof_message(uint8_t seq_no, uint16_t status,
                                         uint16_t warnings) {
  msg_buffer out_buffer;
  encode_msg_begin(out_buffer);

  append_byte(out_buffer, 0xfe);  // ok
  append_int(out_buffer, status);
  append_int(out_buffer, warnings);

  encode_msg_end(out_buffer, seq_no);
  return out_buffer;
}

void MySQLProtocolEncoder::encode_msg_begin(msg_buffer &out_buffer) {
  // reserve space for header
  append_int(out_buffer, static_cast<uint32_t>(0x0));
}

void MySQLProtocolEncoder::encode_msg_end(msg_buffer &out_buffer, uint8_t seq_no) {
  assert(out_buffer.size() >= 4);
  // fill the header
  uint32_t msg_len = static_cast<uint32_t>(out_buffer.size()) - 4;
  if (msg_len > 0xffffff) {
    throw std::runtime_error("Invalid message length: " + std::to_string(msg_len));
  }
  uint32_t header = msg_len | static_cast<uint32_t>(seq_no << 24);

  auto len = sizeof(header);
  for (size_t i = 0; len > 0; ++i, --len) {
    out_buffer[i] = static_cast<byte>(header);
    header = static_cast<decltype(header)>(header >> 8);
  }
}

void MySQLProtocolEncoder::append_byte(msg_buffer& buffer, byte value) {
  buffer.push_back(value);
}

void MySQLProtocolEncoder::append_str(msg_buffer &buffer, const std::string &value) {
  buffer.insert(buffer.end(), value.begin(), value.end());
}

void MySQLProtocolEncoder::append_buffer(msg_buffer &buffer, const msg_buffer &value) {
  buffer.insert(buffer.end(), value.begin(), value.end());
}

void MySQLProtocolEncoder::append_lenenc_int(msg_buffer &buffer, uint64_t val) {
  if (val < 251) {
    append_byte(buffer, static_cast<byte>(val));
  }
  else if (val < (1 << 16)) {
    append_byte(buffer, 0xfc);
    append_int(buffer, static_cast<uint16_t>(val));
  }
  else {
    append_byte(buffer, 0xfe);
    append_int(buffer, val);
  }
}

void MySQLProtocolEncoder::append_lenenc_str(msg_buffer &buffer, const std::string &value) {
  append_lenenc_int(buffer, value.length());
  append_str(buffer, value);
}

uint8_t MySQLProtocolEncoder::column_type_from_string(const std::string& type) {
  int res = 0;

  try {
    res =  std::stoi(type);
  }
  catch (const std::invalid_argument&) {
    if (type == "TINY") return static_cast<uint8_t>(MySQLColumnType::TINY);
    if (type == "LONG") return static_cast<uint8_t>(MySQLColumnType::LONG);
    if (type == "LONGLONG") return static_cast<uint8_t>(MySQLColumnType::LONGLONG);
    if (type == "STRING") return static_cast<uint8_t>(MySQLColumnType::STRING);

    throw std::invalid_argument("Unknown type: \"" + type + "\"");
  }

  return static_cast<uint8_t>(res);
}

} // namespace
