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

#include "mod_mysqlx.h"

using namespace mysh;


#include <iostream>

#include <boost/asio.hpp>
#include <string>
#include <iostream>

X_connection::X_connection(const std::string &uri, const char *password)
: Base_connection(uri, password), m_wstream(ios, 1024*1024), _next_stmt_id(0)
{
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 33060;
  std::string sock;
  std::string db;
  int pwd_found;
  
  if (!parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");

  if (password)
    pass.assign(password);
  
   tcp::resolver resolver(ios);
   std::stringstream ss;
   ss << port;

   tcp::resolver::query query(host, ss.str());

   // A default constructed iterator represents the end of the list.
   tcp::resolver::iterator end, endpoint_iterator = resolver.resolve(query);
   boost::system::error_code error;

   do {
    m_wstream.close();
    m_wstream.next_layer().connect(*endpoint_iterator++, error);
   } while (error && endpoint_iterator != end);

   if (error)
     throw shcore::Exception::runtime_error(error.message());
  
  auth(user.c_str(), pass.c_str());
}

void X_connection::send_message(int mid, Message *msg)
{
  char buf[5];
  *(int32_t*)buf = htonl(msg->ByteSize()+5);
  buf[4] = mid;
  m_wstream.write_some(boost::asio::buffer(buf, 5));
  std::string mbuf;
  msg->SerializeToString(&mbuf);
  m_wstream.write_some(boost::asio::buffer(mbuf.data(), mbuf.length()));
  flush();
}

void X_connection::send_message(int mid, const std::string &mdata)
{
  char buf[5];
  *(int32_t*)buf = htonl(mdata.size()+5);
  buf[4] = mid;
  m_wstream.write_some(boost::asio::buffer(buf, 5));
  m_wstream.write_some(boost::asio::buffer(mdata.data(), mdata.length()));
  flush();
}


Message *X_connection::read_response(int &mid)
{
  char buf[5];

  Message* ret_val = NULL;

  boost::system::error_code error;

  m_wstream.read_some(boost::asio::buffer(buf, 5), error);

  if (!error)
  {
    int msglen = ntohl(*(int32_t*)buf) - 5;
    mid = buf[4];
    char *mbuf = new char[msglen];

    m_wstream.read_some(boost::asio::buffer(mbuf, msglen), error);

    if (!error)
    {
      switch (mid)
      {
        case Mysqlx::ServerMessages_Type_SESS_AUTH_OK:
          ret_val = new Mysqlx::Session::AuthenticateOk();
          break;
        case  Mysqlx::ServerMessages_Type_SESS_AUTH_FAIL:
          ret_val = new Mysqlx::Session::AuthenticateFail();
          break;
        case Mysqlx::ServerMessages_Type_OK:
          ret_val = new Mysqlx::Ok();
          break;
        case Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE:
          ret_val = new Mysqlx::Sql::CursorFetchDone();
          break;
        case Mysqlx::ServerMessages_Type_SQL_CURSOR_CLOSE_OK:
          ret_val = new Mysqlx::Sql::CursorCloseOk();
          break;
        case Mysqlx::ServerMessages_Type_SQL_COLUMN_META:
          ret_val = new Mysqlx::Sql::ColumnMetaData();
          break;
        case Mysqlx::ServerMessages_Type_SQL_ROW:
          ret_val = new Mysqlx::Sql::Row();
          break;
        case Mysqlx::ServerMessages_Type_SQL_PREP_STMT_EXEC_OK:
          ret_val = new Mysqlx::Sql::PreparedStmtExecuteOk();
          break;
        case Mysqlx::ServerMessages_Type_SQL_PREP_STMT_OK:
          ret_val = new Mysqlx::Sql::PrepareStmtOk();
          break;
        case Mysqlx::ServerMessages_Type_ERROR:
          ret_val = new Mysqlx::Error();
          break;
        default:
          delete[] mbuf;

          std::stringstream ss;
          ss << "Unknown message received from server: ";
          ss << mid;
          throw shcore::Exception::logic_error(ss.str());
          break;
      }

      // TODO: SERVER: Sending incomplete error messages, i.e. invalid cursor id
      // error->ParseFromString(mbuf);
      if (mid == Mysqlx::ServerMessages_Type_ERROR)
        ret_val->ParsePartialFromString(mbuf);
      else
        ret_val->ParseFromString(mbuf);
    }

    delete[] mbuf;
  }

  // Throws an exception in case of error
  if (error)
    throw std::runtime_error(error.message());

  return ret_val;
}
/* 
 * send_receive_message
 *
 * Handles a happy path of communication with the server:
 * - Sends a message
 * - Receives response and validates it
 *
 * If any of the expected responses is not received then an exception is trown
 */
Message *X_connection::send_receive_message(int& mid, Message *msg, std::set<int> responses, const std::string& info)
{
  send_message(mid, msg);

  Message *response = read_response(mid);

  if (responses.find(mid) == responses.end())
    handle_wrong_response(mid, response, info);

  return response;
}

/*
 * send_receive_message
 *
 * Handles a happy path of communication with the server:
 * - Sends a message
 * - Receives response and validates it
 *
 * If the expected response is not received then an exception is trown
 */
Message *X_connection::send_receive_message(int& mid, Message *msg, int response_id, const std::string& info)
{
  send_message(mid, msg);

  Message *response = read_response(mid);

  if (mid != response_id)
    handle_wrong_response(mid, response, info);

  return response;
}

/*
 * handle_wrong_response
 * Throws the proper exception based on the received response on the
 * send_receive_message methods above
 */
void X_connection::handle_wrong_response(int mid, Message *msg, const std::string& info)
{
  if (mid == Mysqlx::ServerMessages_Type_ERROR)
  {
    Mysqlx::Error *error = dynamic_cast<Mysqlx::Error *>(msg);
    std::string error_message(error->msg());
    delete error;
    throw shcore::Exception::runtime_error(error_message);
  }
  else
  {
    std::string message;
    if (msg)
      delete msg;

    std::stringstream ss;
    ss << "Unexpected response [" << mid << "], " << info;

    throw shcore::Exception::logic_error(ss.str());
  }
}

void X_connection::auth(const char *user, const char *pass)
{
    Mysqlx::Session::AuthenticateStart m;

    m.set_mech_name("plain");
    m.set_auth_data(user);
    m.set_initial_response(pass);
    send_message(Mysqlx::ClientMessages_Type_SESS_AUTH_START, &m);

    int mid;
    Message *r = read_response(mid);

    if (r)
      delete r;

    if (mid != Mysqlx::ServerMessages_Type_SESS_AUTH_OK )
    {
      throw shcore::Exception::argument_error("Authentication failed...");
    }
}


void X_connection::flush()
{
  m_wstream.flush();
}


boost::shared_ptr<shcore::Object_bridge> X_connection::create(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, "Mysqlx_connection()");
  return boost::shared_ptr<shcore::Object_bridge>(new X_connection(args.string_at(0),
                                                                       args.size() > 1 ? args.string_at(1).c_str() : NULL));
}


