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

#ifndef MODULES_UTIL_LOAD_SCHEMA_LOAD_PROGRESS_LOG_H_
#define MODULES_UTIL_LOAD_SCHEMA_LOAD_PROGRESS_LOG_H_

#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "modules/util/load/load_errors.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {

class Load_progress_log final {
 public:
  enum Status { PENDING, INTERRUPTED, DONE };

  struct Progress_status {
    Status status;
    uint64_t bytes_completed;
    uint64_t raw_bytes_completed;
  };

  Progress_status init(std::unique_ptr<mysqlshdk::storage::IFile> file,
                       bool dry_run, bool rewrite_on_flush) {
    mysqlshdk::storage::IFile *existing_file = file.get();

    // rewrite_on_flush is meant for storage backends that do not support
    // neither appending nor flushing partially written contents (e.g. REST
    // based storage services). In that case, we write to an in-memory file
    // and every time we need to flush, we rewrite the entire file remotely.
    if (rewrite_on_flush) {
      m_real_file = std::move(file);
      auto mem_file =
          std::make_unique<mysqlshdk::storage::backend::Memory_file>("");
      m_memfile_contents = &mem_file->content();
      m_file = std::move(mem_file);
    } else {
      m_file = std::move(file);
    }

    Status status;
    std::string data;
    uint64_t bytes_completed = 0;
    uint64_t raw_bytes_completed = 0;

    if (existing_file && existing_file->exists()) {
      existing_file->open(mysqlshdk::storage::Mode::READ);
      data = mysqlshdk::storage::read_file(existing_file);
      existing_file->close();

      try {
        shcore::str_itersplit(
            data,
            [this, &bytes_completed,
             &raw_bytes_completed](const std::string &line) -> bool {
              if (!shcore::str_strip(line).empty()) {
                shcore::Value doc = shcore::Value::parse(line);
                shcore::Dictionary_t entry = doc.as_map();

                bool done = entry->get_int("done") != 0;

                std::string key = entry->get_string("op");

                if (entry->has_key("schema"))
                  key += ":`" + entry->get_string("schema") + "`";

                if (entry->has_key("table"))
                  key += ":`" + entry->get_string("table") + "`";

                if (entry->has_key("partition"))
                  key += ":`" + entry->get_string("partition") + "`";

                if (entry->has_key("chunk"))
                  key += ":" + std::to_string(entry->get_int("chunk"));

                if (entry->has_key("subchunk"))
                  key += ":" + std::to_string(entry->get_int("subchunk"));

                auto iter = m_last_state.find(key);
                if (iter == m_last_state.end() || !done) {
                  m_last_state.emplace(key, Status_details{Status::INTERRUPTED,
                                                           std::move(entry)});
                } else {
                  if (entry->has_key("bytes"))
                    bytes_completed += entry->get_uint("bytes");

                  if (entry->has_key("raw_bytes"))
                    raw_bytes_completed += entry->get_uint("raw_bytes");

                  iter->second.status = Status::DONE;
                  iter->second.details = std::move(entry);
                }
              }
              return true;
            },
            "\n");
      } catch (const std::exception &e) {
        THROW_ERROR(SHERR_LOAD_PROGRESS_FILE_ERROR,
                    existing_file->full_path().masked().c_str(), e.what());
      }
    }

    status = m_last_state.empty() ? Status::PENDING : Status::INTERRUPTED;

    if (dry_run) {
      m_file.reset();
      m_real_file.reset();
    } else {
      m_file->open(mysqlshdk::storage::Mode::WRITE);
      if (!data.empty()) {
        m_file->write(data.data(), data.size());
        m_file->write("\n", 1);  // separator for new attempt
        flush();
      }
    }
    return {status, bytes_completed, raw_bytes_completed};
  }

  void reset_progress() {
    if (m_file) {
      m_file->close();
      m_file->remove();

      // m_real_file can be present only if m_file is present
      if (m_real_file) {
        m_real_file->remove();

        auto mem_file =
            std::make_unique<mysqlshdk::storage::backend::Memory_file>("");
        m_memfile_contents = &mem_file->content();
        m_file = std::move(mem_file);
      }

      m_file->open(mysqlshdk::storage::Mode::WRITE);
    }

    m_last_state.clear();
  }

  void cleanup() {
    flush();

    if (m_file) {
      m_file->close();
    }
  }

  Status schema_ddl_status(const std::string &schema) const {
    auto it = m_last_state.find("SCHEMA-DDL:`" + schema + "`");
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second.status;
  }

  Status triggers_ddl_status(const std::string &schema,
                             const std::string &table) const {
    auto it =
        m_last_state.find("TRIGGERS-DDL:`" + schema + "`:`" + table + "`");
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second.status;
  }

