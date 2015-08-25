/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_MYSQL_H_
#define _MOD_MYSQL_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "utils_time.h"
#include <boost/enable_shared_from_this.hpp>

#if WIN32
#  include <winsock2.h>
#endif

#include <mysql.h>
#include "mod_common.h"

namespace mysh
{
  namespace mysql
  {
    /*    class Session :public ISession, public shcore::Cpp_object_bridge, public boost::enable_shared_from_this<Mysql_connection>
        {
        private:
        MySQL_timer _timer;
        std::string _uri;
        std::string _pwd;
        };*/

    class SHCORE_PUBLIC Field
    {
    public:
      Field(const std::string& catalog, const std::string& db, const std::string& table, const std::string& otable, const std::string& name, const std::string& oname, int length, int type, int flags, int decimals, int charset);

      const std::string& catalog() { return _catalog; };
      const std::string& db() { return _db; };
      const std::string& table() { return _table; };
      const std::string& org_table() { return _org_table; };
      const std::string& name() { return _name; };
      const std::string& org_name() { return _org_name; };
      long length() { return _length; }
      int type() { return _type; }
      int flags() { return _flags; }
      int decimals() { return _decimals; }
      int charset() { return _charset; }

      long max_length() { return _max_length; }
      void max_length(int length_) { _max_length = length_; }

      long name_length() { return _name_length; }

    private:
      std::string _catalog;
      std::string _db;
      std::string _table;
      std::string _org_table;
      std::string _name;
      std::string _org_name;
      long _length;
      int _type;
      int _flags;
      int _decimals;
      int _charset;

      long _max_length;
      long _name_length;
    };

    class Row
    {
    public:
      Row(MYSQL_ROW row, unsigned long *lengths, std::vector<Field>* metadata = NULL);
      virtual ~Row() {}

      virtual shcore::Value get_value(int index);
      virtual std::string get_value_as_string(int index);

    private:
      MYSQL_ROW _row;
      unsigned long *_lengths;
      std::vector<Field> *_metadata;
    };

    class Connection;
    class SHCORE_PUBLIC Result
    {
    public:
      Result(boost::shared_ptr<Connection> owner, my_ulonglong affected_rows, unsigned int warning_count, const char *info);
      virtual ~Result();

      void reset(boost::shared_ptr<MYSQL_RES> res, unsigned long duration);

    public:
      std::vector<Field>& get_metadata(){ return _metadata; };

      // Data Retrieving
      Row *next();
      bool next_result();
      Result *query_warnings();

      bool has_resultset() { return _has_resultset; }

      // Metadata retrieving
      uint64_t affected_rows() { return _affected_rows; }
      uint64_t fetched_row_count() { return _fetched_row_count; }
      int warning_count(){ return _warning_count; }
      unsigned long execution_time(){ return _execution_time; }
      uint64_t last_insert_id() { return _last_insert_id; }
      std::string info() { return _info; }

      //private:
      int fetch_metadata();
      int fetch_warnings();

    private:
      boost::shared_ptr<Connection> _connection;
      std::vector<Field>_metadata;

      boost::weak_ptr<MYSQL_RES> _result;
      uint64_t _affected_rows;
      uint64_t _last_insert_id;
      unsigned int _warning_count;
      unsigned int _fetched_row_count;
      std::string _info;
      unsigned long _execution_time;
      bool _has_resultset;
    };

    class SHCORE_PUBLIC Connection : public boost::enable_shared_from_this<Connection>
    {
    public:
      Connection(const std::string &uri, const char *password = NULL);
      Connection(const std::string &host, int port, const std::string &socket, const std::string &user, const std::string &password, const std::string &schema);
      ~Connection();

      void close();
      Result *executeSql(const std::string &sql);
      bool next_result(Result *target, bool first_result = false);
      std::string uri() { return _uri; }

    private:
      std::string _uri;
      MYSQL *_mysql;
      MySQL_timer _timer;

      boost::shared_ptr<MYSQL_RES> _prev_result;
    };
  };
};

#endif
