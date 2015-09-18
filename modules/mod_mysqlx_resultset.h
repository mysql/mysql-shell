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
    * Allows browsing through the result information after performing an operation on the database.
    * This class allows access to the result set from the classic MySQL data model to be retrieved from Dev API queries.
    * \todo delete warningCount and getWarningCount()
    * \todo delete fetchedRowCount and getfetchedRowCount()
    * \todo Implement and Document buffer()
    * \todo Implement and Document flush()
    * \todo Implement and Document rewind()
    */
    class Resultset : public BaseResultset, public boost::enable_shared_from_this<Resultset>
    {
    public:
      Resultset(boost::shared_ptr< ::mysqlx::Result> result);

      virtual ~Resultset();

      virtual std::string class_name() const { return "Resultset"; }
      virtual shcore::Value get_member(const std::string &prop) const;

      virtual shcore::Value next(const shcore::Argument_list &args);
      virtual shcore::Value all(const shcore::Argument_list &args);
      virtual shcore::Value next_result(const shcore::Argument_list &args);
      virtual shcore::Value buffer(const shcore::Argument_list &args);
      virtual shcore::Value flush(const shcore::Argument_list &args);
      virtual shcore::Value rewind(const shcore::Argument_list &args);

      int get_cursor_id() { return _cursor_id; }
#ifdef DOXYGEN
      List columnMetadata; //!< Same as getColumnMetadata()
      Integer lastInsertId; //!< Same as getLastInsertId()
      Integer affectedRows; //!< Same as getAffectedRows()
      Integer warnings; //!< Same as getWarnings()
      String info; //!< Same as getInfo()
      String executionTime; //!< Same as getExecutionTime()
      Bool hasData; //!< Same as getHasData()

      List getColumnMetadata();
      Integer getLastInsertId();
      Integer getAffectedRows();
      Integer getWarnings();
      String getExecutionTime();
      String getInfo();
      Bool getHasData();
      Bool nextDataSet();
      Row next();
      List all();
      Resultset buffer();
      Undefined flush();
      Undefined rewind();

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
      List all();
      Document next();
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
