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

    protected:
      boost::shared_ptr< ::mysqlx::Result> _result;

    private:
      int _cursor_id;
    };

    class Collection_resultset : public Resultset
    {
    public:
      Collection_resultset(boost::shared_ptr< ::mysqlx::Result> result);

      virtual std::string class_name() const  { return "CollectionResultset"; }
      virtual shcore::Value next(const shcore::Argument_list &args);
      virtual shcore::Value print(const shcore::Argument_list &args);
    };
  }
};

#endif
