/*
  Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "json_statement_reader.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <cassert>
#include <memory>

namespace {

// default allocator for rapidJson (MemoryPoolAllocator) is broken for SparcSolaris
using JsonDocument = rapidjson::GenericDocument<rapidjson::UTF8<>,  rapidjson::CrtAllocator>;
using JsonValue = rapidjson::GenericValue<rapidjson::UTF8<>,  rapidjson::CrtAllocator>;

std::string get_json_value_as_string(const JsonValue& value) {
  if (value.IsString()) return value.GetString();
  if (value.IsInt()) return std::to_string(value.GetInt());
  if (value.IsUint()) return std::to_string(value.GetUint());
  if (value.IsDouble()) return std::to_string(value.GetDouble());
  // TODO: implement other types when needed

  throw(std::runtime_error("Unsupported json value type: "
                           + std::to_string(static_cast<int>(value.GetType()))));
}

}

namespace server_mock {

struct QueriesJsonReader::Pimpl {

  JsonDocument json_document_;
  size_t current_stmt_{0u};

  void read_result_info(const JsonValue& stmt,
                        statement_info &out_statement_info);

  Pimpl(): json_document_() {}
};

QueriesJsonReader::QueriesJsonReader(const std::string &filename):
              pimpl_(new Pimpl()){
  if (filename.empty()) return;

#ifndef _WIN32
  const char* OPEN_MODE = "rb"; // after rapidjson doc
#else
  const char* OPEN_MODE = "r";
#endif
  FILE* fp = fopen(filename.c_str(), OPEN_MODE);
  if (!fp) {
    throw std::runtime_error("Could not open json queries file for reading: " + filename);
  }
  std::shared_ptr<void> exit_guard(nullptr, [&](void*){fclose(fp);});

  char readBuffer[65536];
  rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

  pimpl_->json_document_.ParseStream(is);

  if (!pimpl_->json_document_.HasMember("stmts")) {
    throw std::runtime_error("Wrong statements document structure: missing \"stmts\"");
  }

  if (!pimpl_->json_document_["stmts"].IsArray()) {
    throw std::runtime_error("Wrong statements document structure: \"stmts\" has to be an array");
  }

}

// this is needed for pimpl, otherwise compiler complains
// about pimpl unknown size in std::unique_ptr
QueriesJsonReader::~QueriesJsonReader() {}

QueriesJsonReader::statement_info QueriesJsonReader::get_next_statement() {
  statement_info result;

  const JsonValue& stmts = pimpl_->json_document_["stmts"];
  if (pimpl_->current_stmt_ >= stmts.Size()) return result;

  auto& stmt = stmts[pimpl_->current_stmt_++];
  if (!stmt.HasMember("stmt") && !stmt.HasMember("stmt.regex")) {
    throw std::runtime_error("Wrong statements document structure: missing \"stmt\" or \"stmt.regex\"");
  }

  std::string name{"stmt"};
  if (stmt.HasMember("stmt.regex")) {
    name = "stmt.regex";
    result.statement_is_regex = true;
  }

  if (!stmt[name.c_str()].IsString()) {
    throw std::runtime_error("Wrong statements document structure: \"stmt\" has to be a string");
  }

  result.statement = stmt[name.c_str()].GetString();

  if (stmt.HasMember("ok")) {
    result.result_type = statement_result_type::STMT_RES_OK;
  } else if (stmt.HasMember("error")) {
    result.result_type = statement_result_type::STMT_RES_ERROR;
  } else if (stmt.HasMember("result")) {
    result.result_type = statement_result_type::STMT_RES_RESULT;
    pimpl_->read_result_info(stmt, result);
  } else {
    throw std::runtime_error("Wrong statements document structure: \"stmt\" has to be a string");
  }

  return result;
}

void QueriesJsonReader::Pimpl::read_result_info(const JsonValue &stmt,
                                                statement_info &out_statement_info) {
  // only asserting as this should have been checked before if we got here
  assert(stmt.HasMember("result"));

  const auto& result = stmt["result"];

  // read columns
  if (result.HasMember("columns")) {
    const auto& columns = result["columns"];
    if (!columns.IsArray()) {
      throw std::runtime_error("Wrong statements document structure: \"columns\" has to be an array");
    }

    const std::vector<std::string> required_members{"type", "name"};

    for (size_t i = 0; i < columns.Size(); ++i) {
      auto& column = columns[i];
      column_info_type column_info;
      for (auto it = column.MemberBegin(); it != column.MemberEnd(); ++it) {
         std::string key = it->name.GetString();
         column_info[key] = column[key.c_str()].GetString();
      }

      for (const auto& required: required_members) {
         if (column_info.count(required) == 0) {
           throw std::runtime_error("Wrong statements document structure: \"columns\" instance has to have \"" + required + "\"");
         }
      }
      out_statement_info.resultset.columns.push_back(column_info);
    }
  }

  // read rows
  if (result.HasMember("rows")) {
    const auto& rows = result["rows"];
    if (!rows.IsArray()) {
      throw std::runtime_error("Wrong statements document structure: \"rows\" has to be an array");
    }

    auto columns_size = out_statement_info.resultset.columns.size();

    for (size_t i = 0; i < rows.Size(); ++i) {
      auto& row = rows[i];
      if (!row.IsArray()) {
        throw std::runtime_error("Wrong statements document structure: \"rows\" instance has to be an array");
      }

      if (row.Size() != columns_size) {
        throw std::runtime_error(std::string("Wrong statements document structure: ") +
            "number of row fields different than number of columns " +
            std::to_string(row.Size()) + " != " + std::to_string(columns_size));
      }

      row_values_type row_values;
      for (size_t j = 0; j < row.Size(); ++j) {
        row_values.push_back(get_json_value_as_string(row[j]));
      }

      out_statement_info.resultset.rows.push_back(row_values);
    }
  }
}

} // namespace
