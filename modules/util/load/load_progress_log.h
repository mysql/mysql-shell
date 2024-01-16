/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_LOAD_LOAD_PROGRESS_LOG_H_
#define MODULES_UTIL_LOAD_LOAD_PROGRESS_LOG_H_

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "modules/util/load/load_errors.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/storage/backend/memory_file.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlsh {

namespace progress {

namespace entry {

namespace detail {

template <typename U, typename T>
concept Value_type = std::is_convertible_v<U, T>;

template <typename T>
struct Value {
  constexpr Value() = default;

  template <Value_type<T> U>
  constexpr Value(U &&v) : value{std::forward<U>(v)} {}

  // not initialized on purpose - compiler will warn if an initializer is
  // missing
  T value;
};

}  // namespace detail

struct Worker_id : detail::Value<std::size_t> {
  static constexpr std::string_view key = "_worker";
  using Value::Value;
};

struct Weight : detail::Value<uint64_t> {
  static constexpr std::string_view key = "_srvthreads";
  using Value::Value;
};

struct Indexes : detail::Value<uint64_t> {
  static constexpr std::string_view key = "_nindexes";
  using Value::Value;
};

struct Task {
  entry::Worker_id worker_id{};
  entry::Weight weight{};
};

struct Operation : detail::Value<std::string_view> {
  static constexpr std::string_view key = "op";
  using Value::Value;
};

struct Schema : detail::Value<std::string_view> {
  static constexpr std::string_view key = "schema";
  using Value::Value;
};

struct Table : detail::Value<std::string_view> {
  static constexpr std::string_view key = "table";
  using Value::Value;
};

struct Partition : detail::Value<std::string_view> {
  static constexpr std::string_view key = "partition";
  using Value::Value;
};

struct Chunk : detail::Value<ssize_t> {
  static constexpr std::string_view key = "chunk";
  using Value::Value;
};

struct Subchunk : detail::Value<uint64_t> {
  static constexpr std::string_view key = "subchunk";
  using Value::Value;
};

struct Data_bytes : detail::Value<std::size_t> {
  static constexpr std::string_view key = "bytes";
  using Value::Value;
};

struct File_bytes : detail::Value<std::size_t> {
  static constexpr std::string_view key = "raw_bytes";
  using Value::Value;
};

struct Rows : detail::Value<std::size_t> {
  static constexpr std::string_view key = "rows";
  using Value::Value;
};

struct Transaction_bytes : detail::Value<uint64_t> {
  static constexpr std::string_view key = "transaction_bytes";
  using Value::Value;
};

}  // namespace entry

// status entries

struct Gtid_update {
  static constexpr entry::Operation op{"GTID-UPDATE"};

  std::string key() const { return std::string{op.value}; }
};

struct Schema_ddl {
  static constexpr entry::Operation op{"SCHEMA-DDL"};

  entry::Schema schema;

  std::string key() const {
    std::string k;

    k.reserve(op.value.length() + 2 + schema.value.length() + 1);

    k += op.value;
    k += ":`";
    k += schema.value;
    k += '`';

    return k;
  }
};

struct Table_ddl {
  static constexpr entry::Operation op{"TABLE-DDL"};

  entry::Schema schema;
  entry::Table table;

  std::string key() const {
    std::string k;

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() + 1);

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;
    k += '`';

    return k;
  }
};

struct Triggers_ddl {
  static constexpr entry::Operation op{"TRIGGERS-DDL"};

  entry::Schema schema;
  entry::Table table;

  std::string key() const {
    std::string k;

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() + 1);

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;
    k += '`';

    return k;
  }
};

struct Table_indexes {
  static constexpr entry::Operation op{"TABLE-INDEX"};

  entry::Schema schema;
  entry::Table table;

  std::string key() const {
    std::string k;

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() + 1);

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;
    k += '`';

    return k;
  }
};

struct Analyze_table {
  static constexpr entry::Operation op{"TABLE-ANALYZE"};

