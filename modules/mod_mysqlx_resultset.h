/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_XRESULT_H_
#define _MOD_XRESULT_H_

#include "base_resultset.h"

namespace mysqlx {
class Result;
}

namespace mysqlsh {
namespace mysqlx {
/**
* \ingroup XDevAPI
* $(BASERESULT_BRIEF)
*/
class SHCORE_PUBLIC BaseResult : public mysqlsh::ShellBaseResult {
public:
  BaseResult(std::shared_ptr< ::mysqlx::Result> result);
  virtual ~BaseResult() {}

  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  // The execution time is not available at the moment of creating the resultset
  void set_execution_time(unsigned long execution_time) { _execution_time = execution_time; }

  // C++ Interface
  std::string get_execution_time() const;
  uint64_t get_warning_count() const;
  virtual void buffer();
  virtual bool rewind();
  virtual bool tell(size_t &dataset, size_t &record);
  virtual bool seek(size_t dataset, size_t record);

#if DOXYGEN_JS
  Integer warningCount; //!< Same as getwarningCount()
  List warnings; //!< Same as getWarnings()
  String executionTime; //!< Same as getExecutionTime()

  Integer getWarningCount();
  List getWarnings();
  String getExecutionTime();
#elif DOXYGEN_PY
  int warning_count; //!< Same as get_warning_count()
  list warnings; //!< Same as get_warnings()
  str execution_time; //!< Same as get_execution_time()

  int get_warning_count();
  list get_warnings();
  str get_execution_time();
#endif

protected:
  std::shared_ptr< ::mysqlx::Result> _result;
  unsigned long _execution_time;
};

/**
* \ingroup XDevAPI
* $(RESULT_BRIEF)
*
* $(RESULT_DETAIL)
*
* $(RESULT_DETAIL1)
* $(RESULT_DETAIL2)
*
* $(RESULT_DETAIL3)
*
* $(RESULT_DETAIL4)
* $(RESULT_DETAIL5)
*/
class SHCORE_PUBLIC Result : public BaseResult {
public:
  Result(std::shared_ptr< ::mysqlx::Result> result);

  virtual ~Result() {};

  virtual std::string class_name() const { return "Result"; }
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  // C++ Interface
  int64_t get_affected_item_count() const;
  int64_t get_auto_increment_value() const;
  std::string get_last_document_id() const;
  const std::vector<std::string> get_last_document_ids() const;

#if DOXYGEN_JS
  Integer affectedItemCount; //!< Same as getAffectedItemCount()
  Integer autoIncrementValue; //!< Same as getAutoIncrementValue()
  Integer lastDocumentId; //!< Same as getLastDocumentId()

  Integer getAffectedItemCount();
  Integer getAutoIncrementValue();
  String getLastDocumentId();
#elif DOXYGEN_PY
  int affected_item_count; //!< Same as get_affected_item_count()
  int auto_increment_value; //!< Same as get_auto_increment_value()
  int last_document_id; //!< Same as get_last_document_id()

  int get_affected_item_count();
  int get_auto_increment_value();
  str get_last_document_id();
#endif
};

/**
* \ingroup XDevAPI
* $(DOCRESULT_BRIEF)
*/
class SHCORE_PUBLIC DocResult : public BaseResult {
public:
  DocResult(std::shared_ptr< ::mysqlx::Result> result);

  virtual ~DocResult() {};

  shcore::Value fetch_one(const shcore::Argument_list &args) const;
  shcore::Value fetch_all(const shcore::Argument_list &args) const;

  virtual std::string class_name() const { return "DocResult"; }
  virtual void append_json(shcore::JSON_dumper& dumper) const;

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
  RowResult(std::shared_ptr< ::mysqlx::Result> result);

  virtual ~RowResult() {};

  shcore::Value fetch_one(const shcore::Argument_list &args) const;
  shcore::Value fetch_all(const shcore::Argument_list &args) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  virtual std::string class_name() const { return "RowResult"; }
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  // C++ Interface
  int64_t get_column_count() const;
  std::vector<std::string> get_column_names() const;
  shcore::Value::Array_type_ref get_columns() const;

#if DOXYGEN_JS
  Row fetchOne();
  List fetchAll();

  Integer columnCount; //!< Same as getColumnCount()
  List columnNames; //!< Same as getColumnNames()
  List columns; //!< Same as getColumns()

  Integer getColumnCount();
  List getColumnNames();
  List getColumns();
#elif DOXYGEN_PY
  Row fetch_one();
  list fetch_all();

  int column_count; //!< Same as get_column_count()
  list column_names; //!< Same as get_column_names()
  list columns; //!< Same as get_columns()

  int get_column_count();
  list get_column_names();
  list get_columns();
#endif

private:
  mutable shcore::Value::Array_type_ref _columns;
};

/**
* \ingroup XDevAPI
* $(SQLRESULT_BRIEF)
*/
class SHCORE_PUBLIC SqlResult : public RowResult {
public:
  SqlResult(std::shared_ptr< ::mysqlx::Result> result);

  virtual ~SqlResult() {};

  virtual std::string class_name() const { return "SqlResult"; }
  virtual shcore::Value get_member(const std::string &prop) const;

  shcore::Value has_data(const shcore::Argument_list &args) const;
  virtual shcore::Value next_data_set(const shcore::Argument_list &args);
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  // C++ Interface
  int64_t get_affected_row_count() const;
  int64_t get_auto_increment_value() const;
  bool hasData();

  // TODO: Enable it once the way to have a reference to the unmanaged object is found
  //bool nextDataSet() const;

#if DOXYGEN_JS
  Integer autoIncrementValue; //!< Same as getAutoIncrementValue()
  Integer affectedRowCount; //!< Same as getAffectedRowCount()

  Integer getAutoIncrementValue();
  Integer getAffectedRowCount();
  Bool hasData();
  Bool nextDataSet();
#elif DOXYGEN_PY
  int auto_increment_value; //!< Same as get_auto_increment_value()
  int affected_row_count; //!< Same as get_affected_row_count()

  int get_auto_increment_value();
  int get_affected_row_count();
  bool has_data();
  bool next_data_set();
#endif
};
}
};

#endif
