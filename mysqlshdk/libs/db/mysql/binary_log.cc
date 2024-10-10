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

#include "mysqlshdk/libs/db/mysql/binary_log.h"

#include <mysql.h>
#include <sql/rpl_constants.h>

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>

#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace db {
namespace mysql {

namespace {

FI_DEFINE(binlog, [](const mysqlshdk::utils::FI::Args &args) {
  throw std::runtime_error{args.get_string("msg")};
});

#define EVENT_TIME_OFFSET 0
// copied from libs/mysql/binlog/event/binlog_event.h
#define EVENT_TYPE_OFFSET 4
#define EVENT_LEN_OFFSET 9
#define LOG_POS_OFFSET 13

constexpr uint8_t k_rotate_event_type = 4;

#if IS_BIG_ENDIAN
inline uint32_t uint4korr(const unsigned char *ptr) {
  return static_cast<uint32_t>(A[0]) | (static_cast<uint32_t>(A[1]) << 8) |
         (static_cast<uint32_t>(A[2]) << 16) |
         (static_cast<uint32_t>(A[3]) << 24);
}
#else
inline uint32_t uint4korr(const unsigned char *ptr) {
  uint32_t ret;
  memcpy(&ret, ptr, sizeof(ret));
  return ret;
}
#endif

inline Binary_log_event parse_buffer(const unsigned char *buffer) {
  Binary_log_event event;

  event.buffer = buffer;
  event.timestamp = uint4korr(buffer + EVENT_TIME_OFFSET);
  event.type = static_cast<uint8_t>(buffer[EVENT_TYPE_OFFSET]);
  event.size = uint4korr(buffer + EVENT_LEN_OFFSET);
  // NOTE: next position is stored on 4 bytes, maximum binlog size is 1GB
  // (though files can be bigger than that, if i.e. a large transaction
  // happens), but Log_event_header::log_pos is a 8 byte unsigned integer -
  // only a heartbeat event v2 is capable of transmitting such value, we may
  // need to track those to handle positions greater than 4GB
  event.next_position = uint4korr(buffer + LOG_POS_OFFSET);
  event.position = event.next_position - event.size;

  return event;
}

}  // namespace

class Binary_log::Impl {
 public:
  Impl(MYSQL *mysql, std::string_view file_name, uint64_t position)
      : m_mysql(mysql), m_file_name(file_name) {
    assert(mysql);

    memset(&m_rpl, 0, sizeof(m_rpl));

    m_rpl.file_name_length = m_file_name.length();
    m_rpl.file_name = m_file_name.data();
    m_rpl.start_position = position;
    // MYSQL_RPL::server_id - not set
    m_rpl.flags =
        BINLOG_DUMP_NON_BLOCK | MYSQL_RPL_SKIP_HEARTBEAT | MYSQL_RPL_GTID;
    // we set MYSQL_RPL_GTID to be able to use a 64bit position, but we don't
    // actually use the GTID-related fields of MYSQL_RPL: gtid_set_encoded_size,
    // fix_gtid_set, gtid_set_arg

    try {
      setup();

      if (mysql_binlog_open(m_mysql, &m_rpl)) {
        throw_error("unable to open the binlog");
      }
    } catch (...) {
      mysql_close(m_mysql);
      m_mysql = nullptr;

      throw;
    }
  }

  Impl(const Impl &) = delete;
  Impl(Impl &&) = delete;

  Impl &operator=(const Impl &) = delete;
  Impl &operator=(Impl &&) = delete;

  ~Impl() {
    if (m_mysql) {
      // the connection is automatically killed, no need to call mysql_close()
      mysql_binlog_close(m_mysql, &m_rpl);
    }
  }

  Binary_log_event next_event() {
    while (true) {
      if (mysql_binlog_fetch(m_mysql, &m_rpl)) {
        throw_error("failed to fetch a binary log file event");
      }

      Binary_log_event event;

      if (0 == m_rpl.size) {
        // EOF
        memset(&event, 0, sizeof(event));
        return event;
      }

      event = parse_buffer(m_rpl.buffer + 1);

      if (m_rpl.size - 1 != event.size) [[unlikely]] {
        throw_error("the binary log file event has invalid size");
      }

      if (k_rotate_event_type == event.type && 0 == event.timestamp)
          [[unlikely]] {
        // fake rotate event
        continue;
      }

      FI_TRIGGER_TRAP(binlog,
                      mysqlshdk::utils::FI::Trigger_options(
                          {{"file", m_file_name},
                           {"position", std::to_string(event.position)}}));

      if (event.position < m_current_position) [[unlikely]] {
        // server sent an event from the next binlog file
        memset(&event, 0, sizeof(event));
        return event;
      }

      m_current_position = event.next_position;

      return event;
    }
  }

  const std::string &name() const noexcept { return m_file_name; }

 private:
  void setup() {
    // check_master_version() in mysqlbinlog.cc

    // TODO(pawel): if we won't decode the events, then checking the server
    // version is not really needed, unless we want to make sure a supported
    // version is used
    MYSQL_RES *res = nullptr;

    if (mysql_query(m_mysql, "SELECT VERSION()") ||
        !(res = mysql_store_result(m_mysql))) {
      throw_error("could not find server version - query failed");
    }

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> deleter{
        res, &mysql_free_result};

    MYSQL_ROW row;

    if (!(row = mysql_fetch_row(res))) {
      throw_error("could not find server version - server returned no data");
    }

    const char *version;

    if (!(version = row[0])) {
      throw_error("could not find server version - server returned NULL");
    }

    switch (*version) {
      case '5':
      case '8':
      case '9':
        break;

      default:
        throw_error(
            shcore::str_format("could not find server version - server "
                               "reported unrecognized MySQL version '%s'",
                               version)
                .c_str());
        break;
    }

    if (mysql_query(m_mysql,
                    "SET @master_binlog_checksum = 'NONE', "
                    "@source_binlog_checksum = 'NONE'")) {
      throw_error("could not notify server about checksum awareness");
    }
  }

  [[noreturn]] void throw_error(const char *msg) const {
    assert(m_mysql);

    const auto error = mysql_error(m_mysql);

    throw std::runtime_error{shcore::str_format(
        "While reading the '%s' binary log file: %s%s%s", m_file_name.c_str(),
        msg, *error ? ": " : "", *error ? error : "")};
  }

  MYSQL *m_mysql;
  std::string m_file_name;
  // this variable is not set in the constructor, but always initialized to 0;
  // this allows us to accept a single fake format description event which is
  // sent by the server when reading does not start from the beginning
  uint64_t m_current_position = 0;
  MYSQL_RPL m_rpl;
};

Binary_log::Binary_log(MYSQL *mysql, std::string_view file_name,
                       uint64_t position)
    : m_impl(std::make_unique<Impl>(mysql, file_name, position)) {}

Binary_log::Binary_log(Binary_log &&) = default;

Binary_log &Binary_log::operator=(Binary_log &&) = default;

Binary_log::~Binary_log() = default;

Binary_log_event Binary_log::parse_event(const unsigned char *buffer) {
  return parse_buffer(buffer);
}

Binary_log_event Binary_log::next_event() { return m_impl->next_event(); }

const std::string &Binary_log::name() const { return m_impl->name(); }

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
