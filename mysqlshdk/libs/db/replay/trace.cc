/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/replay/trace.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <utility>
#include "my_dbug.h"
#include "mysqlshdk/libs/db/replay/replayer.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"

// TODO(alfredo) warnings, multi-results, mysqlx

namespace mysqlshdk {
namespace db {
namespace replay {

sequence_error::sequence_error(const std::string &what)
    : db::Error(what.c_str(), 9999) {
  std::cerr << "SESSION REPLAY ERROR: " << what << "\n";

  mysqlshdk::utils::print_stacktrace();

  if (getenv("TEST_DEBUG")) assert(0);
}

template <typename T>
void set(rapidjson::Document *doc, const char *key, T value) {
  rapidjson::Value v(value);
  rapidjson::Value k(key, doc->GetAllocator());
  doc->AddMember(k, v, doc->GetAllocator());
}

void set(rapidjson::Document *doc, const char *key, const char *value) {
  rapidjson::Value v;
  rapidjson::Value k(key, doc->GetAllocator());
  if (!value) {
    v.SetNull();
  } else {
    v.SetString(value, doc->GetAllocator());
  }
  doc->AddMember(k, v, doc->GetAllocator());
}

void set(rapidjson::Document *doc, const char *key, const std::string &value) {
  rapidjson::Value v(value.c_str(), doc->GetAllocator());
  rapidjson::Value k(key, doc->GetAllocator());
  doc->AddMember(k, v, doc->GetAllocator());
}

template <typename T>
void set(rapidjson::Document *doc, rapidjson::Value *obj, const char *key,
         T value) {
  rapidjson::Value v(value);
  rapidjson::Value k(key, doc->GetAllocator());
  obj->AddMember(k, v, doc->GetAllocator());
}

void set(rapidjson::Document *doc, rapidjson::Value *obj, const char *key,
         const std::string &value) {
  rapidjson::Value v(value.c_str(), doc->GetAllocator());
  rapidjson::Value k(key, doc->GetAllocator());
  obj->AddMember(k, v, doc->GetAllocator());
}

std::string to_json(rapidjson::Value *value) {
  rapidjson::Document doc;
  doc.CopyFrom(*value, doc.GetAllocator());
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

std::string get_string(rapidjson::Value &value) {  // NOLINT
  if (!value.IsString()) {
    throw std::logic_error("Bad JSON data. String expected, got " +
                           to_json(&value));
  }
  return value.GetString();
}

int64_t get_int(rapidjson::Value &value) {  // NOLINT
  if (!value.IsInt64()) {
    throw std::logic_error("Bad JSON data. Int expected, got " +
                           to_json(&value));
  }
  return value.GetInt64();
}

uint64_t get_uint(rapidjson::Value &value) {  // NOLINT
  if (!value.IsUint64()) {
    throw std::logic_error("Bad JSON data. Uint expected, got " +
                           to_json(&value));
  }
  return value.GetUint64();
}

std::string make_json(
    const std::string &type, const std::string &subtype,
    const std::vector<std::pair<std::string, std::string>> &items, int i) {
  rapidjson::Document doc;
  doc.SetObject();
  set(&doc, "type", type);
  set(&doc, "subtype", subtype);
  set(&doc, "index", i);
  for (const auto &i : items) {
    set(&doc, i.first.c_str(), i.second);
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

void Trace_writer::serialize_connect(
    const mysqlshdk::db::Connection_options &data,
    const std::string &protocol) {
  _stream << make_json("request", "CONNECT",
                       {{"uri", data.as_uri(uri::formats::full())},
                        {"protocol", protocol}},
                       ++_idx)
          << ",\n";

  _log_label = shcore::path::basename(_path);
  auto ext = _log_label.rfind('.');
  if (ext != std::string::npos) _log_label = _log_label.substr(0, ext);
  _log_label.append("/")
      .append(data.get_host())
      .append(":")
      .append(std::to_string(data.get_port()));

  DBUG_LOG("sql",
           _log_label << ": connect " << data.as_uri(uri::formats::full()));
}

void Trace_writer::serialize_close() {
  DBUG_LOG("sql", _log_label << ": close");
  _stream << make_json("request", "CLOSE", {}, ++_idx) << ",\n";
}

void Trace_writer::serialize_query(const std::string &sql) {
  DBUG_LOG("sqlall", _log_label << ": " << sql);
  _stream << make_json("request", "QUERY", {{"sql", sql}}, ++_idx) << ",\n";
}

void Trace_writer::serialize_ok() {
  _stream << make_json("response", "OK", {}, ++_idx) << ",\n";
}

void Trace_writer::serialize_connect_ok(
    const std::map<std::string, std::string> &info) {
  rapidjson::Document doc;
  doc.SetObject();
  set(&doc, "type", "response");
  set(&doc, "subtype", "CONNECT_OK");
  set(&doc, "index", ++_idx);
  for (const auto &it : info) {
    set(&doc, it.first.c_str(), it.second.c_str());
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  _stream << buffer.GetString() << ",\n";
}

void serialize_result_metadata(rapidjson::Document *doc,
                               std::shared_ptr<db::IResult> result) {
  rapidjson::Value clist;
  clist.SetArray();
  for (const auto &column : result->get_metadata()) {
    rapidjson::Value cobj;
    cobj.SetObject();
    set(doc, &cobj, "schema", column.get_schema());
    set(doc, &cobj, "table_name", column.get_table_name());
    set(doc, &cobj, "table_label", column.get_table_label());
    set(doc, &cobj, "column_name", column.get_column_name());
    set(doc, &cobj, "column_label", column.get_column_label());
    set(doc, &cobj, "length", column.get_length());
    set(doc, &cobj, "fractional", column.get_fractional());
    set(doc, &cobj, "type", to_string(column.get_type()));
    set(doc, &cobj, "collation", column.get_collation_name());
    set(doc, &cobj, "charset", column.get_charset_name());
    set(doc, &cobj, "collation_id", column.get_collation());
    set(doc, &cobj, "unsigned", column.is_unsigned());
    set(doc, &cobj, "zerofill", column.is_zerofill());
    set(doc, &cobj, "binary", column.is_binary());
    clist.PushBack(cobj, doc->GetAllocator());
  }
  doc->AddMember("columns", clist, doc->GetAllocator());
}

void push_field(
    rapidjson::Document *doc, rapidjson::Value *array, const db::IRow *row,
    uint32_t i,
    const std::function<std::string(const std::string &value)> &hook) {
  rapidjson::Value value;
  if (row->is_null(i)) {
    value.SetNull();
  } else {
    switch (row->get_type(i)) {
      case Type::Null:
        value.SetNull();
        break;
      case Type::Integer:
        value = row->get_int(i);
        break;
      case Type::UInteger:
        value = row->get_uint(i);
        break;
      case Type::Float:
        value = row->get_float(i);
        break;
      case Type::Double:
        value = row->get_double(i);
        break;
      case Type::Decimal:
        value.SetString(row->get_as_string(i).c_str(), doc->GetAllocator());
        break;
      case Type::Bit:
        value = row->get_bit(i);
        break;
      case Type::String:
      case Type::Bytes:
      case Type::Geometry:
      case Type::Json:
      case Type::Date:
      case Type::Time:
      case Type::DateTime:
      case Type::Enum:
      case Type::Set:
        if (hook) {
          value.SetString(hook(row->get_as_string(i)).c_str(),
                          doc->GetAllocator());
        } else {
          value.SetString(row->get_as_string(i).c_str(), doc->GetAllocator());
        }
        break;
    }
  }
  array->PushBack(value, doc->GetAllocator());
}

bool is_set_as_string(Type type) {
  switch (type) {
    case Type::String:
    case Type::Bytes:
    case Type::Geometry:
    case Type::Json:
    case Type::Date:
    case Type::Time:
    case Type::DateTime:
    case Type::Enum:
    case Type::Set:
      return true;
    default:
      return false;
  }
}

void serialize_result_rows(
    rapidjson::Document *doc, std::shared_ptr<db::IResult> result,
    const std::function<std::string(const std::string &value)> &hook) {
  rapidjson::Value rlist;
  rlist.SetArray();
  const db::IRow *row = result->fetch_one();
  while (row) {
    rapidjson::Value fields;
    fields.SetArray();
    for (uint32_t i = 0; i < row->num_fields(); i++) {
      push_field(doc, &fields, row, i, hook);
    }
    rlist.PushBack(fields, doc->GetAllocator());
    row = result->fetch_one();
  }
  doc->AddMember("rows", rlist, doc->GetAllocator());
}

void Trace_writer::serialize_result(
    std::shared_ptr<db::IResult> result,
    const std::function<std::string(const std::string &value)> &hook) {
  try {
    rapidjson::Document doc;
    doc.SetObject();
    set(&doc, "type", "response");
    set(&doc, "subtype", "RESULT");
    set(&doc, "index", ++_idx);

    set(&doc, "auto_increment_value", result->get_auto_increment_value());
    set(&doc, "affected_rows", result->get_affected_row_count());
    set(&doc, "warning_count", result->get_warning_count());
    set(&doc, "info", result->get_info());

    // Recording of gtids is not mandatory, i.e. is done only when they
    // are available, and it only occurs for Classic Sessions at the moment
    try {
      std::vector<std::string> gtids = result->get_gtids();
      if (!gtids.empty()) {
        set(&doc, "gtids", shcore::str_join(gtids, ","));
      }
    } catch (const std::logic_error &error) {
      // NO-OP: for 'not implemented' on the x protocol
      std::string msg(error.what());
      if (msg != "not implemented") throw;
    }
    if (result->has_resultset()) {
      serialize_result_metadata(&doc, result);
      serialize_result_rows(&doc, result, hook);
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    _stream << buffer.GetString() << ",\n";
  } catch (std::exception &e) {
    std::cerr << "Exception serializing result trace: " << e.what() << "\n";
    throw;
  }
}

void Trace_writer::serialize_error(const db::Error &e) {
  DBUG_LOG("sql",
           _log_label << ": MySQL error: " << e.what() << " (" << e.code());
  _stream << make_json("response", "ERROR",
                       {{"code", std::to_string(e.code())},
                        {"msg", e.what()},
                        {"sqlstate", e.sqlstate()}},
                       ++_idx)
          << ",\n";
}

void Trace_writer::serialize_error(const std::runtime_error &e) {
  DBUG_LOG("sql", "Runtime error in " << _path << ": " << e.what());
  _stream << make_json("response", "ERROR",
                       {{"code", ""}, {"msg", e.what()}, {"sqlstate", ""}},
                       ++_idx)
          << ",\n";
}

Trace_writer *Trace_writer::create(const std::string &path) {
  return new Trace_writer(path);
}

void Trace_writer::set_metadata(
    const std::map<std::string, std::string> &meta) {
  rapidjson::Document doc;
  doc.SetObject();

  rapidjson::Value value;
  value.SetObject();
  for (const auto &i : meta) {
    rapidjson::Value k(i.first.c_str(), doc.GetAllocator());
    rapidjson::Value v(i.second.c_str(), doc.GetAllocator());
    value.AddMember(k, v, doc.GetAllocator());
  }
  doc.AddMember("metadata", value, doc.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  _stream << buffer.GetString() << ",\n";
}

Trace_writer::Trace_writer(const std::string &path) : _path(path) {
  _log_label = shcore::path::basename(path);
  DBUG_LOG("sql", "Creating trace file " << path);
  _stream.open(path);
  if (_stream.bad()) throw std::logic_error(path + ": " + strerror(errno));
  _stream.rdbuf()->pubsetbuf(0, 0);
  _stream << "[\n";
}

Trace_writer::~Trace_writer() {
  _stream << "null]\n";
  DBUG_LOG("sql", "Closed trace file " << _path << " (" << _idx << " entries)");
}

// ------------------------------------------------

Trace::Trace(const std::string &path) : _trace_path(path) {
  std::FILE *file;
  char buffer[1024 * 4];

  DBUG_LOG("sql", "Opening trace file " << path);

  file = std::fopen(path.c_str(), "r");
  if (!file) throw std::logic_error(path + ": " + strerror(errno));

  _index = 0;
  rapidjson::FileReadStream stream(file, buffer, sizeof(buffer));
  _doc.ParseStream(stream);
  std::fclose(file);
  if (_doc.HasParseError()) {
    std::cerr << "REPLAY ERROR: Error parsing trace file " << path << ":"
              << _doc.GetErrorOffset() << ":"
              << rapidjson::GetParseError_En(_doc.GetParseError()) << "\n";
    throw std::logic_error(
        shcore::str_format("Error parsing trace file %s:%zu:%s", path.c_str(),
                           _doc.GetErrorOffset(),
                           rapidjson::GetParseError_En(_doc.GetParseError())));
  }
  assert(_doc.IsArray());
}

Trace::~Trace() {}

void Trace::next(rapidjson::Value *entry) {
  if (_index >= _doc.Size() - 1) throw sequence_error("Session trace is over");

  *entry = _doc[_index++];

  if (0) {
    std::cerr << "Trace read: " << to_json(entry) << "\n";
  }
}

std::map<std::string, std::string> Trace::get_metadata() { return {}; }

void Trace::expect_request(rapidjson::Value *doc, const char *subtype,
                           const char *detail) {
  if (_got_error)
    throw sequence_error("Session " + _trace_path + " is invalidated");

  if (strcmp((*doc)["type"].GetString(), "request") != 0) {
    _got_error = true;
    throw sequence_error(("Attempted a request in replayed session " +
                          _trace_path +
                          ", but got something else: " + to_json(doc))
                             .c_str());
  }
  if (strcmp((*doc)["subtype"].GetString(), subtype) != 0) {
    _got_error = true;
    if (detail)
      throw sequence_error(
          shcore::str_format(
              "Attempting '%s' (%s) but replayed session %s has %s", subtype,
              detail, _trace_path.c_str(), to_json(doc).c_str())
              .c_str());
    else
      throw sequence_error(
          shcore::str_format("Attempting '%s' but replayed session %s has %s",
                             subtype, _trace_path.c_str(), to_json(doc).c_str())
              .c_str());
  }
}

mysqlshdk::db::Connection_options Trace::expected_connect() {
  rapidjson::Value obj;
  next(&obj);

  expect_request(&obj, "CONNECT");
  DBUG_LOG("sqlall", shcore::path::basename(_trace_path)
                         << ": connect: " << obj["uri"].GetString());
  return mysqlshdk::db::Connection_options(obj["uri"].GetString());
}

void Trace::expected_close() {
  rapidjson::Value obj;
  next(&obj);

  expect_request(&obj, "CLOSE");
}

std::string Trace::expected_query(const std::string &expected) {
  rapidjson::Value obj;
  next(&obj);

  expect_request(&obj, "QUERY", expected.c_str());
  std::string query = obj["sql"].GetString();
  DBUG_LOG("sqlall", shcore::path::basename(_trace_path) << ": " << query);
  return query;
}

void throw_error(rapidjson::Value *doc) {
  const char *subtype = (*doc)["subtype"].GetString();
  if (strcmp(subtype, "ERROR") == 0) {
    throw db::Error((*doc)["msg"].GetString(),
                    std::atoi((*doc)["code"].GetString()),
                    (*doc)["sqlstate"].GetString());
  } else {
    throw std::logic_error(
        shcore::str_format("Unexpected entry in replayed session %s",
                           to_json(doc).c_str())
            .c_str());
  }
}

void Trace::expected_status() {
  rapidjson::Value obj;
  next(&obj);

  if (strcmp(obj["type"].GetString(), "response") != 0)
    throw sequence_error(
        "Expected response in session trace, but got something else");

  const char *subtype = obj["subtype"].GetString();
  if (strcmp(subtype, "OK") == 0) {
  } else {
    throw_error(&obj);
  }
}

void Trace::expected_connect_status(
    std::map<std::string, std::string> *out_info) {
  rapidjson::Value obj;
  next(&obj);

  if (strcmp(obj["type"].GetString(), "response") != 0)
    throw sequence_error(
        "Expected response in session trace, but got something else");

  const char *subtype = obj["subtype"].GetString();
  if (strcmp(subtype, "CONNECT_OK") == 0) {
    for (auto itr = obj.MemberBegin(); itr != obj.MemberEnd(); ++itr) {
      if (itr->value.IsString())
        (*out_info)[itr->name.GetString()] = itr->value.GetString();
    }
  } else {
    throw_error(&obj);
  }
}

void unserialize_result_metadata(rapidjson::Value *clist,
                                 std::vector<Column> *metadata) {
  for (unsigned i = 0; i < clist->Size(); i++) {
    rapidjson::Value &cobj((*clist)[i]);
    db::Column column(
        "", get_string(cobj["schema"]), get_string(cobj["table_name"]),
        get_string(cobj["table_label"]), get_string(cobj["column_name"]),
        get_string(cobj["column_label"]), get_int(cobj["length"]),
        get_int(cobj["fractional"]),
        db::string_to_type(get_string(cobj["type"])),
        get_int(cobj["collation_id"]), cobj["unsigned"].GetBool(),
        cobj["zerofill"].GetBool(), cobj["binary"].GetBool());
    metadata->push_back(column);
  }
}

class Row_unserializer : public db::IRow {
  rapidjson::Value &_fields;
  const std::vector<db::Column> &_columns;

 public:
  Row_unserializer(rapidjson::Value &fields,  // NOLINT(runtime/references)
                   const std::vector<db::Column> &columns)
      : _fields(fields), _columns(columns) {}

  uint32_t num_fields() const override { return _columns.size(); }

  Type get_type(uint32_t index) const override {
    return _columns[index].get_type();
  }

  bool is_null(uint32_t index) const override {
    return _fields[index].IsNull();
  }

  std::string get_as_string(uint32_t index) const override {
    if (_fields[index].IsString()) return _fields[index].GetString();
    return to_json(&_fields[index]);
  }

  std::string get_string(uint32_t index) const override {
    return replay::get_string(_fields[index]);
  }

  int64_t get_int(uint32_t index) const override {
    return replay::get_int(_fields[index]);
  }

  uint64_t get_uint(uint32_t index) const override {
    return _fields[index].GetUint64();
  }

  float get_float(uint32_t index) const override {
    return _fields[index].GetDouble();
  }

  double get_double(uint32_t index) const override {
    return _fields[index].GetDouble();
  }

  std::pair<const char *, size_t> get_string_data(uint32_t) const override {
    throw std::logic_error("not implemented");
  }

  uint64_t get_bit(uint32_t index) const override {
    return replay::get_int(_fields[index]);
  }
};

void Trace::unserialize_result_rows(
    rapidjson::Value *rlist, std::shared_ptr<Result_mysql> result,
    std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept) {
  for (unsigned i = 0; i < rlist->Size(); i++) {
    if (intercept) {
      std::unique_ptr<IRow> row_copier(intercept(std::unique_ptr<IRow>{
          new Row_unserializer((*rlist)[i], result->_metadata)}));
      result->_rows.emplace_back(*row_copier);
    } else {
      result->_rows.emplace_back(
          Row_unserializer((*rlist)[i], result->_metadata));
    }
  }
}

void Trace::unserialize_result_rows(
    rapidjson::Value *rlist, std::shared_ptr<Result_mysqlx> result,
    std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept) {
  for (unsigned i = 0; i < rlist->Size(); i++) {
    if (intercept) {
      std::unique_ptr<IRow> row_copier(intercept(std::unique_ptr<IRow>{
          new Row_unserializer((*rlist)[i], result->_metadata)}));
      result->_rows.emplace_back(*row_copier);
    } else {
      result->_rows.emplace_back(
          Row_unserializer((*rlist)[i], result->_metadata));
    }
  }
}

std::shared_ptr<Result_mysql> Trace::expected_result(
    std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept) {
  rapidjson::Value obj;
  next(&obj);

  if (strcmp(obj["type"].GetString(), "response") != 0)
    throw sequence_error(
        "Expected response in session trace, but got something else");

  const char *subtype = obj["subtype"].GetString();
  if (strcmp(subtype, "RESULT") == 0) {
    auto last_insert_id = get_int(obj["auto_increment_value"]);
    auto affected_rows = get_uint(obj["affected_rows"]);
    auto warning_count = get_int(obj["warning_count"]);
    auto info = get_string(obj["info"]);
    std::string gtids = obj.HasMember("gtids") ? get_string(obj["gtids"]) : "";

    std::shared_ptr<Result_mysql> result(new Result_mysql(
        affected_rows, warning_count, last_insert_id, info.c_str(),
        gtids.empty() ? std::vector<std::string>()
                      : shcore::str_split(gtids, ",")));

    if (obj.HasMember("columns")) {
      result->_has_resultset = true;
      rapidjson::Value &cols(obj["columns"]);
      unserialize_result_metadata(&cols, &result->_metadata);
    }
    if (obj.HasMember("rows")) {
      rapidjson::Value &rows(obj["rows"]);
      unserialize_result_rows(&rows, result, intercept);
    }
    return result;
  } else {
    throw_error(&obj);
  }
  return {};
}

std::shared_ptr<Result_mysqlx> Trace::expected_result_x(
    std::function<std::unique_ptr<IRow>(std::unique_ptr<IRow>)> intercept) {
  rapidjson::Value obj;
  next(&obj);

  if (strcmp(obj["type"].GetString(), "response") != 0)
    throw sequence_error(
        "Expected response in session trace, but got something else");

  const char *subtype = obj["subtype"].GetString();
  if (strcmp(subtype, "RESULT") == 0) {
    auto last_insert_id = get_int(obj["auto_increment_value"]);
    auto affected_rows = get_uint(obj["affected_rows"]);
    auto warning_count = get_int(obj["warning_count"]);
    auto info = get_string(obj["info"]);

    std::shared_ptr<Result_mysqlx> result(new Result_mysqlx(
        affected_rows, warning_count, last_insert_id, info.c_str()));

    if (obj.HasMember("columns")) {
      result->_has_resultset = true;
      rapidjson::Value &cols(obj["columns"]);
      unserialize_result_metadata(&cols, &result->_metadata);
    }
    if (obj.HasMember("rows")) {
      rapidjson::Value &rows(obj["rows"]);
      unserialize_result_rows(&rows, result, intercept);
    }
    return result;
  } else {
    throw_error(&obj);
  }
  return {};
}

//--------------

void save_info(const std::string &path,
               const std::map<std::string, std::string> &state) {
  rapidjson::Document doc;
  doc.SetObject();
  for (const auto &m : state) {
    set(&doc, m.first.c_str(), m.second.c_str());
  }

  std::FILE *file;
  char buffer[1024 * 4];

  file = std::fopen(path.c_str(), "w+");
  if (!file) throw std::runtime_error(path + ": " + strerror(errno));

  rapidjson::FileWriteStream stream(file, buffer, sizeof(buffer));
  rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);
  doc.Accept(writer);
  std::fclose(file);
}

std::map<std::string, std::string> load_info(const std::string &path) {
  std::FILE *file;
  char buffer[1024 * 4];

  file = std::fopen(path.c_str(), "r");
  if (!file) throw std::runtime_error(path + ": " + strerror(errno));

  rapidjson::Document doc;
  rapidjson::FileReadStream stream(file, buffer, sizeof(buffer));
  doc.ParseStream(stream);
  std::fclose(file);

  std::map<std::string, std::string> map;
  for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
    map[itr->name.GetString()] = itr->value.GetString();
  }
  return map;
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
