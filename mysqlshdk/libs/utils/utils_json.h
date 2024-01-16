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
#include <memory>
#include <string>
#include <string_view>

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

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_JSON_H_
