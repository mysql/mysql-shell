/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/connectivity_check.h"

#include <mysql.h>
#include <mysqld_error.h>
#include <string>
#include <utility>
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/async_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {

namespace {

// replication errors that indicate the connection check succeeded
constexpr const int k_successful_errors[] = {
#ifdef ER_SLAVE_FATAL_ERROR
    ER_SLAVE_FATAL_ERROR,
    ER_SLAVE_MASTER_COM_FAILURE  // Master command COM_REGISTER_SLAVE failed:
// Access denied for user 'mysqlsh.test'@'%'
// (using password: NO) (Errno: 1045)
#else
    ER_REPLICA_FATAL_ERROR,
    ER_REPLICA_SOURCE_COM_FAILURE  //
#endif
};

void throw_ssl_connection_error_diagnostic(std::string_view error,
                                           const std::string &printable_error,
                                           std::string_view from_address,
                                           std::string_view to_address,
                                           Cluster_ssl_mode ssl_mode,
                                           Replication_auth_type auth_type) {
  if (error.find(":certificate verify failed") != std::string::npos) {
    auto console = current_console();

    bool known_case{false};
    switch (ssl_mode) {
      case Cluster_ssl_mode::VERIFY_CA:
        known_case = true;
        console->print_error(shcore::str_format(
            "SSL certificate issuer verification (VERIFY_CA) of instance "
            "'%.*s' failed at '%.*s'",
            static_cast<int>(from_address.size()), from_address.data(),
            static_cast<int>(to_address.size()), to_address.data()));

        console->print_info(shcore::str_format(
            "Ensure server certificate of '%.*s' was issued by a CA "
            "recognized by all members of the topology.",
            static_cast<int>(from_address.size()), from_address.data()));
        break;

      case Cluster_ssl_mode::VERIFY_IDENTITY:
        known_case = true;
        console->print_error(shcore::str_format(
            "SSL certificate subject verification (VERIFY_IDENTITY) of "
            "instance '%.*s' failed at '%.*s'",
            static_cast<int>(from_address.size()), from_address.data(),
            static_cast<int>(to_address.size()), to_address.data()));

        console->print_info(shcore::str_format(
            "Ensure server certificate of '%.*s' was issued by a CA "
            "recognized by all members of the topology.",
            static_cast<int>(from_address.size()), from_address.data()));
        break;

      default:
        if (auth_type != Replication_auth_type::PASSWORD) {
          known_case = true;
          console->print_error(shcore::str_format(
              "SSL certificate verification of instance '%.*s' failed at "
              "'%.*s'",
              static_cast<int>(from_address.size()), from_address.data(),
              static_cast<int>(to_address.size()), to_address.data()));

          console->print_info(shcore::str_format(
              "Ensure server certificate of '%.*s' was issued by a CA "
              "recognized by all members of the topology.",
              static_cast<int>(from_address.size()), from_address.data()));
        }
        break;
    }

    if (!known_case) console->print_error(printable_error);
  }

  throw shcore::Exception::runtime_error("SSL connection check error");
}

void throw_connect_error_diagnostic(
    const mysqlshdk::mysql::Replication_channel::Error &error,
    std::string_view from_address, std::string_view to_instance_address,
    std::string_view to_address, std::string_view to_address_source,
    std::string_view cert_issuer, std::string_view cert_subject,
    Cluster_ssl_mode ssl_mode, Replication_auth_type auth_type) {
  if (std::find(std::begin(k_successful_errors), std::end(k_successful_errors),
                error.code) != std::end(k_successful_errors))
    return;

  log_info(
      "Test replication channel between %.*s and %.*s (%.*s) failed with error "
      "%i: %s",
      static_cast<int>(from_address.size()), from_address.data(),
      static_cast<int>(to_address.size()), to_address.data(),
      static_cast<int>(to_address_source.size()), to_address_source.data(),
      error.code, error.message.c_str());

  switch (error.code) {
    case CR_CONN_HOST_ERROR:
    case CR_UNKNOWN_HOST:
      current_console()->print_error(shcore::str_format(
          "MySQL server at '%.*s' can't connect to '%.*s'. Verify configured "
          "%.*s and that the address is valid at that host.",
          static_cast<int>(from_address.size()), from_address.data(),
          static_cast<int>(to_address.size()), to_address.data(),
          static_cast<int>(to_address_source.size()),
          to_address_source.data()));

      throw shcore::Exception::runtime_error(
          "Server address configuration error");

    case CR_SSL_CONNECTION_ERROR: {
      auto printable_error = shcore::str_format(
          "Test replication channel between %.*s and %.*s (%.*s) failed with "
          "error %i: %s",
          static_cast<int>(from_address.size()), from_address.data(),
          static_cast<int>(to_address.size()), to_address.data(),
          static_cast<int>(to_address_source.size()), to_address_source.data(),
          error.code, error.message.c_str());

      throw_ssl_connection_error_diagnostic(error.message, printable_error,
                                            from_address, to_instance_address,
                                            ssl_mode, auth_type);
    }

    break;

    case ER_ACCESS_DENIED_ERROR:
      switch (auth_type) {
        case Replication_auth_type::PASSWORD:
          break;

        case Replication_auth_type::CERT_ISSUER:
        case Replication_auth_type::CERT_ISSUER_PASSWORD:
          current_console()->print_error(shcore::str_format(
              "Authentication error during connection check. Please ensure "
              "that the SSL certificate (@@ssl_cert) configured for '%.*s' was "
              "issued by '%.*s' (certIssuer), is recognized at '%.*s' and that "
              "the value specified for certIssuer itself is correct.",
              static_cast<int>(from_address.size()), from_address.data(),
              static_cast<int>(cert_issuer.size()), cert_issuer.data(),
              static_cast<int>(to_instance_address.size()),
              to_instance_address.data()));
          break;

        case Replication_auth_type::CERT_SUBJECT:
        case Replication_auth_type::CERT_SUBJECT_PASSWORD:
          current_console()->print_error(shcore::str_format(
              "Authentication error during connection check. Please ensure "
              "that the SSL certificate (@@ssl_cert) configured for '%.*s' was "
              "issued by '%.*s' (certIssuer), specifically for '%.*s' "
              "(certSubject) and is recognized at '%.*s'.",
              static_cast<int>(from_address.size()), from_address.data(),
              static_cast<int>(cert_issuer.size()), cert_issuer.data(),
              static_cast<int>(cert_subject.size()), cert_subject.data(),
              static_cast<int>(to_instance_address.size()),
              to_instance_address.data()));
          break;
      }
      throw shcore::Exception::runtime_error(
          "Authentication error during connection check");
  }
}

std::string create_temp_account(const mysqlshdk::mysql::IInstance &instance,
                                std::string_view username,
                                Replication_auth_type member_auth,
                                std::string_view cert_issuer,
                                std::string_view cert_subject) {
  mysqlshdk::mysql::Set_variable nobinlog(instance, "sql_log_bin", 0, false);

  // TODO(alfredo) create users with replicationAllowedHost
  instance.executef("DROP USER IF EXISTS ?@'%'", username);

  mysqlshdk::mysql::IInstance::Create_user_options user_options;

  switch (member_auth) {
    case Replication_auth_type::CERT_SUBJECT:
    case Replication_auth_type::CERT_SUBJECT_PASSWORD:
      user_options.cert_subject = cert_subject;
      [[fallthrough]];
    case Replication_auth_type::CERT_ISSUER:
    case Replication_auth_type::CERT_ISSUER_PASSWORD:
      user_options.cert_issuer = cert_issuer;
      break;
    default:
      break;
  }

  std::string new_pwd;
  mysqlshdk::mysql::create_user_with_random_password(instance, username, {"%"},
                                                     user_options, &new_pwd);
  return new_pwd;
}

void downgrade_temp_account_to_issuer_only(
    const mysqlshdk::mysql::IInstance &instance, std::string_view username,
    std::string_view cert_issuer) {
  mysqlshdk::mysql::Set_variable nobinlog(instance, "sql_log_bin", 0, false);

  instance.executef("ALTER USER ?@'%' REQUIRE ISSUER ?", username, cert_issuer);
}

mysqlshdk::mysql::Replication_channel::Error try_connect_channel(
    const mysqlshdk::mysql::IInstance &from_instance,
    std::string_view to_address) {
  mysqlshdk::mysql::start_replication_receiver(from_instance, "mysqlsh.test");

  auto channel = mysqlshdk::mysql::wait_replication_done_connecting(
      from_instance, "mysqlsh.test");

  log_info("Connection check %s -> %.*s io_state=%s io_error=%s",
           from_instance.descr().c_str(), static_cast<int>(to_address.size()),
           to_address.data(),
           mysqlshdk::mysql::to_string(channel.receiver.state).c_str(),
           mysqlshdk::mysql::to_string(channel.receiver.last_error).c_str());

  return channel.receiver.last_error;
}
}  // namespace

/**
 * @brief Opens a connection to verify network and SSL
 *
 * Sets up a dummy replication channel to itself to verify network and SSL
 * settings between instances.
 *
 * This ensures hostname and port (or @@report_host and @@report_port) are
 * correctly configured and can be reached from itself. It also verifies that
 * SSL certificates (whether one instance recognizes the other's certificate)
 * and related options are valid if ssl_mode and member_auth_type expects
 * them. If errors are detected, it will attempt to narrow down the issue
 * before reporting them.
 *
 * If localAddress is being used with GR, then that address must be reachable as
 * well in addition to the regular address. It's not enough that localAddress
 * alone is reachable because recovery happens through the regular network
 * interface.
 */
void test_async_channel_connection(
    const mysqlshdk::mysql::IInstance &from_instance,
    const mysqlshdk::mysql::IInstance &to_instance, std::string_view to_address,
    std::string_view to_address_source, Cluster_ssl_mode ssl_mode,
    Replication_auth_type member_auth_type, const std::string &user,
    const std::string &password, std::string_view cert_issuer,
    std::string_view cert_subject) {
  assert(member_auth_type == Replication_auth_type::PASSWORD ||
         ssl_mode != Cluster_ssl_mode::NONE);

  log_info(
      "Checking connection from %s to %s@%.*s:%i (%.*s) ssl_mode=%s "
      "auth_type=%s "
      "cert_issuer=%.*s cert_subject=%.*s",
      from_instance.descr().c_str(), user.c_str(),
      static_cast<int>(to_address.size()), to_address.data(),
      to_instance.get_canonical_port(),
      static_cast<int>(to_address_source.size()), to_address_source.data(),
      to_string(ssl_mode).c_str(), to_string(member_auth_type).c_str(),
      static_cast<int>(cert_issuer.size()), cert_issuer.data(),
      static_cast<int>(cert_subject.size()), cert_subject.data());

  // setup channel
  mysqlshdk::mysql::Auth_options creds;
  creds.user = user;
  creds.password = password;

  creds.ssl_options =
      prepare_replica_ssl_options(from_instance, ssl_mode, member_auth_type);

  mysqlshdk::mysql::change_master(
      from_instance, std::string{to_address.data(), to_address.size()},
      to_instance.get_canonical_port(), "mysqlsh.test", creds, 0, 0, {}, 0,
      false);

  shcore::Scoped_callback cleanup_channel([&from_instance]() {
    try {
      mysqlshdk::mysql::stop_replication_receiver(from_instance,
                                                  "mysqlsh.test");
    } catch (...) {
    }
    mysqlshdk::mysql::reset_slave(from_instance, "mysqlsh.test", true);
  });

  auto io_error = try_connect_channel(from_instance, to_address);

  if (io_error.code == ER_ACCESS_DENIED_ERROR &&
      (member_auth_type == Replication_auth_type::CERT_SUBJECT ||
       member_auth_type == Replication_auth_type::CERT_SUBJECT_PASSWORD)) {
    log_debug(
        "Authentication from %s to %.*s with REQUIRE SUBJECT failed, retrying "
        "with just issuer",
        from_instance.descr().c_str(), static_cast<int>(to_address.size()),
        to_address.data());
    downgrade_temp_account_to_issuer_only(to_instance, "mysqlsh.test",
                                          cert_issuer);
    mysqlshdk::mysql::stop_replication_receiver(from_instance, "mysqlsh.test");
    auto io_error2 = try_connect_channel(from_instance, to_address);
    // neither subject nor issuer succeeded
    if (io_error2.code == ER_ACCESS_DENIED_ERROR) {
      // If both REQUIRE SUBJECT and REQUIRE ISSUER fail, we complain first
      // about certIssuer
      member_auth_type = Replication_auth_type::CERT_ISSUER;
    }
  }

  throw_connect_error_diagnostic(
      io_error, from_instance.get_canonical_address(),
      to_instance.get_canonical_address(), to_address, to_address_source,
      cert_issuer, cert_subject, ssl_mode, member_auth_type);
}

namespace {
std::string get_ip(std::string_view address) {
  if (address.front() == '[') {
    auto pos = address.find(']');
    if (pos != std::string_view::npos) {
      return std::string{address.data() + 1, pos - 1};
    }
  }
  auto pos = address.rfind(':');
  if (pos == std::string_view::npos) pos = address.size();
  return std::string{address.data(), pos};
}

bool match_address(std::string_view address, const std::string &hostname) {
  auto ips = mysqlshdk::utils::Net::get_hostname_ips(hostname);

  if (address == hostname) return true;

  return std::find(ips.begin(), ips.end(), address) != ips.end();
}
}  // namespace

void test_self_connection(const mysqlshdk::mysql::IInstance &instance,
                          std::string_view local_address,
                          Cluster_ssl_mode ssl_mode,
                          Replication_auth_type member_auth,
                          std::string_view cert_issuer,
                          std::string_view cert_subject) {
  mysqlshdk::mysql::Set_variable slave_net_timeout(
      instance, "slave_net_timeout", 5, true);

  // create a temporary user without any grants
  auto password = create_temp_account(instance, "mysqlsh.test", member_auth,
                                      cert_issuer, cert_subject);
  shcore::Scoped_callback cleanup_user([&instance]() {
    mysqlshdk::mysql::Set_variable nobinlog(instance, "sql_log_bin", 0, false);

    instance.execute("DROP USER IF EXISTS `mysqlsh.test`@'%'");
  });

  std::string local_address_ip;
  if (!local_address.empty()) local_address_ip = get_ip(local_address);

  test_async_channel_connection(
      instance, instance, instance.get_canonical_hostname(), "report_host",
      ssl_mode, member_auth, "mysqlsh.test", password, cert_issuer,
      cert_subject);

  if (!local_address_ip.empty() &&
      !match_address(local_address_ip, instance.get_canonical_hostname())) {
    test_async_channel_connection(
        instance, instance, local_address_ip, "localAddress", ssl_mode,
        member_auth, "mysqlsh.test", password, cert_issuer, cert_subject);
  }
}

void test_peer_connection(const mysqlshdk::mysql::IInstance &from_instance,
                          std::string_view from_local_address,
                          std::string_view from_cert_subject,
                          const mysqlshdk::mysql::IInstance &to_instance,
                          std::string_view to_local_address,
                          std::string_view to_cert_subject,
                          Cluster_ssl_mode ssl_mode,
                          Replication_auth_type member_auth,
                          std::string_view cert_issuer, bool skip_self_check) {
  // We perform full tests from from to to, but only connectivity checks the
  // other way around

  std::string from_local_address_ip;
  if (!from_local_address.empty())
    from_local_address_ip = get_ip(from_local_address);

  shcore::Scoped_callback cleanup_user([&to_instance, &from_instance]() {
    {
      mysqlshdk::mysql::Set_variable nobinlog(from_instance, "sql_log_bin", 0,
                                              false);
      from_instance.execute("DROP USER IF EXISTS `mysqlsh.test`@'%'");
      from_instance.execute("DROP USER IF EXISTS `mysqlsh-lo.test`@'%'");
    }
    {
      mysqlshdk::mysql::Set_variable nobinlog(to_instance, "sql_log_bin", 0,
                                              false);
      to_instance.execute("DROP USER IF EXISTS `mysqlsh.test`@'%'");
    }
  });

  // create a temporary user without any grants
  // Note: this will fail at the primary of a replica cluster, in that case
  // we just skip the tests
  bool has_to_account = true;
  bool has_from_account = true;
  std::string to_password;
  std::string from_password;
  std::string from_local_password;

  log_debug("Creating test account at %s", to_instance.descr().c_str());
  try {
    from_password =
        create_temp_account(to_instance, "mysqlsh.test", member_auth,
                            cert_issuer, from_cert_subject);
  } catch (const shcore::Error &e) {
    if (e.code() != ER_OPTION_PREVENTS_STATEMENT) throw;
    log_info("Cannot create test account at %s: %s",
             to_instance.descr().c_str(), e.format().c_str());
    has_to_account = false;
  }
  log_debug("Creating test account at %s", from_instance.descr().c_str());
  try {
    to_password =
        create_temp_account(from_instance, "mysqlsh.test", member_auth,
                            cert_issuer, to_cert_subject);

    from_local_password =
        create_temp_account(from_instance, "mysqlsh-lo.test", member_auth,
                            cert_issuer, from_cert_subject);
  } catch (const shcore::Error &e) {
    if (e.code() != ER_OPTION_PREVENTS_STATEMENT) throw;
    log_info("Cannot create test account at %s: %s",
             from_instance.descr().c_str(), e.format().c_str());
    has_from_account = false;
  }

  {
    mysqlshdk::mysql::Set_variable slave_net_timeout1(
        from_instance, "slave_net_timeout", 5, true);

    if (!skip_self_check) {
      if (has_from_account)
        test_async_channel_connection(
            from_instance, from_instance,
            from_local_address_ip.empty()
                ? from_instance.get_canonical_hostname()
                : from_local_address_ip,
            "localAddress", ssl_mode, member_auth, "mysqlsh-lo.test",
            from_local_password, cert_issuer, from_cert_subject);
    }

    std::string local_address_ip;
    if (!to_local_address.empty()) local_address_ip = get_ip(to_local_address);

    if (has_to_account)
      test_async_channel_connection(
          from_instance, to_instance, to_instance.get_canonical_hostname(),
          "report_host", ssl_mode, member_auth, "mysqlsh.test", from_password,
          cert_issuer, from_cert_subject);

    if (!local_address_ip.empty() &&
        !match_address(local_address_ip,
                       to_instance.get_canonical_hostname())) {
      if (has_to_account)
        test_async_channel_connection(
            from_instance, to_instance, local_address_ip, "localAddress",
            ssl_mode, member_auth, "mysqlsh.test", from_password, cert_issuer,
            from_cert_subject);
    }
  }
  {
    mysqlshdk::mysql::Set_variable slave_net_timeout2(
        to_instance, "slave_net_timeout", 5, true);

    if (has_from_account)
      test_async_channel_connection(
          to_instance, from_instance, from_instance.get_canonical_hostname(),
          "report_host", ssl_mode, member_auth, "mysqlsh.test", to_password,
          cert_issuer, to_cert_subject);

    if (!from_local_address_ip.empty() &&
        !match_address(from_local_address_ip,
                       from_instance.get_canonical_hostname())) {
      if (has_from_account)
        test_async_channel_connection(
            to_instance, from_instance, from_local_address_ip, "localAddress",
            ssl_mode, member_auth, "mysqlsh.test", to_password, cert_issuer,
            to_cert_subject);
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
