/*
 * Copyright (c) 2023, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/custom_result.h"

#include <optional>

#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/utils/error.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace {

const shcore::Value NULL_VALUE = shcore::Value::Null();

void deduce_metadata(const shcore::Value *value,
                     std::optional<mysqlshdk::db::Type> &type,
                     std::optional<uint32_t> &length,
                     std::set<std::string> *flags) {
  // Deduce the type if not defined
  if (!type.has_value() && value) {
    switch (value->get_type()) {
      case shcore::Value_type::Array:
      case shcore::Value_type::Map:
        type = mysqlshdk::db::Type::Json;
        break;
      case shcore::Value_type::Binary:
        type = mysqlshdk::db::Type::Bytes;
        break;
      case shcore::Value_type::Bool:
        type = mysqlshdk::db::Type::UInteger;
        break;
      case shcore::Value_type::Float:
        type = mysqlshdk::db::Type::Double;
        break;
      case shcore::Value_type::Integer:
        type = mysqlshdk::db::Type::Integer;
        break;
      case shcore::Value_type::Null:
      case shcore::Value_type::Undefined:
      case shcore::Value_type::String:
        type = mysqlshdk::db::Type::String;
        break;
      case shcore::Value_type::Object: {
        auto date = value->as_object<shcore::Date>();
        if (date) {
          if (date->has_date() && date->has_time()) {
            type = mysqlshdk::db::Type::DateTime;

          } else if (date->has_date()) {
            type = mysqlshdk::db::Type::Date;
          } else {
            type = mysqlshdk::db::Type::Time;
          }
        } else {
          throw std::runtime_error(
              shcore::str_format("Unsupported data type in custom results: %s",
                                 shcore::type_name(value->get_type()).c_str()));
        }
        break;
      }
      default:
        throw std::runtime_error(
            shcore::str_format("Unsupported data type in custom results: %s",
                               shcore::type_name(value->get_type()).c_str()));
    }
  }

  // Deduce the length if not defined
  if (!length.has_value() && type.has_value()) {
    if (type == mysqlshdk::db::Type::Integer) {
      // For the purpose of the custom results, we pick the highest size for
      // interger types
      length = 20;
    } else if (type == mysqlshdk::db::Type::UInteger) {
      if (value && value->get_type() == shcore::Value_type::Bool) {
        length = 4;
      } else {
        length = 10;
      }
    } else if (type == mysqlshdk::db::Type::Double) {
      length = 22;
    } else if (type == mysqlshdk::db::Type::Float) {
      length = 12;
    } else if (type == mysqlshdk::db::Type::Time ||
               type == mysqlshdk::db::Type::DateTime ||
               type == mysqlshdk::db::Type::Date ||
               type == mysqlshdk::db::Type::Bytes ||
               type == mysqlshdk::db::Type::String) {
      if (value) {
        length = value->descr().size();
      }
    } else if (type == mysqlshdk::db::Type::Json) {
      if (value) {
        length = value->descr().size();
      }
    }
  }

  // If the length was not deduced yet, assign 0
  if (!length.has_value()) {
    length = 0;
  }

  // Deduce flags if provided
  if (flags && type.has_value()) {
    switch (*type) {
      case mysqlshdk::db::Type::Bytes:
      case mysqlshdk::db::Type::Date:
      case mysqlshdk::db::Type::Time:
      case mysqlshdk::db::Type::DateTime:
      case mysqlshdk::db::Type::Json:
        flags->insert("BINARY");
        break;
      case mysqlshdk::db::Type::Integer:
      case mysqlshdk::db::Type::UInteger:
      case mysqlshdk::db::Type::Double:
        flags->insert("NUM");
      default:
        break;
    }
  }
}

/**
 * Custom result mapping from allowed type names to DB types.
 */
