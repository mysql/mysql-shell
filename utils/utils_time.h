/*
* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __mysh__utils_time__
#define __mysh__utils_time__

#include "shellcore/types_common.h"
#include "shellcore/common.h"
#include <string>

class SHCORE_PUBLIC MySQL_timer
{
public:
  unsigned long get_time();
  unsigned long start();
  unsigned long end();
  unsigned long raw_duration() { return _end - _start; }
  static std::string format_legacy(unsigned long raw_time, bool part_seconds);
  static void parse_duration(unsigned long raw_time, int &days, int &hours, int &minutes, float &seconds);

private:
  unsigned long _start;
  unsigned long _end;
};

#endif /* defined(__mysh__utils_time__) */