shcore::Value X_connection::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Mysqlx_connection::close");

  // This should be logged, for now commenting to
  // avoid having unneeded output on the script mode
  // shcore::print("disconnect\n");
  m_wstream.close();
  /*Mysqlx::Connection::close close;
  std::string data;
  close.SerializeToString(&data);
  send_message(Mysqlx::ClientMessages_Type_CON_CLOSE, data);*/

  return shcore::Value();
}

X_resultset *X_connection::_sql(const std::string &query, shcore::Value options)
{
  int mid;
  Message* msg;

  // Reads any remaining stuff from the previos result
  // To let the comm in a clean state
  if (_last_result && !_last_result->is_all_fetch_done())
  {
    shcore::Value waste(_last_result->fetch_all(shcore::Argument_list()));
  }

  _timer.start();
  ++_next_stmt_id;

  // Prepares the SQL
  Mysqlx::Sql::PrepareStmt stmt;
  stmt.set_stmt_id(_next_stmt_id);
  stmt.set_stmt(query);
  mid = Mysqlx::ClientMessages_Type_SQL_PREP_STMT;
  msg = send_receive_message(mid, &stmt, Mysqlx::ServerMessages_Type_SQL_PREP_STMT_OK, "preparing statement: " + query);

  // Executes the query
  Mysqlx::Sql::PreparedStmtExecute stmt_exec;
  stmt_exec.set_stmt_id(_next_stmt_id);
  stmt_exec.set_cursor_id(_next_stmt_id);
  mid = Mysqlx::ClientMessages_Type_SQL_PREP_STMT_EXEC;
  msg = send_receive_message(mid, &stmt_exec, Mysqlx::ServerMessages_Type_SQL_PREP_STMT_EXEC_OK, "executing statement: " + query);

  // Retrieves the affected rows
  Mysqlx::Sql::PreparedStmtExecuteOk *exec_done = dynamic_cast<Mysqlx::Sql::PreparedStmtExecuteOk *>(msg);
  uint64_t affected_rows = 0;
  if (exec_done->has_rows_affected())
    affected_rows = exec_done->rows_affected();

  // Creates the resultset
  // TODO: The warning count is not returned upon SQL Execution
  X_resultset* result = new X_resultset(shared_from_this(), _next_stmt_id, affected_rows, 0, NULL, options && options.type == shcore::Map ? options.as_map() : boost::shared_ptr<shcore::Value::Map_type>());

  return result;
}

