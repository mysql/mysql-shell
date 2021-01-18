/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_CONNECTION_H_
#define MYSQLSHDK_LIBS_UTILS_CONNECTION_H_
#include <string>
#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/nullable_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {

class SHCORE_PUBLIC IConnection {
 public:
  IConnection(
      const std::string &options_name,
      utils::Comparison_mode mode = utils::Comparison_mode::CASE_INSENSITIVE)
      : m_mode(mode), m_options(m_mode, options_name) {
    for (auto o :
         {mysqlshdk::db::kHost, mysqlshdk::db::kScheme, mysqlshdk::db::kUser,
          mysqlshdk::db::kPassword, mysqlshdk::db::kPath})
      m_options.set(o, nullptr,
                    mysqlshdk::utils::nullable_options::Set_mode::CREATE);
  }

  utils::Comparison_mode get_mode() const { return m_mode; }

  virtual void set_scheme(const std::string &scheme) {
    _set_fixed(db::kScheme, scheme);
  }
  virtual void set_user(const std::string &user) {
    _set_fixed(db::kUser, user);
  }
  virtual void set_password(const std::string &pwd) {
    _set_fixed(db::kPassword, pwd);
  }
  virtual void set_host(const std::string &host) {
    _set_fixed(db::kHost, host);
  }

  virtual void set_path(const std::string &path) {
    _set_fixed(db::kPath, path);
  }

  virtual void set_port(int port) {
    if (m_port.is_null()) {
      m_port = port;
    } else {
      throw std::invalid_argument(shcore::str_format(
          "The option %s '%d' is already defined.", db::kPort, *m_port));
    }
  }

  virtual const std::string &get_scheme() const {
    return get_value(db::kScheme);
  }
  virtual const std::string &get_user() const { return get_value(db::kUser); }
  virtual const std::string &get_password() const {
    return get_value(db::kPassword);
  }
  virtual const std::string &get_host() const { return get_value(db::kHost); }
  virtual const std::string &get_path() const { return get_value(db::kPath); }
  virtual int get_port() const {
    if (m_port.is_null()) {
      throw std::invalid_argument(
          shcore::str_format("The option '%s' has no value.", db::kPort));
    }

    return *m_port;
  }

  virtual bool has_scheme() const { return has_value(db::kScheme); }
  virtual bool has_user() const { return has_value(db::kUser); }
  virtual bool has_password() const { return has_value(db::kPassword); }
  virtual bool has_host() const { return has_value(db::kHost); }
  virtual bool has_port() const { return !m_port.is_null(); }
  virtual bool has_path() const { return has_value(db::kPath); }

  virtual bool has(const std::string &name) const {
    return m_options.has(name);
  }
  virtual bool has_value(const std::string &name) const {
    if (m_options.has(name)) return m_options.has_value(name);
    return false;
  }

  virtual bool has_data() const {
    for (auto o :
         {mysqlshdk::db::kHost, mysqlshdk::db::kScheme, mysqlshdk::db::kUser,
          mysqlshdk::db::kPassword, mysqlshdk::db::kPath}) {
      if (has_value(o)) return true;
    }
    return false;
  }

  virtual void clear_scheme() { clear_value(db::kScheme); }
  virtual void clear_user() { clear_value(db::kUser); }
  virtual void clear_password() { clear_value(db::kPassword, true); }
  virtual void clear_host() { clear_value(db::kHost); }
  virtual void clear_port() { m_port.reset(); }
  virtual void clear_path() { clear_value(db::kPath); }

  virtual std::string as_uri(
      db::uri::Tokens_mask format =
          db::uri::formats::full_no_password()) const = 0;

  virtual void set_default_data() = 0;

  int compare(const std::string &lhs, const std::string &rhs) const {
    return m_options.compare(lhs, rhs);
  }

  virtual ~IConnection() = default;

 protected:
  virtual inline const std::string &get_value(const std::string &option) const {
    return m_options.get_value(option);
  }

  virtual inline void clear_value(const std::string &options,
                                  bool secure = false) {
    return m_options.clear_value(options, secure);
  }

  void _set_fixed(const std::string &key, const std::string &val) {
    m_options.set(key, val, utils::nullable_options::Set_mode::UPDATE_NULL);
  }
  utils::Comparison_mode m_mode;
  utils::Nullable_options m_options;
  utils::nullable<int> m_port;
};
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_UTILS_CONNECTION_H_
