/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "utils/utils_string.h"
#include <algorithm>
#include <bitset>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace shcore {

std::string str_strip(const std::string &s, const std::string &chars) {
  size_t begin = s.find_first_not_of(chars);
  size_t end = s.find_last_not_of(chars);
  if (begin == std::string::npos) return std::string();
  return s.substr(begin, end - begin + 1);
}

std::string str_lstrip(const std::string &s, const std::string &chars) {
  size_t begin = s.find_first_not_of(chars);
  if (begin == std::string::npos) return std::string();
  return s.substr(begin);
}

std::string str_rstrip(const std::string &s, const std::string &chars) {
  size_t end = s.find_last_not_of(chars);
  if (end == std::string::npos) return std::string();
  return s.substr(0, end + 1);
}

std::string str_format(const char *formats, ...) {
  static const int kBufferSize = 256;
  std::string buffer;
  buffer.resize(kBufferSize);
  int len;
  va_list args;

#ifdef WIN32
  va_start(args, formats);
  len = _vscprintf(formats, args);
  va_end(args);
  if (len < 0) throw std::invalid_argument("Could not format string");
  buffer.resize(len + 1);
  va_start(args, formats);
  len = vsnprintf(&buffer[0], buffer.size(), formats, args);
  va_end(args);
  if (len < 0) throw std::invalid_argument("Could not format string");
  buffer.resize(len);
#else
  va_start(args, formats);
  len = vsnprintf(&buffer[0], buffer.size(), formats, args);
  va_end(args);
  if (len < 0) throw std::invalid_argument("Could not format string");
  if (len + 1 >= kBufferSize) {
    buffer.resize(len + 1);
    va_start(args, formats);
    len = vsnprintf(&buffer[0], buffer.size(), formats, args);
    va_end(args);
    if (len < 0) throw std::invalid_argument("Could not format string");
  }
  buffer.resize(len);
#endif

  return buffer;
}

std::string str_replace(const std::string &s, const std::string &from,
                        const std::string &to) {
  std::string str;
  int offs = from.length();
  str.reserve(s.length());

  if (from.empty()) {
    str.append(to);
    for (char c : s) {
      str.push_back(c);
      str.append(to);
    }
  } else {
    std::string::size_type start = 0, p = s.find(from);
    while (p != std::string::npos) {
      if (p > start) str.append(s, start, p - start);
      str.append(to);
      start = p + offs;
      p = s.find(from, start);
    }
    if (start < s.length()) str.append(s, start, s.length() - start);
  }
  return str;
}

static const char *k_bits[256] = {
    "00000000", "00000001", "00000010", "00000011", "00000100", "00000101",
    "00000110", "00000111", "00001000", "00001001", "00001010", "00001011",
    "00001100", "00001101", "00001110", "00001111", "00010000", "00010001",
    "00010010", "00010011", "00010100", "00010101", "00010110", "00010111",
    "00011000", "00011001", "00011010", "00011011", "00011100", "00011101",
    "00011110", "00011111", "00100000", "00100001", "00100010", "00100011",
    "00100100", "00100101", "00100110", "00100111", "00101000", "00101001",
    "00101010", "00101011", "00101100", "00101101", "00101110", "00101111",
    "00110000", "00110001", "00110010", "00110011", "00110100", "00110101",
    "00110110", "00110111", "00111000", "00111001", "00111010", "00111011",
    "00111100", "00111101", "00111110", "00111111", "01000000", "01000001",
    "01000010", "01000011", "01000100", "01000101", "01000110", "01000111",
    "01001000", "01001001", "01001010", "01001011", "01001100", "01001101",
    "01001110", "01001111", "01010000", "01010001", "01010010", "01010011",
    "01010100", "01010101", "01010110", "01010111", "01011000", "01011001",
    "01011010", "01011011", "01011100", "01011101", "01011110", "01011111",
    "01100000", "01100001", "01100010", "01100011", "01100100", "01100101",
    "01100110", "01100111", "01101000", "01101001", "01101010", "01101011",
    "01101100", "01101101", "01101110", "01101111", "01110000", "01110001",
    "01110010", "01110011", "01110100", "01110101", "01110110", "01110111",
    "01111000", "01111001", "01111010", "01111011", "01111100", "01111101",
    "01111110", "01111111", "10000000", "10000001", "10000010", "10000011",
    "10000100", "10000101", "10000110", "10000111", "10001000", "10001001",
    "10001010", "10001011", "10001100", "10001101", "10001110", "10001111",
    "10010000", "10010001", "10010010", "10010011", "10010100", "10010101",
    "10010110", "10010111", "10011000", "10011001", "10011010", "10011011",
    "10011100", "10011101", "10011110", "10011111", "10100000", "10100001",
    "10100010", "10100011", "10100100", "10100101", "10100110", "10100111",
    "10101000", "10101001", "10101010", "10101011", "10101100", "10101101",
    "10101110", "10101111", "10110000", "10110001", "10110010", "10110011",
    "10110100", "10110101", "10110110", "10110111", "10111000", "10111001",
    "10111010", "10111011", "10111100", "10111101", "10111110", "10111111",
    "11000000", "11000001", "11000010", "11000011", "11000100", "11000101",
    "11000110", "11000111", "11001000", "11001001", "11001010", "11001011",
    "11001100", "11001101", "11001110", "11001111", "11010000", "11010001",
    "11010010", "11010011", "11010100", "11010101", "11010110", "11010111",
    "11011000", "11011001", "11011010", "11011011", "11011100", "11011101",
    "11011110", "11011111", "11100000", "11100001", "11100010", "11100011",
    "11100100", "11100101", "11100110", "11100111", "11101000", "11101001",
    "11101010", "11101011", "11101100", "11101101", "11101110", "11101111",
    "11110000", "11110001", "11110010", "11110011", "11110100", "11110101",
    "11110110", "11110111", "11111000", "11111001", "11111010", "11111011",
    "11111100", "11111101", "11111110", "11111111"};

