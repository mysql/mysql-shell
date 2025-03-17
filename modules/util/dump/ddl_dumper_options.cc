/*
 * Copyright (c) 2020, 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/ddl_dumper_options.h"

#include <array>
#include <vector>

#include "modules/util/common/dump/utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/storage/backend/oci_par_directory_config.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/compatibility.h"
#include "modules/util/dump/compatibility_option.h"

namespace mysqlsh {
namespace dump {

using mysqlshdk::utils::expand_to_bytes;
using mysqlshdk::utils::Version;

namespace {

constexpr auto k_minimum_chunk_size = "128k";

// The chunk size will determine the size of transactions when loading them and
// GR has a default group_replication_transaction_size_limit of 143MB. Because
// chunk sizes will not be exact and can offshoot what's configured, we leave
// the default chunk size at 64MB to leave a healthy margin and be on the safe
// side.
constexpr auto k_default_chunk_size = "64M";

}  // namespace

Ddl_dumper_options::Ddl_dumper_options(const char *name)
    : Dump_options(name),
      m_bytes_per_chunk(expand_to_bytes(k_default_chunk_size)) {}

const shcore::Option_pack_def<Ddl_dumper_options>
    &Ddl_dumper_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Ddl_dumper_options>()
          .include<Dump_options>()
          .optional("chunking", &Ddl_dumper_options::m_split)
          .optional("bytesPerChunk", &Ddl_dumper_options::set_bytes_per_chunk)
          .optional("threads", &Ddl_dumper_options::set_threads)
          .optional("triggers", &Ddl_dumper_options::m_dump_triggers)
          .optional("tzUtc", &Ddl_dumper_options::m_timezone_utc)
          .optional("ddlOnly", &Ddl_dumper_options::m_ddl_only)
          .optional("dataOnly", &Ddl_dumper_options::m_data_only)
          .optional("dryRun", &Ddl_dumper_options::set_dry_run)
          .optional("consistent", &Ddl_dumper_options::m_consistent_dump)
          .optional("skipConsistencyChecks",
                    &Ddl_dumper_options::m_skip_consistency_checks)
          .optional("ocimds", &Ddl_dumper_options::set_ocimds)
          .optional("compatibility",
                    &Ddl_dumper_options::set_compatibility_options)
          .optional("targetVersion",
                    &Ddl_dumper_options::set_target_version_str)
          .optional("skipUpgradeChecks",
                    &Ddl_dumper_options::m_skip_upgrade_checks)
          .include(&Ddl_dumper_options::m_filtering_options,
                   &mysqlshdk::db::Filtering_options::triggers)
          .optional("where", &Ddl_dumper_options::set_where_clause)
          .optional("partitions", &Ddl_dumper_options::set_partitions)
          .optional("checksum", &Ddl_dumper_options::m_checksum)
          .optional("lakehouseTarget",
                    &Ddl_dumper_options::set_lakehouse_target)
          .on_done(&Ddl_dumper_options::on_unpacked_options);

  return opts;
}

void Ddl_dumper_options::set_ocimds(bool value) {
  if (value) {
    enable_mds_compatibility_checks();
  }
}

void Ddl_dumper_options::set_compatibility_options(
    const std::vector<std::string> &options) {
  for (const auto &option : options) {
    set_compatibility_option(to_compatibility_option(option));
  }
}

void Ddl_dumper_options::set_target_version_str(const std::string &value) {
  if (value.empty()) {
    throw std::invalid_argument(
        "Invalid value of the 'targetVersion' option: empty");
  }

  Version ver;

  try {
    ver = Version(value);
  } catch (const std::exception &e) {
    throw std::invalid_argument(
        "Invalid value of the 'targetVersion' option: '" + value + "', " +
        e.what());
  }

  set_target_version(ver);
}

void Ddl_dumper_options::on_unpacked_options() {
  if (m_bytes_per_chunk < expand_to_bytes(k_minimum_chunk_size)) {
    throw std::invalid_argument(
        "The value of 'bytesPerChunk' option must be greater than or equal "
        "to " +
        std::string(k_minimum_chunk_size) + ".");
  }

  if (0 == m_threads) {
    throw std::invalid_argument(
        "The value of 'threads' option must be greater than 0.");
  }

  if (m_ddl_only && m_data_only) {
    throw std::invalid_argument(
        "The 'ddlOnly' and 'dataOnly' options cannot be both set to true.");
  }

  if (compatibility_options().is_set(
          Compatibility_option::CREATE_INVISIBLE_PKS) &&
      compatibility_options().is_set(
          Compatibility_option::IGNORE_MISSING_PKS)) {
    throw std::invalid_argument(shcore::str_format(
        "The '%s' and '%s' compatibility options cannot be used at the same "
        "time.",
        to_string(Compatibility_option::CREATE_INVISIBLE_PKS).c_str(),
        to_string(Compatibility_option::IGNORE_MISSING_PKS).c_str()));
  }

  // WL16802-FR1.2.3: 'lakehouseTarget' is set, but 'ocimds' is false
  if (has_lakehouse_target() && !mds_compatibility()) {
    throw std::invalid_argument(
        "The 'ocimds' option must be enabled when using the 'lakehouseTarget' "
        "option.");
  }

  m_filter_conflicts |= filters().triggers().error_on_conflicts();
}

void Ddl_dumper_options::set_bytes_per_chunk(const std::string &value) {
  if (value.empty()) {
    throw std::invalid_argument(
        "The option 'bytesPerChunk' cannot be set to an empty string.");
  }

  if (!split()) {
    throw std::invalid_argument(
        "The option 'bytesPerChunk' cannot be used if the 'chunking' "
        "option is set to false.");
  }

  m_bytes_per_chunk = expand_to_bytes(value);
}

void Ddl_dumper_options::enable_mds_compatibility_checks() {
  enable_mds_compatibility();
}

void Ddl_dumper_options::set_dry_run(bool dry_run) {
  set_dry_run_mode(dry_run ? Dry_run::DONT_WRITE_ANY_FILES : Dry_run::DISABLED);
}

void Ddl_dumper_options::set_threads(uint64_t threads) {
  m_threads = threads;

  // By default, m_worker_threads is equal to m_threads
  m_worker_threads = threads;
}

void Ddl_dumper_options::on_set_url(
    const std::string &url, Storage_type storage,
    const mysqlshdk::storage::Config_ptr &config) {
  Dump_options::on_set_url(url, storage, config);

  if (Storage_options::Storage_type::Oci_prefix_par == storage) {
    // For dumps with PAR prefix, doubles the number of worker threads
    m_worker_threads = m_threads * 2;
  } else if (Storage_options::Storage_type::Oci_par == storage ||
             Storage_options::Storage_type::Http == storage) {
    current_console()->print_warning("The given URL is not a prefix PAR.");
  }
}

void Ddl_dumper_options::on_configure() {
  Dump_options::on_configure();

  // WL16802-FR1.1: special handling of InnoDB vector store tables is enabled
  // only when dumping with 'ocimds': true
  if (!mds_compatibility()) {
    disable_innodb_vector_store_tables_handling();
  }
}

}  // namespace dump
}  // namespace mysqlsh
