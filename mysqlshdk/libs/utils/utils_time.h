/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __mysh__utils_time__
#define __mysh__utils_time__

#include "scripting/common.h"
#include <string>

class SHCORE_PUBLIC MySQL_timer {
public:
  uint64_t get_time();
  uint64_t start();
  uint64_t end();
  uint64_t raw_duration() { return _end - _start; }
  static std::string format_legacy(uint64_t raw_time, int part_seconds, bool in_seconds = false);
  static void parse_duration(uint64_t raw_time, int &days, int &hours, int &minutes, float &seconds, bool in_seconds = false);

  static uint64_t seconds_to_duration(float s);

private:
  uint64_t _start;
  uint64_t _end;
};

#endif /* defined(__mysh__utils_time__) */
