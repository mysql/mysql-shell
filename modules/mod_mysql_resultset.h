/*
 * Copyright (c) 2014, 2020, Oracle and/or its affiliates.
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

#include <list>
#include <string>
#include <vector>
#include "modules/devapi/base_resultset.h"
#include "mysqlshdk/libs/db/mysql/result.h"
#include "mysqlshdk/libs/db/mysql/row.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
namespace mysql {

/**
 * \ingroup ShellAPI
 * $(CLASSICRESULT_BRIEF)
 *
 * $(CLASSICRESULT)
 */
class SHCORE_PUBLIC ClassicResult : public ShellBaseResult {
 public:
#if DOXYGEN_JS
  Integer affectedItemsCount;  //!< Same as getAffectedItemsCount()
  /**
   * Same as getAffectedRowCount()
   *
   * @attention This property will be removed in a future release, use the
   * <b>affectedItemsCount</b> property instead.
   */
  Integer affectedRowCount;
  Integer columnCount;         //!< Same as getColumnCount()
  List columnNames;            //!< Same as getColumnNames()
  List columns;                //!< Same as getColumns()
  String executionTime;        //!< Same as getExecutionTime()
  String info;                 //!< Same as getInfo()
  Integer autoIncrementValue;  //!< Same as getAutoIncrementValue()
  List warnings;               //!< Same as getWarnings()
  /**
   * Same as getWarningCount()
   *
   * @attention This property will be removed in a future release, use the
   * <b>warningsCount</b> property instead.
   */
  Integer warningCount;
  Integer warningsCount;  //!< Same as getWarningsCount()

  Row fetchOne();
  Dictionary fetchOneObject();
  List fetchAll();
  Integer getAffectedItemsCount();
  Integer getAffectedRowCount();
  Integer getColumnCount();
  List getColumnNames();
  List getColumns();
  String getExecutionTime();
  Bool hasData();
  String getInfo();
  Integer getAutoIncrementValue();
  Integer getWarningCount();
  Integer getWarningsCount();
  List getWarnings();
  Bool nextDataSet();
  Bool nextResult();
#elif DOXYGEN_PY
  int affected_items_count;  //!< Same as get_affected_items_count()
  /**
   * Same as get_affected_row_count()
   *
   * @attention This property will be removed in a future release, use the
   * <b>affected_items_count</b> property instead.
   */
  int affected_row_count;
  int column_count;          //!< Same as get_column_count()
  list column_names;         //!< Same as get_column_names()
  list columns;              //!< Same as get_columns()
  str execution_time;        //!< Same as get_execution_time()
  str info;                  //!< Same as get_info()
  int auto_increment_value;  //!< Same as get_auto_increment_value()
  list warnings;             //!< Same as get_warnings()
  /**
   * Same as get_warning_count()
   *
   * @attention This property will be removed in a future release, use the
   * <b>warnings_count</b> property instead.
   */
  int warning_count;
  int warnings_count;  //!< Same as get_warnings_count()

  Row fetch_one();
  dict fetch_one_object();
  list fetch_all();
  int get_affected_items_count();
  int get_affected_row_count();
  int get_column_count();
  list get_column_names();
  list get_columns();
  str get_execution_time();
  bool has_data();
  str get_info();
  int get_auto_increment_value();
  int get_warning_count();
  int get_warnings_count();
  list get_warnings();
  bool next_data_set();
  bool next_result();
#endif

  explicit ClassicResult(std::shared_ptr<mysqlshdk::db::mysql::Result> result);

  virtual std::string class_name() const { return "ClassicResult"; }
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  bool has_data() const;
  std::shared_ptr<Row> fetch_one() const;
  shcore::Dictionary_t _fetch_one_object();
  shcore::Array_t fetch_all() const;
  bool next_data_set();
  bool next_result();

  shcore::Value::Array_type_ref get_columns() const;

  virtual mysqlshdk::db::IResult *get_result() const { return _result.get(); };
  virtual std::shared_ptr<std::vector<std::string>> get_column_names() const {
    return _column_names;
  }

 private:
  std::shared_ptr<mysqlshdk::db::mysql::Result> _result;
  std::shared_ptr<std::vector<std::string>> _column_names;
  mutable shcore::Value::Array_type_ref _columns;
};
}  // namespace mysql
}  // namespace mysqlsh

#endif
