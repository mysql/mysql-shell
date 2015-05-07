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

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/
static void translate_exception()
{
  try
  {
    throw;
  }
  catch (boost::system::system_error &e)
  {
    throw shcore::Exception::runtime_error(e.what());
  }
  catch (std::runtime_error &e)
  {
    throw shcore::Exception::runtime_error(e.what());
  }
  catch (std::logic_error &e)
  {
    throw shcore::Exception::logic_error(e.what());
  }
  catch (...)
  {
    throw;
  }
}

#define CATCH_AND_TRANSLATE()   \
  catch (...)                   \
{ translate_exception(); }

X_connection::X_connection(const std::string &uri, const char *password)
: Base_connection(uri, password), _next_stmt_id(0)
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

  std::stringstream ss;
  ss << port;

  try
  {
    _protobuf = boost::shared_ptr<Mysqlx_test_connector>(new Mysqlx_test_connector(host, ss.str()));
  }
  CATCH_AND_TRANSLATE();

  auth(user.c_str(), pass.c_str());
}

void X_connection::auth(const char *user, const char *pass)
{
  Mysqlx::Session::AuthenticateStart m;

  m.set_mech_name("plain");
  m.set_auth_data(user);
  m.set_initial_response(pass);
  _protobuf->send_message(Mysqlx::ClientMessages_Type_SESS_AUTH_START, &m);

  int mid;
  try
  {
    Message *r = _protobuf->read_response(mid);

    if (r)
      delete r;
  }
  CATCH_AND_TRANSLATE();

  if (mid != Mysqlx::ServerMessages_Type_SESS_AUTH_OK)
  {
    throw shcore::Exception::argument_error("Authentication failed...");
  }
}

boost::shared_ptr<shcore::Object_bridge> X_connection::create(const shcore::Argument_list &args)
{
  args.ensure_count(1, 2, "Mysqlx_connection()");
  return boost::shared_ptr<shcore::Object_bridge>(new X_connection(args.string_at(0),
    args.size() > 1 ? args.string_at(1).c_str() : NULL));
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

  uint64_t affected_rows = 0;

  try
  {
    // Prepares the SQL
    Mysqlx::Sql::PrepareStmt stmt;
    stmt.set_stmt_id(_next_stmt_id);
    stmt.set_stmt(query);
    mid = Mysqlx::ClientMessages_Type_SQL_PREP_STMT;
    msg = _protobuf->send_receive_message(mid, &stmt, Mysqlx::ServerMessages_Type_SQL_PREP_STMT_OK, "preparing statement: " + query);

    // Executes the query
    Mysqlx::Sql::PreparedStmtExecute stmt_exec;
    stmt_exec.set_stmt_id(_next_stmt_id);
    stmt_exec.set_cursor_id(_next_stmt_id);
    mid = Mysqlx::ClientMessages_Type_SQL_PREP_STMT_EXEC;
    msg = _protobuf->send_receive_message(mid, &stmt_exec, Mysqlx::ServerMessages_Type_SQL_PREP_STMT_EXEC_OK, "executing statement: " + query);

    // Retrieves the affected rows
    Mysqlx::Sql::PreparedStmtExecuteOk *exec_done = dynamic_cast<Mysqlx::Sql::PreparedStmtExecuteOk *>(msg);
    if (exec_done->has_rows_affected())
      affected_rows = exec_done->rows_affected();
  }
  CATCH_AND_TRANSLATE();

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

shcore::Value X_connection::close(const shcore::Argument_list &args)
{
  _protobuf->close();

  return shcore::Value();
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
  Message* msg = NULL;

  // Just to ensure the right instance was received as parameter
  if (real_target)
  {
    // Switching to the next result will be done only
    // if there is still data to be read
    try
    {
      if (!real_target->is_all_fetch_done())
      {
        // Burns out the remaining records for the current result
        if (!first_result && !real_target->is_current_fetch_done())
        {
          do
          {
            msg = _protobuf->read_response(mid);

            if (msg)
            {
              delete msg;
              msg = NULL;
            }
          } while (mid == Mysqlx::ServerMessages_Type_SQL_ROW);
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

            msg = _protobuf->send_receive_message(mid, &fetch_rows, responses, "fetching rows");
          }
        }
      }
    }
    CATCH_AND_TRANSLATE();

    // Sets the response with the required data
    _timer.end();
    real_target->reset(_timer.raw_duration(), mid, msg);
  }
  else
    throw shcore::Exception::logic_error("Invalid parameter received, target should be an instance of X_resultset;");

  return ret_val;
}

X_row::X_row(std::vector<Field>& metadata, bool key_by_index, Mysqlx::Sql::Row *row) : Base_row(metadata, key_by_index), _row(row)
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
      try
      {
        // Reads the next message if not already done by the owner
        // This variable must be cleaned out at the end
        if (!_next_message)
          _next_message = owner->get_protobuf()->read_response(_next_mid);

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
          Message *response = owner->get_protobuf()->send_receive_message(mid, &fetch_done, Mysqlx::ServerMessages_Type_SQL_CURSOR_CLOSE_OK, "closing cursor");

          delete response;
        }
        else if (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE)
          _current_fetch_done = true;
        else
        {
          // TODO: Should probably trow exception???
        }
      }
      CATCH_AND_TRANSLATE();
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

    try
    {
      int mid = Mysqlx::ClientMessages_Type_SQL_CURSOR_FETCH_META;
      std::set<int> responses;
      responses.insert(Mysqlx::ServerMessages_Type_SQL_COLUMN_META);
      responses.insert(Mysqlx::ServerMessages_Type_ERROR);
      Message* msg = owner->get_protobuf()->send_receive_message(mid, &fetch_meta, responses, "fetching metadata");

      if (mid == Mysqlx::ServerMessages_Type_SQL_COLUMN_META)
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

          msg = owner->get_protobuf()->read_response(mid);
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
    CATCH_AND_TRANSLATE();
  }

  return ret_val;
}

void X_resultset::reset(unsigned long duration, int next_mid, ::google::protobuf::Message* next_message)
{
  _next_mid = next_mid;

  // Nothing to be read anymnore on these cases
  _all_fetch_done = (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE ||
    _next_mid == 0);

  // Nothing to be read for the current result on these cases
  _current_fetch_done = (_next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE ||
    _next_mid == Mysqlx::ServerMessages_Type_SQL_CURSOR_FETCH_DONE_MORE);

  _raw_duration = duration;
  _next_message = next_message;

  // The statement may have a set of rows (may be empty tho)
  _has_resultset = (_next_mid != 0);
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