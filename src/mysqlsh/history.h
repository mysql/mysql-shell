/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef SRC_MYSQLSH_HISTORY_H_
#define SRC_MYSQLSH_HISTORY_H_

#include <deque>
#include <functional>
#include <string>

namespace mysqlsh {
class History {
 public:
  History();
  bool load(const std::string &file);

  void clear();
  void dump(const std::function<void(const std::string &)> &print);
  void pause(bool flag);
  void add(const std::string &s);
  void del(uint32_t serial_first, uint32_t serial_last);
  void set_limit(uint32_t limit);

  uint32_t size() const;

  uint32_t first_entry() const;
  uint32_t last_entry() const;

 private:
  uint32_t _limit = 0;
  // history index -> serial id mapping
  std::deque<uint32_t> _serials;
  uint32_t _serial = 0;
  int _paused = 0;
};

}  // namespace mysqlsh

#endif  // SRC_MYSQLSH_HISTORY_H_
