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

Load_binlogs_options::Load_binlogs_options()
    : Common_options(Common_options::Config{"util.loadBinlogs", true, false}) {}

const shcore::Option_pack_def<Load_binlogs_options>
    &Load_binlogs_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Load_binlogs_options>()
          .include<Common_options>()
          .optional("ignoreVersion", &Load_binlogs_options::m_ignore_version)
          .optional("ignoreGtidGap", &Load_binlogs_options::m_ignore_gtid_gap)
          .optional("stopBefore", &Load_binlogs_options::set_stop_before)
          .optional("dryRun", &Load_binlogs_options::m_dry_run);

  return opts;
}

void Load_binlogs_options::set_url(const std::string &url) {
  throw_on_url_and_storage_conflict(url, "loading from a URL");

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

  m_url = url;
}

void Load_binlogs_options::set_stop_before(const std::string &stop_before) {
  if (stop_before.empty()) {
    // WL15977-FR3.2.3.2
    throw std::invalid_argument{
        "The 'stopBefore' option cannot be set to an empty string."};
  }

  // WL15977-FR3.2.3.1:
  // parse the input string, format is <binlog-file>:<binlog-position>
  // this can also be a GTID: source_id:[tag:]transaction_id

  // both have the same general format, we can use the GTID parsing to verify if
  // format is correct, then decide if this a file or a GTID depending on
  // whether source_id matches the UUID format

  using mysqlshdk::mysql::Gtid_range;
  using mysqlshdk::mysql::Gtid_set;

  static constexpr auto k_invalid_binlog =
      "Invalid value of the 'stopBefore' option, expected: "
      "<binary-log-file>:<binary-log-file-position>.";
  static constexpr auto k_invalid_gtid =
      "Invalid value of the 'stopBefore' option, expected a GTID in format: "
      "source_id:[tag:]:transaction_id";

  Gtid_range range;
  Gtid_set::from_normalized_string(stop_before)
      .enumerate_ranges([&range](Gtid_range r) {
        if (!r) {
          // WL15977-FR3.2.3.2
          // invalid GTID or parsing has failed
          throw std::invalid_argument{k_invalid_binlog};
        }

        if (range) {
          // WL15977-FR3.2.3.2
          // we already have a range, this means multiple intervals were given
          throw std::invalid_argument{k_invalid_gtid};
        }

        range = std::move(r);
      });

  if (range) {
    if (!range.is_single()) {
      // WL15977-FR3.2.3.2
      // we don't want an interval, but a single transaction ID
      throw std::invalid_argument{k_invalid_gtid};
    }

    std::string_view uuid = range.uuid_tag;

    // range.uuid_tag may include a tag, remove it if it's there
    if (const auto pos = uuid.find_first_of(':');
        std::string_view::npos != pos) {
      uuid = uuid.substr(0, pos);
    }

    if (shcore::is_uuid(uuid)) {
      m_stop_at_gtid = range.str();
    } else {
      m_stop_at.name = std::move(range.uuid_tag);
      m_stop_at.position = range.begin;
    }
  } else {
    // WL15977-FR3.2.3.2
    // parsing has failed
    throw std::invalid_argument{k_invalid_binlog};
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

  read_dump(make_directory(m_url));
  gather_binlogs();
}

void Load_binlogs_options::gather_binlogs() {
  // WL15977-FR3.1.2
  std::vector<Binlog_file> binlogs;
  std::vector<Binlog_file> candidates;
  bool begin_found = false;
  bool end_found = false;
  const auto &s = session();

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
      if (!m_stop_at_gtid.empty()) {
        // check if dump contains the stopping GTID
        if (s->queryf("SELECT GTID_SUBSET(?,?)", m_stop_at_gtid,
                      dump.gtid_set())
                ->fetch_one_or_throw()
                ->get_uint(0)) {
          // find the binlog which contains the stopping GTID
          for (auto it = candidates.begin(), end = candidates.end(); it != end;
               ++it) {
            if (s->queryf("SELECT GTID_SUBSET(?,?)", m_stop_at_gtid,
                          it->gtid_set)
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
      } else if (!m_stop_at.name.empty()) {
        for (auto it = candidates.begin(), end = candidates.end(); it != end;
             ++it) {
          if (m_stop_at.name == it->name && m_stop_at.position >= it->begin &&
              ((0 == it->end &&
                m_stop_at.position <= it->data_bytes + it->begin) ||
               m_stop_at.position <= it->end)) {
            candidates.erase(++it, end);
            end_found = true;
            break;
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
    if (!m_stop_at_gtid.empty()) {
      throw std::invalid_argument{
          shcore::str_format("Could not find the final binary log file to be "
                             "loaded that contains GTID '%s'.",
                             m_stop_at_gtid.c_str())};
    } else if (!m_stop_at.name.empty()) {
      throw std::invalid_argument{
          shcore::str_format("Could not find the last binary log file to be "
                             "loaded '%s' that contains position %" PRIu64 ".",
                             m_stop_at.name.c_str(), m_stop_at.position)};
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
