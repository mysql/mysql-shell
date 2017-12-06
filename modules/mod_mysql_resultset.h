/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_RESULT_H_
#define _MOD_RESULT_H_

#include <vector>
#include<string>
#include <list>
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "modules/devapi/base_resultset.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/db/mysql/row.h"

namespace mysqlsh {
namespace mysql {

/**
* \ingroup ShellAPI
* $(CLASSICRESULT_BRIEF)
*
* $(CLASSICRESULT_DETAIL)
*/
class SHCORE_PUBLIC ClassicResult : public ShellBaseResult {
public:
  explicit ClassicResult(std::shared_ptr<mysqlshdk::db::mysql::Result> result);

  // TODO(rennox): This function should be removed, the callers of this function
  // should either use the low level implementation (ISession) or the high level
  // implementation ClassicResult as exposed to the user API.
  const mysqlshdk::db::IRow* fetch_one() const;

  virtual std::string class_name() const { return "ClassicResult"; }
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  shcore::Value has_data(const shcore::Argument_list &args) const;
  virtual shcore::Value fetch_one(const shcore::Argument_list &args) const;
  virtual shcore::Value fetch_all(const shcore::Argument_list &args) const;
  virtual shcore::Value next_data_set(const shcore::Argument_list &args);

  shcore::Value::Array_type_ref get_columns() const;

  void set_execution_time(uint64_t execution_time) {
    _execution_time = execution_time;
  }

 private:
  std::shared_ptr<mysqlshdk::db::mysql::Result> _result;
  uint64_t _execution_time;
  std::shared_ptr<std::vector<std::string>> _column_names;
  mutable shcore::Value::Array_type_ref _columns;


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