  entry::Schema schema;
  entry::Table table;

  std::string key() const {
    std::string k;

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() + 1);

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;
    k += '`';

    return k;
  }
};

struct Table_chunk {
  static constexpr entry::Operation op{"TABLE-DATA"};

  entry::Schema schema;
  entry::Table table;
  entry::Partition partition;
  entry::Chunk chunk;

  std::string key() const {
    std::string k;
    const auto c = std::to_string(chunk.value);

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() +
              (partition.value.empty() ? 0 : 3 + partition.value.length()) + 2 +
              c.length());

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;

    if (!partition.value.empty()) {
      k += "`:`";
      k += partition.value;
    }

    k += "`:";
    k += c;

    return k;
  }
};

struct Table_subchunk {
  static constexpr entry::Operation op{"TABLE-SUB-DATA"};

  entry::Schema schema;
  entry::Table table;
  entry::Partition partition;
  entry::Chunk chunk;
  entry::Subchunk subchunk;

  std::string key() const {
    std::string k;
    const auto c = std::to_string(chunk.value);
    const auto s = std::to_string(subchunk.value);

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() +
              (partition.value.empty() ? 0 : 3 + partition.value.length()) + 2 +
              c.length() + 1 + s.length());

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;

    if (!partition.value.empty()) {
      k += "`:`";
      k += partition.value;
    }

    k += "`:";
    k += c;
    k += ':';
    k += s;

    return k;
  }
};

struct Bulk_load {
  static constexpr entry::Operation op{"TABLE-DATA-BULK"};

  entry::Schema schema;
  entry::Table table;
  entry::Partition partition;

  std::string key() const {
    std::string k;

    k.reserve(op.value.length() + 2 + schema.value.length() + 3 +
              table.value.length() +
              (partition.value.empty() ? 0 : 3 + partition.value.length()) + 1);

    k += op.value;
    k += ":`";
    k += schema.value;
    k += "`:`";
    k += table.value;

    if (!partition.value.empty()) {
      k += "`:`";
      k += partition.value;
    }

    k += '`';

    return k;
  }
};

template <typename T>
concept Status_entry =
    std::is_base_of_v<Gtid_update, T> || std::is_base_of_v<Schema_ddl, T> ||
    std::is_base_of_v<Table_ddl, T> || std::is_base_of_v<Triggers_ddl, T> ||
    std::is_base_of_v<Table_indexes, T> ||
    std::is_base_of_v<Analyze_table, T> || std::is_base_of_v<Table_chunk, T> ||
    std::is_base_of_v<Table_subchunk, T> || std::is_base_of_v<Bulk_load, T>;

namespace start {

struct Gtid_update : public progress::Gtid_update {};

struct Schema_ddl : public progress::Schema_ddl {};

struct Table_ddl : public progress::Table_ddl {
  entry::Task task;
};

struct Triggers_ddl : public progress::Triggers_ddl {};

struct Table_indexes : public progress::Table_indexes {
  entry::Task task;
  entry::Indexes indexes;
};

struct Analyze_table : public progress::Analyze_table {
  entry::Task task;
};

struct Table_chunk : public progress::Table_chunk {
  entry::Task task;
};

struct Table_subchunk : public progress::Table_subchunk {
  entry::Task task;
};

struct Bulk_load : public progress::Bulk_load {
  entry::Task task;
};

template <typename T>
concept Entry =
    std::is_same_v<T, Gtid_update> || std::is_same_v<T, Schema_ddl> ||
    std::is_same_v<T, Table_ddl> || std::is_same_v<T, Triggers_ddl> ||
    std::is_same_v<T, Table_indexes> || std::is_same_v<T, Analyze_table> ||
    std::is_same_v<T, Table_chunk> || std::is_same_v<T, Table_subchunk> ||
    std::is_same_v<T, Bulk_load>;

}  // namespace start

