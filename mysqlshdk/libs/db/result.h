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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _CORELIBS_DB_IRESULT_H_
#define _CORELIBS_DB_IRESULT_H_

#include "mysqlshdk_export.h"
#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/db/column.h"
#include <memory>
#include <vector>

namespace mysqlshdk {
namespace db {
class SHCORE_PUBLIC IResult {
public:
  // Data fetching
  virtual std::unique_ptr<IRow> fetch_one() = 0;
  virtual bool next_data_set() = 0;
  virtual std::unique_ptr<IRow> fetch_one_warning() = 0;

  // Metadata retrieval
  virtual int64_t get_auto_increment_value() const = 0;
  virtual bool has_resultset() = 0;

  virtual uint64_t get_affected_row_count() const = 0;
  virtual uint64_t get_fetched_row_count() const = 0;
  virtual uint64_t get_warning_count() const = 0;
  virtual unsigned long get_execution_time() const = 0;
  virtual std::string get_info() const = 0;

  virtual const std::vector<Column>& get_metadata() const = 0;

  virtual ~IResult() {}
};
}
}
#endif
