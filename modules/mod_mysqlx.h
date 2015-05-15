/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

    shcore::Value table_insert(shcore::Value::Map_type_ref data);

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

    void add_field(shcore::Value value);

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

  class Crud_definition : public shcore::Cpp_object_bridge
  {
  public:
    Crud_definition(const shcore::Argument_list &args);
    // T the moment will put these since we don't really care about them
    virtual std::string class_name() const { return "TableInsert"; }
    virtual bool operator == (const Object_bridge &other) const { return false; }

    // Overrides to consider enabled/disabled status
    virtual std::vector<std::string> get_members() const;
    virtual shcore::Value get_member(const std::string &prop) const;
    virtual bool has_member(const std::string &prop) const;
    virtual shcore::Value call(const std::string &name, const shcore::Argument_list &args);

  protected:
    boost::weak_ptr<X_connection> _conn;
    shcore::Value::Map_type_ref _data;

    // The CRUD operations will use "dynamic" functions to control the method chaining.
    // A dynamic function is one that will be enabled/disabled based on the method
    // chain sequence.

    // Holds the dynamic functions enable/disable status
    std::map<std::string, bool> _enabled_functions;

    // Holds relation of 'enabled' states for every dynamic function
    std::map<std::string, std::set<std::string> > _enable_paths;

    // Registers a dynamic function and it's associated 'enabled' states
    void register_dynamic_function(const std::string& name, const std::string& enable_after);

    // Enable/disable functions based on the received and registered states
    void update_functions(const std::string& state);
    void enable_function(const char *name, bool enable);
  };

  class TableInsert : public Crud_definition
  {
  private:
    TableInsert(const shcore::Argument_list &args);
  public:
    static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
    shcore::Value insert(const shcore::Argument_list &args);
    shcore::Value values(const shcore::Argument_list &args);
    shcore::Value bind(const shcore::Argument_list &args);
    shcore::Value run(const shcore::Argument_list &args);
  };
};

#endif
