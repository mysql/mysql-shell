/*
 * Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_MYSQLX_H_
#define _MOD_MYSQLX_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "mod_connection.h"

#include <list>
#include "mysqlx_test_connector.h"

namespace mysh
{
  class X_resultset;
  class X_connection : public mysh::Base_connection, public boost::enable_shared_from_this<X_connection>
  {
    typedef boost::asio::ip::tcp tcp;
  public:
    X_connection(const std::string &uri, const char *password = NULL);
    virtual ~X_connection();

    virtual std::string class_name() const { return "X_connection"; }

    void auth(const char *user, const char *pass);
    void flush();

    // Connector Implementation
    virtual shcore::Value close(const shcore::Argument_list &args);
    virtual shcore::Value sql(const std::string &query, shcore::Value options);
    virtual shcore::Value sql_one(const std::string &query);

    shcore::Value crud_execute(const std::string& id, shcore::Value::Map_type_ref data);
    shcore::Value crud_table_insert(shcore::Value::Map_type_ref data);
    shcore::Value crud_collection_add(shcore::Value::Map_type_ref data);

    virtual bool next_result(Base_resultset *target, bool first_result = false);

    static boost::shared_ptr<Object_bridge> create(const shcore::Argument_list &args);

    boost::shared_ptr<mysqlx::Mysqlx_test_connector> get_protobuf() { return _protobuf; }

    void flush_result(X_resultset *target, bool complete);

  protected:
    boost::shared_ptr<mysqlx::Mysqlx_test_connector> _protobuf;

  private:
    int _next_stmt_id;
    X_resultset *_sql(const std::string &query, shcore::Value options);
    boost::shared_ptr<X_resultset> _last_result;
  };

  class X_row : public Base_row
  {
  public:
    X_row(Mysqlx::Sql::Row *row, std::vector<Field>* metadata = NULL);
    virtual ~X_row();

    virtual shcore::Value get_value(int index);
    virtual std::string get_value_as_string(int index);

    static void add_field(Mysqlx::Sql::Row *row, shcore::Value value);

  private:
    Mysqlx::Sql::Row *_row;
  };

  class X_resultset : public Base_resultset
  {
  public:
    X_resultset(boost::shared_ptr<X_connection> owner, bool has_data, int cursor_id, uint64_t affected_rows, uint64_t last_insert_id, int warning_count, const char *info, int next_mid, Message* next_message, boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());
    virtual ~X_resultset();

    virtual std::string class_name() const  { return "X_resultset"; }
    virtual int fetch_metadata();

    void reset(unsigned long duration = -1, int next_mid = 0, ::google::protobuf::Message* next_message = NULL);
    void set_result_metadata(Message *msg);
    int get_cursor_id() { return _cursor_id; }
    bool is_all_fetch_done()  { return _all_fetch_done; }
    bool is_current_fetch_done()  { return _current_fetch_done; }

  protected:
    virtual Base_row* next_row();

    boost::weak_ptr<X_connection> _xowner;

  private:
    int _cursor_id;
    int _next_mid;
    Message* _next_message;

    bool _current_fetch_done;
    bool _all_fetch_done;
  };
};

#endif