mysqlshdk::db::Type db_type(const std::string &value) {
  if (shcore::str_caseeq("string", value)) {
    return mysqlshdk::db::Type::String;
  } else if (shcore::str_caseeq("integer", value)) {
    return mysqlshdk::db::Type::Integer;
  } else if (shcore::str_caseeq("float", value)) {
    return mysqlshdk::db::Type::Double;
  } else if (shcore::str_caseeq("double", value)) {
    return mysqlshdk::db::Type::Double;
  } else if (shcore::str_caseeq("json", value)) {
    return mysqlshdk::db::Type::Json;
  } else if (shcore::str_caseeq("date", value)) {
    return mysqlshdk::db::Type::Date;
  } else if (shcore::str_caseeq("time", value)) {
    return mysqlshdk::db::Type::Time;
  } else if (shcore::str_caseeq("datetime", value)) {
    return mysqlshdk::db::Type::DateTime;
  } else if (shcore::str_caseeq("bytes", value)) {
    return mysqlshdk::db::Type::Bytes;
  }

  throw std::runtime_error("Unsupported data type.");
}

void try_parse(const std::string &context, std::function<void()> function) {
  try {
    function();
  } catch (const std::exception &error) {
    throw std::runtime_error(
        shcore::str_format("%s: %s", context.c_str(), error.what()));
  }
}

mysqlshdk::db::Warning parse_warning(const shcore::Dictionary_t &data) {
  std::string token;
  mysqlshdk::db::Warning warning;

  try {
    token = "message";
    auto message = data->get_string(token);

    if (message.empty()) {
      throw shcore::Exception::runtime_error(
          "mandatory field, can not be empty");
    }
    token = "level";
    auto level_str = data->get_string(token);
    mysqlshdk::db::Warning::Level level;
    if (shcore::str_caseeq(level_str, "warning")) {
      level = mysqlshdk::db::Warning::Level::Warn;
    } else if (shcore::str_caseeq(level_str, "note")) {
      level = mysqlshdk::db::Warning::Level::Note;
    } else {
      throw shcore::Exception::runtime_error(
          shcore::str_format("Invalid value for level '%s' at '%s', "
                             "allowed values: warning, note",
                             level_str.c_str(), message.c_str()));
    }

    token = "code";
    uint64_t code = data->get_uint(token);

    warning.level = level;
    warning.msg = message;
    warning.code = code;
  } catch (const std::exception &error) {
    throw shcore::Exception::runtime_error(shcore::str_format(
        "Error processing result warning %s: %s", token.c_str(), error.what()));
  }

  return warning;
}

}  // namespace

Custom_row::Custom_row(const std::shared_ptr<Custom_result> &result,
                       const shcore::Dictionary_t &data, size_t row_index)
    : m_num_fields(result->get_metadata().size()),
      m_result(result),
      m_data(data),
      m_row_index(row_index) {}

Custom_row::Custom_row(const std::shared_ptr<Custom_result> &result,
                       const shcore::Array_t &data, size_t row_index)
    : m_num_fields(result->get_metadata().size()),
      m_result(result),
      m_data(data),
      m_row_index(row_index) {}

#define FIELD_ERROR(index, msg) \
  std::invalid_argument(        \
      shcore::str_format("%s(%zi): " msg, __FUNCTION__, index).c_str())

#define FIELD_ERROR1(index, msg, arg) \
  std::invalid_argument(              \
      shcore::str_format("%s(%zi): " msg, __FUNCTION__, index, arg).c_str())

#define VALIDATE_INDEX(index)                          \
  do {                                                 \
    if (index >= m_num_fields)                         \
      throw FIELD_ERROR(index, "index out of bounds"); \
  } while (0)

uint32_t Custom_row::num_fields() const { return m_num_fields; }

const mysqlshdk::db::Column &Custom_row::column(size_t index) const {
  VALIDATE_INDEX(index);
  auto result = m_result.lock();
  if (result) {
    return result->get_metadata().at(index);
  } else {
    throw std::runtime_error("The column information is no longer available.");
  }
}

/**
 * This function is used to rethrow value access errors to append to the error
 * information about the row number and the column that caused the problem.
 */
