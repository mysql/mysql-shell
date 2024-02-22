/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/mysql/async_replication.h"

#include <mysqld_error.h>
#include <tuple>
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace mysql {

namespace {
std::string prepare_ssl_options(const mysqlshdk::db::Ssl_options &ssl_options,
                                const std::string &source_term) {
  std::string options;

  if (ssl_options.has_mode()) {
    auto ssl_mode = ssl_options.get_mode();

    if (ssl_mode != mysqlshdk::db::Ssl_mode::Disabled) {
      options.append(", " + source_term + "_SSL=1");

      if (ssl_options.has_ca())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_CA=?",
                                         ssl_options.get_ca()));

      if (ssl_options.has_capath())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_CAPATH=?",
                                         ssl_options.get_capath()));

      if (ssl_options.has_crl())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_CRL=?",
                                         ssl_options.get_crl()));

      if (ssl_options.has_crlpath())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_CRLPATH=?",
                                         ssl_options.get_crlpath()));

      if (ssl_options.has_cipher())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_CIPHER=?",
                                         ssl_options.get_cipher()));

      if (ssl_options.has_cert())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_CERT=?",
                                         ssl_options.get_cert()));

      if (ssl_options.has_key())
        options.append(shcore::sqlformat(", " + source_term + "_SSL_KEY=?",
                                         ssl_options.get_key()));

      if (ssl_options.has_tls_version())
        options.append(shcore::sqlformat(", " + source_term + "_TLS_VERSION=?",
                                         ssl_options.get_tls_version()));

      if (ssl_options.has_tls_ciphersuites())
        options.append(
            shcore::sqlformat(", " + source_term + "_TLS_CIPHERSUITES=?",
                              ssl_options.get_tls_ciphersuites()));

      if (ssl_mode == mysqlshdk::db::Ssl_mode::VerifyIdentity)
        options.append(", " + source_term + "_SSL_VERIFY_SERVER_CERT=1");
    } else {
      options.append(", " + source_term + "_SSL=0");
    }
  }
  return options;
}
}  // namespace

void change_master(const mysqlshdk::mysql::IInstance &instance,
                   const std::string &master_host, int master_port,
                   const std::string &channel_name,
                   const Auth_options &credentials,
                   std::optional<int> master_connect_retry,
                   std::optional<int> master_retry_count,
                   std::optional<bool> auto_failover,
                   std::optional<int> master_delay, bool source_auto_position) {
  assert(!master_host.empty());
  assert(credentials.ssl_options.has_mode() &&
         credentials.ssl_options.get_mode() !=
             mysqlshdk::db::Ssl_mode::Preferred);
  log_info(
      "Setting up async source for channel '%s' of %s to %s:%i (user %s)",
      channel_name.c_str(), instance.descr().c_str(), master_host.c_str(),
      master_port,
      (credentials.user.empty() ? "unchanged" : "'" + credentials.user + "'")
          .c_str());

  try {
    auto [source_term, source_term_cmd] =
        mysqlshdk::mysql::get_replication_source_keywords(
            instance.get_version());

    std::string options;

    if (master_retry_count.has_value())
      options.append(shcore::str_format(
          ", %s_RETRY_COUNT=%d", source_term.c_str(), *master_retry_count));

    if (master_connect_retry.has_value())
      options.append(shcore::str_format(
          ", %s_CONNECT_RETRY=%d", source_term.c_str(), *master_connect_retry));

    if (auto_failover.has_value())
      options.append(shcore::str_format(", SOURCE_CONNECTION_AUTO_FAILOVER=%d",
                                        *auto_failover ? 1 : 0));

    if (master_delay.has_value())
      options.append(shcore::str_format(", %s_DELAY=%d", source_term.c_str(),
                                        *master_delay));

    options.append(prepare_ssl_options(credentials.ssl_options, source_term));

    auto ssl_mode = credentials.ssl_options.has_mode()
                        ? credentials.ssl_options.get_mode()
                        : mysqlshdk::db::Ssl_mode::Preferred;
    if (ssl_mode == mysqlshdk::db::Ssl_mode::Disabled ||
        ssl_mode == mysqlshdk::db::Ssl_mode::Preferred)
      options.append("/*!80011 , get_master_public_key=1 */");

    if (source_auto_position)
      options.append(", " + source_term + "_AUTO_POSITION=1");

    auto query = shcore::str_format(
        "CHANGE %s TO %s_HOST=/*(*/ ? /*)*/, %s_PORT=/*(*/ ? /*)*/, %s_USER=?, "
        "%s_PASSWORD=/*((*/ ? /*))*/%s FOR CHANNEL ?",
        source_term_cmd.c_str(), source_term.c_str(), source_term.c_str(),
        source_term.c_str(), source_term.c_str(), options.c_str());

    instance.executef(query, master_host, master_port, credentials.user,
                      credentials.password.value_or(""), channel_name);
  } catch (const std::exception &e) {
    log_error("Error setting up async replication: %s", e.what());
    throw;
  }
}

void change_master_host_port(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &master_host, int master_port,
                             const std::string &channel_name) {
  log_info("Changing source address for channel '%s' of %s to %s:%i",
           channel_name.c_str(), instance.descr().c_str(), master_host.c_str(),
           master_port);

  auto [source_term, source_term_cmd] =
      mysqlshdk::mysql::get_replication_source_keywords(instance.get_version());

  try {
    instance.executef("CHANGE " + source_term_cmd + " TO " + source_term +
                          "_HOST=/*(*/ ? /*)*/, " + source_term +
                          "_PORT=/*(*/ ? /*)*/ "
                          "FOR CHANNEL ?",
                      master_host, master_port, channel_name);
  } catch (const std::exception &e) {
    log_error("Error setting up async replication: %s", e.what());
    throw;
  }
}