shcore::Value X_connection::sql_one(const std::string &sql)
{
  X_resultset* result = _sql(sql, shcore::Value());

  shcore::Value ret_val = shcore::Value::Null();

  // Prepares the resultset for processing
  if (next_result(result, true))
  {
    shcore::Argument_list no_args;

    ret_val = result->next(no_args);

    // Reads any remaining record to keep the tx buffers consistent
    while (result->next(no_args));
  }

  delete result;

  return ret_val;
}

shcore::Value X_connection::sql(const std::string &query, shcore::Value options)
{
  shcore::Value ret_val = shcore::Value::Null();

  // Creates the resultset
  X_resultset* result = _sql(query, options);

  // Prepares the resultset for processing
  if (next_result(result, true))
  {
    _last_result = boost::shared_ptr<X_resultset>(result);
    ret_val = shcore::Value(boost::shared_ptr<shcore::Object_bridge>(_last_result));
  }
  else
    delete result;

  return ret_val;
}

X_connection::~X_connection()
{
  close(shcore::Argument_list());
}



bool X_connection::next_result(Base_resultset *target, bool first_result)
{
  bool ret_val = false;
  X_resultset *real_target = dynamic_cast<X_resultset *> (target);

  int mid = Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE;
  Message* msg;

  // Just to ensure the right instance was received as parameter
  if (real_target)
  {
    // Switching to the next result will be done only
    // if there is still data to be read
    if (!real_target->is_all_fetch_done())
    {
      // Burns out the remaining records for the current result
      if (!first_result && !real_target->is_current_fetch_done())
      {
        do
        {
          msg = read_response(mid);

          if (msg)
          {
            delete msg;
            msg = NULL;
          }

        }
        while(mid == Mysqlx::ServerMessages_Type_SQL_ROW);
      }

      // Still results to be read
      if (mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE)
      {
        mid = 0;
        msg = NULL;
        ret_val = true;

        // Loads the metadata
        if (target->fetch_metadata() > 0)
        {

          // Row loading comes after the metadata, sends this here
          Mysqlx::Sql::CursorFetchRows fetch_rows;
          fetch_rows.set_cursor_id(real_target->get_cursor_id());
          fetch_rows.set_fetch_limit(0); // This is not used anyways

          mid = Mysqlx::ClientMessages_Type_SQL_CURSOR_FETCH_ROWS;
          std::set<int> responses;
          responses.insert(Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE);
          responses.insert(Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE);
          responses.insert(Mysqlx::ServerMessages_Type_SQL_ROW);

          msg = send_receive_message(mid, &fetch_rows, responses, "fetching rows");
        }
      }

      // Sets the response with the required data
      _timer.end();
      real_target->reset(_timer.raw_duration(), mid, msg);
    }
  }
  else
    throw std::logic_error("Invalid parameter received, target should be an instance of X_resultset;");

  return ret_val;
}

X_row::X_row(std::vector<Field>& metadata, bool key_by_index, Mysqlx::Sql::Row *row): Base_row(metadata, key_by_index), _row(row)
{
}

shcore::Value X_row::get_value(int index)
{

  Mysqlx::Datatypes::Any field(_row->field(index));

  // TODO: Is there a way a field has nothing (like in mysql NULL)
  // TODO: What if field.has_object and field.has_array
  if (field.has_scalar())
  {
    if (field.scalar().has_v_bool())
      return shcore::Value(field.scalar().v_bool());
    else if (field.scalar().has_v_double())
      return shcore::Value(field.scalar().v_double());
    else if (field.scalar().has_v_float())
      return shcore::Value(field.scalar().v_float());
    //else if (field.scalar().has_v_null())
    //  return shcore::Value::Null();
    else if (field.scalar().has_v_opaque())
      return shcore::Value(field.scalar().v_opaque());
    else if (field.scalar().has_v_signed_int())
      return shcore::Value(field.scalar().v_signed_int());
    else if (field.scalar().has_v_unsigned_int())
      return shcore::Value(int64_t(field.scalar().v_unsigned_int()));
  }

  return shcore::Value::Null();
}

std::string X_row::get_value_as_string(int index)
{
  shcore::Value value = get_value(index);
  return value.repr();
}

X_row::~X_row()
{
  if (_row)
    delete _row;
}


