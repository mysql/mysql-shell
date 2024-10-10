/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_MYSQL_BINARY_LOG_H_
#define MYSQLSHDK_LIBS_DB_MYSQL_BINARY_LOG_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct MYSQL;

namespace mysqlshdk {
namespace db {
namespace mysql {

struct Binary_log_event {
  // actual header is longer, but we read just these bytes
  static constexpr std::size_t k_header_length = 17;

  const unsigned char *buffer;

  uint32_t timestamp;  // seconds, corresponds to timeval::tv_sec
  uint8_t type;
  uint32_t size;
  uint64_t next_position;  // position of the next event
  uint64_t position;       // position of this event
};

class Binary_log final {
 public:
  static constexpr auto k_binlog_magic = "\xfe\x62\x69\x6e";
  static constexpr std::size_t k_binlog_magic_size = 4;

  /**
   * Takes ownership of the MYSQL structure.
   */
  Binary_log(MYSQL *mysql, std::string_view file_name, uint64_t position);

  Binary_log(const Binary_log &) = delete;
  Binary_log(Binary_log &&);

  Binary_log &operator=(const Binary_log &) = delete;
  Binary_log &operator=(Binary_log &&);

  ~Binary_log();

  /**
   * Parses the buffer, extracting the binary log even information.
   *
   * Buffer needs to be at least Binary_log_event::k_header_length long.
   */
  static Binary_log_event parse_event(const unsigned char *buffer);

  static inline Binary_log_event parse_event(const char *buffer) {
    return parse_event(reinterpret_cast<const unsigned char *>(buffer));
  }

  /**
   * Fetches next binary log event.
   *
   * The buffer pointer is valid till the next call to this method.
   * If there are no more events, all fields are set to zero.
   *
   * @throws std::runtime_error If fetching the event fails.
   */
  Binary_log_event next_event();

  /**
   * Name of the binary log file.
   */
  const std::string &name() const;

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_MYSQL_BINARY_LOG_H_