shcore::Exception Custom_row::format_exception(const shcore::Exception &error,
                                               size_t index) const {
  auto result = m_result.lock();
  if (result) {
    return shcore::Exception::type_error(shcore::str_format(
        "%s, at row %zd, column '%s'", error.what(), m_row_index,
        result->get_metadata().at(index).get_column_label().c_str()));
  }

  return error;
}

const shcore::Value &Custom_row::data(size_t index) const {
  if (std::holds_alternative<shcore::Dictionary_t>(m_data)) {
    const auto &label = column(index).get_column_label();
    const auto &data = std::get<shcore::Dictionary_t>(m_data);
    return data->has_key(label) ? data->at(label) : NULL_VALUE;
  } else {
    return (*std::get<shcore::Array_t>(m_data))[index];
  }
}

mysqlshdk::db::Type Custom_row::get_type(uint32_t index) const {
  return column(index).get_type();
}

bool Custom_row::is_null(uint32_t index) const {
  return data(index).get_type() == shcore::Value_type::Null;
}

std::string Custom_row::get_as_string(uint32_t index) const {
  try {
    return data(index).get_string();
  } catch (const shcore::Exception &err) {
    return data(index).descr();
  }
}

std::string Custom_row::get_string(uint32_t index) const {
  return get_as_string(index);
}

std::wstring Custom_row::get_wstring(uint32_t index) const {
  try {
    return shcore::utf8_to_wide(get_string(index));
  } catch (const shcore::Exception &err) {
    throw format_exception(err, index);
  }
}

int64_t Custom_row::get_int(uint32_t index) const {
  try {
    return data(index).as_int();
  } catch (const shcore::Exception &err) {
    throw format_exception(err, index);
  }
}

uint64_t Custom_row::get_uint(uint32_t index) const {
  try {
    return data(index).as_uint();
  } catch (const shcore::Exception &err) {
    throw format_exception(err, index);
  }
}

float Custom_row::get_float(uint32_t index) const {
  try {
    return get_double(index);
  } catch (const shcore::Exception &err) {
    throw format_exception(err, index);
  }
}

double Custom_row::get_double(uint32_t index) const {
  try {
    return data(index).as_double();
  } catch (const shcore::Exception &err) {
    throw format_exception(err, index);
  }
}

std::pair<const char *, size_t> Custom_row::get_string_data(
    uint32_t index) const {
  const std::string &value = data(index).get_string();
  return {value.c_str(), value.size()};
}

void Custom_row::get_raw_data(uint32_t index, const char **out_data,
                              size_t *out_size) const {
  m_raw_data_cache = get_string(index);
  if (*out_data) {
    *out_data = m_raw_data_cache.c_str();
  }

  if (out_size) {
    *out_size = m_raw_data_cache.size();
  }
}

std::tuple<uint64_t, int> Custom_row::get_bit(uint32_t /*index*/) const {
  return {};
}

Custom_result::Custom_result(const shcore::Dictionary_t &data)
    : m_raw_result(data) {
  // This is a valid result, would be an OK result with no additional
  // information
  if (!m_raw_result) return;

  try_parse("Error processing result warnings", [this]() {
    auto warnings = m_raw_result->get_array("warnings");
    if (warnings) {
      for (const auto &warning : (*warnings)) {
        m_warnings.push_back(parse_warning(warning.as_map()));
      }
    }
  });

  try_parse("Error processing result columns", [this]() {
    m_result_columns = m_raw_result->get_array("columns");
  });

  try_parse("Error processing result data",
            [this]() { m_result_data = m_raw_result->get_array("data"); });

  if (m_result_data && !m_result_data->empty()) {
    check_data_consistency();

    if (m_result_columns && !m_result_columns->empty()) {
      parse_column_metadata();
    } else {
      deduce_column_metadata();
    }

    m_has_result = true;
  }

  try_parse("Error processing result executionTime", [this]() {
    auto execution_time = m_raw_result->get_double("executionTime");
    if (execution_time < 0) {
      throw std::runtime_error("the value can not be negative.");
    }
    set_execution_time(m_raw_result->get_double("executionTime"));
  });

  try_parse("Error processing result affectedItemsCount", [this]() {
    m_affected_items_count = m_raw_result->get_uint("affectedItemsCount");
  });

  try_parse("Error processing result info",
            [this]() { m_info = m_raw_result->get_string("info"); });

  try_parse("Error processing result autoIncrementValue", [this]() {
    m_auto_increment_value = m_raw_result->get_int("autoIncrementValue");
  });
}

