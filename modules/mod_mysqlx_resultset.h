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
#include <boost/enable_shared_from_this.hpp>

namespace mysqlx
{
  class Result;
}

namespace mysh
{
  namespace mysqlx
  {
    /**
    * Base class for the different types of results returned by the server.
    */
    class BaseResult : public mysh::ShellBaseResult
    {
    public:
      BaseResult(boost::shared_ptr< ::mysqlx::Result> result);
      virtual ~BaseResult() {}

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual void append_json(shcore::JSON_dumper& dumper) const;

#ifdef DOXYGEN
      Integer warningCount; //!< Same as getwarningCount()
      List warnings; //!< Same as getWarnings()
      String executionTime; //!< Same as getExecutionTime()

      Integer getWarningCount();
      List getWarnings();
      String getExecutionTime();
#endif

    protected:
      boost::shared_ptr< ::mysqlx::Result> _result;
    };

    /**
    * Allows retrieving information about non query operations performed on the database.
    *
    * An instance of this class will be returned on the CRUD operations that change the content of the database:
    *
    * - On Table: insert, update and delete
    * - On Collection: add, modify and remove
    *
    * Other functions on the BaseSession class also return an instance of this class:
    *
    * - Transaction handling functions
    * - Drop functions
    */
    class Result : public BaseResult, public boost::enable_shared_from_this<Result>
    {
    public:
      Result(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~Result(){};

      virtual std::string class_name() const { return "Result"; }
      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual void append_json(shcore::JSON_dumper& dumper) const;

#ifdef DOXYGEN
      Integer affectedItemCount; //!< Same as getAffectedItemCount()
      Integer lastInsertId; //!< Same as getLastInsertId()
      Integer lastDocumentId; //!< Same as getLastDocumentId()

      Integer getAffectedItemCount();
      Integer getLastInsertId();
      String getLastDocumentId();
#endif
    };

    /**
    * Allows traversing the DbDoc objects returned by a Collection.find operation.
    */
    class DocResult : public BaseResult, public boost::enable_shared_from_this<DocResult>
    {
    public:
      DocResult(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~DocResult(){};

      shcore::Value fetch_one(const shcore::Argument_list &args) const;
      shcore::Value fetch_all(const shcore::Argument_list &args) const;

      virtual std::string class_name() const { return "DocResult"; }
      virtual void append_json(shcore::JSON_dumper& dumper) const;

#ifdef DOXYGEN
      Document fetchOne();
      List fetchAll();
#endif
    };

    /**
    * Allows traversing the Row objects returned by a Table.select operation.
    */
    class RowResult : public BaseResult, public boost::enable_shared_from_this<RowResult>
    {
    public:
      RowResult(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~RowResult(){};

      shcore::Value fetch_one(const shcore::Argument_list &args) const;
      shcore::Value fetch_all(const shcore::Argument_list &args) const;

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      virtual std::string class_name() const { return "RowResult"; }
      virtual void append_json(shcore::JSON_dumper& dumper) const;

#ifdef DOXYGEN
      Row fetchOne();
      List fetchAll();

      Integer columnCount; //!< Same as getColumnCount()
      List columnNames; //!< Same as getColumnNames()
      List columns; //!< Same as getColumns()

      Integer getColumnCount();
      List getColumnNames();
      List getColumns();

#endif
    };

    /**
    * Allows browsing through the result information after performing an operation on the database
    * done through NodeSession.sql
    */
    class SqlResult : public RowResult, public boost::enable_shared_from_this<SqlResult>
    {
    public:
      SqlResult(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~SqlResult(){};

      virtual std::string class_name() const { return "SqlResult"; }
      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      virtual shcore::Value next_data_set(const shcore::Argument_list &args);
      virtual void append_json(shcore::JSON_dumper& dumper) const;

#ifdef DOXYGEN
      Integer lastInsertId; //!< Same as getLastInsertId()
      Integer affectedRowCount; //!< Same as getAffectedRowCount()
      Bool hasData; //!< Same as getHasData()

      Integer getLastInsertId();
      Integer getAffectedRowCount();
      Bool getHasData();
      Bool nextDataSet();
#endif
    };
  }
};

#endif
