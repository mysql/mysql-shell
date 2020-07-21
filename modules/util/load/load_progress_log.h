/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
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

                if (entry->has_key("chunk"))
                  key += ":" + std::to_string(entry->get_int("chunk"));

                auto iter = m_last_state.find(key);
                if (iter == m_last_state.end() || !done) {
                  m_last_state.emplace(key, Status::INTERRUPTED);
                } else {
                  if (entry->has_key("bytes"))
                    bytes_completed += entry->get_uint("bytes");

                  if (entry->has_key("raw_bytes"))
                    raw_bytes_completed += entry->get_uint("raw_bytes");

                  iter->second = Status::DONE;
                }
              }
              return true;
            },
            "\n");
      } catch (const std::exception &e) {
        throw std::runtime_error("Error loading load progress file '" +
                                 existing_file->full_path() + "': " + e.what());
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
    if (m_memfile_contents) {
      if (m_file) m_file->close();

      auto mem_file =
          std::make_unique<mysqlshdk::storage::backend::Memory_file>("");
      m_memfile_contents = &mem_file->content();
      m_file = std::move(mem_file);
    }

    if (m_real_file) {
      m_real_file->remove();
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
    return it->second;
  }

  Status triggers_ddl_status(const std::string &schema,
                             const std::string &table) const {
    auto it =
        m_last_state.find("TRIGGERS-DDL:`" + schema + "`:`" + table + "`");
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second;
  }

  Status table_ddl_status(const std::string &schema,
                          const std::string &table) const {
    auto it = m_last_state.find("TABLE-DDL:`" + schema + "`:`" + table + "`");
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second;
  }

  Status table_chunk_status(const std::string &schema, const std::string &table,
                            ssize_t chunk) const {
    auto it = m_last_state.find("TABLE-DATA:`" + schema + "`:`" + table +
                                "`:" + std::to_string(chunk));
    if (it == m_last_state.end()) return Status::PENDING;
    return it->second;
  }

  // schema DDL includes the schema script and views
  void start_schema_ddl(const std::string &schema) {
    if (schema_ddl_status(schema) != Status::DONE)
      log(false, "SCHEMA-DDL", schema, "");
  }

  void end_schema_ddl(const std::string &schema) {
    if (schema_ddl_status(schema) != Status::DONE)
      log(true, "SCHEMA-DDL", schema, "");
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
                         ssize_t index) {
    if (table_chunk_status(schema, table, index) != Status::DONE)
      log(false, "TABLE-DATA", schema, table, index, 0, 0);
  }

  void end_table_chunk(const std::string &schema, const std::string &table,
                       ssize_t index, size_t bytes_loaded,
                       size_t raw_bytes_loaded) {
    if (table_chunk_status(schema, table, index) != Status::DONE)
      log(true, "TABLE-DATA", schema, table, index, bytes_loaded,
          raw_bytes_loaded);
  }

 private:
  std::unique_ptr<mysqlshdk::storage::IFile> m_file;
  std::unique_ptr<mysqlshdk::storage::IFile> m_real_file;
  const std::string *m_memfile_contents = nullptr;

  std::unordered_map<std::string, Status> m_last_state;

  void log(bool end, const std::string &op, const std::string &schema,
           const std::string &table) {
    if (m_file) {
      shcore::JSON_dumper json;

      json.start_object();
      json.append_string("op", op);
      json.append_bool("done", end);
      if (!schema.empty()) {
        json.append_string("schema", schema);
        if (!table.empty()) {
          json.append_string("table", table);
        }
      }
      json.end_object();

      mysqlshdk::storage::fputs(json.str() + "\n", m_file.get());
      flush();
    }
  }

  void log(bool end, const std::string &op, const std::string &schema,
           const std::string &table, ssize_t chunk_index, size_t bytes_loaded,
           size_t raw_bytes_loaded) {
    if (m_file) {
      shcore::JSON_dumper json;

      json.start_object();
      json.append_string("op", op);
      json.append_bool("done", end);
      json.append_string("schema", schema);
      json.append_string("table", table);
      json.append_int("chunk", chunk_index);
      if (end) {
        json.append_uint64("bytes", bytes_loaded);
        json.append_uint64("raw_bytes", raw_bytes_loaded);
      }
      json.end_object();

      mysqlshdk::storage::fputs(json.str() + "\n", m_file.get());
      flush();
    }
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