  Status table_ddl_status(const std::string &schema,
                          const std::string &table) const {
    auto it = m_last_state.find("TABLE-DDL:`" + schema + "`:`" + table + "`");
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second.status;
  }

  Status table_chunk_status(const std::string &schema, const std::string &table,
                            const std::string &partition, ssize_t chunk) const {
    auto it = m_last_state.find("TABLE-DATA:`" + schema + "`:`" + table +
                                (partition.empty() ? "" : "`:`" + partition) +
                                "`:" + std::to_string(chunk));
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second.status;
  }

  Status table_subchunk_status(const std::string &schema,
                               const std::string &table,
                               const std::string &partition, ssize_t chunk,
                               uint64_t subchunk) const {
    auto it = m_last_state.find(
        encode_subchunk(schema, table, partition, chunk, subchunk));
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second.status;
  }

  uint64_t table_subchunk_size(const std::string &schema,
                               const std::string &table,
                               const std::string &partition, ssize_t chunk,
                               uint64_t subchunk) const {
    auto it = m_last_state.find(
        encode_subchunk(schema, table, partition, chunk, subchunk));
    if (it == m_last_state.end()) return 0;
    return it->second.details->get_uint("transaction_bytes");
  }

  Status gtid_update_status() const {
    auto it = m_last_state.find("GTID-UPDATE");
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second.status;
  }

  std::string server_uuid() const {
    auto it = m_last_state.find("SERVER-UUID");
    if (it == m_last_state.end()) return {};
    return it->second.details->get_string("uuid");
  }

  void set_server_uuid(const std::string &uuid) {
    const auto saved_uuid = server_uuid();

    if (saved_uuid.empty()) {
      log(true, "SERVER-UUID", {}, {}, {},
          [&](Dumper *json) { json->append_string("uuid", uuid); });
    } else if (!shcore::str_caseeq(uuid, saved_uuid)) {
      THROW_ERROR(SHERR_LOAD_PROGRESS_FILE_UUID_MISMATCH, saved_uuid.c_str(),
                  uuid.c_str());
    }
  }

  // schema DDL includes the schema script and views
  void start_schema_ddl(const std::string &schema) {
    if (schema_ddl_status(schema) != Status::DONE)
      log(false, "SCHEMA-DDL", schema);
  }

  void end_schema_ddl(const std::string &schema) {
    if (schema_ddl_status(schema) != Status::DONE)
      log(true, "SCHEMA-DDL", schema);
  }

  void start_table_ddl(const std::string &schema, const std::string &table) {
    if (table_ddl_status(schema, table) != Status::DONE)
      log(false, "TABLE-DDL", schema, table);
  }

  void end_table_ddl(const std::string &schema, const std::string &table) {
    if (table_ddl_status(schema, table) != Status::DONE)
      log(true, "TABLE-DDL", schema, table);
  }

  void start_triggers_ddl(const std::string &schema, const std::string &table) {
    if (triggers_ddl_status(schema, table) != Status::DONE)
      log(false, "TRIGGERS-DDL", schema, table);
  }

  void end_triggers_ddl(const std::string &schema, const std::string &table) {
    if (triggers_ddl_status(schema, table) != Status::DONE)
      log(true, "TRIGGERS-DDL", schema, table);
  }

  void start_table_chunk(const std::string &schema, const std::string &table,
                         const std::string &partition, ssize_t index) {
    if (table_chunk_status(schema, table, partition, index) != Status::DONE)
      log_chunk_started(schema, table, partition, index);
  }

  void end_table_chunk(const std::string &schema, const std::string &table,
                       const std::string &partition, ssize_t index,
                       size_t bytes_loaded, size_t raw_bytes_loaded) {
    if (table_chunk_status(schema, table, partition, index) != Status::DONE)
      log_chunk_finished(schema, table, partition, index, bytes_loaded,
                         raw_bytes_loaded);
  }

  void start_table_subchunk(const std::string &schema, const std::string &table,
                            const std::string &partition, ssize_t index,
                            uint64_t subchunk) {
    if (table_subchunk_status(schema, table, partition, index, subchunk) !=
        Status::DONE) {
      log_subchunk_started(schema, table, partition, index, subchunk);
    }
  }

  void end_table_subchunk(const std::string &schema, const std::string &table,
                          const std::string &partition, ssize_t index,
                          uint64_t subchunk, uint64_t bytes) {
    if (table_subchunk_status(schema, table, partition, index, subchunk) !=
        Status::DONE) {
      log_subchunk_finished(schema, table, partition, index, subchunk, bytes);
    }
  }

  void start_gtid_update() {
    if (gtid_update_status() != Status::DONE) log(false, "GTID-UPDATE");
  }

