/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_RESULT_H_
#define MYSQLSHDK_LIBS_DB_RESULT_H_

#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/column.h"
#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/db/row_map.h"
#include "mysqlshdk_export.h"

namespace mysqlshdk {
namespace db {

struct Warning {
  enum class Level { Note, Warn, Error };
  Level level;
  std::string msg;
  uint32_t code;
};

class SHCORE_PUBLIC IResult {
 public:
  // Data fetching

  /**
   * Fetches one row from the resultset.
   * @return Reference to the row object
   *
   * The returned row object is only valid for as long as its result object
   * is valid and up until the next call to fetch_one().
   *
   * To keep a long-living reference to a Row object, copy it into a
   * Row_copy object.
   */
  virtual const IRow *fetch_one() = 0;

  Row_ref_map fetch_one_map() {
    return Row_ref_map(field_names(), fetch_one());
  }

  double get_execution_time() const { return m_execution_time; }
  void set_execution_time(double time) { m_execution_time = time; }

  virtual bool next_resultset() = 0;

  /**
   * Fetches warnings generated while executing query.
   * @return Warning or null if no more warnings
   *
   * Note: in classic protocol, warnings must be fetched after the complete
   * results are fetched.
   */
  virtual std::unique_ptr<Warning> fetch_one_warning() = 0;

  // Metadata retrieval
  virtual int64_t get_auto_increment_value() const = 0;
  virtual bool has_resultset() = 0;

  virtual uint64_t get_affected_row_count() const = 0;
  virtual uint64_t get_fetched_row_count() const = 0;
  virtual uint64_t get_warning_count() const = 0;
  virtual std::string get_info() const = 0;
  virtual const std::vector<std::string> &get_gtids() const = 0;

  virtual const std::vector<Column> &get_metadata() const = 0;
  virtual std::shared_ptr<Field_names> field_names() const = 0;

  virtual void buffer() = 0;
  virtual void rewind() = 0;

  virtual ~IResult() {}

 protected:
  double m_execution_time;
};

}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_RESULT_H_
