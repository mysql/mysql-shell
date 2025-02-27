/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "modules/util/binlog/load_binlogs_options.h"

#include <iterator>
#include <string_view>
#include <utility>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"

#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/mysql/gtid_utils.h"
#include "mysqlshdk/libs/mysql/version_compatibility.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_uuid.h"

#include "modules/util/binlog/constants.h"

namespace mysqlsh {
namespace binlog {

namespace {

std::string validate_gtid(const std::string &gtid, const char *option) {
  if (gtid.empty()) {
    // WL15977-FR3.2.3.2
    throw std::invalid_argument{shcore::str_format(
        "The '%s' option cannot be set to an empty string.", option)};
  }

  const auto k_invalid_gtid = shcore::str_format(
      "Invalid value of the '%s' option, expected a GTID in format: "
      "source_id:[tag:]:transaction_id",
      option);

  // WL15977-FR3.2.3.1 - check if this is a single-transaction GTID

  using mysqlshdk::mysql::Gtid_range;
  using mysqlshdk::mysql::Gtid_set;

  Gtid_range range;
  Gtid_set::from_normalized_string(gtid).enumerate_ranges(
      [&range, &k_invalid_gtid](Gtid_range r) {
        if (!r || range) {
          // WL15977-FR3.2.3.2
          // invalid GTID or parsing has failed
          //   or
          // we already have a range, this means multiple intervals were given
          throw std::invalid_argument{k_invalid_gtid};
        }

        range = std::move(r);
      });

  if (!range || !range.is_single()) {
    // WL15977-FR3.2.3.2
    // parsing has failed
    //   or
    // we don't want an interval, but a single transaction ID
    throw std::invalid_argument{k_invalid_gtid};
  }

  return range.str();
}

}  // namespace

Load_binlogs_options::Load_binlogs_options()
    : Common_options(Common_options::Config{
          "util.loadBinlogs", "loading from a URL", true, false, true}) {}

const shcore::Option_pack_def<Load_binlogs_options>
    &Load_binlogs_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Load_binlogs_options>()
          .include<Common_options>()
          .optional("ignoreVersion", &Load_binlogs_options::m_ignore_version)
          .optional("ignoreGtidGap", &Load_binlogs_options::m_ignore_gtid_gap)
          .optional("stopBefore", &Load_binlogs_options::set_stop_before)
          .optional("stopAfter", &Load_binlogs_options::set_stop_after)
          .optional("dryRun", &Load_binlogs_options::m_dry_run)
          .on_done(&Load_binlogs_options::on_unpacked_options);

  return opts;
}

void Load_binlogs_options::on_set_url(
    const std::string &url, Storage_type storage,
    const mysqlshdk::storage::Config_ptr &config) {
  Common_options::on_set_url(url, storage, config);

  {
    const auto dump = make_directory(url);

    log_info("Validating dump directory: %s",
             dump->full_path().masked().c_str());

    if (!dump->exists()) {
      // WL15977-FR3.1.1.1
      throw std::invalid_argument{
          shcore::str_format("Directory '%s' does not exist.",
                             dump->full_path().masked().c_str())};
    }

    if (!dump->file(k_root_metadata_file)->exists()) {
      // WL15977-FR3.1.1.2
      throw std::invalid_argument{shcore::str_format(
          "Directory '%s' is not a valid binary log dump directory.",
          dump->full_path().masked().c_str())};
    }
  }
}

void Load_binlogs_options::set_stop_before(const std::string &stop_before) {
  m_stop_before_gtid = validate_gtid(stop_before, "stopBefore");
}

void Load_binlogs_options::set_stop_after(const std::string &stop_after) {
  m_stop_after_gtid = validate_gtid(stop_after, "stopAfter");
}

void Load_binlogs_options::on_unpacked_options() {
  if (!m_stop_before_gtid.empty() && !m_stop_after_gtid.empty()) {
    // WL15977-FR3.2.3.6
    throw std::invalid_argument{
        "The 'stopBefore' and 'stopAfter' options cannot be both set."};
  }
}

void Load_binlogs_options::read_dump(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dir) {
  m_dump = std::make_shared<Binlog_dump_info>(std::move(dir));

  for (const auto &dump : m_dump->dumps()) {
    if (dump.source_instance().version.number != m_target_version.number &&
        DBUG_EVALUATE_IF(
            "load_binlogs_unsupported_version", true,
            mysqlshdk::mysql::Replication_version_compatibility::INCOMPATIBLE ==
                mysqlshdk::mysql::verify_compatible_replication_versions(
                    dump.source_instance().version.number,
                    m_target_version.number))) {
      const auto incompatible_version = shcore::str_format(
          "Version of the source instance (%s) is incompatible with version of "
          "the target instance (%s).",
          dump.source_instance().version.number.get_base().c_str(),
          m_target_version.number.get_base().c_str());

      if (m_ignore_version) {
        // WL15977-FR3.2.1.1
        const auto console = current_console();
        console->print_warning(incompatible_version);
        console->print_note("The 'ignoreVersion' option is set, continuing.");
        // no need to check other dumps
        break;
      } else {
        // WL15977-FR3.3.3
        throw std::invalid_argument{shcore::str_format(
            "%s Enable the 'ignoreVersion' option to load anyway.",
            incompatible_version.c_str())};
      }
    }
  }

  if (const auto missing =
          session()
              ->queryf("SELECT GTID_SUBTRACT(?,?)",
                       m_dump->dumps().front().start_from().gtid_executed,
                       m_start_from.gtid_executed)
              ->fetch_one_or_throw()
              ->get_string(0);
      !missing.empty()) {
    const auto gtid_gap = shcore::str_format(
        "The target instance is missing some transactions which are not "
        "available in the dump: %s.",
        missing.c_str());

    if (m_ignore_gtid_gap) {
      // WL15977-FR3.2.2.1
      const auto console = current_console();
      console->print_warning(gtid_gap);
      console->print_note("The 'ignoreGtidGap' option is set, continuing.");
    } else {
      // WL15977-FR3.1.1.6
      throw std::invalid_argument{shcore::str_format(
          "%s Enable the 'ignoreGtidGap' option to load anyway.",
          gtid_gap.c_str())};
    }
  }
}

void Load_binlogs_options::on_set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  Common_options::on_set_session(session);

