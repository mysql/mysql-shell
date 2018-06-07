/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_JSON_IMPORTER_H_
#define MODULES_UTIL_JSON_IMPORTER_H_

#include <memory>
#include <string>
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "utils/utils_buffered_input.h"

namespace mysqlsh {

class Json_importer;

class Prepare_json_import {
  template <class T>
  using nullable = mysqlshdk::utils::nullable<T>;

 public:
  explicit Prepare_json_import(
      const std::shared_ptr<mysqlshdk::db::mysqlx::Session> &session)
      : m_session(session) {}
  Prepare_json_import(const Prepare_json_import &other) = delete;
  Prepare_json_import(Prepare_json_import &&other) = delete;

  Prepare_json_import &operator=(const Prepare_json_import &other) = delete;
  Prepare_json_import &operator=(Prepare_json_import &&other) = delete;

  ~Prepare_json_import() = default;

  Prepare_json_import &session(
      const std::shared_ptr<mysqlshdk::db::mysqlx::Session> &session) {
    m_session = session;
    return *this;
  }

  Prepare_json_import &use_stdin() {
    m_source.tag = Source::STDIN;
    return *this;
  }

  Prepare_json_import &path(const std::string &name) {
    m_source.path = name;
    m_source.tag = Source::FILE;
    return *this;
  }

  Prepare_json_import &schema(const std::string &name) {
    if (!name.empty()) {
      m_schema = name;
    }
    return *this;
  }

  Prepare_json_import &collection(const std::string &name) {
    m_put_to_collection = true;
    m_collection = name;
    return *this;
  }

  Prepare_json_import &table(const std::string &name) {
    m_put_to_collection = false;
    m_table = name;
    return *this;
  }

  Prepare_json_import &column(const std::string &name) {
    m_put_to_collection = false;
    m_column = name;
    return *this;
  }

  std::string to_string() {
    return std::string{
        "Importing from " +
        (m_source == Source::FILE ? std::string("file ") : std::string()) +
        m_source.to_string() + " to " +
        (m_put_to_collection ? "collection" : "table") + " `" + *m_schema +
        "`.`" + (m_put_to_collection ? *m_collection : *m_table) + "`"};
  }

  Json_importer build();

 private:
  /**
   * Set missing parameters to it's defaults.
   */
  void set_defaults();

  /**
   * Validate if mandatory parameters are set.
   *
   * schema, collection, table, column, result
   * 0       0           0      0       fail, no schema
   * 0       0           0      1       fail, no schema
   * 0       0           1      0       fail, no schema
   * 0       0           1      1       fail, no schema
   * 0       1           0      0       fail, no schema
   * 0       1           0      1       fail, no schema
   * 0       1           1      0       fail, no schema
   * 0       1           1      1       fail, no schema
   * 1       0           0      0       guess collection name from file
   * 1       0           0      1       guess table name from file, because
   *                                    user set column
   * 1       0           1      0       import to table
   * 1       0           1      1       import to column in table
   * 1       1           0      0       import to collection
   * 1       1           0      1       fail, collection and column set
   * 1       1           1      0       fail, collection and table set
   * 1       1           1      1       fail, collection and (table-column) set
   */
  void validate();

  nullable<std::string> target_name_from_path();

  /**
   * Extract name for collection from file name.
   */
  void guess_collection();

  /**
   * Extract name for table from file name.
   */
  void guess_table();

  bool create_default_collection(const std::string &name) const;

  bool create_default_table(const std::string &name,
                            const std::string &column_name) const;

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> m_session = nullptr;
  struct Source {
    enum Source_tag { NONE, FILE, STDIN } tag;
    std::string path;

    bool operator==(Source_tag other) { return tag == other; }

    std::string to_string() {
      switch (tag) {
        case NONE:
          return "-none-";
        case FILE:
          return "\"" + shcore::path::expand_user(path) + "\"";
        case STDIN:
          return "-stdin-";
      }
      return {};
    }
  } m_source;