std::string bits_to_string(uint64_t bits, int nbits) {
  std::string r;
  std::div_t length = std::div(nbits, 8);

  switch (length.quot) {
    case 8:
      r.append(k_bits[(bits >> 56) & 0xff]);
      // fallthrough
    case 7:
      r.append(k_bits[(bits >> 48) & 0xff]);
      // fallthrough
    case 6:
      r.append(k_bits[(bits >> 40) & 0xff]);
      // fallthrough
    case 5:
      r.append(k_bits[(bits >> 32) & 0xff]);
      // fallthrough
    case 4:
      r.append(k_bits[(bits >> 24) & 0xff]);
      // fallthrough
    case 3:
      r.append(k_bits[(bits >> 16) & 0xff]);
      // fallthrough
    case 2:
      r.append(k_bits[(bits >> 16) & 0xff]);
      // fallthrough
    case 1:
      r.append(k_bits[(bits >> 8) & 0xff]);
      // fallthrough
    case 0:
      r.append(k_bits[bits & 0xff]);
      break;
  }
  r = r.substr(8 - length.rem);
  return r;
}

std::pair<uint64_t, int> string_to_bits(const std::string &s) {
  int nbits = s.length();
  if (nbits > 64)
    throw std::invalid_argument("bit string length must be <= 64");
  std::bitset<64> bits(s);
  return {bits.to_ullong(), nbits};
}

std::string quote_string(const std::string &s, char quote) {
  const std::string q{quote};
  const std::string backslash = str_replace(s, "\\", "\\\\");
  const std::string esc = str_replace(backslash, q, "\\" + q);
  return std::string(q + esc + q);
}

std::string unquote_string(const std::string &s, char quote) {
  const std::string q{quote};
  auto result = s;

  if (result.length() >= 2 && result[0] == quote &&
      result[result.length() - 1] == quote) {
    result = result.substr(1, result.length() - 2);
  }

  result = shcore::str_replace(result, "\\" + q, q);
  result = shcore::str_replace(result, "\\\\", "\\");

  return result;
};

std::vector<std::string> str_break_into_lines(const std::string &line,
                                              std::size_t line_width) {
  std::vector<std::string> result;
  std::string rem(line);
  std::string::size_type nl_pos = std::string::npos;
  std::string::size_type split_point = std::string::npos;

  while (rem.length() > line_width ||
         (nl_pos = rem.find('\n')) != std::string::npos) {
    if (nl_pos != std::string::npos) {
      split_point = nl_pos;
    } else {
      split_point = line_width - 1;
      while (!std::isspace(rem[split_point]) && split_point > 0) --split_point;

      if (split_point == 0 && !std::isspace(rem[0])) {
        for (split_point = line_width; split_point < rem.length();
             split_point++)
          if (std::isspace(rem[split_point])) break;
        if (split_point == rem.length()) break;
      } else {
        for (int i = split_point - 1; i >= 0; --i)
          if (rem[i] == '\n') split_point = i;
      }
    }

    if (split_point == 0)
      result.push_back("");
    else
      result.push_back(rem.substr(0, split_point));
    rem = split_point + 1 >= rem.length() ? std::string()
                                          : rem.substr(split_point + 1);
  }
  if (!rem.empty()) result.push_back(rem);
  return result;
}

std::pair<std::string::size_type, std::string::size_type> get_quote_span(
    const char quote_char, const std::string &str) {
  bool escaped = false;

  // if string has less than 2 chars  we assume matching quotes were not found.
  if (str.size() < 2)
    return std::make_pair(std::string::npos, std::string::npos);

  std::string::size_type open_quote_pos = 0;

  // find opening quote char
  for (std::string::size_type i = 0, end = str.size();
       (str[i] != quote_char || escaped) && i < end; ++i) {
    open_quote_pos++;
    escaped = (str[i]) == '\\' && !escaped;
  }
  // if no quotes were found
  if (open_quote_pos == str.size())
    return std::make_pair(std::string::npos, std::string::npos);

  // find closing quote char
  std::string::size_type close_quote_pos = open_quote_pos + 1;
  for (std::string::size_type i = close_quote_pos, end = str.size();
       (str[i] != quote_char || escaped) && i < end; ++i) {
    close_quote_pos++;
    escaped = (str[i]) == '\\' && !escaped;
  }

  // closing quote was not found
  if (close_quote_pos == str.size()) close_quote_pos = std::string::npos;
  return std::make_pair(open_quote_pos, close_quote_pos);
}
}  // namespace shcore
