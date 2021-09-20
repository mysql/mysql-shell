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

#ifndef MYSQLSHDK_LIBS_MYSQL_GTID_UTILS_H_
#define MYSQLSHDK_LIBS_MYSQL_GTID_UTILS_H_

#include <functional>
#include <string>
#include <tuple>

#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

using Gtid = std::string;
using Gtid_range = std::tuple<std::string, uint64_t, uint64_t>;

std::string to_string(const Gtid_range &range);

class Gtid_set {
 public:
  Gtid_set() : m_normalized(true) {}

  explicit Gtid_set(const Gtid_range &range) : m_normalized(true) {
    add(range);
  }

  static Gtid_set from_string(const std::string &gtid_set) {
    return Gtid_set(gtid_set, false);
  }

  static Gtid_set from_normalized_string(const std::string &gtid_set) {
    return Gtid_set(gtid_set, true);
  }

  static Gtid_set from_gtid_executed(
      const mysqlshdk::mysql::IInstance &server) {
    return Gtid_set(server.queryf_one_string(0, "", "select @@gtid_executed"),
                    true);
  }

  static Gtid_set from_gtid_purged(const mysqlshdk::mysql::IInstance &server) {
    return Gtid_set(server.queryf_one_string(0, "", "select @@gtid_purged"),
                    true);
  }

  static Gtid_set from_received_transaction_set(
      const mysqlshdk::mysql::IInstance &server, const std::string &channel) {
    return Gtid_set(server.queryf_one_string(
                        0, "",
                        "select received_transaction_set"
                        " from performance_schema.replication_connection_status"
                        " where channel_name=?",
                        channel),
                    true);
  }

  Gtid_set &normalize(const mysqlshdk::mysql::IInstance &server);

  Gtid_set &subtract(const Gtid_set &other,
                     const mysqlshdk::mysql::IInstance &server);
  Gtid_set &add(const Gtid &gtid);
  Gtid_set &add(const Gtid_set &other);
  Gtid_set &add(const Gtid_range &gtids);

  Gtid_set get_gtids_from(const std::string &uuid) const;

  bool contains(const Gtid_set &other,
                const mysqlshdk::mysql::IInstance &server) const;

  void enumerate(const std::function<void(const Gtid &)> &fn) const;

  void enumerate_ranges(
      const std::function<void(const Gtid_range &)> &fn) const;

  bool empty() const { return m_gtid_set.empty(); }

  uint64_t count() const;

  operator std::string() const { return m_gtid_set; }

  inline const std::string &str() const { return m_gtid_set; }

  bool operator==(const Gtid_set &other) const {
    if (!m_normalized || !other.m_normalized)
      throw std::invalid_argument("Can't compare un-normalized Gtid_set");
    return m_gtid_set == other.m_gtid_set;
  }

  bool operator!=(const Gtid_set &other) const {
    if (!m_normalized || !other.m_normalized)
      throw std::invalid_argument("Can't compare un-normalized Gtid_set");
    return m_gtid_set != other.m_gtid_set;
  }

 private:
  std::string m_gtid_set;
  bool m_normalized;

  Gtid_set(const std::string &gtid_set, bool normalized)
      : m_gtid_set(gtid_set), m_normalized(normalized) {}
};

// TODO(alfredo) move pure gtid related functions from replication.h

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_GTID_UTILS_H_