/**
 * Creates the result data based on a list of documents.
 *
 * On this case, the column metadata is automatically deducted based on the
 * first document where:
 *
 * - Each document item represents a column.
 * - The data type is determined by the item value type.
 *
 * PROs:
 * - Simple to define a result in this format
 *
 * CONs:
 * - Column metadata is limited: only column label and type can be deducted.
 * - Limited data type support (only basic types are possible).
 * - Order of columns in the result is not guaranteed (no way around it).
 */
void Custom_result::check_data_consistency() {
  size_t num_columns = m_result_columns ? m_result_columns->size() : 0;
  size_t d_count = 0;
  size_t a_count = 0;

  // Make sure everything on the result is a document.
  for (const auto &record : *m_result_data) {
    if (record.get_type() == shcore::Value_type::Map) {
      d_count++;
    } else if (record.get_type() == shcore::Value_type::Array) {
      if (num_columns) {
        if (record.as_array()->size() != num_columns) {
          throw std::runtime_error(
              "The number of values in a record must match the number of "
              "columns.");
        } else {
          a_count++;
        }
      } else {
        throw std::runtime_error(
            "A record can not be represented as a list of values if the "
            "columns are not defined.");
      }
    } else {
      throw std::runtime_error(shcore::str_format(
          "A record is represented as a %sdictionary, unexpected format: %s",
          num_columns ? "list of values or a " : "", record.descr().c_str()));
    }
  }

  if (d_count && a_count) {
    throw std::runtime_error(
        "Inconsistent data in result, all the records should be either lists "
        "or dictionaries, but not mixed.");
  }
}

void Custom_result::parse_column_metadata() {
  shcore::Array_t a_result;
  shcore::Dictionary_t d_result;
  if (m_result_data->at(0).get_type() == shcore::Value_type::Array) {
    a_result = m_result_data->at(0).as_array();
  } else {
    d_result = m_result_data->at(0).as_map();
  }

  // Using an index in case we need to access the record data
  for (size_t index = 0; index < m_result_columns->size(); index++) {
    const auto &column = m_result_columns->at(index);
    std::string name;

    // Metadata containers
    std::optional<mysqlshdk::db::Type> type;
    std::optional<uint32_t> length;
    std::set<std::string> flags;
    bool flags_defined = false;

    static const std::set<std::string, shcore::Case_insensitive_comparator>
        k_allowed_flags = {"BLOB",   "TIMESTAMP", "UNSIGNED", "ZEROFILL",
                           "BINARY", "ENUM",      "SET"};

    auto has_flag = [&](const std::string &flag) {
      return flags.find(flag) != flags.end();
    };

    if (column.get_type() == shcore::Value_type::String) {
      name = column.as_string();
    } else if (column.get_type() == shcore::Value_type::Map) {
      const auto &column_map = column.as_map();
      if (column_map->has_key("name")) {
        try_parse(shcore::str_format("Error processing name for column #%zd",
                                     index + 1),
                  [&]() { name = column_map->at("name").as_string(); });
      } else {
        throw std::runtime_error(shcore::str_format(
            "Missing column name at column #%zd", index + 1));
      }

      if (column_map->has_key("type")) {
        try_parse(
            shcore::str_format("Error processing type for column #%zd",
                               index + 1),
            [&]() { type = db_type(column_map->at("type").as_string()); });
      }

      if (column_map->has_key("flags")) {
        try_parse(shcore::str_format("Error processing flags for column #%zd",
                                     index + 1),
                  [&]() {
                    auto str_flags = column_map->at("flags").as_string();
                    auto tmp_flags =
                        shcore::str_split(str_flags, ", \t", -1, true);
                    for (const auto &flag : tmp_flags) {
                      if (flag.empty()) continue;

                      // Indicates at least 1 flag was specified
                      flags_defined = true;

                      if (k_allowed_flags.find(flag) == k_allowed_flags.end()) {
                        throw std::runtime_error(shcore::str_format(
                            "unsupported flag: '%s'", flag.c_str()));
                      } else {
                        flags.insert(shcore::str_upper(flag));
                      }
                    }
                  });
      }

      if (column_map->has_key("length")) {
        try_parse(shcore::str_format("Error processing length for column #%zd",
                                     index + 1),
                  [&]() { length = column_map->at("length").as_uint(); });
      }

    } else {
      throw std::runtime_error(shcore::str_format(
          "Unsupported column definition format: %s", column.descr().c_str()));
    }

    // In case of missing metadata, it is deducted from a value in the result
    const shcore::Value *value = nullptr;
    if (a_result) {
      value = &a_result->at(index);
    } else if (d_result) {
      if (d_result->has_key(name)) {
        value = &d_result->at(name);
      }
    }

    deduce_metadata(value, type, length, flags_defined ? nullptr : &flags);

    m_columns.emplace_back("", "", "", "", "", name, *length, 0,
                           type.value_or(mysqlshdk::db::Type::String), 0,
                           has_flag("UNSIGNED"), has_flag("ZEROFILL"),
                           has_flag("BINARY"), shcore::str_join(flags, " "),
                           "");
  }
}

