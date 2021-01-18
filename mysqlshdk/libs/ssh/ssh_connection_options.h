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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_CONNECTION_OPTIONS_H_
#define MYSQLSHDK_LIBS_SSH_SSH_CONNECTION_OPTIONS_H_
#include <set>
#include <string>
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/connection.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace ssh {

enum class Ssh_fingerprint { STORE, REJECT };

/**
 * @brief Holds configuration of the SSH connection, the user auth data as also
 * basic connection config.
 *
 */
class Ssh_connection_options : public mysqlshdk::IConnection {
 public:
  Ssh_connection_options(const std::string &uri,
                         mysqlshdk::utils::nullable_options::Comparison_mode =
                             mysqlshdk::utils::nullable_options::
                                 Comparison_mode::CASE_INSENSITIVE);
  explicit Ssh_connection_options(
      mysqlshdk::utils::nullable_options::Comparison_mode mode = mysqlshdk::
          utils::nullable_options::Comparison_mode::CASE_INSENSITIVE);

  void set_key_file_password(const std::string &pwd) { m_key_password = pwd; }

  void set_remote_host(const std::string &host) {
    _set_fixed(mysqlshdk::db::kSshRemoteHost, host);
  }

  void set_remote_port(int port) {
    if (m_remote_port.is_null()) {
      m_remote_port = port;
    } else {
      throw std::invalid_argument(shcore::str_format(
          "The option ssh-remote-port '%d' is already defined.",
          *m_remote_port));
    }
  }

  void set_local_port(int port) {
    if (m_local_port.is_null()) {
      m_local_port = port;
    } else {
      throw std::invalid_argument(shcore::str_format(
          "The option ssh-local-port '%d' is already defined.", *m_local_port));
    }
  }

  void set_key_file(const std::string &path);

  void set_config_file(const std::string &file);

  void set_buffer_size(std::size_t buffer_size) { m_buffer_size = buffer_size; }

  void set_fingerprint(const std::string &fingerprint) {
    m_fingerprint = fingerprint;
  }

  void clear_key_file_password() { shcore::clear_buffer(&m_key_password); }

  void clear_remote_host() { clear_value(mysqlshdk::db::kSshRemoteHost); }
  void clear_remote_port() { m_remote_port.reset(); }
  void clear_local_port() { m_local_port.reset(); }
  void clear_config_file() { clear_value(mysqlshdk::db::kSshConfigFile); }
  void clear_key_file() { clear_value(mysqlshdk::db::kSshIdentityFile); }

  bool has_keyfile_password() const { return !m_key_password.empty(); }
  virtual bool has_remote_host() const {
    return has_value(mysqlshdk::db::kSshRemoteHost);
  }
  virtual bool has_remote_port() const { return !m_remote_port.is_null(); }
  virtual bool has_local_port() const { return !m_local_port.is_null(); }

  bool has_key_file() const {
    return has_value(mysqlshdk::db::kSshIdentityFile);
  }

  bool has_config_file() const {
    return has_value(mysqlshdk::db::kSshConfigFile);
  }

  const std::string &get_key_file_password() const { return m_key_password; }
  /**
   * The host where the remote service is located behind the NAT.
   */
  const std::string &get_remote_host() const {
    return get_value(mysqlshdk::db::kSshRemoteHost);
  }

  /**
   * The port of the remote service that will be forwarded.
   */
  const int &get_remote_port() const {
    if (m_remote_port.is_null()) {
      throw std::runtime_error(
          "Internal option 'ssh-remote-port' has no value.");
    }

    return *m_remote_port;
  }

  const int &get_local_port() const {
    if (m_local_port.is_null()) {
      throw std::runtime_error(
          "Internal option 'ssh-local-port' has no value.");
    }

    return *m_local_port;
  }

  const std::size_t &get_connection_timeout() const {
    return m_connection_timeout;
  }

  const std::string &get_source_host() const { return m_sourcehost; }

  const std::string &get_key_file() const {
    return get_value(mysqlshdk::db::kSshIdentityFile);
  }

  bool get_key_encrypted() const { return m_key_encrypted; }

  const std::string &get_config_file() const {
    return get_value(mysqlshdk::db::kSshConfigFile);
  }

  const std::size_t &get_buffer_size() const { return m_buffer_size; }

  const std::string &get_fingerprint() const { return m_fingerprint; }

  std::string get_server() const {
    return as_uri(mysqlshdk::db::uri::formats::only_transport());
  }

  void dump_config() const;

  bool compare_connection(const Ssh_connection_options &other) const;

  bool has_data() const override { return has_host(); }

  std::string as_uri(
      mysqlshdk::db::uri::Tokens_mask format =
          mysqlshdk::db::uri::formats::no_schema_no_query()) const override;

  std::string key_file_uri() const;

  void set_default_data() override;

  void preload_ssh_config();

  bool interactive();

 private:
  void check_key_encryption(const std::string &path);

  mysqlshdk::utils::nullable<int> m_remote_port;
  mysqlshdk::utils::nullable<int> m_local_port;
  std::size_t m_connection_timeout = 10;
  std::string m_sourcehost = "127.0.0.1";
  bool m_key_encrypted = false;
  std::string m_fingerprint;
  std::string m_key_password;

  // Not really an SSH option, used to pass the configured shell option
  std::size_t m_buffer_size = 10240;
};
}  // namespace ssh
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_SSH_SSH_CONNECTION_OPTIONS_H_
