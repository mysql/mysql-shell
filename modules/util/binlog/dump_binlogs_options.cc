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

#include "modules/util/binlog/dump_binlogs_options.h"

#include <mysql/binlog/event/binlog_event.h>
#include <mysql/binlog/event/control_events.h>
#include <mysql/gtid/gtid.h>
#include <mysql/gtid/gtidset.h>

#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"

#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/binary_log.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/binlog/binlog_dump_info.h"
#include "modules/util/binlog/constants.h"
#include "modules/util/binlog/utils.h"
#include "modules/util/common/dump/constants.h"
#include "modules/util/common/dump/dump_info.h"
#include "modules/util/common/dump/dump_version.h"
#include "modules/util/common/dump/utils.h"

namespace mysqlsh {
namespace binlog {

namespace {

bool verify_dump_instance_options(const shcore::json::Value &options) {
  if (options.ObjectEmpty()) {
    return true;
  }

  // these options need to be set to true, their default is true
  for (const auto option : {"events", "routines", "users", "triggers"}) {
    if (!shcore::json::optional_bool(options, option).value_or(true)) {
      return false;
    }
  }

  // these options need to be unset or empty
  for (const auto option : {"where", "partitions"}) {
    if (const auto o = shcore::json::optional_object(options, option);
        o.has_value() && !o->ObjectEmpty()) {
      return false;
    }
  }

  // these options need to be set to false, their default is false
  for (const auto option : {"ddlOnly", "dataOnly"}) {
    if (shcore::json::optional_bool(options, option).value_or(false)) {
      return false;
    }
  }

  // any of the non-empty include options means that dump is partial
  // excludeUsers falls into the same category
  for (const auto option : {
           "includeSchemas",
           "includeTables",
           "includeEvents",
           "includeRoutines",
           "includeUsers",
           "excludeUsers",
           "includeTriggers",
       }) {
    if (const auto a = shcore::json::optional_array(options, option);
        a.has_value() && !a->Empty()) {
      return false;
    }
  }

  {
    static constexpr auto k_system_schemas = {
        "INFORMATION_SCHEMA",
        "PERFORMANCE_SCHEMA",
        "mysql",
        "sys",
    };
    static constexpr auto k_system_schemas_begin = std::begin(k_system_schemas);
    static constexpr auto k_system_schemas_end = std::end(k_system_schemas);

    std::string schema;
    std::string table;
    std::string object;
    std::string *name;

    // if any of the exclude options contains a non-system object, dump is
    // partial
    for (const auto option : {
             "excludeSchemas",
             "excludeTables",
             "excludeEvents",
             "excludeRoutines",
             "excludeTriggers",
         }) {
      if (const auto a = shcore::json::optional_string_array(options, option);
          a.has_value() && !a->empty()) {
        for (const auto &excluded : *a) {
          shcore::split_schema_table_and_object(excluded, &schema, &table,
                                                &object);
          // first non-empty value is the schema name
          name =
              !schema.empty() ? &schema : (!table.empty() ? &table : &object);

          if (k_system_schemas_end ==
              std::find(k_system_schemas_begin, k_system_schemas_end, *name)) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

}  // namespace

Dump_binlogs_options::Dump_binlogs_options()
    : Common_options(Common_options::Config{
          "util.dumpBinlogs", "dumping to a URL", false, false, true}) {}

const shcore::Option_pack_def<Dump_binlogs_options>
    &Dump_binlogs_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_binlogs_options>()
          .include<Common_options>()
          .on_start(&Dump_binlogs_options::on_start_unpack)
          .optional("since", &Dump_binlogs_options::set_since)
          .optional("startFrom", &Dump_binlogs_options::set_start_from)
          .optional("ignoreDdlChanges",
                    &Dump_binlogs_options::m_ignore_ddl_changes)
          .optional("threads", &Dump_binlogs_options::set_threads)
          .optional("dryRun", &Dump_binlogs_options::m_dry_run)
          .optional("compression", &Dump_binlogs_options::set_compression)
          .on_done(&Dump_binlogs_options::on_unpacked_options);

  return opts;
}

std::unique_ptr<mysqlshdk::storage::IDirectory> Dump_binlogs_options::output()
    const {
  return make_directory(url());
}

void Dump_binlogs_options::on_start_unpack(
    const shcore::Dictionary_t &options) {
  m_options = options;
}

void Dump_binlogs_options::set_since(const std::string &since) {
  if (since.empty()) {
    throw std::invalid_argument{
        "The 'since' option cannot be set to an empty string."};
  }

  m_since = since;
}

void Dump_binlogs_options::set_start_from(const std::string &start_from) {
  if (start_from.empty()) {
    // WL15977-FR2.2.2.2
    throw std::invalid_argument{
        "The 'startFrom' option cannot be set to an empty string."};
  }

  // WL15977-FR2.2.2.1:
  // parse the input string, format is <binlog-file>[:<binlog-position>]
  if (const auto pos = start_from.find_last_of(':'); std::string::npos != pos) {
    m_start_from_binlog.file.name = start_from.substr(0, pos);

    try {
      // WL15977-FR2.2.2.5
      m_start_from_binlog.file.position = shcore::lexical_cast<uint64_t>(
          std::string_view{start_from}.substr(pos + 1));
    } catch (const std::exception &) {
      // WL15977-FR2.2.2.2
      throw std::invalid_argument{
          "The 'startFrom' option must be in the form "
          "<binlog-file>[:<binlog-position>], but binlog-position could not be "
          "converted to a number."};
    }
  } else {
    m_start_from_binlog.file.name = start_from;
    // WL15977-FR2.2.2.4
    m_start_from_binlog.file.position = 0;
  }

  m_start_from = start_from;
}

void Dump_binlogs_options::set_threads(uint64_t threads) {
  if (0 == threads) {
    // WL15977-FR2.2.5.1
    throw std::invalid_argument{
        "The value of 'threads' option must be greater than 0."};
  }

  m_threads = threads;
}

void Dump_binlogs_options::set_compression(const std::string &compression) {
  // WL15977-FR2.2.7.1
  if (compression.empty()) {
    throw std::invalid_argument{
        "The 'compression' option cannot be set to an empty string."};
  }

  m_compression =
      mysqlshdk::storage::to_compression(compression, &m_compression_options);
}

void Dump_binlogs_options::on_unpacked_options() {
  if (!m_since.empty() && !m_start_from.empty()) {
    // WL15977-FR2.2.2.6
    throw std::invalid_argument{
        "The 'since' and 'startFrom' options cannot be both set."};
  }

  if (m_ignore_ddl_changes.has_value() && m_since.empty()) {
    // WL15977-FR2.2.9.2
    throw std::invalid_argument{
        "The 'ignoreDdlChanges' option cannot be used when the 'since' option "
        "is not set."};
  }
}

void Dump_binlogs_options::on_validate() const {
  Common_options::on_validate();
  const auto fail_if_no_starting_point = [this](const auto &dir) {
    if (m_since.empty() && m_start_from.empty()) {
      // WL15977-FR2.2.3
      throw std::invalid_argument{shcore::str_format(
          "One of the 'since' or 'startFrom' options must be set because the "
          "destination directory '%s' does not contain any dumps yet.",
          dir.full_path().masked().c_str())};
    }
  };

  if (const auto dump = output(); dump->exists()) {
    if (dump->file(k_root_metadata_file)->exists()) {
      if (!m_since.empty()) {
        // WL15977-FR2.2.4
        throw std::invalid_argument{shcore::str_format(
            "The 'since' option cannot be used because '%s' points to a "
            "pre-existing binary log dump directory. You may omit that option "
            "to dump starting from the most recent entry there.",
            dump->full_path().masked().c_str())};
      }

      if (!m_start_from.empty()) {
        // WL15977-FR2.2.4
        throw std::invalid_argument{shcore::str_format(
            "The 'startFrom' option cannot be used because '%s' points to a "
            "pre-existing binary log dump directory. You may omit that option "
            "to dump starting from the most recent entry there.",
            dump->full_path().masked().c_str())};
      }
    } else if (!dump->is_empty()) {
      // WL15977-FR2.1.3
      throw std::invalid_argument{
          shcore::str_format("The '%s' points to an existing directory but it "
                             "does not contain a binary log dump.",
                             dump->full_path().masked().c_str())};
    } else {
      fail_if_no_starting_point(*dump);
    }
  } else {
    fail_if_no_starting_point(*dump);
  }

  if (!m_since.empty()) {
    const auto dump = make_directory(m_since);

    log_info("Using a dump directory as the starting point: %s",
             dump->full_path().masked().c_str());

    if (!dump->exists()) {
      // WL15977-FR2.2.1.1.1
      throw std::invalid_argument{shcore::str_format(
          "Directory '%s' specified in option 'since' does not exist.",
          dump->full_path().masked().c_str())};
    }
  }
}

void Dump_binlogs_options::on_configure() {
  Common_options::on_configure();

  // fetch information about the source instance
  initialize_source_instance();

  // WL15977-FR2.3.4
  verify_startup_options();

  // read information from the previous dump
  if (!m_since.empty()) {
    read_dump(make_directory(m_since));
  } else if (m_start_from.empty()) {
    read_dump_binlogs(output());
  }

  validate_continuity();
  validate_start_from();
  gather_binlogs();
}

void Dump_binlogs_options::read_dump(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dir) {
  if (dir->file(dump::common::k_root_metadata_file)->exists()) {
    read_dump_instance(std::move(dir));
  } else if (dir->file(k_root_metadata_file)->exists()) {
    read_dump_binlogs(std::move(dir));
  } else {
    // WL15977-FR2.2.1.1.2
    throw std::invalid_argument{
        shcore::str_format("Directory '%s' specified in option 'since' is not "
                           "a valid dump directory.",
                           dir->full_path().masked().c_str())};
  }
}

void Dump_binlogs_options::read_dump_instance(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dir) {
  const auto json =
      dump::common::fetch_json(dir->file(dump::common::k_root_metadata_file));
  auto dump_info = dump::common::dump_info(json);

  if ("dumpInstance" != dump_info.origin) {
    // WL15977-FR2.2.1.1.2
    throw std::invalid_argument{
        shcore::str_format("Directory '%s' specified in option 'since' is not "
                           "a valid dump directory.",
                           dir->full_path().masked().c_str())};
  }

  // we could check for this first, but we read the metadata file instead, to
  // make sure that dump was indeed created by dumpInstance (so the error
  // message is accurate)
  if (!dir->file(dump::common::k_done_metadata_file)->exists()) {
    // WL15977-FR2.4.1
    throw std::invalid_argument{shcore::str_format(
        "Directory '%s' specified in option 'since' is incomplete and is not a "
        "valid dump directory.",
        dir->full_path().masked().c_str())};
  }

  if (dump_info.source.topology.canonical_address.empty()) {
    // this information is only present in dumps created by Shell 9.2.0+
    // WL15977-FR2.2.1.1.3
    throw std::invalid_argument{
        "The 'since' option should be set to a directory which contains a dump "
        "created by MySQL Shell 9.2.0 or newer."};
  }

  // WL15977-FR2.2.1.1.4
  dump::common::validate_dumper_version(dump_info.version);

  if (const auto options = shcore::json::optional_object(json, "options");
      options.has_value()) {
    if (!verify_dump_instance_options(*options)) {
      // WL15977-FR2.2.1.1.5
      throw std::invalid_argument{
          "The 'since' option is set to a directory which contains a partial "
          "dump of an instance."};
    }
  }

  if (dump_info.mds_compatibility || !dump_info.compatibility_options.empty()) {
    constexpr auto k_modified_ddl =
        "The 'since' option is set to a directory which contains a dump with "
        "modified DDL.";

    if (m_ignore_ddl_changes.value_or(false)) {
      // WL15977-FR2.2.9.1
      const auto console = current_console();
      console->print_warning(k_modified_ddl);
      console->print_note("The 'ignoreDdlChanges' option is set, continuing.");
    } else {
      // WL15977-FR2.2.1.1.6
      throw std::invalid_argument{shcore::str_format(
          "%s Enable the 'ignoreDdlChanges' option to dump anyway.",
          k_modified_ddl)};
    }
  }

  if (!dump_info.consistent) {
    // WL15977-FR2.2.1.1.7
    throw std::invalid_argument{
        "The 'since' option is set to a directory which contains an "
        "inconsistent dump."};
  }

  // WL15977-FR2.2.1.2:
  m_previous_server_uuid = dump_info.source.sysvars.server_uuid;
  m_previous_version = std::move(dump_info.source.version);
  m_previous_topology = std::move(dump_info.source.topology);
  m_start_from_binlog = std::move(dump_info.source.binlog);

  // store information about the source dump
  m_previous_dump = dir->full_path().masked();
  m_previous_dump_timestamp = shcore::json::required(json, "begin", false);
}

void Dump_binlogs_options::read_dump_binlogs(
    std::unique_ptr<mysqlshdk::storage::IDirectory> dir) {
  const auto info = Binlog_dump_info{std::move(dir)};
  const auto &dump_info = info.dumps().back();

  // WL15977-FR2.1.4 + WL15977-FR2.2.1.2:
  m_previous_server_uuid = dump_info.source_instance().sysvars.server_uuid;
  m_previous_version = dump_info.source_instance().version;
  m_previous_topology = dump_info.source_instance().topology;
  m_start_from_binlog = dump_info.end_at();

  // store information about the source dump
  m_previous_dump = dump_info.full_path();
  m_previous_dump_timestamp = dump_info.timestamp();
}

void Dump_binlogs_options::on_set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  Common_options::on_set_session(session);

  mysqlshdk::mysql::Instance instance{session};

  if (!DBUG_EVALUATE_IF("dump_binlogs_binlog_disabled", false,
                        instance.get_sysvar_bool("log_bin").value_or(false))) {
    // WL15977-FR2.3.1
    throw std::invalid_argument{
        "The binary logging on the source instance is disabled."};
  }

  if (const auto mode = DBUG_EVALUATE_IF(
          "dump_binlogs_gtid_disabled", "OFF",
          instance.get_sysvar_string("gtid_mode").value_or("OFF"));
      !shcore::str_ibeginswith(mode, "ON")) {
    // WL15977-FR2.3.2
    throw std::invalid_argument{shcore::str_format(
        "The 'gtid_mode' system variable on the source instance is set to "
        "'%s'. This utility requires GTID support to be enabled.",
        mode.c_str())};
  }
}

void Dump_binlogs_options::verify_startup_options() const {
  const auto has_do_db =
      DBUG_EVALUATE_IF("dump_binlogs_has_do_db", true,
                       !m_source_instance.binlog.startup_options.do_db.empty());

  const auto has_ignore_db = DBUG_EVALUATE_IF(
      "dump_binlogs_has_ignore_db", true,
      !m_source_instance.binlog.startup_options.ignore_db.empty());

  if (has_do_db) {
    current_console()->print_note(
        "The source instance was started with the --binlog-do-db option.");
  }

  if (has_ignore_db) {
    current_console()->print_note(
        "The source instance was started with the --binlog-ignore-db option.");
  }

  if (has_do_db || has_ignore_db) {
    current_console()->print_note(
        "The binary log on the source instance is filtered and does not "
        "contain all transactions.");
  }
}

void Dump_binlogs_options::validate_start_from() {
  // gtid_executed is empty when the startFrom option was used
  if (start_from().gtid_executed.empty()) {
    return;
  }

  if (!session()
           ->queryf("SELECT GTID_SUBSET(?,?)", start_from().gtid_executed,
                    end_at().gtid_executed)
           ->fetch_one_or_throw()
           ->get_uint(0)) {
    // WL15977-FR2.4.3
    throw std::invalid_argument{
        "The base dump contains transactions that are not available in the "
        "source instance."};
  }

  m_gtid_set = session()
                   ->queryf("SELECT GTID_SUBTRACT(?,?)", end_at().gtid_executed,
                            start_from().gtid_executed)
                   ->fetch_one_or_throw()
                   ->get_string(0);

  // there is no gap if:
  //   (end_at() - start_from()) - gtid_purged == (end_at() - start_from())
  // in other words end_at() - start_from() cannot intersect with gtid_purged
  if (!session()
           ->queryf("SELECT GTID_SUBTRACT(?,@@GLOBAL.GTID_PURGED)=?",
                    m_gtid_set, m_gtid_set)
           ->fetch_one_or_throw()
           ->get_uint(0)) {
    // WL15977-FR2.4.4
    throw std::invalid_argument{
        "Some of the transactions to be dumped have already been purged."};
  }
}

void Dump_binlogs_options::gather_binlogs() {
  // WL15977-FR2.2.2.3 + WL15977-FR2.2.2.5.1 + WL15977-FR2.4.4
  std::vector<mysqlshdk::storage::File_info> binlogs;

  {
    const auto result = session()->query("SHOW BINARY LOGS");

    while (const auto row = result->fetch_one()) {
      binlogs.emplace_back(row->get_string(0), row->get_uint(1));
    }
  }

  find_start_from(binlogs);

  m_binlogs.reserve(binlogs.size());

  bool begin_found = false;
  bool end_found = false;

  for (auto &binlog : binlogs) {
    if (!begin_found && start_from().file.name == binlog.name()) {
      if (start_from().file.position > binlog.size()) {
        throw std::invalid_argument{shcore::str_format(
            "The starting binary log file '%s' does not contain the %" PRIu64
            " position.",
            start_from().file.name.c_str(), start_from().file.position)};
      }

      begin_found = true;
    }

    if (end_at().file.name == binlog.name()) {
      if (!begin_found) {
        throw std::invalid_argument{shcore::str_format(
            "Found final binary log file '%s' before finding the starting "
            "binary log file '%s'.",
            end_at().file.name.c_str(), start_from().file.name.c_str())};
      }

      if (end_at().file.position > binlog.size()) {
        throw std::invalid_argument{shcore::str_format(
            "The final binary log file '%s' does not contain the %" PRIu64
            " position.",
            end_at().file.name.c_str(), end_at().file.position)};
      }

      end_found = true;
    }

    if (begin_found) {
      m_binlogs.emplace_back(std::move(binlog));
    }

    if (end_found) {
      break;
    }
  }

  if (!begin_found) {
    throw std::invalid_argument{
        shcore::str_format("Could not find the starting binary log file '%s'.",
                           start_from().file.name.c_str())};
  }

  if (!end_found) {
    throw std::invalid_argument{
        shcore::str_format("Could not find the final binary log file '%s'.",
                           end_at().file.name.c_str())};
  }
}

void Dump_binlogs_options::initialize_source_instance() {
  m_source_instance.version = dump::common::server_version(session());
  m_source_instance.sysvars = dump::common::server_variables(session());
  m_source_instance.binlog =
      dump::common::binlog(session(), m_source_instance.version);
  m_source_instance.topology = dump::common::replication_topology(session());
}

void Dump_binlogs_options::find_start_from(
    const std::vector<mysqlshdk::storage::File_info> &binlogs) {
  // if dump is done from the same instance, we already have the correct binlog
  // file and position
  if (is_same_instance()) {
    return;
  }

  current_console()->print_note(shcore::str_format(
      "Previous dump was taken from '%s', current instance is: '%s', scanning "
      "binary log files for starting position...",
      m_previous_topology.canonical_address.c_str(),
      m_source_instance.topology.canonical_address.c_str()));

  // we need to scan the binlogs and find the first GTID event that does not
  // belong to the gtid_executed from the previous dump
  mysql::gtid::Gtid_set gtid_set;
  to_gtid_set(start_from().gtid_executed, &gtid_set);

  using Format_description_event =
      mysql::binlog::event::Format_description_event;
  using mysql::binlog::event::Gtid_event;
  using mysql::binlog::event::Log_event_type;
  using mysqlshdk::db::mysql::Binary_log;

  std::unique_ptr<Binary_log> prev_binlog;
  Format_description_event prev_description{BINLOG_VERSION, ""};

  std::unique_ptr<Binary_log> curr_binlog;
  Format_description_event curr_description{BINLOG_VERSION, ""};

  mysqlshdk::db::mysql::Binary_log_event event;
  Log_event_type type;
  const char *buffer;
  bool contains_gtid = true;

  const auto gtid_set_contains = [&gtid_set](const Gtid_event &e) {
    return gtid_set.contains(mysql::gtid::Gtid{e.get_tsid(), e.get_gno()});
  };

  // find a binlog where the first GTID event is in gtid_executed and which is
  // followed by a binlog where the first GTID event is not in gtid_executed,
  // scan only the former

  // TODO(pawel): use binary search to find the files

  for (const auto &file : binlogs) {
    curr_description = Format_description_event{BINLOG_VERSION, ""};
    curr_binlog = std::make_unique<Binary_log>(
        mysqlshdk::db::mysql::open_session(connection_options())
            ->release_connection(),
        file.name(), Binary_log::k_binlog_magic_size);

    bool has_transactions = false;

    while (true) {
      event = curr_binlog->next_event();

      if (!event.buffer) [[unlikely]] {
        break;
      }

      type = static_cast<Log_event_type>(event.type);
      buffer = reinterpret_cast<const char *>(event.buffer);

      if (Log_event_type::FORMAT_DESCRIPTION_EVENT == type) {
        curr_description = Format_description_event{buffer, &curr_description};
      } else if (Log_event_type::GTID_LOG_EVENT == type ||
                 Log_event_type::GTID_TAGGED_LOG_EVENT == type) {
        contains_gtid = gtid_set_contains({buffer, &curr_description});
        has_transactions = true;
        break;
      }
    }

    if (has_transactions) {
      if (contains_gtid) {
        // move to the next binlog, if this is the last one, curr_binlog will be
        // NULL
        prev_binlog = std::move(curr_binlog);
        prev_description = std::move(curr_description);
      } else {
        if (prev_binlog) {
          // check the previous binlog for transactions
          break;
        } else {
          // this is the first non-empty binlog and it has a transaction that is
          // not in gtid_executed, use this file, start from the beginning
          m_start_from_binlog.file.name = file.name();
          m_start_from_binlog.file.position = Binary_log::k_binlog_magic_size;
          return;
        }
      }
    }
  }

  if (!prev_binlog) {
    // all binlogs were empty
    throw std::runtime_error{
        "None of the binary log files contains the starting GTID."};
  }

  while (true) {
    event = prev_binlog->next_event();

    if (!event.buffer) [[unlikely]] {
      break;
    }

    type = static_cast<Log_event_type>(event.type);
    buffer = reinterpret_cast<const char *>(event.buffer);

    if (Log_event_type::FORMAT_DESCRIPTION_EVENT == type) {
      prev_description = Format_description_event{buffer, &prev_description};
    } else if (Log_event_type::GTID_LOG_EVENT == type ||
               Log_event_type::GTID_TAGGED_LOG_EVENT == type) {
      if (!gtid_set_contains({buffer, &prev_description})) {
        // we've found the first event that's not in gtid_executed
        m_start_from_binlog.file.name = prev_binlog->name();
        m_start_from_binlog.file.position = event.position;
        return;
      }
    }
  }

  // we've scanned the whole binlog file and all transactions were in
  // gtid_executed
  if (curr_binlog) {
    // start from the next binlog
    m_start_from_binlog.file.name = curr_binlog->name();
    m_start_from_binlog.file.position = Binary_log::k_binlog_magic_size;
  } else {
    // that was the last file, all transactions are in gtid_executed, there's
    // nothing to be dumped
    m_start_from_binlog.file = end_at().file;
  }
}

bool Dump_binlogs_options::is_same_instance() const noexcept {
  // if server_uuid of the previous dump is empty, we're dumping with startFrom
  return m_previous_server_uuid.empty() ||
         m_previous_server_uuid == m_source_instance.sysvars.server_uuid;
}

// WL15977-FR2.4.2
void Dump_binlogs_options::validate_continuity() const {
  if (m_previous_version.number &&
      m_previous_version.number.get_short() !=
          m_source_instance.version.number.get_short()) {
    throw std::invalid_argument{shcore::str_format(
        "Version of the source instance (%s) is incompatible with version of "
        "the instance used in the previous dump (%s).",
        m_source_instance.version.number.get_short().c_str(),
        m_previous_version.number.get_short().c_str())};
  }

  if (is_same_instance()) {
    return;
  }

  class Instance_error : public std::invalid_argument {
   public:
    explicit Instance_error(const char *msg)
        : invalid_argument(
              shcore::str_format("The source instance is different from the "
                                 "one used in the previous dump%s",
                                 msg)) {}

    explicit Instance_error(const std::string &msg)
        : Instance_error(msg.c_str()) {}
  };

  try {
    const auto &s = session();
    bool online = false;

    {
      const auto result = s->query(
          "SELECT MEMBER_STATE FROM "
          "performance_schema.replication_group_members WHERE "
          "MEMBER_ID=@@server_uuid");
      const auto row = result->fetch_one();

      if (!row) {
        throw Instance_error{shcore::str_format(
            " and it's not part of %s InnoDB Cluster group.",
            m_previous_topology.group_name.empty() ? "an" : "the same")};
      }

      const auto status = row->get_string(0);
      online = shcore::str_caseeq(status, "ONLINE");

      if (!online && shcore::str_caseeq(status, "OFFLINE")) {
        try {
          // check if this is a group member, provide a custom error if it's not
          mysqlsh::dba::Instance instance{s};
          mysqlsh::dba::MetadataStorage metadata{instance};
          if (mysqlsh::dba::Instance_type::GROUP_MEMBER !=
              metadata
                  .get_instance_by_uuid(m_source_instance.sysvars.server_uuid)
                  .instance_type) {
            throw Instance_error{
                " and is not an ONLINE member of the same InnoDB Cluster "
                "replication group."};
          }
        } catch (const Instance_error &) {
          // rethrow our exception
          throw;
        } catch (...) {
          // just use the error below
        }
      }
    }

    if (m_previous_topology.group_name.empty()) {
      // previous instance was standalone
      if (!online) {
        // this instance is not online, information is outdated
        throw Instance_error{
            " and is not an ONLINE member of an InnoDB Cluster group."};
      }

      // check if it was added to this group in the meantime
      if (!s->queryf(
                "SELECT 1 FROM performance_schema.replication_group_members "
                "WHERE MEMBER_ID=?",
                m_previous_server_uuid)
               ->fetch_one()) {
        // throw std::runtime_error in order use generic error message
        throw std::runtime_error{"Previous instance was standalone."};
      }
    } else {
      if (m_previous_topology.group_name !=
          m_source_instance.topology.group_name) {
        throw Instance_error{
            " and they do not belong to the same InnoDB Cluster group."};
      }

      if (!online) {
        throw Instance_error{
            ", it's part of the same InnoDB Cluster group, but is not "
            "currently ONLINE."};
      }
    }
  } catch (const Instance_error &) {
    // rethrow our exceptions
    throw;
  } catch (...) {
    // something went wrong, throw generic error message
    throw Instance_error{"."};
  }
}

}  // namespace binlog
}  // namespace mysqlsh
