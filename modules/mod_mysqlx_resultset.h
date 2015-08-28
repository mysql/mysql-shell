/*
 * Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
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

namespace mysqlx
{
  class Result;
}

namespace mysh
{
  namespace mysqlx
  {
    /**
    * Allows browsing through the result information after performing an operation on the database.
    * This class allows access to the result set from the classic MySQL data model to be retrieved from Dev API queries.
    * \todo delete warningCount and getWarningCount()
    * \todo delete fetchedRowCount and getfetchedRowCount()
    * \todo Implement and Document buffer()
    * \todo Implement and Document flush()
    * \todo Implement and Document rewind()
    */
    class Resultset : public BaseResultset
    {
    public:
      Resultset(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~Resultset();

      virtual std::string class_name() const { return "Resultset"; }
      virtual shcore::Value get_member(const std::string &prop) const;

      virtual shcore::Value next(const shcore::Argument_list &args);
      virtual shcore::Value all(const shcore::Argument_list &args);
      virtual shcore::Value next_result(const shcore::Argument_list &args);

      int get_cursor_id() { return _cursor_id; }
#ifdef DOXYGEN

      /**
      * Returns the metadata for the result set. See getColumnMetadata for more details.
      * \sa getColumnMetadata()
      * \return a map array with metadata information.
      */
      Map[] columnMetadata;

      /**
      * Returns the metadata for the result set.
      * \return the metadata Map[]
      * The metadata returned its an array of map each one per field the map accepts the following data:
      * 
      * Map key     | Meaning                        |
      * ----------: | :----------------------------: |
      * catalog     | the catalog name               |
      * table       | the table name                 |
      * org_table   | original table name            |
      * name        | the column name                |
      * org_name    | the original column name       |
      * collation   | collation (from charset)       |
      * length      | the column length              |
      * type        | the column type                |
      * flags       | flags (to be documented)       |
      * decimal     | decimal precision              |
      * max_length  | max length allowed             |
      * name_length | length of column name          |
      * is_numeric  | bool, true if type is numeric  |
      */
      Map[] getColumnMetadata()
      {}

      /**
      * The last insert id auto generated (from an insert operation)
      * \sa getLastInsertId(), getAffectedRows(), getWarnings(), getExcutionTime(), getInfo()
      */
      Integer lastInsertId;

      /**
      * The last insert id auto generated (from an insert operation)
      * For more details, see https://dev.mysql.com/doc/refman/5.7/en/information-functions.html#function_last-insert-id
      * \sa getAffectedRows(), getWarnings(), getExcutionTime(), getInfo()
      * \return the integer representing the last insert id
      */
      Integer getLastInsertId() 
      {}

      /**
      * The affected rows, for more details see getAffectedRows.
      * \sa getAffectedRows()
      */
      Integer affectedRows;

      /**
      * The the number of affected rows for the last operation.
      * This is the value of the C API mysql_affected_rows(), see https://dev.mysql.com/doc/refman/5.7/en/mysql-affected-rows.html
      * \sa getLastInsertId(), getWarnings(), getExcutionTime(), getInfo()
      * \return the number of affected rows.
      */
      Integer getAffectedRows() 
      {}

      /**
      * The number of warnings produced by the last statement execution. See getWarnings() for more details.
      * \sa getWarnings()
      */
      Integer warnings;

      /**
      * The number of warnings produced by the last statement execution. See getWarnings() for more details.
      * This is the same value than C API mysql_warning_count, see https://dev.mysql.com/doc/refman/5.7/en/mysql-warning-count.html
      * \sa warnings
      * \return the number of warnings.
      */
      Integer getWarnings() 
      {}

      /**
      * Gets a string representation of the time spent time by the last statement executed.
      * \sa getExecutionTime()
      */
      String executionTime;

      /**
      * Gets a string representation of the time spent time by the last statement executed.
      * \sa executionTime()
      * \return the execution time as an String.
      */
      String getExecutionTime() 
      {}

      /**
      * Gets an string with information on the last statement executed. See for details getInfo().
      * \sa getInfo().
      */
      String info;

      /**
      * Gets an string with information on the last statement executed. See for details getInfo().
      * This is the same value then C API mysql_info, see https://dev.mysql.com/doc/refman/5.7/en/mysql-info.html
      * \sa getInfo().
      */
      String getInfo() 
      {}

      /**
      * Returns true if the last statement execution has a result set. See for details getHasData().
      * \sa getHasData()
      */
      Bool hasData;

      /**
      * Returns true if the last statement execution has a result set. As opposite as having only the basic info of affected rows, info, exectution time, warnings, last insert id.
      * \sa getLastInsertId(), getInfo(), getExecutionTime(), getWarnings(), getAffectedRows().
      * \sa getHasData()
      */
      Bool getHasData() 
      {}

      /**
      * Returns the next result set.
      * Before returning the next result set, this operation takes care of flushing an ongoing result set stream reading.
      * After reading (and discarding) all the packages of the current result set stream, it will attempt to read the metadata of the next result set or the OK msg.
      * \sa next(), all()
      * \return true if more data available, otherwise false.
      */
      Bool nextDataSet() 
      {}

      Row next();

      Row[] all();

#endif
    protected:
      boost::shared_ptr< ::mysqlx::Result> _result;

    private:
      int _cursor_id;
    };

    /**
    * Allows browsing through the result information after performing an operation on a Collection.
    * Works similar to the Resultset class except that next() and all() return Documents.
    */
    class Collection_resultset : public Resultset
    {
#ifdef DOXYGEN

      /**
      * Calls successively next() until the whole result set is read and returns all the rows read.
      * \sa next()
      * \return an array of Row objects.
      */
      Row[] all()
      {}

      Row next();

#endif
    public:
      Collection_resultset(boost::shared_ptr< ::mysqlx::Result> result);

      virtual std::string class_name() const  { return "CollectionResultset"; }
      virtual shcore::Value next(const shcore::Argument_list &args);
      virtual shcore::Value print(const shcore::Argument_list &args);
    };
  }
};

#endif