void Custom_result::deduce_column_metadata() {
  // Create the column metadata
  for (const auto &item : *m_result_data->at(0).as_map()) {
    // Metadata containers
    std::optional<mysqlshdk::db::Type> type;
    std::optional<uint32_t> length;
    std::set<std::string> flags;

    // Deduce the type
    deduce_metadata(&item.second, type, length, &flags);

    auto has_flag = [&](const std::string &flag) {
      return flags.find(flag) != flags.end();
    };

    m_columns.emplace_back("", "", "", "", "", item.first, *length, 0,
                           type.value_or(mysqlshdk::db::Type::String), 0,
                           has_flag("UNSIGNED"), has_flag("ZEROFILL"),
                           has_flag("BINARY"), shcore::str_join(flags, " "),
                           "");
  }
}

const mysqlshdk::db::IRow *Custom_result::fetch_one() {
  if (!m_result_data || m_result_data->empty()) return nullptr;

  if (m_row_index >= m_rows.size() && m_row_index < m_result_data->size()) {
    const auto record_type = m_result_data->at(m_row_index).get_type();
    if (record_type == shcore::Value_type::Map) {
      m_rows.push_back(std::make_unique<Custom_row>(
          shared_from_this(), m_result_data->at(m_row_index).as_map(),
          m_row_index));
    } else if (record_type == shcore::Value_type::Array) {
      m_rows.push_back(std::make_unique<Custom_row>(
          shared_from_this(), m_result_data->at(m_row_index).as_array(),
          m_row_index));
    } else {
      throw std::runtime_error(
          "Unsupported format for record in custom result.");
    }
  }

  if (m_row_index < m_rows.size()) {
    return m_rows[m_row_index++].get();
  }

  return nullptr;
}

bool Custom_result::next_resultset() { return false; }

std::unique_ptr<mysqlshdk::db::Warning> Custom_result::fetch_one_warning() {
  if (m_warnings.empty()) return nullptr;

  if (m_warning_index < m_warnings.size()) {
    return std::make_unique<mysqlshdk::db::Warning>(
        m_warnings.at(m_warning_index++));
  }

  return nullptr;
}

bool Custom_result::has_resultset() { return m_has_result; }

uint64_t Custom_result::get_affected_row_count() const {
  return m_affected_items_count;
}

