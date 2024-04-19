/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_MOD_SHELL_RESULT_H_
#define MODULES_MOD_SHELL_RESULT_H_

#include <memory>
#include "modules/devapi/base_resultset.h"

namespace mysqlsh {

/**
 * \ingroup ShellAPI
 * $(SHELLRESULT_BRIEF)
 *
 * $(SHELLRESULT)
 */
class ShellResult : public ShellBaseResult {
 public:
#if DOXYGEN_JS
  Integer affectedItemsCount;  //!< Same as getAffectedItemsCount()
  Integer autoIncrementValue;  //!< Same as getAutoIncrementValue()
  Integer columnCount;         //!< Same as getColumnCount()
  List columnNames;            //!< Same as getColumnNames()
  List columns;                //!< Same as getColumns()
  String executionTime;        //!< Same as getExecutionTime()
  String info;                 //!< Same as getInfo()
  List warnings;               //!< Same as getWarnings()
  Integer warningsCount;       //!< Same as getWarningsCount()

  Row fetchOne();
  // Dictionary fetchOneObject();
  List fetchAll();
  Integer getAffectedItemsCount();
  Integer getAutoIncrementValue();
  Integer getColumnCount();
  List getColumnNames();
  List getColumns();
  String getExecutionTime();
  Bool hasData();
  String getInfo();
  Integer getWarningsCount();
  List getWarnings();
  Bool nextResult();
#elif DOXYGEN_PY
  int affected_items_count;  //!< Same as get_affected_items_count()
  int auto_increment_value;  //!< Same as get_auto_increment_value()
  int column_count;          //!< Same as get_column_count()
  list column_names;         //!< Same as get_column_names()
  list columns;              //!< Same as get_columns()
  str execution_time;        //!< Same as get_execution_time()
  str info;                  //!< Same as get_info()
  list warnings;             //!< Same as get_warnings()
  int warnings_count;        //!< Same as get_warnings_count()

  Row fetch_one();
  // dict fetch_one_object();
  list fetch_all();
  int get_affected_items_count();
  int get_auto_increment_value();
  int get_column_count();
  list get_column_names();
  list get_columns();
  str get_execution_time();
  bool has_data();
  str get_info();
  int get_warnings_count();
  list get_warnings();
  bool next_result();
#endif
  ShellResult(std::shared_ptr<mysqlshdk::db::IResult> result);

  bool has_data() const override;
  bool next_result();

  std::string class_name() const override { return "ShellResult"; }
  shcore::Value get_member(const std::string &prop) const override;

  std::shared_ptr<mysqlsh::Row> fetch_one() const;
  shcore::Array_t fetch_all() const;

  std::shared_ptr<mysqlshdk::db::IResult> get_result() const override {
    return m_result;
  }

  const std::vector<mysqlshdk::db::Column> &get_metadata() const override {
    return m_result->get_metadata();
  }

  std::string get_protocol() const override { return "mysql"; }

  void rewind();

 private:
  std::shared_ptr<mysqlshdk::db::IResult> m_result;
};

}  // namespace mysqlsh

#endif  // MODULES_MOD_SHELL_RESULT_H_
