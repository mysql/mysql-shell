/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>
#include <cctype>

#include "utils_connection.h"

using namespace shcore;

std::string MapSslModeNameToValue::_empty = "";

int MapSslModeNameToValue::get_value(const std::string& value) {
  std::string my_value(value);
  std::transform(my_value.begin(), my_value.end(), my_value.begin(), ::toupper);

  if (my_value == "DISABLED")
    return 1;
  else if (my_value == "PREFERRED")
    return 2;
  else if (my_value == "REQUIRED")
    return 3;
  else if (my_value == "VERIFY_CA")
    return 4;
  else if (my_value == "VERIFY_IDENTITY")
    return 5;
  else
    return 0;
}

const std::string MapSslModeNameToValue::get_value(int value) {
  switch (value) {
  case 0: return "";
  case 1: return "DISABLED";
  case 2: return "PREFERRED";
  case 3: return "REQUIRED";
  case 4: return "VERIFY_CA";
  case 5: return "VERIFY_IDENTITY";
  }
}

SslInfo::SslInfo(const SslInfo& s) : skip(s.skip), mode(s.mode), ca(s.ca), capath(s.capath),
    crl(s.crl), crlpath(s.crlpath), ciphers(s.ciphers), tls_version(s.tls_version),
    cert(s.cert), key(s.key)
{}
