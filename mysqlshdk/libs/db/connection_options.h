/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_CONNECTION_OPTIONS_H_
#define MYSQLSHDK_LIBS_DB_CONNECTION_OPTIONS_H_

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/include/shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/ssl_options.h"
#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/connection.h"
#include "mysqlshdk/libs/utils/nullable_options.h"

namespace mysqlsh {
enum class SessionType { Auto, X, Classic };
}  // namespace mysqlsh

namespace mysqlshdk {
namespace db {
using utils::nullable;
using utils::Nullable_options;
using utils::nullable_options::Comparison_mode;
enum Transport_type { Tcp, Socket, Pipe };
std::string to_string(Transport_type type);

using Mfa_passwords = std::array<std::optional<std::string>, 3>;

constexpr int k_default_mysql_port = 3306;
constexpr int k_default_mysql_x_port = 33060;

class SHCORE_PUBLIC Connection_options : public IConnection {
 public:
  explicit Connection_options(
      Comparison_mode mode = Comparison_mode::CASE_INSENSITIVE);
  Connection_options(const std::string &uri,
                     Comparison_mode mode = Comparison_mode::CASE_INSENSITIVE);
  Connection_options(const Connection_options &other) = default;
  Connection_options(Connection_options &&other) = default;

  Connection_options &operator=(const Connection_options &other) = default;
  Connection_options &operator=(Connection_options &&other) = default;

  ~Connection_options() = default;

  void set_login_options_from(const Connection_options &options);
  void set_ssl_options(const Ssl_options &options);
  void set_ssh_options(const ssh::Ssh_connection_options &options);
  const std::string &get_schema() const { return get_value(kSchema); }
  const std::string &get_socket() const { return get_value(kSocket); }
  const std::string &get_pipe() const { return get_value(kSocket); }
  int get_port() const override;
  int get_target_port() const;

  const std::string &get_password() const override;

  Transport_type get_transport_type() const;
  const std::string &get_compression() const {
    return m_extra_options.get_value(kCompression);
  }
  const std::string &get_compression_algorithms() const {
    return m_extra_options.get_value(kCompressionAlgorithms);
  }
  int64_t get_compression_level() const;
  const Mfa_passwords &get_mfa_passwords() const;

  const std::string &get(const std::string &name) const;

  const Ssl_options &get_ssl_options() const { return m_ssl_options; }
  Ssl_options &get_ssl_options() { return m_ssl_options; }

  const ssh::Ssh_connection_options &get_ssh_options() const {
    return m_ssh_options;
  }

  ssh::Ssh_connection_options &get_ssh_options_handle(int fallback_port = 0);

  const std::vector<std::string> &get_warnings() const { return m_warnings; }
  bool has_password() const override {
    return has_value(kPassword) || m_mfa_passwords[0].has_value();
  }

  bool has_mfa_passwords() const;

  bool has_data() const override;
  bool has_schema() const { return has_value(kSchema); }
  bool has_socket() const { return has_value(kSocket); }
  bool has_pipe() const { return has_value(kSocket); }
  bool has_transport_type() const { return !m_transport_type.is_null(); }
  bool has_compression() const { return m_extra_options.has(kCompression); }
  bool has_compression_algorithms() const {
    return m_extra_options.has(kCompressionAlgorithms);
  }
  bool has_compression_level() const { return !m_compress_level.is_null(); }

  bool has(const std::string &name) const override;
  bool has_value(const std::string &name) const override;
  bool has_ssh_options() const;

  void set_host(const std::string &host) override;
  void set_port(int port) override;
  void set_schema(const std::string &schema) { _set_fixed(kSchema, schema); }
  void set_socket(const std::string &socket);
  void set_pipe(const std::string &pipe);
  void set_compression(const std::string &compress);
  void set_compression_algorithms(const std::string &compression_algorithms);
  void set_compression_level(int64_t compression_level) {
    m_compress_level = compression_level;
  }
  void set_mfa_passwords(const Mfa_passwords &mfa_passwords) {
    m_mfa_passwords = mfa_passwords;
  }

  void set(const std::string &attribute,
           const std::vector<std::string> &values);
  void set(const std::string &attribute, const std::string &value);
  void set_unchecked(const std::string &name, const char *value = nullptr);

  void clear_host() override;
  void clear_port() override;
  void clear_mfa_passwords();
  void clear_schema() { clear_value(kSchema); }
  void clear_socket();
  void clear_pipe();
  void clear_compression_level() { m_compress_level.reset(); }

  void clear_warnings() { m_warnings.clear(); }

  void remove(const std::string &name);

  bool operator==(const Connection_options &other) const;
  bool operator!=(const Connection_options &other) const;

  std::string as_uri(uri::Tokens_mask format =
                         uri::formats::full_no_password()) const override;

  std::string uri_endpoint() const {
    return as_uri(uri::formats::only_transport());
  }

  const Nullable_options &get_extra_options() const { return m_extra_options; }
  bool is_connection_attributes_enabled() const {
    return m_enable_connection_attributes;
  }
  const Nullable_options &get_connection_attributes() const {
    return m_connection_attributes;
  }

  void set_connection_attributes(const std::vector<std::string> &attributes);
  void set_connection_attribute(const std::string &attribute,
                                const std::string &value);

  mysqlsh::SessionType get_session_type() const;

  /**
   * Sets the default connection data, if they're missing:
   * - uses system user if no user is specified,
   * - uses localhost if host is not specified and transport type is not
   *   specified or it's TCP.
   */

  void set_default_data() override;

  void set_plugins_dir();

  int compare(const std::string &lhs, const std::string &rhs) const {
    return m_options.compare(lhs, rhs);
  }

  static void throw_invalid_connect_timeout(const std::string &value);

  bool is_auth_method(const std::string &method_id) const;

 private:
  void _set_fixed(const std::string &key, const std::string &val);
  std::string get_iname(const std::string &name) const;
  bool is_extra_option(const std::string &option);
  bool is_bool_value(const std::string &value);

  void raise_connection_type_error(const std::string &source);
  void check_compression_conflicts();

  nullable<Transport_type> m_transport_type;
  nullable<int64_t> m_compress_level;

  Ssl_options m_ssl_options;
  ssh::Ssh_connection_options m_ssh_options;
  Nullable_options m_extra_options;
  bool m_enable_connection_attributes;
  Nullable_options m_connection_attributes;
  mutable Mfa_passwords m_mfa_passwords;

  std::vector<std::string> m_warnings;
};

int64_t default_connect_timeout();

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_CONNECTION_OPTIONS_H_
