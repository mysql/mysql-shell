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

#include "mysqlshdk/libs/mysql/gtid_utils.h"

#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlshdk {
namespace mysql {

std::string to_string(const Gtid_range &range) {
  if (std::get<1>(range) == std::get<2>(range))
    return std::get<0>(range) + ":" + std::to_string(std::get<1>(range));
  else
    return std::get<0>(range) + ":" + std::to_string(std::get<1>(range)) + "-" +
           std::to_string(std::get<2>(range));
}

Gtid_set &Gtid_set::normalize(const mysqlshdk::mysql::IInstance &server) {
  if (!m_normalized) {
    m_normalized = true;
    m_gtid_set = server.queryf_one_string(0, "", "SELECT gtid_subtract(?, '')",
                                          m_gtid_set);
  }
  return *this;
}

Gtid_set &Gtid_set::subtract(const Gtid_set &other,
                             const mysqlshdk::mysql::IInstance &server) {
  m_normalized = true;
  m_gtid_set = server.queryf_one_string(0, "", "SELECT gtid_subtract(?, ?)",
                                        m_gtid_set, other.m_gtid_set);
  return *this;
}

Gtid_set &Gtid_set::add(const Gtid &gtid) {
  if (m_gtid_set.empty()) {
    m_normalized = true;
    m_gtid_set = gtid;
  } else {
    m_normalized = false;
    m_gtid_set += "," + gtid;
  }

  return *this;
}

Gtid_set &Gtid_set::add(const Gtid_set &other) {
  if (m_gtid_set.empty()) {
    m_normalized = other.m_normalized;
    m_gtid_set = other.m_gtid_set;
  } else if (other.m_gtid_set.empty()) {
  } else {
    m_normalized = false;
    m_gtid_set += "," + other.m_gtid_set;
  }

  return *this;
}

Gtid_set &Gtid_set::add(const Gtid_range &range) {
  if (m_gtid_set.empty()) {
    m_normalized = true;
    m_gtid_set = to_string(range);
  } else {
    m_normalized = false;
    m_gtid_set += "," + to_string(range);
  }

  return *this;
}

Gtid_set Gtid_set::get_gtids_from(const std::string &uuid) const {
  mysqlshdk::mysql::Gtid_set matches;

  if (!uuid.empty()) {
    enumerate_ranges(
        [&matches, uuid](const mysqlshdk::mysql::Gtid_range &range) {
          if (std::get<0>(range) == uuid) {
            matches.add(range);
          }
        });
  }

  return matches;
}

bool Gtid_set::contains(const Gtid_set &other,
                        const mysqlshdk::mysql::IInstance &server) const {
  return server.queryf_one_int(0, 0, "SELECT gtid_subtract(?, ?) = ''",
                               other.m_gtid_set, m_gtid_set) != 0;
}

uint64_t Gtid_set::count() const {
  if (!m_normalized)
    throw std::invalid_argument("Can't get count of un-normalized Gtid_set");

  uint64_t count = 0;
  enumerate_ranges([&count](const Gtid_range &range) {
    count += std::get<2>(range) - std::get<1>(range) + 1;
  });
  return count;
}

void Gtid_set::enumerate(const std::function<void(const Gtid &)> &fn) const {
  enumerate_ranges([fn](const Gtid_range &range) {
    std::string prefix = std::get<0>(range) + ":";
    for (auto i = std::get<1>(range); i <= std::get<2>(range); ++i)
      fn(prefix + std::to_string(i));
  });
}

void Gtid_set::enumerate_ranges(
    const std::function<void(const Gtid_range &)> &fn) const {
  if (!m_normalized)
    throw std::invalid_argument("Can't enumerate un-normalized Gtid_set");

  shcore::str_itersplit(
      m_gtid_set,
      [fn](const std::string &s) {
        size_t p = s.find(':');
        size_t offs = (s.front() == '\n') ? 1 : 0;
        // strip \n from previous range as in range,\nrange
        std::string prefix = s.substr(offs, p - offs);

        if (p != std::string::npos) {
          shcore::str_itersplit(
              s.substr(p + 1),
              [fn, prefix](const std::string &ss) {
                uint64_t begin, end;
                switch (sscanf(&ss[0], "%" PRIu64 "-%" PRIu64, &begin, &end)) {
                  case 2:
                    fn(std::make_tuple(Gtid(prefix), begin, end));
                    break;
                  case 1:
                    fn(std::make_tuple(Gtid(prefix), begin, begin));
                    break;
                }
                return true;
              },
              ":");
        }
        return true;
      },
      ",");
}

}  // namespace mysql
}  // namespace mysqlshdk
