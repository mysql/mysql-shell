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

#include "mysqlshdk/libs/utils/utils_lexing.h"
#include <cctype>

namespace mysqlshdk {
namespace utils {

SQL_string_iterator::SQL_string_iterator(const std::string &str,
                                         std::string::size_type offset)
    : m_s(str), m_offset(offset - 1) {
  // Let's make sure we start from valid SQL
  ++(*this);
}

SQL_string_iterator &SQL_string_iterator::operator++() {
  if (++m_offset > m_s.length())
    throw std::out_of_range("SQL_string_iterator offset out of range");

  bool incremented = false;
  do {
    switch (m_s[m_offset]) {
      case '\'':
        m_offset = span_quoted_string_sq(m_s, m_offset);
        break;
      case '"':
        m_offset = span_quoted_string_dq(m_s, m_offset);
        break;
      case '`':
        m_offset = span_quoted_sql_identifier_bt(m_s, m_offset);
        break;
      case '/':
        if (m_s[m_offset + 1] == '*')
          m_offset = span_cstyle_comment(m_s, m_offset);
        else
          incremented = true;
        break;
      case '#':
        m_offset = span_to_eol(m_s, m_offset + 1);
        break;
      case '-':
        if (m_offset + 2 < m_s.length() && m_s[m_offset + 1] == '-' &&
            std::isspace(m_s[m_offset + 2]))
          m_offset = span_to_eol(m_s, m_offset + 2);
        else
          incremented = true;
        break;
      default:
        incremented = true;
    }
    if (m_offset == std::string::npos) m_offset = m_s.length();
  } while (!incremented && m_offset < m_s.length());

  return *this;
}

using byte = unsigned char;
enum class BsonObjectIdStripState : byte {
  START = 0,
  DICT,
  K0,
  K1,
  K2,
  K3,
  K4,
  K5,
  COLON,
  OID_BEGIN,
  OID,
  OID_END,
  TERM
};

void strip_bson_object_id(std::string *json_doc) {
  using position = std::string::size_type;
  using S = BsonObjectIdStripState;
  position array_first = 0;
  position array_last_prim = 0;
  position oid_first = 0;

  S s = S::START;

  for (position i = 0; i < json_doc->size(); i++) {
    const byte &p = (*json_doc)[i];
    switch (s) {
      case S::START:
        if (p == '{') {
          s = S::DICT;
          array_first = i;
        } else {
          s = S::START;
        }
        break;
      case S::DICT:
        if (::isspace(p)) {
        } else if (p == '"') {
          s = S::K0;
        } else {
          s = S::START;
        }
        break;
      case S::K0:
        if (p == '$') {
          s = S::K1;
        } else {
          s = S::START;
        }
        break;
      case S::K1:
        if (p == 'o') {
          s = S::K2;
        } else {
          s = S::START;
        }
        break;
      case S::K2:
        if (p == 'i') {
          s = S::K3;
        } else {
          s = S::START;
        }
        break;
      case S::K3:
        if (p == 'd') {
          s = S::K4;
        } else {
          s = S::START;
        }
        break;
      case S::K4:
        if (p == '"') {
          s = S::K5;
        } else {
          s = S::START;
        }
        break;
      case S::K5:
        if (::isspace(p)) {
        } else if (p == ':') {
          s = S::COLON;
        } else {
          s = S::START;
        }
        break;
      case S::COLON:
        if (::isspace(p)) {
        } else if (p == '"') {
          s = S::OID_BEGIN;
          oid_first = i;
        } else {
          s = S::START;
        }
        break;
      case S::OID_BEGIN:
        if (::isxdigit(p)) {
          s = S::OID;
        } else {
          s = S::START;
        }
        break;
      case S::OID:
        if (::isxdigit(p)) {
        } else if (p == '"') {
          s = S::OID_END;
        } else {
          s = S::START;
        }
        break;
      case S::OID_END:
        if (::isspace(p)) {
        } else if (p == '}') {
          s = S::TERM;
          array_last_prim = i;
        } else {
          s = S::START;
        }
        break;
      case S::TERM:
        for (position e = array_first; e < oid_first; e++) {
          (*json_doc)[e] = ' ';
        }
        (*json_doc)[array_last_prim] = ' ';
        s = S::START;
        --i;
        break;
      default:
        s = S::START;
        break;
    }
  }
}

}  // namespace utils
}  // namespace mysqlshdk
