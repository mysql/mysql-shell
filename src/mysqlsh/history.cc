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

#include "mysqlsh/history.h"
#include <errno.h>
#include <algorithm>
#include <cassert>
#include <iostream>

#include "ext/linenoise-ng/include/linenoise.h"

namespace mysqlsh {

History::History() {
  linenoiseHistoryFree();
}

bool History::load(const std::string &path) {
  clear();
  linenoiseHistorySetMaxLen(_limit);
  if (linenoiseHistoryLoad(path.c_str()) < 0) {
    linenoiseHistorySetMaxLen(_limit + 1);
    return errno == ENOENT;  // file not found is OK
  } else {
    _serial = 0;
    for (int c = linenoiseHistorySize(), i = 0; i < c; i++) {
      _serials.push_back(++_serial);
    }
    linenoiseHistorySetMaxLen(_limit + 1);
    return true;
  }
}

void History::clear() {
  _serial = 0;
  _serials.clear();
  linenoiseHistoryFree();
}

void History::pause(bool flag) {
  if (flag)
    _paused++;
  else
    _paused--;
  assert(_paused >= 0);
}

void History::dump(const std::function<void(const std::string &)> &print) {
  uint32_t c = size();
  for (uint32_t i = 0; i < c; i++) {
    const char *ptr = linenoiseHistoryLine(i);
    assert(ptr != nullptr);
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%5u  ", _serials[i]);
    print(std::string(buffer) + ptr);
  }
}

uint32_t History::first_entry() const {
  if (_serials.empty())
    return 0;
  return _serials.front();
}

uint32_t History::last_entry() const {
  if (_serials.empty())
    return 0;
  return _serials.back();
}

uint32_t History::size() const {
  // Return the real number of items (ignoring the current line buffer)
  return static_cast<uint32_t>(_serials.size());
}

void History::add(const std::string &s) {
  if (_paused == 0 && _limit > 0) {
    // use our own tracking of entries, so that we don't get confused by the
    // current line buffer

    if (size() == _limit) {
      if (linenoiseHistoryAdd(s.c_str())) {
        _serials.push_back(++_serial);

        linenoiseHistoryDelete(0);
        _serials.pop_front();
      }
    } else {
      if (linenoiseHistoryAdd(s.c_str()))
        _serials.push_back(++_serial);
    }
  }
}

void History::set_limit(uint32_t limit) {
  if (limit > (1 << 31))
    throw std::invalid_argument("invalid history limit");
  // Note: the linenoise history includes the "current" line being edited, thus
  // it can't be set to 0. OTOH users are not expecting that line to be counted
  // when configuring history size, so we add an extra entry for it.
  // That will make a history size of 0 be 1 to account for the current
  // line, but as soon as Enter is pressed on the keyboard, the current line
  // is cleared from history.

  if (limit < linenoiseHistorySize()) {
    int del_count = linenoiseHistorySize() - limit;
    // shrinking history so delete whatever will be pushed out
    _serials.erase(_serials.begin(), _serials.begin() + del_count);
    for (int i = 0; i < del_count; i++)
      linenoiseHistoryDelete(0);
  }
  _limit = limit;
  linenoiseHistorySetMaxLen(static_cast<int>(limit + 1));
}

void History::del(uint32_t serial_first, uint32_t serial_last) {
  assert(serial_first > 0);
  assert(serial_first >= first_entry() && serial_first <= last_entry());
  assert(serial_last >= first_entry() && serial_last <= last_entry());
  assert(serial_first <= serial_last);

  for (uint32_t ser = serial_last; ser >= serial_first; ser--) {
    auto iter = std::find(_serials.begin(), _serials.end(), ser);
    if (iter != _serials.end()) {
      int index = iter - _serials.begin();
      if (!linenoiseHistoryDelete(index))
        break;
      _serials.erase(iter);
    }
  }
}

}  // namespace mysqlsh
