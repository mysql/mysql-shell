/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_RESULT_H_
#define _MOD_RESULT_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "base_resultset.h"
#include <list>

namespace mysqlsh {
namespace mysql {
class Result;
class Row;

/**
* \ingroup ShellAPI
* $(CLASSICRESULT_BRIEF)
*
* $(CLASSICRESULT_DETAIL)
*/
class SHCORE_PUBLIC ClassicResult : public ShellBaseResult {
public:
  ClassicResult(std::shared_ptr<Result> result);

  std::shared_ptr<mysql::Row> fetch_one() const;
  std::vector<std::shared_ptr<mysql::Row>> fetch_all() const;

  virtual std::string class_name() const { return "ClassicResult"; }
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  shcore::Value has_data(const shcore::Argument_list &args) const;
  virtual shcore::Value fetch_one(const shcore::Argument_list &args) const;
  virtual shcore::Value fetch_all(const shcore::Argument_list &args) const;
  virtual shcore::Value next_data_set(const shcore::Argument_list &args);

protected:
  std::shared_ptr<Result> _result;

#if DOXYGEN_JS
  Integer affectedRowCount; //!< Same as getAffectedItemCount()
  Integer columnCount; //!< Same as getColumnCount()
  List columnNames; //!< Same as getColumnNames()
  List columns; //!< Same as getColumns()
  String executionTime; //!< Same as getExecutionTime()
  String info; //!< Same as getInfo()
  Integer autoIncrementValue; //!< Same as getAutoIncrementValue()
  List warnings; //!< Same as getWarnings()
  Integer warningCount; //!< Same as getWarningCount()

  Row fetchOne();
  List fetchAll();
  Integer getAffectedRowCount();
  Integer getColumnCount();
  List getColumnNames();
  List getColumns();
  String getExecutionTime();
  Bool hasData();
  String getInfo();
  Integer getAutoIncrementValue();
  Integer getWarningCount();
  List getWarnings();
  Bool nextDataSet();
#elif DOXYGEN_PY
  int affected_row_count; //!< Same as get_affected_item_count()
  int column_count; //!< Same as get_column_count()
  list column_names; //!< Same as get_column_names()
  list columns; //!< Same as get_columns()
  str execution_time; //!< Same as get_execution_time()
  str info; //!< Same as get_info()
  int auto_increment_value; //!< Same as get_auto_increment_value()
  list warnings; //!< Same as get_warnings()
  int warning_count; //!< Same as get_warning_count()

  Row fetch_one();
  list fetch_all();
  int get_affected_row_count();
  int get_column_count();
  list get_column_names();
  list get_columns();
  str get_execution_time();
  bool has_data();
  str get_info();
  int get_auto_increment_value();
  int get_warning_count();
  list get_warnings();
  bool next_data_set();
#endif
};
}
};

#endif