  void end_gtid_update() {
    if (gtid_update_status() != Status::DONE) log(true, "GTID-UPDATE");
  }

 private:
  using Dumper = shcore::JSON_dumper;
  using Callback = std::function<void(Dumper *)>;

  struct Status_details {
    Status status;
    shcore::Dictionary_t details;
  };

  std::unique_ptr<mysqlshdk::storage::IFile> m_file;
  std::unique_ptr<mysqlshdk::storage::IFile> m_real_file;
  const std::string *m_memfile_contents = nullptr;

  std::unordered_map<std::string, Status_details> m_last_state;

  void log(bool end, const std::string &op, const std::string &schema = "",
           const std::string &table = "", const std::string &partition = "",
           const Callback &more = {}) {
    if (m_file) {
      Dumper json;

      json.start_object();
      json.append_string("op", op);
      json.append_bool("done", end);
      json.append_int64("timestamp",
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count());
      if (!schema.empty()) {
        json.append_string("schema", schema);
        if (!table.empty()) {
          json.append_string("table", table);
          if (!partition.empty()) {
            json.append_string("partition", partition);
          }
        }
      }
      if (more) {
        more(&json);
      }
      json.end_object();

      mysqlshdk::storage::fputs(json.str() + "\n", m_file.get());
      flush();
    }
  }

  void log(bool end, const std::string &op, const std::string &schema,
           const std::string &table, const std::string &partition,
           ssize_t chunk_index, const Callback &more = {}) {
    log(end, op, schema, table, partition, [&](Dumper *json) {
      json->append_int("chunk", chunk_index);

      if (more) {
        more(json);
      }
    });
  }

  void log_chunk(bool end, const std::string &schema, const std::string &table,
                 const std::string &partition, ssize_t chunk_index,
                 const Callback &more = {}) {
    log(end, "TABLE-DATA", schema, table, partition, chunk_index, more);
  }

  void log_chunk_started(const std::string &schema, const std::string &table,
                         const std::string &partition, ssize_t chunk_index) {
    log_chunk(false, schema, table, partition, chunk_index);
  }

  void log_chunk_finished(const std::string &schema, const std::string &table,
                          const std::string &partition, ssize_t chunk_index,
                          size_t bytes_loaded, size_t raw_bytes_loaded) {
    log_chunk(true, schema, table, partition, chunk_index, [&](Dumper *json) {
      json->append_uint64("bytes", bytes_loaded);
      json->append_uint64("raw_bytes", raw_bytes_loaded);
    });
  }

  void log_subchunk(bool end, const std::string &schema,
                    const std::string &table, const std::string &partition,
                    ssize_t chunk_index, uint64_t subchunk,
                    const Callback &more = {}) {
    log(end, "TABLE-SUB-DATA", schema, table, partition, chunk_index,
        [&](Dumper *json) {
          json->append_int("subchunk", subchunk);

          if (more) {
            more(json);
          }
        });
  }

  void log_subchunk_started(const std::string &schema, const std::string &table,
                            const std::string &partition, ssize_t chunk_index,
                            uint64_t subchunk) {
    log_subchunk(false, schema, table, partition, chunk_index, subchunk);
  }

  void log_subchunk_finished(const std::string &schema,
                             const std::string &table,
                             const std::string &partition, ssize_t chunk_index,
                             uint64_t subchunk, uint64_t bytes) {
    log_subchunk(
        true, schema, table, partition, chunk_index, subchunk,
        [&](Dumper *json) { json->append_uint64("transaction_bytes", bytes); });
  }

  void flush() {
    if (m_file) {
      m_file->flush();
    }
    if (m_real_file) {
      m_real_file->open(mysqlshdk::storage::Mode::WRITE);
      m_real_file->write(m_memfile_contents->data(),
                         m_memfile_contents->size());
      m_real_file->close();
    }
  }

  std::string encode_subchunk(const std::string &schema,
                              const std::string &table,
                              const std::string &partition, ssize_t chunk,
                              uint64_t subchunk) const {
    return "TABLE-SUB-DATA:`" + schema + "`:`" + table +
           (partition.empty() ? "" : "`:`" + partition) +
           "`:" + std::to_string(chunk) + ":" + std::to_string(subchunk);
  }
};

inline std::string to_string(Load_progress_log::Status status) {
  switch (status) {
    case Load_progress_log::DONE:
      return "DONE";
    case Load_progress_log::INTERRUPTED:
      return "INTERRUPTED";
    case Load_progress_log::PENDING:
      return "PENDING";
  }
  throw std::logic_error("Internal error");
}

}  // namespace mysqlsh

#endif  // MODULES_UTIL_LOAD_SCHEMA_LOAD_PROGRESS_LOG_H_
