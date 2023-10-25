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

#include "mysqlshdk/libs/mysql/gtid_utils.h"

#include <algorithm>
#include <optional>

#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk::mysql {

namespace {
bool read_range(std::string_view range, uint64_t *range_begin,
                uint64_t *range_end) {
  assert(range_begin && range_end);

  range = shcore::str_strip_view(range);

  const auto p = range.find('-');
  if (p == std::string_view::npos) {
    try {
      *range_begin = *range_end = shcore::lexical_cast<uint64_t>(range);
      return true;
    } catch (const std::invalid_argument &) {
      log_debug("Unable to parse GTID range: %.*s",
                static_cast<int>(range.size()), range.data());
      return false;
    }
  }

  try {
    *range_begin = shcore::lexical_cast<uint64_t>(range.substr(0, p));
    *range_end = shcore::lexical_cast<uint64_t>(range.substr(p + 1));
    return true;
  } catch (const std::invalid_argument &) {
    log_debug("Unable to parse GTID range: %.*s",
              static_cast<int>(range.size()), range.data());
    return false;
  }
}

bool is_gtid_tag(std::string_view tag) {
  if (tag.empty()) return false;

  // tag specification taken from WL15294

  // tag consist of up to 32 characters (<=32)
  if (tag.size() > 32) return false;

  // tag accepts letters with ASCII codes between 'a'-'z' and 'A'-'Z', numbers
  // (0-9), and the underscore character
  if (!std::all_of(tag.begin(), tag.end(), [](char token) {
        return ((token >= 'a') && (token <= 'z')) ||
               ((token >= 'A') && (token <= 'Z')) ||
               ((token >= '0') && (token <= '9')) || (token == '_');
      }))
    return false;

  // tag must start with a letter or underscore
  if (auto first_char = tag.front(); (first_char >= '0') && (first_char <= '9'))
    return false;

  // it's a valid tag
  return true;
}

void iter_tag_ranges(std::string_view ranges, auto &&cb) {
  std::string_view cur_tag;
  shcore::str_itersplit(
      ranges,
      [&cb, &cur_tag](std::string_view range) {
        if (range.empty()) return true;

        // if it's a tag, check if it's immediately after the UUID and specified
        // only once, then store it and move on
        if (is_gtid_tag(range)) {
          cur_tag = range;
          return true;
        }

        // it should be a numeric range
        uint64_t begin, end;
        if (read_range(range, &begin, &end)) {
          cb(cur_tag, begin, end);
        }

        return true;
      },
      ":");
}

void iter_ranges(std::string_view gtids, auto &&cb) {
  shcore::str_itersplit(
      gtids,
      [&cb](std::string_view gtid) {
        gtid = shcore::str_strip_view(gtid);

        const auto p = gtid.find(':');
        if (p == std::string_view::npos) return true;

        const auto uuid = gtid.substr(0, p);

        iter_tag_ranges(
            gtid.substr(p + 1),
            [&cb, &uuid](std::string_view tag, uint64_t begin, uint64_t end) {
              cb(uuid, tag, begin, end);
            });

        return true;
      },
      ",");
}
}  // namespace

Gtid_range::Gtid_range(std::string_view range_uuid, std::string_view range_tag,
                       uint64_t range_begin, uint64_t range_end) {
  if (range_tag.empty()) {
    uuid_tag = std::string{range_uuid};
  } else {
    uuid_tag.reserve(range_uuid.size() + range_tag.size() + 1);

    uuid_tag.append(range_uuid).append(":");
    std::transform(range_tag.begin(), range_tag.end(),
                   std::back_inserter(uuid_tag), ::tolower);
  }

  begin = range_begin;
  end = range_end;
}

Gtid_range Gtid_range::from_gtid(std::string_view gtid) {
  Gtid_range range;

  iter_ranges(gtid,
              [&range](std::string_view gtid_uuid, std::string_view gtid_tag,
                       uint64_t gtid_begin, uint64_t gtid_end) {
                range = Gtid_range{gtid_uuid, gtid_tag, gtid_begin, gtid_end};
              });

  return range;
}

std::string Gtid_range::str() const {
  if (begin == end)
    return shcore::str_format("%s:%" PRIu64, uuid_tag.c_str(), begin);

  return shcore::str_format("%s:%" PRIu64 "-%" PRIu64, uuid_tag.c_str(), begin,
                            end);
}
uint64_t Gtid_range::count() const noexcept { return end - begin + 1; }

Gtid_set Gtid_set::from_gtid_executed(
    const mysqlshdk::mysql::IInstance &server) {
  return Gtid_set(get_executed_gtid_set(server), true);
}

Gtid_set Gtid_set::from_gtid_purged(const mysqlshdk::mysql::IInstance &server) {
  return Gtid_set(get_purged_gtid_set(server), true);
}