uint64_t Custom_result::get_fetched_row_count() const {
  return m_result_data ? m_result_data->size() : 0;
}

uint64_t Custom_result::get_warning_count() const { return m_warnings.size(); }

std::string Custom_result::get_info() const { return m_info; }

const std::vector<std::string> &Custom_result::get_gtids() const {
  return m_gtids;
}

const std::vector<mysqlshdk::db::Column> &Custom_result::get_metadata() const {
  return m_columns;
}
std::shared_ptr<mysqlshdk::db::Field_names> Custom_result::field_names() const {
  return std::make_shared<mysqlshdk::db::Field_names>(m_columns);
}

void Custom_result::buffer() {
  // NOOP: Custom results are already buffered...
}

void Custom_result::rewind() {
  m_row_index = 0;
  m_warning_index = 0;
}

Custom_result_set::Custom_result_set(const shcore::Array_t &data)
    : m_raw_data(data) {
  m_next_result_index = 0;
  next_resultset();
}

const mysqlshdk::db::IRow *Custom_result_set::fetch_one() {
  return m_current_result ? m_current_result->fetch_one() : nullptr;
}

bool Custom_result_set::next_resultset() {
  if (!m_raw_data || m_raw_data->empty()) return false;

  if (m_next_result_index >= m_results.size() &&
      m_next_result_index < m_raw_data->size()) {
    const auto &raw_result = m_raw_data->at(m_next_result_index).as_map();
    if (raw_result->has_key("error")) {
      const auto sqlstate = raw_result->get_string("sqlstate");
      if (sqlstate.empty()) {
        throw shcore::Error(raw_result->get_string("error").c_str(),
                            raw_result->get_int("code"));
      } else {
        throw shcore::Error(
            shcore::str_format("%s (%s)",
                               raw_result->get_string("error").c_str(),
                               sqlstate.c_str()),
            raw_result->get_int("code"));
      }
    } else {
      m_results.push_back(std::make_shared<Custom_result>(raw_result));
    }
  }

  if (m_next_result_index < m_results.size()) {
    m_current_result = m_results.at(m_next_result_index);
    m_next_result_index++;
  } else {
    m_current_result = nullptr;
  }

  return m_current_result != nullptr;
}

std::unique_ptr<mysqlshdk::db::Warning> Custom_result_set::fetch_one_warning() {
  return m_current_result ? m_current_result->fetch_one_warning() : nullptr;
}

bool Custom_result_set::has_resultset() {
  return m_current_result ? m_current_result->has_resultset() : false;
}

uint64_t Custom_result_set::get_affected_row_count() const {
  return m_current_result ? m_current_result->get_affected_row_count() : 0;
}

uint64_t Custom_result_set::get_fetched_row_count() const {
  return m_current_result ? m_current_result->get_fetched_row_count() : 0;
}

double Custom_result_set::get_execution_time() const {
  return m_current_result ? m_current_result->get_execution_time()
                          : mysqlshdk::db::IResult::get_execution_time();
}

uint64_t Custom_result_set::get_warning_count() const {
  return m_current_result ? m_current_result->get_warning_count() : 0;
}

std::string Custom_result_set::get_info() const {
  return m_current_result ? m_current_result->get_info() : "";
}
const std::vector<std::string> &Custom_result_set::get_gtids() const {
  return m_gtids;
}

const std::vector<mysqlshdk::db::Column> &Custom_result_set::get_metadata()
    const {
  return m_current_result ? m_current_result->get_metadata() : m_metadata;
}
std::shared_ptr<mysqlshdk::db::Field_names> Custom_result_set::field_names()
    const {
  return m_current_result ? m_current_result->field_names() : nullptr;
}

void Custom_result_set::buffer() {
  // NOOP: Custom results are already buffered...
}

void Custom_result_set::rewind() {
  for (auto &result : m_results) {
    result->rewind();
  }

  m_next_result_index = 0;
  next_resultset();
}
}  // namespace mysqlshdk