  mysqlsh::dba::Instance instance{session};

  if (!DBUG_EVALUATE_IF("load_binlogs_binlog_disabled", false,
                        instance.get_sysvar_bool("log_bin").value_or(false))) {
    // WL15977-FR3.3.1
    throw std::invalid_argument{
        "The binary logging on the target instance is disabled."};
  }

  if (const auto mode = DBUG_EVALUATE_IF(
          "load_binlogs_gtid_disabled", "OFF",
          instance.get_sysvar_string("gtid_mode").value_or("OFF"));
      !shcore::str_ibeginswith(mode, "ON")) {
    // WL15977-FR3.3.2
    throw std::invalid_argument{shcore::str_format(
        "The 'gtid_mode' system variable on the target instance is set to "
        "'%s'. This utility requires GTID support to be enabled.",
        mode.c_str())};
  }

  // fetch information about the target instance
  m_target_version = dump::common::server_version(session);
  m_start_from = dump::common::binlog(session, m_target_version);

  DBUG_EXECUTE_IF("load_binlogs_unsupported_version", {
    m_target_version.number = mysqlshdk::utils::Version(100, 0, 0);
  });
}

void Load_binlogs_options::on_configure() {
  Common_options::on_configure();

  read_dump(make_directory(url()));
  gather_binlogs();
}

void Load_binlogs_options::gather_binlogs() {
  // WL15977-FR3.1.2
  std::vector<Binlog_file> binlogs;
  std::vector<Binlog_file> candidates;
  bool begin_found = false;
  bool end_found = false;
  const auto &s = session();
  const auto &stop_at_gtid =
      !m_stop_before_gtid.empty() ? m_stop_before_gtid : m_stop_after_gtid;

  for (const auto &dump : m_dump->dumps()) {
    if (!begin_found) {
      // find the first dump which is not fully in gtid_executed (some or all
      // transactions were not loaded)
      if (s->queryf("SELECT GTID_SUBSET(?,?)", dump.gtid_set(),
                    m_start_from.gtid_executed)
              ->fetch_one_or_throw()
              ->get_uint(0)) {
        continue;
      }

      auto files = dump.binlog_files();

      // find the first binlog file which is not fully in gtid_executed
      for (auto it = files.begin(), end = files.end(); it != end; ++it) {
        if (!s->queryf("SELECT GTID_SUBSET(?,?)", it->gtid_set,
                       m_start_from.gtid_executed)
                 ->fetch_one_or_throw()
                 ->get_uint(0)) {
          m_first_gtid_set = s->queryf("SELECT GTID_SUBTRACT(?,?)",
                                       it->gtid_set, m_start_from.gtid_executed)
                                 ->fetch_one_or_throw()
                                 ->get_string(0);

          candidates.insert(candidates.end(), std::make_move_iterator(it),
                            std::make_move_iterator(end));
          begin_found = true;
          break;
        }
      }

      if (!begin_found) {
        throw std::invalid_argument{
            shcore::str_format("Could not find the starting binary log file in "
                               "the '%s' dump subdirectory.",
                               dump.directory_name().c_str())};
      }
    } else {
      candidates = dump.binlog_files();
    }

    if (!end_found) {
      if (!stop_at_gtid.empty()) {
        // check if dump contains the stopping GTID
        if (s->queryf("SELECT GTID_SUBSET(?,?)", stop_at_gtid, dump.gtid_set())
                ->fetch_one_or_throw()
                ->get_uint(0)) {
          // find the binlog which contains the stopping GTID
          for (auto it = candidates.begin(), end = candidates.end(); it != end;
               ++it) {
            if (s->queryf("SELECT GTID_SUBSET(?,?)", stop_at_gtid, it->gtid_set)
                    ->fetch_one_or_throw()
                    ->get_uint(0)) {
              candidates.erase(++it, end);
              end_found = true;
              break;
            }
          }

          if (!end_found) {
            throw std::invalid_argument{
                shcore::str_format("Could not find the final binary log file "
                                   "in the '%s' dump subdirectory.",
                                   dump.directory_name().c_str())};
          }
        }
      }
    }

    binlogs.insert(binlogs.end(), std::make_move_iterator(candidates.begin()),
                   std::make_move_iterator(candidates.end()));

    if (end_found) {
      break;
    }
  }

  if (!begin_found) {
    // all data from the dump is already loaded
    return;
  }

  if (!end_found) {
    if (!stop_at_gtid.empty()) {
      throw std::invalid_argument{
          shcore::str_format("Could not find the final binary log file to be "
                             "loaded that contains GTID '%s'.",
                             stop_at_gtid.c_str())};
    }
  }

  m_binlogs = std::move(binlogs);

  for (const auto &binlog : m_binlogs) {
    if (binlog.compressed) {
      m_has_compressed_binlogs = true;
      break;
    }
  }
}

}  // namespace binlog
}  // namespace mysqlsh
