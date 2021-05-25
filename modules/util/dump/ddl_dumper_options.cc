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

#include "modules/util/dump/ddl_dumper_options.h"

#include <vector>

#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/dump/compatibility_option.h"

namespace mysqlsh {
namespace dump {

using mysqlshdk::utils::expand_to_bytes;

namespace {

constexpr auto k_minimum_chunk_size = "128k";

// The chunk size will determine the size of transactions when loading them and
// GR has a default group_replication_transaction_size_limit of 143MB. Because
// chunk sizes will not be exact and can offshoot what's configured, we leave
// the default chunk size at 64MB to leave a healthy margin and be on the safe
// side.
constexpr auto k_default_chunk_size = "64M";

}  // namespace

Ddl_dumper_options::Ddl_dumper_options()
    : Dump_options(),
      m_bytes_per_chunk(expand_to_bytes(k_default_chunk_size)) {}

const shcore::Option_pack_def<Ddl_dumper_options>
    &Ddl_dumper_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Ddl_dumper_options>()
          .include<Dump_options>()
          .optional("chunking", &Ddl_dumper_options::m_split)
          .optional("bytesPerChunk", &Ddl_dumper_options::set_bytes_per_chunk)
          .optional("threads", &Ddl_dumper_options::m_threads)
          .optional("triggers", &Ddl_dumper_options::m_dump_triggers)
          .optional("tzUtc", &Ddl_dumper_options::m_timezone_utc)
          .optional("ddlOnly", &Ddl_dumper_options::m_ddl_only)
          .optional("dataOnly", &Ddl_dumper_options::m_data_only)
          .optional("dryRun", &Ddl_dumper_options::m_dry_run)
          .optional("consistent", &Ddl_dumper_options::m_consistent_dump)
          .optional("ocimds", &Ddl_dumper_options::set_ocimds)
          .optional("compatibility",
                    &Ddl_dumper_options::set_compatibility_options)
          .include(&Ddl_dumper_options::m_oci_option_unpacker)
          .on_done(&Ddl_dumper_options::on_unpacked_options);

  return opts;
}

void Ddl_dumper_options::set_ocimds(bool value) {
  if (value) {
    // When ocimds is enabled, the PAR generation default changes to true.
    // NOTE: This can still be disabled with the explicit OCI option which is
    // read after.
    m_oci_option_unpacker.set_par_manifest_default(value);

    set_mds_compatibility(mysqlshdk::utils::Version(MYSH_VERSION));
  }
}

void Ddl_dumper_options::set_compatibility_options(
    const std::vector<std::string> &options) {
  for (const auto &option : options) {
    set_compatibility_option(to_compatibility_option(option));
  }
}

void Ddl_dumper_options::on_unpacked_options() {
  set_oci_options(m_oci_option_unpacker);

  if (m_bytes_per_chunk < expand_to_bytes(k_minimum_chunk_size)) {
    throw std::invalid_argument(
        "The value of 'bytesPerChunk' option must be greater than or equal "
        "to " +
        std::string{k_minimum_chunk_size} + ".");
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

}  // namespace dump
}  // namespace mysqlsh