Gtid_set Gtid_set::from_received_transaction_set(
    const mysqlshdk::mysql::IInstance &server, std::string_view channel) {
  return Gtid_set(get_received_gtid_set(server, channel), true);
}

Gtid_set &Gtid_set::normalize(const mysqlshdk::mysql::IInstance &server) {
  if (!std::exchange(m_normalized, true)) {
    m_gtid_set = server.queryf_one_string(0, "", "SELECT gtid_subtract(?, '')",
                                          m_gtid_set);
  }
  return *this;
}

Gtid_set &Gtid_set::intersect(const Gtid_set &other,
                              const mysqlshdk::mysql::IInstance &server) {
  if (m_gtid_set.empty() || other.m_gtid_set.empty()) {
    m_gtid_set.clear();
    m_normalized = false;
    return *this;
  }

  // a /\ b = a - (a - b)
  m_normalized = true;
  m_gtid_set = server.queryf_one_string(
      0, "", "SELECT gtid_subtract(?, gtid_subtract(?, ?))", other.m_gtid_set,
      other.m_gtid_set, m_gtid_set);

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
    m_gtid_set.append(",").append(gtid);
  }

  return *this;
}

Gtid_set &Gtid_set::add(const Gtid_set &other) {
  if (m_gtid_set.empty()) {
    m_normalized = other.m_normalized;
    m_gtid_set = other.m_gtid_set;
  } else if (!other.m_gtid_set.empty()) {
    m_normalized = false;
    m_gtid_set.append(",").append(other.m_gtid_set);
  }

  return *this;
}

Gtid_set &Gtid_set::add(const Gtid_range &range) {
  if (m_gtid_set.empty()) {
    m_normalized = true;
    m_gtid_set = range.str();
  } else {
    m_normalized = false;
    m_gtid_set.append(",").append(range.str());
  }

  return *this;
}
Gtid_set Gtid_set::get_gtids_tagged() const {
  mysqlshdk::mysql::Gtid_set matches;
  iter_ranges(m_gtid_set, [&matches](std::string_view gtid_uuid,
                                     std::string_view gtid_tag,
                                     uint64_t gtid_begin, uint64_t gtid_end) {
    if (gtid_tag.empty()) return;

    matches.add(Gtid_range{gtid_uuid, gtid_tag, gtid_begin, gtid_end});
  });

  return matches;
}

Gtid_set Gtid_set::get_gtids_from(std::string_view uuid) const {
  if (uuid.empty()) return {};

  mysqlshdk::mysql::Gtid_set matches;
  iter_ranges(
      m_gtid_set,
      [&matches, &uuid](std::string_view gtid_uuid, std::string_view gtid_tag,
                        uint64_t gtid_begin, uint64_t gtid_end) {
        if (gtid_uuid != uuid) return;

        matches.add(Gtid_range{gtid_uuid, gtid_tag, gtid_begin, gtid_end});
      });

  return matches;
}

Gtid_set Gtid_set::get_gtids_from(std::string_view uuid,
                                  std::string_view tag) const {
  if (uuid.empty() || tag.empty()) return {};

  mysqlshdk::mysql::Gtid_set matches;
  iter_ranges(m_gtid_set, [&matches, &uuid, &tag](std::string_view gtid_uuid,
                                                  std::string_view gtid_tag,
                                                  uint64_t gtid_begin,
                                                  uint64_t gtid_end) {
    if (gtid_uuid != uuid) return;
    if (!shcore::str_caseeq(gtid_tag, tag)) return;

    matches.add(Gtid_range{gtid_uuid, gtid_tag, gtid_begin, gtid_end});
  });

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
  iter_ranges(m_gtid_set,
              [&count](std::string_view, std::string_view, uint64_t begin,
                       uint64_t end) { count += end - begin + 1; });

  return count;
}

void Gtid_set::enumerate(const std::function<void(Gtid)> &fn) const {
  enumerate_ranges([&fn](Gtid_range range) {
    for (auto i = range.begin; i <= range.end; ++i)
      fn(Gtid{shcore::str_format("%s:%" PRIu64, range.uuid_tag.c_str(), i)});
  });
}

void Gtid_set::enumerate_ranges(
    const std::function<void(Gtid_range)> &fn) const {
  if (!m_normalized)
    throw std::invalid_argument("Can't enumerate un-normalized Gtid_set");

  iter_ranges(m_gtid_set, [&fn](std::string_view uuid, std::string_view tag,
                                uint64_t begin, uint64_t end) {
    fn(Gtid_range{uuid, tag, begin, end});
  });
}

bool Gtid_set::has_tags() const {
  bool has_tags{false};
  iter_ranges(m_gtid_set,
              [&has_tags](std::string_view, std::string_view tag, uint64_t,
                          uint64_t) { has_tags |= !tag.empty(); });

  return has_tags;
}

}  // namespace mysqlshdk::mysql
