/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_RESULTSET_H_
#define MODULES_DEVAPI_MOD_MYSQLX_RESULTSET_H_

#include <memory>
#include <string>
#include <vector>
#include "db/mysqlx/result.h"
#include "modules/devapi/base_resultset.h"

namespace mysqlsh {
namespace mysqlx {

class Bufferable_result;

/**
 * \ingroup XDevAPI
 * $(BASERESULT_BRIEF)
 */
class SHCORE_PUBLIC BaseResult : public mysqlsh::ShellBaseResult {
 public:
  explicit BaseResult(std::shared_ptr<mysqlshdk::db::mysqlx::Result> result);
  virtual ~BaseResult();

  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  // C++ Interface
  int64_t get_affected_items_count() const;
  std::string get_execution_time() const;
  uint64_t get_warnings_count() const;

#if DOXYGEN_JS
  Integer affectedItemsCount;  //!< Same as getAffectedItemsCount()
  /**
   * Same as getWarningCount()
   *
   * @attention This property will be removed in a future release, use the
   * <b>warningsCount</b> property instead.
   */
  Integer warningCount;
  Integer warningsCount;  //!< Same as getWarningsCount()
  List warnings;          //!< Same as getWarnings()
  String executionTime;   //!< Same as getExecutionTime()

  Integer getAffectedItemsCount();
  Integer getWarningsCount();
  Integer getWarningCount();
  List getWarnings();
  String getExecutionTime();
#elif DOXYGEN_PY
  int affected_items_count;  //!< Same as get_affected_items_count()
  /**
   * Same as get_warning_count()
   *
   * @attention This property will be removed in a future release, use the
   * <b>warnings_count</b> property instead.
   */
  int warning_count;
  int warnings_count;  //!< Same as get_warnings_count()
  list warnings;       //!< Same as get_warnings()
  str execution_time;  //!< Same as get_execution_time()

  int get_affected_items_count();
  int get_warnings_count();
  int get_warning_count();
  list get_warnings();
  str get_execution_time();
#endif

  virtual mysqlshdk::db::IResult *get_result() const { return _result.get(); };

 protected:
  std::shared_ptr<mysqlshdk::db::mysqlx::Result> _result;
};

/**
 * \ingroup XDevAPI
 * $(RESULT_BRIEF)
 *
 * $(RESULT)
 */
class SHCORE_PUBLIC Result : public BaseResult {
 public:
  explicit Result(std::shared_ptr<mysqlshdk::db::mysqlx::Result> result);

  virtual std::string class_name() const { return "Result"; }
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  // C++ Interface
  int64_t get_auto_increment_value() const;
  const std::vector<std::string> get_generated_ids() const;

#if DOXYGEN_JS
  /**
   * Same as getAffectedItemCount()
   *
   * @attention This property will be removed in a future release, use the
   * <b>affectedItemsCount</b> property instead.
   */
  Integer affectedItemCount;
  Integer autoIncrementValue;  //!< Same as getAutoIncrementValue()
  List generatedIds;           //!< Same as getGeneratedIds()

  Integer getAffectedItemCount();
  Integer getAutoIncrementValue();
  List getGeneratedIds();
#elif DOXYGEN_PY
  /**
   * Same as get_affected_itemCount()
   *
   * @attention This property will be removed in a future release, use the
   * <b>affected_items_count</b> property instead.
   */
  int affected_item_count;
  int auto_increment_value;  //!< Same as get_auto_increment_value()
  list generated_ids;        //!< Same as get_generated_ids()

  int get_affected_item_count();
  int get_auto_increment_value();
  list get_generated_ids();
#endif
};

/**
 * \ingroup XDevAPI
 * $(DOCRESULT_BRIEF)
 */
class SHCORE_PUBLIC DocResult : public BaseResult {
 public:
  explicit DocResult(std::shared_ptr<mysqlshdk::db::mysqlx::Result> result);

  shcore::Dictionary_t fetch_one() const;
  shcore::Array_t fetch_all() const;

  virtual std::string class_name() const { return "DocResult"; }
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  shcore::Value get_metadata() const;

#if DOXYGEN_JS
  Document fetchOne();
  List fetchAll();
#elif DOXYGEN_PY
  Document fetch_one();
  list fetch_all();
#endif

 private:
  mutable shcore::Value _metadata;
};

/**
 * \ingroup XDevAPI
 * $(ROWRESULT_BRIEF)
 */
class SHCORE_PUBLIC RowResult : public BaseResult {
 public:
  explicit RowResult(std::shared_ptr<mysqlshdk::db::mysqlx::Result> result);

  std::shared_ptr<mysqlsh::Row> fetch_one() const;
  shcore::Array_t fetch_all() const;
  shcore::Dictionary_t _fetch_one_object();
  virtual shcore::Value get_member(const std::string &prop) const;

  virtual std::string class_name() const { return "RowResult"; }
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  virtual std::shared_ptr<std::vector<std::string>> get_column_names() const {
    return _column_names;
  }

  // C++ Interface
  int64_t get_column_count() const;
  shcore::Value::Array_type_ref get_columns() const;

#if DOXYGEN_JS
  Row fetchOne();
  Dictionary fetchOneObject();
  List fetchAll();

  Integer columnCount;  //!< Same as getColumnCount()
  List columnNames;     //!< Same as getColumnNames()
  List columns;         //!< Same as getColumns()

  Integer getColumnCount();
  List getColumnNames();
  List getColumns();
#elif DOXYGEN_PY
  Row fetch_one();
  dict fetch_one_object();
  list fetch_all();

  int column_count;   //!< Same as get_column_count()
  list column_names;  //!< Same as get_column_names()
  list columns;       //!< Same as get_columns()

  int get_column_count();
  list get_column_names();
  list get_columns();
#endif

 private:
  std::shared_ptr<std::vector<std::string>> _column_names;
  mutable shcore::Value::Array_type_ref _columns;
};

/**
 * \ingroup XDevAPI
 * $(SQLRESULT_BRIEF)
 */
class SHCORE_PUBLIC SqlResult : public RowResult {
 public:
  explicit SqlResult(std::shared_ptr<mysqlshdk::db::mysqlx::Result> result);

  virtual std::string class_name() const { return "SqlResult"; }
  virtual shcore::Value get_member(const std::string &prop) const;

  bool has_data() const;
  bool next_data_set();
  bool next_result();
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  // C++ Interface
  int64_t get_affected_row_count() const;
  int64_t get_auto_increment_value() const;
  bool hasData();

#if DOXYGEN_JS
  Integer autoIncrementValue;  //!< Same as getAutoIncrementValue()
  /**
   * Same as getAffectedRowCount()
   *
   * @attention This property will be removed in a future release, use the
   * <b>affectedItemsCount</b> property instead.
   */
  Integer affectedRowCount;

  Integer getAutoIncrementValue();
  Integer getAffectedRowCount();
  Bool hasData();
  Bool nextDataSet();
  Bool nextResult();
#elif DOXYGEN_PY
  int auto_increment_value;  //!< Same as get_auto_increment_value()
  /**
   * Same as get_affected_row_count()
   *
   * @attention This property will be removed in a future release, use the
   * <b>affected_items_count</b> property instead.
   */
  int affected_row_count;

  int get_auto_increment_value();
  int get_affected_row_count();
  bool has_data();
  bool next_data_set();
  bool next_result();
#endif
};
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_RESULTSET_H_
