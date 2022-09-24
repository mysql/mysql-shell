/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/utils/nullable_options.h"

#ifndef MYSQLSHDK_LIBS_DB_URI_COMMON_H_
#define MYSQLSHDK_LIBS_DB_URI_COMMON_H_

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

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

enum class Type { DevApi, Ssh, File, Generic };

enum Tokens {
  Scheme = 0,
  User = 1,
  Password = 2,
  Transport = 3,
  Path = 4,
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
      .set(Tokens::Path)
      .set(Tokens::Query);
}
inline Tokens_mask full_no_password() {
  return Tokens_mask(Tokens::Scheme)
      .set(Tokens::User)
      .set(Tokens::Transport)
      .set(Tokens::Path)
      .set(Tokens::Query);
}
inline Tokens_mask no_scheme() {
  return Tokens_mask(Tokens::User)
      .set(Tokens::Password)
      .set(Tokens::Transport)
      .set(Tokens::Path)
      .set(Tokens::Query);
}
inline Tokens_mask no_scheme_no_password() {
  return Tokens_mask(Tokens::User)
      .set(Tokens::Transport)
      .set(Tokens::Path)
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

class IUri_data_base {
 public:
  virtual ~IUri_data_base() = default;
  virtual mysqlshdk::db::uri::Type get_type() const = 0;
  virtual std::string as_uri(
      uri::Tokens_mask format = formats::full_no_password()) const = 0;
};

class IUri_parsable : public virtual IUri_data_base {
 public:
  ~IUri_parsable() override = default;
  virtual void set(const std::string &name, const std::string &value) = 0;
  virtual void set(const std::string &name, int value) = 0;
  virtual void set(const std::string &name,
                   const std::vector<std::string> &values) = 0;
  bool is_allowed_scheme(const std::string &name) const;

 protected:
  void validate_allowed_scheme(const std::string &scheme) const;

 private:
  virtual const std::unordered_set<std::string> &allowed_schemes() const = 0;
};

class IUri_encodable : public virtual IUri_data_base {
 public:
  ~IUri_encodable() override = default;
  virtual bool has_value(const std::string &name) const = 0;
  virtual const std::string &get(const std::string &name) const = 0;
  virtual int get_numeric(const std::string &name) const = 0;
  virtual std::vector<std::pair<std::string, mysqlshdk::null_string>>
  query_attributes() const = 0;
};

class Uri_serializable : public IUri_parsable, public IUri_encodable {
 public:
  explicit Uri_serializable(
      const std::unordered_set<std::string> &allowed_schemes);

  Uri_serializable(const Uri_serializable &) = default;
  Uri_serializable(Uri_serializable &&) = default;

  Uri_serializable &operator=(const Uri_serializable &) = default;
  Uri_serializable &operator=(Uri_serializable &&) = default;

  ~Uri_serializable() override = default;

  const std::unordered_set<std::string> &allowed_schemes() const override;

  std::string as_uri(
      uri::Tokens_mask format = formats::full_no_password()) const override;

 private:
  std::unordered_set<std::string> m_allowed_schemes;
};

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_URI_COMMON_H_