namespace update {

struct Bulk_load : public progress::Bulk_load {
  entry::Data_bytes data_bytes;
};

template <typename T>
concept Entry = std::is_same_v<T, Bulk_load>;

}  // namespace update

namespace end {

struct Gtid_update : public progress::Gtid_update {};

struct Schema_ddl : public progress::Schema_ddl {};

struct Table_ddl : public progress::Table_ddl {};

struct Triggers_ddl : public progress::Triggers_ddl {};

struct Table_indexes : public progress::Table_indexes {};

struct Analyze_table : public progress::Analyze_table {};

struct Table_chunk : public progress::Table_chunk {
  entry::Data_bytes data_bytes;
  entry::File_bytes file_bytes;
  entry::Rows rows;
};

struct Table_subchunk : public progress::Table_subchunk {
  entry::Transaction_bytes bytes;
};

struct Bulk_load : public progress::Bulk_load {
  entry::Data_bytes data_bytes;
  entry::File_bytes file_bytes;
  entry::Rows rows;
};

template <typename T>
concept Entry =
    std::is_same_v<T, Gtid_update> || std::is_same_v<T, Schema_ddl> ||
    std::is_same_v<T, Table_ddl> || std::is_same_v<T, Triggers_ddl> ||
    std::is_same_v<T, Table_indexes> || std::is_same_v<T, Analyze_table> ||
    std::is_same_v<T, Table_chunk> || std::is_same_v<T, Table_subchunk> ||
    std::is_same_v<T, Bulk_load>;

}  // namespace end

template <typename T>
concept Log_entry = start::Entry<T> || update::Entry<T> || end::Entry<T>;

}  // namespace progress

class Load_progress_log final {
 public:
  enum Status { PENDING, INTERRUPTED, DONE };

  struct Progress_status {
    Status status;
    uint64_t data_bytes_completed;
    uint64_t file_bytes_completed;
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
    uint64_t data_bytes_completed = 0;
    uint64_t file_bytes_completed = 0;