X_resultset::X_resultset(boost::shared_ptr<X_connection> owner, int cursor_id, uint64_t affected_rows, int warning_count, const char *info, boost::shared_ptr<shcore::Value::Map_type> options) :
Base_resultset(owner, affected_rows, warning_count, info, options), _xowner(owner), _cursor_id(cursor_id), _next_mid(0),
_current_fetch_done(false), _all_fetch_done(false)
{
}


X_resultset::~X_resultset(){}

Base_row* X_resultset::next_row()
{
  Base_row* ret_val = NULL;

  // TODO: Does it always have a resultset??
  if (has_resultset())
  {
    boost::shared_ptr<X_connection> owner = _xowner.lock();

    if (!is_current_fetch_done() && owner)
    {
      // Reads the next message if not already done by the owner
      // This variable must be cleaned out at the end
      if (!_next_message)
        _next_message = owner.get()->read_response(_next_mid);

      if (_next_mid == Mysqlx::ServerMessages_Type_SQL_ROW)
      {
        ret_val = new X_row(_metadata, _key_by_index, dynamic_cast<Mysqlx::Sql::Row *>(_next_message));

        // Each read row increases the count
        _fetched_row_count++;
      }
      else if (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE)
      {
        _all_fetch_done = true;
        _current_fetch_done = true;

        // Closes the cursor
        Mysqlx::Sql::CursorClose fetch_done;
        fetch_done.set_cursor_id(_cursor_id);
        int mid = Mysqlx::ClientMessages_Type_SQL_CURSOR_CLOSE;
        Message *response = owner->send_receive_message(mid, &fetch_done, Mysqlx::ServerMessages_Type_SQL_CURSOR_CLOSE_OK, "closing cursor");

        delete response;
      }
      else if (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE)
        _current_fetch_done = true;
      else
      {
        // TODO: Should probably trow exception???
      }
    }
  }

  _next_message = NULL;
  return ret_val;
}

int X_resultset::fetch_metadata()
{
  // Fetch the metadata
  int ret_val = 0;
  boost::shared_ptr<X_connection> owner = _xowner.lock();

  _metadata.clear();

  if (owner)
  {
    Mysqlx::Sql::CursorFetchMetaData fetch_meta;
    fetch_meta.set_cursor_id(get_cursor_id());

    int mid = Mysqlx::ClientMessages_Type_SQL_CURSOR_FETCH_META;
    std::set<int> responses;
    responses.insert(Mysqlx::ServerMessages_Type_SQL_COLUMN_META);
    responses.insert(Mysqlx::ServerMessages_Type_ERROR);
    Message* msg = owner->send_receive_message(mid, &fetch_meta, responses, "fetching metadata");

    if (mid == Mysqlx::ServerMessages_Type_SQL_COLUMN_META )
    {
      do
      {
        Mysqlx::Sql::ColumnMetaData *meta = dynamic_cast<Mysqlx::Sql::ColumnMetaData *>(msg);

        _metadata.push_back(Field(meta->catalog(),
                                  meta->schema(),
                                  meta->table(),
                                  meta->original_table(),
                                  meta->name(),
                                  meta->original_name(),
                                  0, // length: not supported yet
                                  meta->type(),
                                  0, // flags: not supported yet
                                  meta->decimals(),
                                  0)); // charset: not supported yet

        msg = owner->read_response(mid);

      } while (mid != Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE);

      ret_val = _metadata.size();
    }
    else if (mid == Mysqlx::ServerMessages_Type_ERROR)
    {
      // Since metadata will be empty, it implies the result had no records
      Mysqlx::Error *error = dynamic_cast<Mysqlx::Error *>(msg);
      if (error->msg() != "Invalid cursor id")
      {
        std::cout << "Unexpected error retrieving metadata: " << error->msg() << std::endl;
        ret_val = -1;
      }
    }
  }

  return ret_val;
}

void X_resultset::reset(unsigned long duration, int next_mid, ::google::protobuf::Message* next_message)
{
  _next_mid = next_mid;

  // Nothing to be read anymnore on these cases
  _all_fetch_done = (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE);

  // Nothing to be read for the current result on these cases
  _current_fetch_done = (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE ||
                         _next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE);

  _raw_duration = duration;
  _next_message = next_message;

  // The statement may have a set of rows (may be empty tho)
  _has_resultset = (_next_mid != Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE);
}

#include "shellcore/object_factory.h"
namespace {
static struct Auto_register {
  Auto_register()
  {
    shcore::Object_factory::register_factory("mysqlx", "Connection", &X_connection::create);
  }
} Mysql_connection_register;

};
