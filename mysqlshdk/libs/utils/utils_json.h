/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_JSON_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_JSON_H_

#include <rapidjson/document.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/include/scripting/types.h"

namespace shcore {

class SHCORE_PUBLIC JSON_dumper final {
 public:
  class Writer_base;

  explicit JSON_dumper(bool pprint = false, size_t binary_limit = 0);
  ~JSON_dumper();

  void start_array();
  void end_array();
  void start_object();
  void end_object();

  void append_value(const shcore::Value &value);
  void append_value(std::string_view key, const shcore::Value &value);
  void append(const shcore::Value &value);
  void append(std::string_view key, const shcore::Value &value);

  void append(const Dictionary_t &value);
  void append(std::string_view key, const Dictionary_t &value);

  void append(const Array_t &value);
  void append(std::string_view key, const Array_t &value);

  void append_null() const;
  void append_null(std::string_view key) const;

  void append_bool(bool data) const;
  void append_bool(std::string_view key, bool data) const;
  void append(bool data) const;
  void append(std::string_view key, bool data) const;

  void append_int(int data) const;
  void append_int(std::string_view key, int data) const;
  void append(int data) const;
  void append(std::string_view key, int data) const;

  void append_uint(unsigned int data) const;
  void append_uint(std::string_view key, unsigned int data) const;
  void append(unsigned int data) const;
  void append(std::string_view key, unsigned int data) const;

  void append_int64(int64_t data) const;
  void append_int64(std::string_view key, int64_t data) const;

  void append_uint64(uint64_t data) const;
  void append_uint64(std::string_view key, uint64_t data) const;

  void append(long int data) const;
  void append(std::string_view key, long int data) const;

  void append(unsigned long int data) const;
  void append(std::string_view key, unsigned long int data) const;

  void append(long long int data) const;
  void append(std::string_view key, long long int data) const;

  void append(unsigned long long int data) const;
  void append(std::string_view key, unsigned long long int data) const;

  void append_string(std::string_view data) const;
  void append_string(std::string_view key, std::string_view data) const;
  void append(std::string_view data) const;
  inline void append(const char *data) const { append_string(data); }
  void append(std::string_view key, std::string_view data) const;
  inline void append(std::string_view key, const char *data) const {
    append_string(key, data);
  }

  void append_float(double data) const;
  void append_float(std::string_view key, double data) const;
  void append(double data) const;
  void append(std::string_view key, double data) const;

  void append_json(const std::string &data) const;

  int deep_level() const { return _deep_level; }

  const std::string &str() const;

 private:
  int _deep_level{0};
  size_t _binary_limit{0};

  std::unique_ptr<Writer_base> _writer;
};

namespace json {

using JSON = rapidjson::Document;
using Value = rapidjson::Value;
using Object = rapidjson::Value::ConstObject;
using Array = rapidjson::Value::ConstArray;

/**
 * Parses a JSON string.
 *
 * NOTE: this does not throw exceptions, parsed document can be potentially
 * invalid.
 *
 * @param json JSON to be parsed.
 *
 * @returns parsed document
 */
JSON parse(std::string_view json);

/**
 * Parses a JSON string, input is expected to be a JSON object.
 *
 * @param json JSON to be parsed.
 *
 * @returns parsed document
 *
 * @throws std::runtime_error if parsing fails, or result is not a JSON object.
 */
JSON parse_object_or_throw(std::string_view json);

/**
 * Fetches a string value of a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 * @param allow_empty Don't throw if value is an empty string.
 *
 * @returns string value
 *
 * @throws std::runtime_error if field does not exist or its value is not a
 * string
 */
std::string required(const Value &object, const char *name, bool allow_empty);

/**
 * Fetches a Boolean value of a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns Boolean value
 *
 * @throws std::runtime_error if field does not exist or its value is not a
 * Boolean
 */
bool required_bool(const Value &object, const char *name);

/**
 * Fetches an unsigned integer value of a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns unsigned integer value
 *
 * @throws std::runtime_error if field does not exist or its value is not an
 * unsigned integer
 */
uint64_t required_uint(const Value &object, const char *name);

/**
 * Fetches an object associated with a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns object
 *
 * @throws std::runtime_error if field does not exist or its value is not an
 * object
 */
Object required_object(const Value &object, const char *name);

/**
 * Fetches a map of strings associated with a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns map of strings
 *
 * @throws std::runtime_error if field does not exist or its value is not a map
 * of strings
 */
std::map<std::string, std::string> required_map(const Value &object,
                                                const char *name);

/**
 * Fetches an unordered map of strings associated with a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns unordered map of strings
 *
 * @throws std::runtime_error if field does not exist or its value is not a map
 * of strings
 */
std::unordered_map<std::string, std::string> required_unordered_map(
    const Value &object, const char *name);

/**
 * Fetches an array associated with a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns array
 *
 * @throws std::runtime_error if field does not exist or its value is not an
 * array
 */
Array required_array(const Value &object, const char *name);

/**
 * Fetches an array of objects associated with a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns array of objects
 *
 * @throws std::runtime_error if field does not exist or its value is not an
 * array of objects
 */
std::vector<Object> required_object_array(const Value &object,
                                          const char *name);

/**
 * Fetches an array of strings associated with a required field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns array of strings
 *
 * @throws std::runtime_error if field does not exist or its value is not an
 * array of strings
 */
std::vector<std::string> required_string_array(const Value &object,
                                               const char *name);

/**
 * Fetches a string value of an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 * @param allow_empty Don't throw if value is an empty string.
 *
 * @returns string value if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not a string
 */
std::optional<std::string> optional(const Value &object, const char *name,
                                    bool allow_empty);

/**
 * Fetches a Boolean value of an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns Boolean value if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not a Boolean
 */
std::optional<bool> optional_bool(const Value &object, const char *name);

/**
 * Fetches an unsigned integer value of an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns unsigned integer value if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not an unsigned integer
 */
std::optional<uint64_t> optional_uint(const Value &object, const char *name);

/**
 * Fetches an object associated with an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns object if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not an object
 */
std::optional<Object> optional_object(const Value &object, const char *name);

/**
 * Fetches a map of strings associated with an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns map of strings if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not a map of strings
 */
std::optional<std::map<std::string, std::string>> optional_map(
    const Value &object, const char *name);

/**
 * Fetches an unordered map of strings associated with an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns unordered map of strings if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not a map of strings
 */
std::optional<std::unordered_map<std::string, std::string>>
optional_unordered_map(const Value &object, const char *name);

/**
 * Fetches an array associated with an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns array if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not an array
 */
std::optional<Array> optional_array(const Value &object, const char *name);

/**
 * Fetches an array of objects associated with an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns array of objects if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not an array of objects
 */
std::optional<std::vector<Object>> optional_object_array(const Value &object,
                                                         const char *name);

/**
 * Fetches an array of strings associated with an optional field.
 *
 * @param object JSON object.
 * @param name Name of the field.
 *
 * @returns array of strings if JSON contains the specified field
 *
 * @throws std::runtime_error if value is not an array of strings
 */
std::optional<std::vector<std::string>> optional_string_array(
    const Value &object, const char *name);

/**
 * Converts a JSON object to a string.
 *
 * @param json A JSON object
 *
 * @returns String representation of a JSON object.
 */
std::string to_string(const Value &json);

/**
 * Converts a JSON object to a nicely formatted string.
 *
 * @param json A JSON object
 *
 * @returns String representation of a JSON object.
 */
std::string to_pretty_string(const Value &json);

}  // namespace json

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_JSON_H_
