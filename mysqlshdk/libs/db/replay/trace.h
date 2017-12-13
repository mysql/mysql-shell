/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_REPLAY_TRACE_H_
#define MYSQLSHDK_LIBS_DB_REPLAY_TRACE_H_

#include <rapidjson/document.h>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
namespace replay {

class Trace_writer {
 public:
  ~Trace_writer();
  static Trace_writer* create(const std::string& path, int print_traces);

  void set_metadata(const std::map<std::string, std::string>& meta);

  void serialize_connect(const mysqlshdk::db::Connection_options& data,
                         const std::string &protocol);
  void serialize_close();
  void serialize_query(const std::string& sql);

  void serialize_connect_ok(const std::map<std::string, std::string> &info);
  void serialize_ok();
  void serialize_result(std::shared_ptr<db::IResult> result);
  void serialize_error(const db::Error& e);

  const std::string &trace_path() const { return _path; }
  int trace_index() const { return _idx; }

 private:
  Trace_writer(const std::string& path, int print_traces);
  std::string _path;
  std::ofstream _stream;
  int _idx = 0;
  int _print_traces = 0;
};

void save_info(const std::string& path,
               const std::map<std::string, std::string>& state);
std::map<std::string, std::string> load_info(const std::string& path);

class sequence_error : public db::Error {
 public:
  explicit sequence_error(const std::string &what);
};

class Row_hook : public db::IRow {
 public:
  explicit Row_hook(std::unique_ptr<db::IRow> source)
      : _source(std::move(source)) {
  }

  uint32_t num_fields() const override {
    return _source->num_fields();
  }

  Type get_type(uint32_t index) const override {
    return _source->get_type(index);
  }

  bool is_null(uint32_t index) const override {
    return _source->is_null(index);
  }

  std::string get_as_string(uint32_t index) const override {
    return _source->get_as_string(index);
  }

  std::string get_string(uint32_t index) const override {
    return _source->get_string(index);
  }

  int64_t get_int(uint32_t index) const override {
    return _source->get_int(index);
  }

  uint64_t get_uint(uint32_t index) const override {
    return _source->get_uint(index);
  }

  float get_float(uint32_t index) const override {
    return _source->get_float(index);
  }

  double get_double(uint32_t index) const override {
    return _source->get_double(index);
  }

  std::pair<const char*, size_t> get_string_data(
      uint32_t index) const override {
    return _source->get_string_data(index);
  }

  uint64_t get_bit(uint32_t index) const override {
    return _source->get_bit(index);
  }

 private:
  std::unique_ptr<db::IRow> _source;
};

using Query_hook = std::function<std::string(const std::string &)>;

using Result_row_hook = std::function<std::unique_ptr<db::IRow>(
    const mysqlshdk::db::Connection_options&, const std::string&,
    std::unique_ptr<db::IRow>)>;

class Result_mysql;
class Result_mysqlx;

class Trace {
 public:
  Trace(const std::string& path, int print_traces);
  ~Trace();

  std::map<std::string, std::string> get_metadata();

  mysqlshdk::db::Connection_options expected_connect();
  void expected_close();
  std::string expected_query();

  void expected_connect_status(std::map<std::string, std::string> *out_info);
  void expected_status();
  std::shared_ptr<Result_mysql> expected_result(
      std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept);
  std::shared_ptr<Result_mysqlx> expected_result_x(
      std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept);

  const std::string& trace_path() const {
    return _trace_path;
  }

  size_t trace_index() const {
    return static_cast<size_t>(_index);
  }

 private:
  void next(rapidjson::Value* entry);
  void unserialize_result_rows(
      rapidjson::Value* rlist, std::shared_ptr<Result_mysql> result,
      std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept);
  void unserialize_result_rows(
      rapidjson::Value* rlist, std::shared_ptr<Result_mysqlx> result,
      std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept);
  void expect_request(rapidjson::Value* doc, const char* subtype);
  rapidjson::Document _doc;
  rapidjson::SizeType _index;
  std::string _trace_path;
  int _print_traces = 0;
};

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_REPLAY_TRACE_H_