void reset_slave(const mysqlshdk::mysql::IInstance &instance,
                 const std::string &channel_name, bool reset_credentials) {
  log_debug("Resetting replica%s channel '%s' for %s...",
            reset_credentials ? " ALL" : "", channel_name.c_str(),
            instance.descr().c_str());
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance.get_version());

  if (reset_credentials) {
    instance.executef("RESET " + replica_term + " ALL FOR CHANNEL ?",
                      channel_name);
  } else {
    instance.executef("RESET " + replica_term + " FOR CHANNEL ?", channel_name);
  }
}

void start_replication(const mysqlshdk::mysql::IInstance &instance,
                       const std::string &channel_name) {
  log_debug("Starting replica channel %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance.get_version());

  instance.executef("START " + replica_term + " FOR CHANNEL ?", channel_name);
}

void stop_replication(const mysqlshdk::mysql::IInstance &instance,
                      const std::string &channel_name) {
  log_debug("Stopping replica channel %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance.get_version());

  instance.executef("STOP " + replica_term + " FOR CHANNEL ?", channel_name);
}

bool stop_replication_safe(const mysqlshdk::mysql::IInstance &instance,
                           const std::string &channel_name, int timeout_sec) {
  log_debug("Stopping replica channel %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance.get_version());

  try {
    while (timeout_sec-- >= 0) {
      instance.executef("STOP " + replica_term + " FOR CHANNEL ?",
                        channel_name);

      auto n = instance.queryf_one_string(
          1, "0", "SHOW STATUS LIKE 'Slave_open_temp_tables'");

      if (n == "0") return true;

      log_warning(
          "Slave_open_tables has unexpected value %s at %s (unsupported SBR in "
          "use?)",
          n.c_str(), instance.descr().c_str());

      instance.executef("START " + replica_term + " FOR CHANNEL ?",
                        channel_name);

      shcore::sleep_ms(1000);
    }
  } catch (const shcore::Exception &e) {
#ifndef ER_REPLICA_CHANNEL_DOES_NOT_EXIST
#define ER_REPLICA_CHANNEL_DOES_NOT_EXIST ER_SLAVE_CHANNEL_DOES_NOT_EXIST
#endif
    if (e.code() == ER_REPLICA_CHANNEL_DOES_NOT_EXIST) {
      log_info("Trying to stop non-existing channel '%s', ignoring...",
               channel_name.c_str());
      return true;
    }
    throw;
  }

  return false;
}

void start_replication_receiver(const mysqlshdk::mysql::IInstance &instance,
                                const std::string &channel_name) {
  log_debug("Starting replica io_thread %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  instance.executef(
      "START " + mysqlshdk::mysql::get_replica_keyword(instance.get_version()) +
          " IO_THREAD FOR CHANNEL ?",
      channel_name);
}

void stop_replication_receiver(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name) {
  log_debug("Stopping replica io_thread %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  instance.executef(
      "STOP " + mysqlshdk::mysql::get_replica_keyword(instance.get_version()) +
          " IO_THREAD FOR CHANNEL ?",
      channel_name);
}

void start_replication_applier(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name) {
  log_debug("Starting replica sql_thread %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  instance.executef(
      "START " + mysqlshdk::mysql::get_replica_keyword(instance.get_version()) +
          " SQL_THREAD FOR CHANNEL ?",
      channel_name);
}

void stop_replication_applier(const mysqlshdk::mysql::IInstance &instance,
                              const std::string &channel_name) {
  log_debug("Stopping replica sql_thread %s for %s...", channel_name.c_str(),
            instance.descr().c_str());
  instance.executef(
      "STOP " + mysqlshdk::mysql::get_replica_keyword(instance.get_version()) +
          " SQL_THREAD FOR CHANNEL ?",
      channel_name);
}

void change_replication_credentials(
    const mysqlshdk::mysql::IInstance &instance, std::string_view channel,
    std::string_view user, const Replication_credentials_options &options) {
  auto [source_term, source_term_cmd] =
      mysqlshdk::mysql::get_replication_source_keywords(instance.get_version());

  // start of the statement
  std::string stmt_fmt;
  {
    auto fmt = shcore::str_format("CHANGE %s TO %s_USER = /*(*/ ? /*)*/",
                                  source_term_cmd.c_str(), source_term.c_str());
    stmt_fmt = (shcore::sqlstring{fmt, 0} << user).str();
  }

  // if the password needs to be included
  if (!options.password.empty()) {
    auto fmt = shcore::str_format(", %s_PASSWORD = /*((*/ ? /*))*/",
                                  source_term.c_str());
    stmt_fmt.append((shcore::sqlstring{fmt, 0} << options.password).str());
  }

  // add SSL options
  stmt_fmt.append(prepare_ssl_options(options.ssl_options, source_term));

  stmt_fmt.append((" FOR CHANNEL ?"_sql << channel).str());

  try {
    instance.execute(stmt_fmt);
  } catch (const std::exception &err) {
    throw std::runtime_error{
        shcore::str_format("Cannot set replication user of channel '%.*s' to "
                           "'%.*s'. Error executing CHANGE %s statement: %s",
                           static_cast<int>(channel.length()), channel.data(),
                           static_cast<int>(user.length()), user.data(),
                           source_term.c_str(), err.what())};
  }
}

}  // namespace mysql
}  // namespace mysqlshdk
