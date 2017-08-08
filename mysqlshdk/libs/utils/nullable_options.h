/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_NULLABLE_OPTIONS_H_
#define MYSQLSHDK_LIBS_UTILS_NULLABLE_OPTIONS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/include/mysqlshdk_export.h"

namespace mysqlshdk {
namespace utils {
namespace nullable_options {
enum class Set_mode {
  CREATE,             // Succeeds if the option does not exist
  CREATE_AND_UPDATE,  // Succeeds all the time
  UPDATE_ONLY,        // Suceeds only of the option already exist
  UPDATE_NULL         // Suceeds only of the option already exist and is NULL
};

enum class Comparison_mode { CASE_SENSITIVE, CASE_INSENSITIVE };

}  // namespace nullable_options

using nullable_options::Set_mode;
using nullable_options::Comparison_mode;

typedef std::map<std::string, mysqlshdk::utils::nullable<std::string>,
                 bool (*)(const std::string&, const std::string&)>
    container;
typedef container::const_iterator const_iterator;
typedef container::iterator iterator;

class SHCORE_PUBLIC Nullable_options {
 public:
  Nullable_options(Comparison_mode mode = Comparison_mode::CASE_INSENSITIVE,
                   const std::string& context = "");

  bool has(const std::string& name) const;
  bool has_value(const std::string& name) const;
  void set(const std::string& name, const std::string& value,
           Set_mode mode = Set_mode::CREATE_AND_UPDATE);
  void set(const std::string& name, const char* value = nullptr,
           Set_mode mode = Set_mode::CREATE_AND_UPDATE);
  void remove(const std::string& name);
  void clear_value(const std::string& name);
  const std::string& get_value(const std::string& name) const;

  size_t size() const { return _options.size(); }

  virtual bool operator==(const Nullable_options& other) const;
  virtual bool operator!=(const Nullable_options& other) const;
  int compare(const std::string& lhs, const std::string& rhs) const;

  const_iterator begin() const { return _options.begin(); }
  iterator begin() { return _options.begin(); }
  const_iterator end() const { return _options.end(); }
  iterator end() { return _options.end(); }
  Comparison_mode get_mode() { return _mode; }

 protected:
  void throw_invalid_option(const std::string& name) const;
  void throw_no_value(const std::string& name) const;
  void throw_already_defined_option(const std::string& name) const;
  std::string _ctx;
  Comparison_mode _mode;

 private:
  static bool comp(const std::string& lhs, const std::string& rhs);
  static bool icomp(const std::string& lhs, const std::string& rhs);

  container _options;
};

}  // namespace utils
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_UTILS_NULLABLE_OPTIONS_H_
