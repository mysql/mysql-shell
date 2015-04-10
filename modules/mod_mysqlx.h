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
#include <boost/asio.hpp>
#include <google/protobuf/message.h>

#undef ERROR //Needed to avoid conflict with ERROR in mysqlx.pb.h
#include "mysqlx.pb.h"

typedef google::protobuf::Message Message;

namespace mysh {

class X_connection : public mysh::Base_connection, public boost::enable_shared_from_this<X_connection>
{
  typedef boost::asio::ip::tcp tcp;
public:
  X_connection(const std::string &uri, const char *password = NULL);
  virtual ~X_connection();

  virtual std::string class_name() const { return "X_connection"; }

  // X Protocol Methods
  void send_message(int mid, Message *msg);
  void send_message(int mid, const std::string &mdata);
  Message *read_response(int &mid);
  Message *send_receive_message(int& mid, Message *msg, std::set<int> responses, const std::string& info);
  Message *send_receive_message(int& mid, Message *msg, int response_id, const std::string& info);
  void handle_wrong_response(int mid, Message *msg, const std::string& info);
  void auth(const char *user, const char *pass);
  void flush();

  // Connector Implementation
  virtual shcore::Value close(const shcore::Argument_list &args);
  virtual shcore::Value sql(const std::string &sql, shcore::Value options);
  virtual shcore::Value sql_one(const std::string &sql);

  virtual bool next_result(Base_resultset *target, bool first_result = false);

  static boost::shared_ptr<Object_bridge> create(const shcore::Argument_list &args);
  
  

protected:
  boost::asio::io_service ios;
  boost::asio::buffered_write_stream<boost::asio::ip::tcp::socket> m_wstream;

private:
  int _next_stmt_id;
};


class X_row : public Base_row
{
public:
  X_row(std::vector<Field>& metadata, bool key_by_index, Mysqlx::Sql::Row *row);
  virtual ~X_row();


  virtual shcore::Value get_value(int index);
  virtual std::string get_value_as_string(int index);

private:
  Mysqlx::Sql::Row *_row;
};


class X_resultset : public Base_resultset
{
public:
  X_resultset(boost::shared_ptr<X_connection> owner, int cursor_id, uint64_t affected_rows, int warning_count, const char *info, boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());
  virtual ~X_resultset();

  virtual std::string class_name() const  { return "X_resultset"; }
  virtual int fetch_metadata();

  void reset(unsigned long duration, int next_mid, ::google::protobuf::Message* next_message);
  int get_cursor_id() { return _cursor_id; }
  bool is_all_fetch_done()  { return _all_fetch_done;     }
  bool is_current_fetch_done()  { return _current_fetch_done; }


protected:
  virtual Base_row* next_row();
  virtual bool next_result();

  boost::weak_ptr<X_connection> _owner;

private:
  int _cursor_id;
  int _next_mid;
  bool _current_fetch_done;
  bool _all_fetch_done;
  Message* _next_message;
};

};

#endif
