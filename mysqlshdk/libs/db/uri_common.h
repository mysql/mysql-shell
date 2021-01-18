/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/libs/utils/enumset.h"

#ifndef MYSQLSHDK_LIBS_DB_URI_COMMON_H_
#define MYSQLSHDK_LIBS_DB_URI_COMMON_H_

namespace mysqlshdk {
namespace db {
namespace uri {

const char DELIMITERS[] = ":/?#[]@";
const char SUBDELIMITERS[] = "!$&'()*+,;=";
const char ALPHA[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
const char DIGIT[] = "0123456789";
const char HEXDIG[] = "ABCDEFabcdef0123456789";
const char ALPHANUMERIC[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789";
const char RESERVED[] =
    ":/?#[]@"       // DELIMITERS
    "!$&'()*+,;=";  // SUBDELIMITERS
const char UNRESERVED[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-._~";

enum Tokens {
  Scheme = 0,
  User = 1,
  Password = 2,
  Transport = 3,
  Schema = 4,
  Query = 5
};

typedef mysqlshdk::utils::Enum_set<Tokens, Tokens::Query> Tokens_mask;

namespace formats {
inline Tokens_mask no_schema_no_query() {
  return Tokens_mask(Tokens::Scheme)
      .set(Tokens::User)
      .set(Tokens::Password)
      .set(Tokens::Transport);
}

inline Tokens_mask full() {
  return Tokens_mask(Tokens::Scheme)
      .set(Tokens::User)
      .set(Tokens::Password)
      .set(Tokens::Transport)
      .set(Tokens::Schema)
      .set(Tokens::Query);
}
inline Tokens_mask full_no_password() {
  return Tokens_mask(Tokens::Scheme)
      .set(Tokens::User)
      .set(Tokens::Transport)
      .set(Tokens::Schema)
      .set(Tokens::Query);
}
inline Tokens_mask no_scheme() {
  return Tokens_mask(Tokens::User)
      .set(Tokens::Password)
      .set(Tokens::Transport)
      .set(Tokens::Schema)
      .set(Tokens::Query);
}
inline Tokens_mask no_scheme_no_password() {
  return Tokens_mask(Tokens::User)
      .set(Tokens::Transport)
      .set(Tokens::Schema)
      .set(Tokens::Query);
}
inline Tokens_mask only_transport() { return Tokens_mask(Tokens::Transport); }
inline Tokens_mask user_transport() {
  return Tokens_mask(Tokens::User).set(Tokens::Transport);
}
inline Tokens_mask scheme_user_transport() {
  return Tokens_mask(Tokens::Scheme).set(Tokens::User).set(Tokens::Transport);
}
}  // namespace formats
}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_URI_COMMON_H_
