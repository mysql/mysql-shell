/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates.
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

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

namespace mysqlshdk::mysql {

class IInstance;

using Gtid = std::string;

/*
 * This represents either a single GTID or a range.
 */
struct Gtid_range {
  // either "uuid:tag" if tag is present or "uuid" otherwise. If tag is present
  // it's stored as lowercase.
  std::string uuid_tag;
  uint64_t begin{0};
  uint64_t end{0};

  Gtid_range() noexcept = default;
  explicit Gtid_range(std::string_view uuid, std::string_view tag,
                      uint64_t begin, uint64_t end);

  static Gtid_range from_gtid(std::string_view gtid);

  explicit operator bool() const noexcept {
    return !uuid_tag.empty() && (begin != 0) && (end != 0);
  }

  bool is_single() const { return (begin == end); }

  std::string str() const;
  uint64_t count() const noexcept;
};

/*
 * This represents a GTID set in a form of a single string. It can store a
 * single GTID or multiple GTIDs from different servers (different UUIDs) with
 * multiple ranges.
 */
class Gtid_set {
 public:
  Gtid_set() = default;
  explicit Gtid_set(const Gtid_range &range) { add(range); }

  static Gtid_set from_string(std::string gtid_set) noexcept {
    return Gtid_set(std::move(gtid_set), false);
  }

  static Gtid_set from_normalized_string(std::string gtid_set) noexcept {
    return Gtid_set(std::move(gtid_set), true);
  }

  static Gtid_set from_gtid_executed(const mysqlshdk::mysql::IInstance &server);
  static Gtid_set from_gtid_purged(const mysqlshdk::mysql::IInstance &server);
  static Gtid_set from_received_transaction_set(
      const mysqlshdk::mysql::IInstance &server, std::string_view channel);

  Gtid_set &normalize(const mysqlshdk::mysql::IInstance &server);

  Gtid_set &subtract(const Gtid_set &other,
                     const mysqlshdk::mysql::IInstance &server);

  Gtid_set &add(const Gtid &gtid);
  Gtid_set &add(const Gtid_set &other);
  Gtid_set &add(const Gtid_range &gtids);

  Gtid_set &intersect(const Gtid_set &other,
                      const mysqlshdk::mysql::IInstance &server);

  Gtid_set get_gtids_tagged() const;
  Gtid_set get_gtids_from(std::string_view uuid) const;
  Gtid_set get_gtids_from(std::string_view uuid, std::string_view tag) const;

  bool contains(const Gtid_set &other,
                const mysqlshdk::mysql::IInstance &server) const;

  void enumerate(const std::function<void(Gtid)> &fn) const;

  void enumerate_ranges(const std::function<void(Gtid_range)> &fn) const;

  bool has_tags() const;

  bool empty() const { return m_gtid_set.empty(); }

  uint64_t count() const;

  const std::string &str() const { return m_gtid_set; }

  bool operator==(const Gtid_set &other) const {
    if (!m_normalized || !other.m_normalized)
      throw std::invalid_argument("Can't compare un-normalized Gtid_set");
    return m_gtid_set == other.m_gtid_set;
  }

  bool operator!=(const Gtid_set &other) const { return !operator==(other); }

 private:
  std::string m_gtid_set;
  bool m_normalized{true};

  Gtid_set(std::string gtid_set, bool normalized) noexcept
      : m_gtid_set(std::move(gtid_set)), m_normalized(normalized) {}
};

}  // namespace mysqlshdk::mysql

#endif  // MYSQLSHDK_LIBS_MYSQL_GTID_UTILS_H_
