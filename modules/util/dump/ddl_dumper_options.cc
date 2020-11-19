/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

Ddl_dumper_options::Ddl_dumper_options(const std::string &output_url)
    : Dump_options(output_url),
      m_bytes_per_chunk(expand_to_bytes(k_default_chunk_size)) {}

void Ddl_dumper_options::unpack_options(shcore::Option_unpacker *unpacker) {
  mysqlshdk::db::nullable<std::string> bytes_per_chunk;
  std::vector<std::string> compatibility_options;
  bool mds = false;

  unpacker->optional("chunking", &m_split)
      .optional("bytesPerChunk", &bytes_per_chunk)
      .optional("threads", &m_threads)
      .optional("triggers", &m_dump_triggers)
      .optional("tzUtc", &m_timezone_utc)
      .optional("ddlOnly", &m_ddl_only)
      .optional("dataOnly", &m_data_only)
      .optional("dryRun", &m_dry_run)
      .optional("consistent", &m_consistent_dump)
      .optional("ocimds", &mds)
      .optional("compatibility", &compatibility_options);

  if (bytes_per_chunk) {
    if (bytes_per_chunk->empty()) {
      throw std::invalid_argument(
          "The option 'bytesPerChunk' cannot be set to an empty string.");
    }

    if (!split()) {
      throw std::invalid_argument(
          "The option 'bytesPerChunk' cannot be used if the 'chunking' option "
          "is set to false.");
    }

    m_bytes_per_chunk = expand_to_bytes(*bytes_per_chunk);
  }

  if (mds) {
    set_mds_compatibility(mysqlshdk::utils::Version(MYSH_VERSION));
  }

  for (const auto &option : compatibility_options) {
    m_compatibility_options |= to_compatibility_option(option);
  }
}

void Ddl_dumper_options::validate_options() const {
  if (m_bytes_per_chunk < expand_to_bytes(k_minimum_chunk_size)) {
    throw std::invalid_argument(
        "The value of 'bytesPerChunk' option must be greater or equal to " +
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
}

}  // namespace dump
}  // namespace mysqlsh
