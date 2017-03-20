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

#include "utils_string.h"
#include <cstdio>
#include <cstdarg>

namespace shcore {

std::string str_strip(const std::string &s, const std::string &chars) {
  size_t begin = s.find_first_not_of(chars);
  size_t end = s.find_last_not_of(chars);
  if (begin == std::string::npos)
    return std::string();
  return s.substr(begin, end-begin+1);
}

std::string str_lstrip(const std::string &s, const std::string &chars) {
  size_t begin = s.find_first_not_of(chars);
  if (begin == std::string::npos)
    return std::string();
  return s.substr(begin);
}

std::string str_rstrip(const std::string &s, const std::string &chars) {
  size_t end = s.find_last_not_of(chars);
  if (end == std::string::npos)
    return std::string();
  return s.substr(0, end+1);
}

std::string str_format(const char* formats, ...) {
  static const int kBufferSize = 256;
  std::string buffer;
  buffer.resize(kBufferSize);
  int len;
  va_list args;

#ifdef WIN32
  va_start(args, formats);
  len = _vscprintf(formats, args);
  va_end(args);
  if (len < 0)
    throw std::invalid_argument("Could not format string");
  buffer.resize(len+1);
  va_start(args, formats);
  len = vsnprintf(&buffer[0], buffer.size(), formats, args);
  va_end(args);
  if (len < 0)
    throw std::invalid_argument("Could not format string");
  buffer.resize(len);
#else
  va_start(args, formats);
  len = vsnprintf(&buffer[0], buffer.size(), formats, args);
  va_end(args);
  if (len < 0)
    throw std::invalid_argument("Could not format string");
  if (len+1 >= kBufferSize) {
    buffer.resize(len+1);
    va_start(args, formats);
    len = vsnprintf(&buffer[0], buffer.size(), formats, args);
    va_end(args);
    if (len < 0)
      throw std::invalid_argument("Could not format string");
  }
  buffer.resize(len);
#endif

  return buffer;
}

}
