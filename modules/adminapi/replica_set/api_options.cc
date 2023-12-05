/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/replica_set/api_options.h"

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "shellcore/shell_options.h"

namespace mysqlsh::dba::replicaset {

const shcore::Option_pack_def<Recovery_progress_option>
    &Recovery_progress_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Recovery_progress_option>().optional(
          kRecoveryProgress, &Recovery_progress_option::set_recovery_progress);

  return opts;
}

void Recovery_progress_option::set_recovery_progress(int value) {
  // Validate recoveryProgress option UInteger [0, 2]
  if (value < 0 || value > 2) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [0, 2].",
                           value, kRecoveryProgress));
  }

  switch (value) {
    case 0:
      m_recovery_progress = Recovery_progress_style::NOINFO;
      break;
    case 1:
      m_recovery_progress = Recovery_progress_style::TEXTUAL;
      break;
    case 2:
      m_recovery_progress = Recovery_progress_style::PROGRESSBAR;
      break;
  }
}

Recovery_progress_style Recovery_progress_option::get_recovery_progress() {
  if (!m_recovery_progress.has_value()) {
    m_recovery_progress = isatty(STDOUT_FILENO)
                              ? Recovery_progress_style::PROGRESSBAR
                              : Recovery_progress_style::TEXTUAL;
  }

  return *m_recovery_progress;
}

const shcore::Option_pack_def<Rejoin_instance_options>
    &Rejoin_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Rejoin_instance_options>()
          .include(&Rejoin_instance_options::clone_options)
          .optional(kDryRun, &Rejoin_instance_options::dry_run)
          .include<Recovery_progress_option>()
          .include<Timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Add_instance_options>
    &Add_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Add_instance_options>()
          .include<Rejoin_instance_options>()
          .optional(kLabel, &Add_instance_options::instance_label)
          .optional(kCertSubject, &Add_instance_options::set_cert_subject)
          .optional(
              shcore::str_format("replication%s", kReplicationConnectRetry),
              &Add_instance_options::set_repl_connect_retry)
          .optional(shcore::str_format("replication%s", kReplicationRetryCount),
                    &Add_instance_options::set_repl_retry_count)
          .optional(
              shcore::str_format("replication%s", kReplicationHeartbeatPeriod),
              &Add_instance_options::set_repl_heartbeat_period)
          .optional(shcore::str_format("replication%s",
                                       kReplicationCompressionAlgorithms),
                    &Add_instance_options::set_repl_compression_algos)
          .optional(shcore::str_format("replication%s",
                                       kReplicationZstdCompressionLevel),
                    &Add_instance_options::set_repl_zstd_compression_level)
          .optional(shcore::str_format("replication%s", kReplicationBind),
                    &Add_instance_options::set_repl_bind)
          .optional(
              shcore::str_format("replication%s", kReplicationNetworkNamespace),
              &Add_instance_options::set_repl_network_namespace);

  return opts;
}

void Add_instance_options::set_cert_subject(const std::string &value) {
  if (value.empty())
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for '%s' option. Value cannot be an empty string.",
        kCertSubject));

  cert_subject = value;
}

void Add_instance_options::set_repl_connect_retry(int value) {
  if (value < 0)
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for 'replication%s' option. Value "
                           "cannot be negative.",
                           kReplicationConnectRetry));

  ar_options.connect_retry = value;
}

void Add_instance_options::set_repl_retry_count(int value) {
  if (value < 0)
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for 'replication%s' option. Value "
                           "cannot be negative.",
                           kReplicationRetryCount));

  ar_options.retry_count = value;
}

void Add_instance_options::set_repl_heartbeat_period(double value) {
  if (value < 0.0)
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for 'replication%s' option. Value "
                           "cannot be negative.",
                           kReplicationHeartbeatPeriod));

  ar_options.heartbeat_period = value;
}

void Add_instance_options::set_repl_compression_algos(
    const std::string &value) {
  ar_options.compression_algos = value;
}

void Add_instance_options::set_repl_zstd_compression_level(int value) {
  if (value < 0)
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for 'replication%s' option. Value "
                           "cannot be negative.",
                           kReplicationZstdCompressionLevel));

  ar_options.zstd_compression_level = value;
}

void Add_instance_options::set_repl_bind(const std::string &value) {
  ar_options.bind = value;
}

void Add_instance_options::set_repl_network_namespace(
    const std::string &value) {
  ar_options.network_namespace = value;
}

const shcore::Option_pack_def<Gtid_wait_timeout_option>
    &Gtid_wait_timeout_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Gtid_wait_timeout_option>().optional(
          kTimeout, &Gtid_wait_timeout_option::m_timeout);

  return opts;
}

int Gtid_wait_timeout_option::timeout() const {
  return m_timeout.value_or(
      current_shell_options()->get().dba_gtid_wait_timeout);
}

const shcore::Option_pack_def<Remove_instance_options>
    &Remove_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Remove_instance_options>()
          .optional(kForce, &Remove_instance_options::force)
          .include<Gtid_wait_timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Status_options> &Status_options::options() {
  static const auto opts = shcore::Option_pack_def<Status_options>().optional(
      kExtended, &Status_options::set_extended);

  return opts;
}

void Status_options::set_extended(int value) {
  // Validate extended option Integer [0, 2] or Boolean.
  if (value < 0 || value > 2) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value '%d' for option '%s'. It must be an integer in the "
        "range [0, 2].",
        value, kExtended));
  }

  extended = value;
}

const shcore::Option_pack_def<Set_primary_instance_options>
    &Set_primary_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Set_primary_instance_options>()
          .optional(kDryRun, &Set_primary_instance_options::dry_run)
          .include<Gtid_wait_timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Force_primary_instance_options>
    &Force_primary_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Force_primary_instance_options>()
          .include<Set_primary_instance_options>()
          .optional(kInvalidateErrorInstances,
                    &Force_primary_instance_options::invalidate_instances);

  return opts;
}

const shcore::Option_pack_def<Dissolve_options> &Dissolve_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dissolve_options>()
          .optional(kForce, &Dissolve_options::force)
          .optional(kTimeout, &Dissolve_options::set_timeout);

  return opts;
}

std::chrono::seconds Dissolve_options::timeout() const {
  if (m_timeout.has_value()) return m_timeout.value();

  return std::chrono::seconds{
      current_shell_options()->get().dba_gtid_wait_timeout};
}

void Dissolve_options::set_timeout(int timeout) {
  if (timeout < 0)
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value '%d' for option '%s'. It must be a positive integer "
        "representing the maximum number of seconds to wait.",
        timeout, kTimeout));

  m_timeout = std::chrono::seconds{timeout};
}

const shcore::Option_pack_def<Rescan_options> &Rescan_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Rescan_options>()
          .optional(kAddUnmanaged, &Rescan_options::add_unmanaged)
          .optional(kRemoveObsolete, &Rescan_options::remove_obsolete);

  return opts;
}

}  // namespace mysqlsh::dba::replicaset
