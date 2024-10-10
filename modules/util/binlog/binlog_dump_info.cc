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

#include "modules/util/binlog/binlog_dump_info.h"

#include <iterator>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/storage/entry.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/binlog/constants.h"
#include "modules/util/common/dump/dump_version.h"
#include "modules/util/common/dump/utils.h"

namespace mysqlsh {
namespace binlog {

namespace {

using mysqlshdk::utils::Version;

std::string join_path(mysqlshdk::storage::IDirectory *dir,
                      const std::string &file) {
  return dir->join_path(dir->full_path().masked(), file);
}

}  // namespace

Binlog_dump::Binlog_dump(mysqlshdk::storage::IDirectory *root,
                         const std::string &dir_name)
    : m_root(root), m_dir_name(dir_name) {
  const auto dump = make_directory();
  m_full_path = dump->full_path().masked();

  if (!dump->file(k_dump_metadata_file)->exists()) {
    throw std::invalid_argument{shcore::str_format(
        "Directory '%s' is not a valid binary log dump directory.",
        dump->full_path().masked().c_str())};
  }

  if (!dump->file(k_done_metadata_file)->exists()) {
    // WL15977-FR2.4.1 + WL15977-FR3.1.1.4
    throw std::invalid_argument{
        shcore::str_format("Directory '%s' is incomplete and is not a valid "
                           "binary log dump directory.",
                           dump->full_path().masked().c_str())};
  }

  {
    const auto md = dump::common::fetch_json(dump->file(k_dump_metadata_file));

    try {
      read_begin_metadata(md);
    } catch (const std::exception &e) {
      throw std::runtime_error{shcore::str_format(
          "Failed to read '%s': %s",
          join_path(dump.get(), k_dump_metadata_file).c_str(), e.what())};
    }
  }

  if (m_start_from.gtid_executed.empty()) {
    // dump used the 'startFrom' option, the starting gtid_executed is computed
    // at the end of the dump
    const auto md = dump::common::fetch_json(dump->file(k_done_metadata_file));

    try {
      read_end_metadata(md);
    } catch (const std::exception &e) {
      throw std::runtime_error{shcore::str_format(
          "Failed to read '%s': %s",
          join_path(dump.get(), k_done_metadata_file).c_str(), e.what())};
    }
  }
}

void Binlog_dump::read_begin_metadata(const shcore::json::JSON &md) {
  m_version = Version{shcore::json::required(md, "version", false)};

  // WL15977-FR2.2.1.1.4 + WL15977-FR3.1.1.3
  dump::common::validate_dumper_version(m_version);

  m_source =
      dump::common::server_info(shcore::json::required_object(md, "source"));
  m_start_from =
      dump::common::binlog(shcore::json::required_object(md, "startFrom"));
  m_end_at = dump::common::binlog(shcore::json::required_object(md, "endAt"));
  m_gtid_set = shcore::json::required(md, "gtidSet", true);

  m_timestamp = shcore::json::required(md, "timestamp", false);

  m_binlogs = shcore::json::required_string_array(md, "binlogs");
  m_basenames = shcore::json::required_unordered_map(md, "basenames");
}

void Binlog_dump::read_end_metadata(const shcore::json::JSON &md) {
  m_start_from =
      dump::common::binlog(shcore::json::required_object(md, "startFrom"));
  m_gtid_set = shcore::json::required(md, "gtidSet", true);
}

std::vector<Binlog_file> Binlog_dump::binlog_files() const {
  std::vector<Binlog_file> result;
  const auto dump = make_directory();

  for (const auto &binlog : m_binlogs) {
    const auto &basename = m_basenames.at(binlog);
    const auto file_name = basename + ".json";
    const auto md = dump::common::fetch_json(dump->file(file_name));

    try {
      Binlog_file file;
      file.name = binlog;
      file.basename = basename;
      file.extension = shcore::json::required(md, "extension", true);
      file.compression = mysqlshdk::storage::to_compression(
          shcore::json::required(md, "compression", false));
      file.compressed =
          mysqlshdk::storage::Compression::NONE != file.compression;

      file.gtid_set = shcore::json::required(md, "gtidSet", true);
      file.data_bytes = shcore::json::required_uint(md, "dataBytes");
      file.file_bytes = shcore::json::required_uint(md, "fileBytes");
      file.events = shcore::json::required_uint(md, "eventCount");

      file.begin = shcore::json::optional_uint(md, "startPosition").value_or(0);
      file.end = shcore::json::optional_uint(md, "endPosition").value_or(0);

      file.dump = this;

      result.emplace_back(std::move(file));
    } catch (const std::exception &e) {
      throw std::runtime_error{shcore::str_format(
          "Failed to read '%s': %s", join_path(dump.get(), file_name).c_str(),
          e.what())};
    }
  }

  return result;
}

std::unique_ptr<mysqlshdk::storage::IDirectory> Binlog_dump::make_directory()
    const {
  return m_root->directory(m_dir_name);
}

std::unique_ptr<mysqlshdk::storage::IFile> Binlog_dump::make_file(
    const Binlog_file &file) const {
  return make_file(make_directory().get(), file);
}

std::unique_ptr<mysqlshdk::storage::IFile> Binlog_dump::make_file(
    mysqlshdk::storage::IDirectory *dir, const Binlog_file &file) {
  return mysqlshdk::storage::make_file(
      dir->file(std::string(file.basename) + file.extension,
                mysqlshdk::storage::mmap_options()),
      file.compression);
}

Binlog_dump_info::Binlog_dump_info(
    std::unique_ptr<mysqlshdk::storage::IDirectory> root)
    : m_root(std::move(root)) {
  {
    const auto md =
        dump::common::fetch_json(m_root->file(k_root_metadata_file));

    // WL15977-FR2.2.1.1.4 + WL15977-FR3.1.1.3
    dump::common::validate_dumper_version(
        Version{shcore::json::required(md, "version", false)});
  }

  std::list<Binlog_dump> dumps;

  for (const auto &subdir :
       mysqlshdk::storage::sort(m_root->list().directories)) {
    dumps.emplace_back(m_root.get(), subdir.name());
  }

  if (dumps.empty()) {
    throw std::invalid_argument{shcore::str_format(
        "The '%s' directory does not contain any binary log dumps.",
        m_root->full_path().masked().c_str())};
  }

  // sort the dumps and check continuity, dumps should already be sorted due to
  // subdirectory naming

  std::list<Binlog_dump> sorted_dumps;

  // move the first dump
  sorted_dumps.emplace_back(std::move(dumps.front()));
  dumps.pop_front();

  while (!dumps.empty()) {
    // find a dump that's before the first sorted or after the last sorted
    const auto size = dumps.size();

    for (auto it = dumps.begin(), end = dumps.end(); it != end;) {
      if (it->end_at().gtid_executed ==
          sorted_dumps.front().start_from().gtid_executed) {
        sorted_dumps.push_front(std::move(*it));
        it = dumps.erase(it);
      } else if (sorted_dumps.back().end_at().gtid_executed ==
                 it->start_from().gtid_executed) {
        sorted_dumps.push_back(std::move(*it));
        it = dumps.erase(it);
      } else {
        ++it;
      }
    }

    if (size == dumps.size()) {
      // WL15977-FR3.1.1.5
      throw std::invalid_argument{shcore::str_format(
          "The binary log dump in '%s' directory is not contiguous.",
          m_root->full_path().masked().c_str())};
    }
  }

  m_dumps.reserve(sorted_dumps.size());
  m_dumps.insert(m_dumps.end(), std::make_move_iterator(sorted_dumps.begin()),
                 std::make_move_iterator(sorted_dumps.end()));
}

}  // namespace binlog
}  // namespace mysqlsh
