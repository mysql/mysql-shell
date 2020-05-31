/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/json_importer.h"
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/select.h>
#endif
#include <deque>
#include <istream>
#include <utility>
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/mysqlx/util/setter_any.h"
#include "mysqlshdk/libs/utils/utils_buffered_input.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "scripting/shexcept.h"
#include "shellcore/interrupt_handler.h"

namespace mysqlsh {

void Prepare_json_import::set_defaults() {
  if (m_collection.is_null() && m_table.is_null()) {
    if (m_put_to_collection) {
      this->guess_collection();
    } else {
      this->guess_table();
    }
  }
}

void Prepare_json_import::validate() {
  if (m_schema.is_null()) {
    throw std::invalid_argument("Target schema must be set.");
  }

  if (m_collection.is_null() && m_table.is_null()) {
    throw std::invalid_argument("Target collection or table must be set.");
  }

  if (!m_collection.is_null() && !m_table.is_null()) {
    throw std::invalid_argument(
        "Conflicting targets: collection and table cannot be used together.");
  }

  if (m_source == Source::NONE) {
    throw std::invalid_argument("Data input source must be set.");
  }

  if (m_source == Source::FILE) {
    if (m_source.path.empty()) {
      throw shcore::Exception::logic_error("Path cannot be empty.");
    }

    auto full_path = shcore::path::expand_user(m_source.path);

    if (!shcore::path_exists(full_path)) {
      throw shcore::Exception::logic_error("Path \"" + full_path +
                                           "\" does not exist.");
    }

    if (!shcore::is_file(full_path) && !shcore::is_fifo(full_path)) {
      throw shcore::Exception::logic_error("Path \"" + full_path +
                                           "\" is not a file.");
    }
  }
}

Prepare_json_import::nullable<std::string>
Prepare_json_import::target_name_from_path() {
  if (m_source == Source::FILE) {
    auto name = std::get<0>(
        shcore::path::split_extension(shcore::path::basename(m_source.path)));
    return nullable<std::string>{name};
  }
  return nullable<std::string>{nullptr};
}

void Prepare_json_import::guess_collection() {
  if (m_collection.is_null()) {
    auto name = target_name_from_path();
    if (!name.is_null()) {
      this->collection(*name);
    }
  }
}

void Prepare_json_import::guess_table() {
  if (m_table.is_null()) {
    auto name = target_name_from_path();
    if (!name.is_null()) {
      this->table(*name);
    }
  }
}

bool Prepare_json_import::create_default_collection(
    const std::string &name) const {
  if (m_schema.is_null()) {
    throw std::invalid_argument("Target schema must be set.");
  }

  ::xcl::Argument_object obj;
  obj["schema"] = *m_schema;
  obj["name"] = name;
  auto statement_args = ::xcl::Argument_value{obj};
  auto arguments = ::xcl::Argument_array{statement_args};
  auto result =
      m_session->execute_stmt("mysqlx", "ensure_collection", arguments);
  // todo(kg): update_collection_cache
  return true;
}

bool Prepare_json_import::create_default_table(
    const std::string &name, const std::string &column_name) const {
  if (m_schema.is_null()) {
    throw std::invalid_argument("Target schema must be set.");
  }

  auto query = shcore::sqlstring(
      "CREATE TABLE IF NOT EXISTS !.! (! JSON,"
      "id INTEGER AUTO_INCREMENT PRIMARY KEY"
      ") CHARSET utf8mb4 ENGINE=InnoDB;",
      0);
  query << *m_schema << name;
  query << column_name;
  auto result = m_session->query(query);
  // todo(kg): update_table_cache
  return true;
}

Json_importer Prepare_json_import::build() {
  set_defaults();
  validate();

  Json_importer importer{m_session};
  if (m_source == Source::FILE) {
    importer.set_path(m_source.path);
  } else if (m_source == Source::STDIN) {
    importer.set_path("");
  }
  if (m_put_to_collection) {
    this->create_default_collection(*m_collection);
    importer.set_target_collection(*m_schema, *m_collection);
  } else {
    this->create_default_table(*m_table, m_column);
    importer.set_target_table(*m_schema, *m_table, m_column);
  }
  return importer;
}

/*
 * 8 inserts per transaction with default 64M (for mysql 8.0)
 * max_allowed_packet, do commit every 512M.
 * It is advised to "Avoid performing rollbacks after inserting, updating, or
 * deleting huge numbers of rows."
 * https://dev.mysql.com/doc/refman/8.0/en/optimizing-innodb-transaction-management.html
 */
static constexpr const int k_inserts_per_transaction = 8;

Json_importer::Json_importer(
    const std::shared_ptr<mysqlshdk::db::mysqlx::Session> &session)
    : m_session(session) {
  // Safe bandwidth by disabling gtids tracking
  session->execute("set session session_track_gtids=OFF");
  auto result = session->query("SELECT @@mysqlx_max_allowed_packet");
  auto row = result->fetch_one();
  if (!row)
    throw std::logic_error("Query result returned fewer rows than expected");
  m_packet_size_tracker.max_packet = row->get_uint(0);
}

void Json_importer::set_target_table(const std::string &schema,
                                     const std::string &table,
                                     const std::string &column) {
  m_batch_insert.mutable_collection()->set_name(table);
  m_batch_insert.mutable_collection()->set_schema(schema);
  m_batch_insert.mutable_projection()->Add()->set_name(column);
  m_batch_insert.set_data_model(Mysqlx::Crud::TABLE);
}

void Json_importer::set_target_collection(const std::string &schema,
                                          const std::string &collection) {
  m_batch_insert.mutable_collection()->set_name(collection);
  m_batch_insert.mutable_collection()->set_schema(schema);
  m_batch_insert.set_data_model(Mysqlx::Crud::DOCUMENT);
}

void Json_importer::set_print_callback(
    const std::function<void(const std::string &)> &callback) {
  m_print = callback;
}

void Json_importer::print_stats() {
  using mysqlshdk::utils::format_bytes;
  using mysqlshdk::utils::format_seconds;
  using mysqlshdk::utils::format_throughput_items;

  m_stats.timer.stage_end();
  double import_time_seconds = m_stats.timer.total_seconds_elapsed();

  if (m_print) {
    auto human_bytes = format_bytes(m_stats.bytes_processed);
    auto human_time = format_seconds(import_time_seconds);
    const std::string msg =
        "\nProcessed " + human_bytes + " in " +
        std::to_string(m_stats.items_processed) +
        (m_stats.items_processed == 1 ? " document" : " documents") + " in " +
        human_time + " (" +
        format_throughput_items("document", "documents",
                                m_stats.items_processed, import_time_seconds) +
        ")" + "\nTotal successfully imported documents " +
        std::to_string(m_stats.documents_successfully_imported) + " (" +
        format_throughput_items("document", "documents",
                                m_stats.documents_successfully_imported,
                                import_time_seconds) +
        ")\n";
    m_print(msg);
  }
}

void Json_importer::load_from(const shcore::Document_reader_options &options) {
  shcore::Buffered_input input{};
  m_stats.timer.stage_begin("Importing documents");

  if (!m_file_path.empty()) {
    auto full_path = shcore::path::expand_user(m_file_path);
    input.open(full_path);
  }

  load_from(&input, options);
}

void Json_importer::load_from(shcore::Buffered_input *input,
                              const shcore::Document_reader_options &options) {
  m_stats.items_processed = 0;
  m_stats.bytes_processed = 0;
  m_packet_size_tracker.inserts_in_this_transaction = 0;

  // schema and collection target are already set here, so we can cache
  // mysqlx::crud::insert header size here
  m_packet_size_tracker.crud_insert_overhead_bytes =
      m_batch_insert.ByteSizeLong();

  m_session->execute("START TRANSACTION");

  bool cancel = false;
  shcore::Interrupt_handler intr_handler([&cancel]() -> bool {
    cancel = true;
    return false;
  });

  shcore::Json_reader reader(input, options);
  reader.parse_bom();

  while (!reader.eof() && !cancel) {
    std::string jd = reader.next();

    if (!jd.empty()) {
      // todo(kg): move jd string all the way to protobuf's
      // Scalar_String::set_value
      put(std::move(jd));
    }
  }

  flush();
  commit(true);

  if (cancel) throw shcore::cancelled("JSON documents import cancelled.");
}

void Json_importer::put(const std::string &item) {
  if (m_packet_size_tracker.will_overflow(item.size())) {
    flush();
    if (m_packet_size_tracker.inserts_in_this_transaction >=
        k_inserts_per_transaction) {
      commit();
    }
  }

  m_stats.bytes_processed += item.size();
  m_stats.items_processed++;
  add_to_request(item);
}

void Json_importer::update_statistics(xcl::XQuery_result *xquery_result) {
  if (xquery_result == nullptr) return;

  uint64_t affected_rows = 0;
  bool ret = xquery_result->try_get_affected_rows(&affected_rows);
  if (ret) {
    m_stats.documents_successfully_imported += affected_rows;
    if (m_print) {
      m_print(".. " + std::to_string(m_stats.documents_successfully_imported));
    }
  }
}

void Json_importer::recv_response(bool block) {
  if (m_pending_response > 0) {
    my_socket fd = m_session->get_driver_obj()
                       ->get_protocol()
                       .get_connection()
                       .get_socket_fd();
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    auto ready = select(fd + 1, &rfds, nullptr, nullptr, &timeout);
    bool fd_ready = FD_ISSET(fd, &rfds);
    if (block || (ready > 0 && fd_ready)) {
      xcl::XError error;
      auto result =
          m_session->get_driver_obj()->get_protocol().recv_resultset(&error);
      update_statistics(result.get());
      if (error) throw mysqlshdk::db::Error(error.what(), error.error());
      m_pending_response--;
    }
  }
}

void Json_importer::flush() {
  if (m_packet_size_tracker.rows_in_insert > 0) {
    xcl::XError error;
    if (m_proto_interleaved) {
      recv_response(true);
      error = m_session->get_driver_obj()->get_protocol().send(m_batch_insert);
      m_pending_response++;
    } else {
      auto result = m_session->get_driver_obj()->get_protocol().execute_insert(
          m_batch_insert, &error);
      update_statistics(result.get());
    }
    if (error) throw mysqlshdk::db::Error(error.what(), error.error());
    m_batch_insert.mutable_row()->Clear();
    m_packet_size_tracker.inserts_in_this_transaction++;
    m_packet_size_tracker.bytes_in_insert = 0;
    m_packet_size_tracker.rows_in_insert = 0;
  }
}

void Json_importer::commit(bool final_commit) {
  if (m_proto_interleaved) {
    xcl::XError error;
    recv_response(true);

    ::Mysqlx::Sql::StmtExecute stmt;
    stmt.set_stmt(!final_commit ? "COMMIT AND CHAIN" : "COMMIT");

    error = m_session->get_driver_obj()->get_protocol().send(stmt);
    if (error) throw mysqlshdk::db::Error(error.what(), error.error());

    m_pending_response++;
    if (final_commit) {
      while (m_pending_response > 0) recv_response(true);
    }
  } else {
    m_session->execute(!final_commit ? "COMMIT AND CHAIN" : "COMMIT");
  }
  m_packet_size_tracker.inserts_in_this_transaction = 0;
}

void Json_importer::add_to_request(const std::string &doc) {
  auto fields = m_batch_insert.mutable_row()->Add()->mutable_field();
  mysqlshdk::db::mysqlx::util::set_scalar(*fields->Add(), doc);

  m_packet_size_tracker.bytes_in_insert += doc.size();
  m_packet_size_tracker.rows_in_insert++;
}
}  // namespace mysqlsh