  nullable<std::string> m_schema{nullptr};
  nullable<std::string> m_collection{nullptr};
  nullable<std::string> m_table{nullptr};
  std::string m_column{"doc"};
  bool m_put_to_collection = true;
};

class Json_importer {
 public:
  explicit Json_importer(
      const std::shared_ptr<mysqlshdk::db::mysqlx::Session> &session);
  ~Json_importer() {}

  void set_target_table(const std::string &schema, const std::string &table,
                        const std::string &column);
  void set_target_collection(const std::string &schema,
                             const std::string &collection);
  void set_print_callback(
      const std::function<void(const std::string &)> &callback);

  /**
   * Set path to JSON document.
   * @param path Path to JSON document. Empty path enables read from stdin.
   */
  void set_path(const std::string &path) { m_file_path = path; }
  void load_from(bool strip_bson_objectid);

  void print_stats();

 private:
  void load_from(shcore::Buffered_input *input, bool strip_bson_objectid);
  void put(const std::string &item);
  void recv_response(bool block = false);
  void flush();
  void commit(bool final_commit = false);
  void add_to_request(const std::string &doc);
  void update_statistics(xcl::XQuery_result *xquery_result);

  ::Mysqlx::Crud::Insert m_batch_insert;
  std::shared_ptr<mysqlshdk::db::mysqlx::Session> m_session;

  struct Packet_size_tracker {
    /**
     * Returns protobuf crud insert packet size after new document append with
     * `doc_size` size.
     *
     * @param doc_size Size of document
     * @return Size of protobuf crud insert packet after new document append
     * with `doc_size` size.
     */
    size_t packet_size(size_t doc_size) const {
      return packet_size() + doc_size + k_overhead_per_document_bytes;
    }

    size_t packet_size() const {
      return crud_insert_overhead_bytes + bytes_in_insert +
             rows_in_insert * k_overhead_per_document_bytes;
    }

    /**
     * Check if we exceed size of mysqlx_max_packet_size after add new document
     * of size `doc_size`.
     *
     * @param doc_size Size of new document.
     * @return true if packet size exceed mysqlx_max_allowed_packet value, false
     * otherwise.
     */
    bool will_overflow(size_t doc_size) const {
      size_t packet_size = this->packet_size(doc_size);
      bool will_overflow_max_packet = packet_size > max_packet;
      if (rows_in_insert == 0 && will_overflow_max_packet) {
        constexpr int64_t k_one_gigabyte = 1024 * 1024 * 1024;
        if (k_one_gigabyte < packet_size) {
          throw std::invalid_argument(
              "JSON document is too large. JSON document packet size is "
              "greater than maximum allowed value for max_allowed_packet and "
              "mysqlx_max_allowed_packet.");
        }
        throw std::invalid_argument(
            "JSON document is too large. Increase mysqlx_max_allowed_packet "
            "value to at least " +
            std::to_string(packet_size + 1) + " bytes.");
      }
      return will_overflow_max_packet;
    }

    /// Protobuf Crud Insert document header size. This value depend on document
    /// size, therefore we set this to maximum observed header size.
    static constexpr size_t k_overhead_per_document_bytes = 44;

    /// Max packet size accepted by target MySQL Server
    size_t max_packet;

    size_t rows_in_insert = 0;
    size_t bytes_in_insert = 0;
    int inserts_in_this_transaction = 0;

    size_t crud_insert_overhead_bytes = 0;
  } m_packet_size_tracker;

// todo(kg): JSON import to MySQL Server for Windows stuck on vio_ssl_write when
// MySQL Shell for Windows has SSL and interleave mode enabled. Therefore we
// disable interleave mode until we fix that problem.
#ifdef _WIN32
  const bool m_proto_interleaved = false;
#else
  const bool m_proto_interleaved = true;
#endif
  int m_pending_response = 0;
  std::function<void(const std::string &)> m_print = nullptr;

  struct {
    uint64_t items_processed = 0;
    uint64_t bytes_processed = 0;
    uint64_t import_filesize = 0;
    uint64_t documents_successfully_imported = 0;
    mysqlshdk::utils::Profile_timer timer;
  } m_stats;

  std::string m_file_path;  //< Path to JSON document
};

}  // namespace mysqlsh

#endif  // MODULES_UTIL_JSON_IMPORTER_H_
