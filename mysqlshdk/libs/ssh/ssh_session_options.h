/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_SESSION_OPTIONS_H_
#define MYSQLSHDK_LIBS_SSH_SSH_SESSION_OPTIONS_H_

#include <libssh/libsshpp.hpp>
#include <memory>
#include <string>

namespace mysqlshdk {
namespace ssh {

/**
 * @brief General class for handling ssh session config
 *
 */

class Ssh_session_options {
 public:
  explicit Ssh_session_options(const std::string &config,
                               const std::string &host);
  Ssh_session_options(const Ssh_session_options &ses) = delete;
  Ssh_session_options(Ssh_session_options &&ses) = delete;
  Ssh_session_options &operator=(Ssh_session_options &) = delete;
  Ssh_session_options &operator=(Ssh_session_options &&) = delete;
  virtual ~Ssh_session_options() {}
  std::string get_host() const;
  std::string get_user() const;
  std::string get_known_hosts() const;
  std::string get_identity_file() const;
  unsigned int get_port() const;

 private:
  friend class Ssh_connection_options;
  friend class Ssh_session;
  std::unique_ptr<::ssh::Session> m_session;
  bool m_no_host = false;
};
}  // namespace ssh
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_SSH_SSH_COMMON_H_
