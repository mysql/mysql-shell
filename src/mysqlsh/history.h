/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef SRC_MYSQLSH_HISTORY_H_
#define SRC_MYSQLSH_HISTORY_H_

#include <cstdint>
#include <deque>
#include <functional>
#include <string>

namespace mysqlsh {
class History {
 public:
  History();
  bool save(const std::string &file);
  bool load(const std::string &file);

  void clear();
  void dump(const std::function<void(const std::string &)> &print);
  void pause(bool flag);
  void add(const std::string &s);
  void add_temporary(const std::string &s);
  void del(uint32_t serial_first, uint32_t serial_last);
  void set_limit(uint32_t limit);

  uint32_t size() const;

  uint32_t first_entry() const;
  uint32_t last_entry() const;
  std::string get_entry(uint32_t serial) const;

 private:
  uint32_t _limit = 0;
  // history index -> serial id mapping
  std::deque<uint32_t> _serials;
  uint32_t _serial = 0;
  int _paused = 0;
  bool _last_entry_was_temporary = false;

  void clear_temporary();
};

}  // namespace mysqlsh

#endif  // SRC_MYSQLSH_HISTORY_H_