    if (existing_file && existing_file->exists()) {
      existing_file->open(mysqlshdk::storage::Mode::READ);
      data = mysqlshdk::storage::read_file(existing_file);
      existing_file->close();

      try {
        const std::string done_key{"done"};
        const std::string op{progress::entry::Operation::key};
        const std::string schema{progress::entry::Schema::key};
        const std::string table{progress::entry::Table::key};
        const std::string partition{progress::entry::Partition::key};
        const std::string chunk{progress::entry::Chunk::key};
        const std::string subchunk{progress::entry::Subchunk::key};
        const std::string bytes{progress::entry::Data_bytes::key};
        const std::string raw_bytes{progress::entry::File_bytes::key};

        shcore::str_itersplit(
            data,
            [&, this](std::string_view line) -> bool {
              if (shcore::str_strip_view(line).empty()) {
                return true;
              }

              shcore::Value doc = shcore::Value::parse(line);
              shcore::Dictionary_t entry = doc.as_map();

              bool done = entry->get_int(done_key) != 0;

              std::string key = entry->get_string(op);

              if (const auto it = entry->find(schema); entry->end() != it) {
                key += ":`";
                key += it->second.get_string();
                key += '`';
              }

              if (const auto it = entry->find(table); entry->end() != it) {
                key += ":`";
                key += it->second.get_string();
                key += '`';
              }

              if (const auto it = entry->find(partition); entry->end() != it) {
                key += ":`";
                key += it->second.get_string();
                key += '`';
              }

              if (const auto it = entry->find(chunk); entry->end() != it) {
                key += ':';
                key += std::to_string(it->second.as_int());
              }

              if (const auto it = entry->find(subchunk); entry->end() != it) {
                key += ':';
                key += std::to_string(it->second.as_uint());
              }

              const auto result = m_last_state.try_emplace(
                  std::move(key), Status_details{Status::INTERRUPTED});

              if (done) {
                result.first->second.status = Status::DONE;

                data_bytes_completed += entry->get_uint(bytes);
                file_bytes_completed += entry->get_uint(raw_bytes);
              }

              if (result.second || done) {
                // store entry if this is a new status, or an end status
                result.first->second.details = std::move(entry);
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
    return {status, data_bytes_completed, file_bytes_completed};
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

  inline Status status(const progress::Status_entry auto &entry) const {
    const auto it = this->find(entry);
    return m_last_state.end() == it ? Status::PENDING : it->second.status;
  }

  uint64_t table_subchunk_size(const progress::Table_subchunk &entry) const {
    const auto it = this->find(entry);
    return m_last_state.end() == it
               ? 0
               : it->second.details->get_uint(
                     std::string{progress::entry::Transaction_bytes::key});
  }

  std::string server_uuid() const {
    const auto it = this->find(Server_uuid{});
    return m_last_state.end() == it
               ? std::string{}
               : it->second.details->get_string(std::string{Uuid::key});
  }

  void set_server_uuid(const std::string &uuid) {
    const auto saved_uuid = server_uuid();

    if (saved_uuid.empty()) {
      write_log(true, Set_server_uuid{{}, uuid});
    } else if (!shcore::str_caseeq(uuid, saved_uuid)) {
      THROW_ERROR(SHERR_LOAD_PROGRESS_FILE_UUID_MISMATCH, saved_uuid.c_str(),
                  uuid.c_str());
    }
  }

  void log(const progress::start::Entry auto &entry) { do_log(false, entry); }

  void log(const progress::update::Entry auto &entry) { do_log(false, entry); }

  void log(const progress::end::Entry auto &entry) { do_log(true, entry); }

 private:
  using Dumper = shcore::JSON_dumper;

  struct Status_details {
    Status status;
    shcore::Dictionary_t details{};
  };

  using Last_state = std::unordered_map<std::string, Status_details>;

  struct Uuid : progress::entry::detail::Value<std::string_view> {
    static constexpr std::string_view key = "uuid";
    using Value::Value;
  };

  struct Server_uuid {
    static constexpr progress::entry::Operation op{"SERVER-UUID"};

    std::string key() const { return std::string{op.value}; }
  };

  struct Set_server_uuid : public Server_uuid {
    Uuid uuid;
  };

  template <typename T>
  inline static void append(Dumper *json,
                            const T &entry) requires(!progress::Log_entry<T>) {
    json->append(entry.key, entry.value);
  }

  static void append(Dumper *json, const progress::entry::Task &entry) {
    append(json, entry.worker_id);
    append(json, entry.weight);
  }

  static void append(Dumper *, const progress::Gtid_update &) {}

  static void append(Dumper *json, const progress::Schema_ddl &entry) {
    append(json, entry.schema);
  }

  static void append(Dumper *json, const progress::Table_ddl &entry) {
    append(json, entry.schema);
    append(json, entry.table);
  }

  static void append(Dumper *json, const progress::start::Table_ddl &entry) {
    append(json, static_cast<const progress::Table_ddl &>(entry));
    append(json, entry.task);
  }

  static void append(Dumper *json, const progress::Triggers_ddl &entry) {
    append(json, entry.schema);
    append(json, entry.table);
  }

  static void append(Dumper *json, const progress::Table_indexes &entry) {
    append(json, entry.schema);
    append(json, entry.table);
  }

  static void append(Dumper *json,
                     const progress::start::Table_indexes &entry) {
    append(json, static_cast<const progress::Table_indexes &>(entry));
    append(json, entry.task);
    append(json, entry.indexes);
  }

  static void append(Dumper *json, const progress::Analyze_table &entry) {
    append(json, entry.schema);
    append(json, entry.table);
  }

  static void append(Dumper *json,
                     const progress::start::Analyze_table &entry) {
    append(json, static_cast<const progress::Analyze_table &>(entry));
    append(json, entry.task);
  }

  static void append(Dumper *json, const progress::Table_chunk &entry) {
    append(json, entry.schema);
    append(json, entry.table);

    if (!entry.partition.value.empty()) {
      append(json, entry.partition);
    }

    append(json, entry.chunk);
  }

  static void append(Dumper *json, const progress::start::Table_chunk &entry) {
    append(json, static_cast<const progress::Table_chunk &>(entry));
    append(json, entry.task);
  }

  static void append(Dumper *json, const progress::end::Table_chunk &entry) {
    append(json, static_cast<const progress::Table_chunk &>(entry));
    append(json, entry.data_bytes);
    append(json, entry.file_bytes);
    append(json, entry.rows);
  }

  static void append(Dumper *json, const progress::Table_subchunk &entry) {
    append(json, entry.schema);
    append(json, entry.table);

    if (!entry.partition.value.empty()) {
      append(json, entry.partition);
    }

    append(json, entry.chunk);
    append(json, entry.subchunk);
  }

  static void append(Dumper *json,
                     const progress::start::Table_subchunk &entry) {
    append(json, static_cast<const progress::Table_subchunk &>(entry));
    append(json, entry.task);
  }

  static void append(Dumper *json, const progress::end::Table_subchunk &entry) {
    append(json, static_cast<const progress::Table_subchunk &>(entry));
    append(json, entry.bytes);
  }

  static void append(Dumper *json, const progress::Bulk_load &entry) {
    append(json, entry.schema);
    append(json, entry.table);

    if (!entry.partition.value.empty()) {
      append(json, entry.partition);
    }
  }

  static void append(Dumper *json, const progress::start::Bulk_load &entry) {
    append(json, static_cast<const progress::Bulk_load &>(entry));
    append(json, entry.task);
  }

  static void append(Dumper *json, const progress::update::Bulk_load &entry) {
    append(json, static_cast<const progress::Bulk_load &>(entry));
    append(json, entry.data_bytes);
  }

  static void append(Dumper *json, const progress::end::Bulk_load &entry) {
    append(json, static_cast<const progress::Bulk_load &>(entry));
    append(json, entry.data_bytes);
    append(json, entry.file_bytes);
    append(json, entry.rows);
  }

  static void append(Dumper *, const Server_uuid &) {}

  static void append(Dumper *json, const Set_server_uuid &entry) {
    append(json, static_cast<const Server_uuid &>(entry));
    append(json, entry.uuid);
  }

  static void begin_log(Dumper *json, bool done,
                        const progress::entry::Operation &op) {
    assert(json);

    json->start_object();
    append(json, op);
    json->append_bool("done", done);
    json->append_int64("timestamp",
                       std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count());
  }

  static void end_log(Dumper *json) {
    assert(json);

    json->end_object();
  }

  void write_log(const Dumper &json) {
    mysqlshdk::storage::fputs(json.str(), m_file.get());
    mysqlshdk::storage::fputs("\n", m_file.get());
    flush();
  }

  inline void do_log(bool done, const progress::Log_entry auto &entry) {
    if (status(entry) == Status::DONE) {
      return;
    }

    write_log(done, entry);
  }

  template <typename T>
  inline void write_log(bool done, const T &entry) requires(
      progress::Log_entry<T> || std::is_same_v<T, Set_server_uuid>) {
    if (!m_file) {
      return;
    }

    Dumper json;

    begin_log(&json, done, entry.op);
    append(&json, entry);
    end_log(&json);
    write_log(json);
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

  template <typename T>
  Last_state::const_iterator find(const T &entry) const
      requires(progress::Status_entry<T> || std::is_base_of_v<Server_uuid, T>) {
    if (m_last_state.empty()) {
      return m_last_state.end();
    }

    return m_last_state.find(entry.key());
  }

  std::unique_ptr<mysqlshdk::storage::IFile> m_file;
  std::unique_ptr<mysqlshdk::storage::IFile> m_real_file;
  const std::string *m_memfile_contents = nullptr;

  Last_state m_last_state;
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

#endif  // MODULES_UTIL_LOAD_LOAD_PROGRESS_LOG_H_
