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

#ifndef _MOD_RESULT_H_
#define _MOD_RESULT_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "base_resultset.h"
#include <list>

namespace mysh
{
  namespace mysql
  {
    class Result;

    /**
    * Allows browsing through the result information after performing an operation on the database through the MySQL Protocol.
    * This class allows access to the result set from the classic MySQL data model to be retrieved from Dev API queries.
    */
    class ClassicResult : public ShellBaseResult
    {
    public:
      ClassicResult(boost::shared_ptr<Result> result);

      virtual std::string class_name() const { return "ClassicResult"; }
      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      virtual shcore::Value fetch_one(const shcore::Argument_list &args);
      virtual shcore::Value fetch_all(const shcore::Argument_list &args);
      virtual shcore::Value next_data_set(const shcore::Argument_list &args);

    protected:
      boost::shared_ptr<Result> _result;

#ifdef DOXYGEN
      Integer affectedRowCount; //!< Same as getAffectedItemCount()
      Integer columnCount; //!< Same as getcolumnCount()
      List columnNames; //!< Same as getColumnNames()
      List columns; //!< Same as getColumns()
      String executionTime; //!< Same as getExecutionTime()
      Bool hasData; //!< Same as getHasData()
      String info; //!< Same as getInfo()
      Integer lastInsertId; //!< Same as getLastInsertId()
      List warnings; //!< Same as getWarnings()
      Integer warningCount; //!< Same as getWarningCount()

      Row fetchOne();
      List fetchAll();
      Integer getAffectedRowCount();
      Integer getColumnCount();
      List getColumnNames();
      List getColumns();
      String getExecutionTime();
      Bool getHasData();
      String getInfo();
      Integer getLastInsertId();
      Integer getWarningCount();
      List getWarnings();
      Bool nextDataSet();
#endif
    };
  }
};

#endif
