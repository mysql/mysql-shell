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
using namespace shcore;

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

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
    _protobuf = boost::shared_ptr<mysqlx::Mysqlx_test_connector>(new mysqlx::Mysqlx_test_connector(host, ss.str()));
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
  _protobuf->send_message(&m);

  int mid;
  try
  {
    Message *r = _protobuf->read_response(mid);

    if (r)
      delete r;
  }
  CATCH_AND_TRANSLATE();

  if (mid != Mysqlx::ServerMessages::SESS_AUTHENTICATE_OK)
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
    flush_result(_last_result.get(), true);

  _timer.start();
  ++_next_stmt_id;

  bool has_data = false;

  try
  {
    Mysqlx::Sql::StmtExecute stmt;
    stmt.set_stmt(query);
    mid = Mysqlx::ClientMessages::SQL_STMT_EXECUTE;

    std::set<int> responses;
    responses.insert(Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
    responses.insert(Mysqlx::ServerMessages::SQL_COLUMN_META_DATA);

    msg = _protobuf->send_receive_message(mid, &stmt, responses, "executing statement: " + query);

    has_data = mid == Mysqlx::ServerMessages::SQL_COLUMN_META_DATA;
  }
  CATCH_AND_TRANSLATE();

  X_resultset* result = new X_resultset(shared_from_this(), has_data, _next_stmt_id, 0, 0, 0, NULL, mid, msg, options && options.type == shcore::Map ? options.as_map() : boost::shared_ptr<shcore::Value::Map_type>());

  // If no data will be returned then loads the resultset metadata
  // And fills the timer data as well
  if (mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
  {
    _timer.end();

    result->set_result_metadata(msg);
    result->reset(_timer.raw_duration());
  }

  return result;
}

shcore::Value X_connection::sql_one(const std::string &query)
{
  shcore::Value ret_val = shcore::Value::Null();

  // Executes the normal query
  sql(query, shcore::Value());

  if (_last_result->has_resultset())
  {
    shcore::Argument_list no_args;
    ret_val = _last_result->next(no_args);

    // Reads whatever is remaining, we do not care about it
    flush_result(_last_result.get(), true);
  }

  return ret_val;
}

shcore::Value X_connection::sql(const std::string &query, shcore::Value options)
{
  // Creates the resultset
  _last_result.reset(_sql(query, options));

  // Calls next result so the metadata is loaded
  // if any result
  if (_last_result->has_resultset())
    next_result(_last_result.get(), true);

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(_last_result));
}

shcore::Value X_connection::close(const shcore::Argument_list &args)
{
  if (_last_result && !_last_result->is_all_fetch_done())
    flush_result(_last_result.get(), true);

  _protobuf->close();

  return shcore::Value();
}

X_connection::~X_connection()
{
  close(shcore::Argument_list());
}

shcore::Value X_connection::table_insert(shcore::Value::Map_type_ref data)
{
  //----SAMPLE IMPLEMENTATION
  // This implementation allows performing a table insert operation as:
  //
  // table.insert(['col1', 'col2','col3']).values(['val1', val2', 'val3']).run()
  //
  // The definitive implementation should consider many different cases
  // and also many validations must be included
  uint64_t affected_rows = 0;
  uint64_t last_insert_id = 0;

  try
  {
    Mysqlx::Crud::Insert insert;
    insert.mutable_collection()->set_schema((*data)["schema"].as_string());
    insert.mutable_collection()->set_name((*data)["collection"].as_string());
    insert.set_data_model(Mysqlx::Crud::DataModel((*data)["data_model"].as_int()));

    if (data->has_key("Fields"))
    {
      if (data->has_key("Values"))
      {
        shcore::Value::Array_type_ref fields = (*data)["Fields"].as_array();
        shcore::Value::Array_type_ref values = (*data)["Values"].as_array();

        if (fields->size() == values->size())
        {
          Mysqlx::Crud::Column *column;
          size_t index;

          // Creates the projection
          for (index = 0; index < fields->size(); index++)
          {
            column = insert.mutable_projection()->Add();
            column->set_name(fields->at(index).as_string());
          }

          // Creates the row list
          X_row *row = new X_row(insert.mutable_row()->Add());
          for (index = 0; index < fields->size(); index++)
          {
            row->add_field(values->at(index));
          }
        }
      }
    }
    else if (data->has_key("FieldsAndValues"))
    {
    }

    int mid = Mysqlx::ClientMessages::CRUD_INSERT;
    Message* msg = _protobuf->send_receive_message(mid, &insert, Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK, "Inserting record...");

    Mysqlx::Sql::StmtExecuteOk *insert_ok = dynamic_cast<Mysqlx::Sql::StmtExecuteOk *>(msg);

    if (insert_ok->has_rows_affected())
      affected_rows = insert_ok->rows_affected();
    if (insert_ok->has_last_insert_id())
      last_insert_id = insert_ok->last_insert_id();
  }
  CATCH_AND_TRANSLATE();

  // Creates the resultset
  X_resultset* result = new X_resultset(shared_from_this(), false, _next_stmt_id, affected_rows, 0, 0, NULL, 0, NULL);

  return shcore::Value(boost::shared_ptr<shcore::Object_bridge>(result));
}

// Used to consume all the remaining messages of the current result
// If complete false it will only flush the messages about the current result
// If it is true it will flush the messages for all the results in the respnse being processed
void X_connection::flush_result(X_resultset *target, bool complete)
{
  Message *msg;
  int mid;
  bool done_flushing = false;
  try
  {
    do
    {
      msg = _protobuf->read_response(mid);

      if (msg && mid != Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
      {
        delete msg;
        msg = NULL;
      }

      if (mid == Mysqlx::ServerMessages::ERROR)
        _protobuf->handle_wrong_response(mid, msg, "flushing result");
      else
      {
        if (complete)
          done_flushing = (mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE ||
                           mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
        else
          done_flushing = (mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE ||
                           mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS ||
                           mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
      }
    } while (!done_flushing);

    // Updates the target based on the last message read
    if (mid != Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
      target->reset(-1, mid, msg);

    // If done reading finally reads the statement exec ok message to get the
    // result metadata
    if (mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE || mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
    {
      if (mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE)
        msg = _protobuf->read_response(mid);

      if (mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK)
      {
        target->set_result_metadata(msg);
        delete msg;
        msg = NULL;
      }
      else
        _protobuf->handle_wrong_response(mid, msg, "fetching result.");
    }
  }
  CATCH_AND_TRANSLATE();
}

bool X_connection::next_result(Base_resultset *target, bool first_result)
{
  bool ret_val = false;
  X_resultset *real_target = dynamic_cast<X_resultset *> (target);

  // Just to ensure the right instance was received as parameter
  if (real_target && real_target->has_resultset())
  {
    // The first call to next result is done to fetch the metadata
    // Subsequent calls to next_result need to ensure all the messages on the
    // tx buffer are cleared out first
    if (!first_result && !real_target->is_current_fetch_done())
      flush_result(real_target, false);

    if (!real_target->is_all_fetch_done())
    {
      ret_val = true;
      real_target->fetch_metadata();

      _timer.end();
      real_target->reset(_timer.raw_duration());
    }
  }

  return ret_val;
}

X_row::X_row(Mysqlx::Sql::Row *row, std::vector<Field>* metadata) : Base_row(metadata), _row(row)
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
    else if (field.scalar().has_v_string())
      return shcore::Value(field.scalar().v_string().value());
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

void X_row::add_field(shcore::Value value)
{
  Mysqlx::Datatypes::Any *field = _row->add_field();

  field->set_type(Mysqlx::Datatypes::Any::SCALAR);

  switch (value.type)
  {
    case Null:
    case Undefined:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_NULL);
      break;
    case Bool:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_BOOL);
      field->mutable_scalar()->set_v_bool(value.as_bool());
      break;
    case Float:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_DOUBLE);
      field->mutable_scalar()->set_v_double(value.as_double());
      break;
    case Integer:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_UINT);
      field->mutable_scalar()->set_v_unsigned_int(value.as_int());
      break;
    case String:
      field->mutable_scalar()->set_type(Mysqlx::Datatypes::Scalar::V_STRING);
      field->mutable_scalar()->mutable_v_string()->set_value(value.as_string());
      break;
    case Array:
      break;
    case Function:
      break;
    case Map:
      break;
    case MapRef:
      break;
    case Object:
      break;
  }
}

X_row::~X_row()
{
  if (_row)
    delete _row;
}

X_resultset::X_resultset(boost::shared_ptr<X_connection> owner, bool has_data, int cursor_id, uint64_t affected_rows, uint64_t last_insert_id, int warning_count, const char *info, int next_mid, Message* next_message, boost::shared_ptr<shcore::Value::Map_type> options) :
Base_resultset(owner, affected_rows, last_insert_id, warning_count, info, options), _xowner(owner), _cursor_id(cursor_id), _next_mid(next_mid), _next_message(next_message),
_current_fetch_done(!has_data), _all_fetch_done(!has_data)
{
  _has_resultset = has_data;
}

X_resultset::~X_resultset(){}

Base_row* X_resultset::next_row()
{
  Base_row* ret_val = NULL;

  if (has_resultset())
  {
    boost::shared_ptr<X_connection> owner = _xowner.lock();

    if (!is_current_fetch_done() && owner)
    {
      try
      {
        // Reads the next message
        // This variable must be cleaned out at the end
        _next_message = owner->get_protobuf()->read_response(_next_mid);

        if (_next_mid == Mysqlx::ServerMessages::SQL_ROW)
        {
          ret_val = new X_row(dynamic_cast<Mysqlx::Sql::Row *>(_next_message), &_metadata);
          ret_val->set_key_by_index(_key_by_index);

          // Each read row increases the count
          _fetched_row_count++;
        }
        else if (_next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE)
        {
          reset(-1, _next_mid, _next_message);
          owner->flush_result(this, true);
        }
        else if (_next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS)
          _current_fetch_done = true;
        else
          owner->get_protobuf()->handle_wrong_response(_next_mid, _next_message, "fetching result.");
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
    try
    {
      while (_next_mid == Mysqlx::ServerMessages::SQL_COLUMN_META_DATA)
      {
        Mysqlx::Sql::ColumnMetaData *meta = dynamic_cast<Mysqlx::Sql::ColumnMetaData *>(_next_message);

        _metadata.push_back(Field(meta->catalog(),
          meta->schema(),
          meta->table(),
          meta->original_table(),
          meta->name(),
          meta->original_name(),
          0, // length: not supported yet
          meta->type(),
          0, // flags: not supported yet
          meta->fractional_digits(),// decimals are now sent as fractional digits
          0)); // charset: not supported yet

        ret_val++;

        _next_message = owner->get_protobuf()->read_response(_next_mid);
      }

      // Validates exit in case of error...
      if (_next_mid == Mysqlx::ServerMessages::ERROR)
      {
        // Since metadata will be empty, it implies the result had no records
        Mysqlx::Error *error = dynamic_cast<Mysqlx::Error *>(_next_message);
        std::cout << "Error retrieving metadata: " << error->msg() << std::endl;
        ret_val = -1;
      }
    }
    CATCH_AND_TRANSLATE();
  }

  return ret_val;
}

void X_resultset::reset(unsigned long duration, int next_mid, ::google::protobuf::Message* next_message)
{
  if (next_mid)
  {
    _next_mid = next_mid;
    _next_message = next_message;

    // Nothing to be read anymnore on these cases
    _all_fetch_done = (_next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE);

    // Nothing to be read for the current result on these cases
    _current_fetch_done = (_next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE ||
                           _next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS);
  }

  if (duration != (unsigned long)-1)
    _raw_duration = duration;
}

void X_resultset::set_result_metadata(Message *msg)
{
  Mysqlx::Sql::StmtExecuteOk *stmt_exec_ok = dynamic_cast<Mysqlx::Sql::StmtExecuteOk*>(msg);

  if (stmt_exec_ok->has_rows_affected())
    _affected_rows = stmt_exec_ok->rows_affected();

  if (stmt_exec_ok->has_last_insert_id())
    _last_insert_id = stmt_exec_ok->last_insert_id();
}

Crud_definition::Crud_definition(const shcore::Argument_list &args)
{
  args.ensure_at_least(1, "Crud_definition");

  boost::shared_ptr<mysh::X_connection> connection = args[0].as_object<mysh::X_connection>();
  _conn = boost::weak_ptr<X_connection>(connection);
}

void Crud_definition::enable_function_after(const std::string& name, const std::string& after_functions)
{
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, after_functions, boost::is_any_of(", "), boost::token_compress_on);
  std::set<std::string> after(tokens.begin(), tokens.end());
  _enable_paths[name] = after;
}

void Crud_definition::update_functions(const std::string& source)
{
  std::map<std::string, bool>::iterator it, end = _enabled_functions.end();

  for (it = _enabled_functions.begin(); it != end; it++)
  {
    size_t count = _enable_paths[it->first].count(source);
    enable_method(it->first.c_str(), _enable_paths[it->first].count(source));
  }
}

/*
* Class constructor represents the call to the first method on the
* call chain, on this case insert.
* It will reveive not only the parameter documented on the insert function
* but also other initialization data for the object:
* - The connection represents the intermediate class in charge of actually
*   creating the message.
* - Message information that is not provided through the different functions
*/
TableInsert::TableInsert(const shcore::Argument_list &args) :
Crud_definition(args)
{
  args.ensure_count(3, "TableInsert");

  std::string path;
  _data.reset(new shcore::Value::Map_type());
  // TODO: Perhaps the data model can be received from the caller
  //       so this class can be used both for tables and collections
  (*_data)["data_model"] = Value(Mysqlx::Crud::TABLE);
  (*_data)["schema"] = args[1];
  (*_data)["collection"] = args[2];

  // The values function should not be enabled if values were already given
  add_method("insert", boost::bind(&TableInsert::insert, this, _1), "data");
  add_method("values", boost::bind(&TableInsert::values, this, _1), "data");
  add_method("bind", boost::bind(&TableInsert::bind, this, _1), "data");
  add_method("run", boost::bind(&TableInsert::run, this, _1), "data");

  // Initial function update
  enable_function_after("insert", "");
  enable_function_after("values", "insert, insertFields");
  enable_function_after("bind", "insert, insertFields, insertFieldsAndValues, values");
  enable_function_after("run", "insert, insertFields, insertFieldsAndValues, values, bind");

  update_functions("");
}

shcore::Value TableInsert::insert(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "TableInsert::insert");

  std::string path;
  if (args.size())
  {
    switch (args[0].type)
    {
      case Array:
        path = "Fields";
        break;
      case Map:
        path = "FieldsAndValues";
        break;
      default:
        throw shcore::Exception::argument_error("Invalid data received on TableInsert::insert");
    }

    // Stores the data
    (*_data)[path] = args[0];
  }

  // Updates the exposed functions
  update_functions("insert" + path);

  return Value(Object_bridge_ref(this));
}

shcore::Value TableInsert::values(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "TableInsert::values");

  // Adds the parameters to the data map
  (*_data)["Values"] = args[0];

  // Updates the exposed functions
  update_functions("values");

  // Returns the same object
  return Value(Object_bridge_ref(this));
}

shcore::Value TableInsert::bind(const shcore::Argument_list &args)
{
  // TODO: Logic to determine the kind of parameter passed
  //       Should end up adding one of the next to the data dictionary:
  //       - ValuesAndSubQueries
  //       - ParamsValuesAndSubQueries
  //       - IteratorObject

  // Updates the exposed functions
  update_functions("bind");

  return Value(Object_bridge_ref(this));
}

shcore::Value TableInsert::run(const shcore::Argument_list &args)
{
  // TODO: Callback handling logic
  shcore::Value ret_val;
  boost::shared_ptr<mysh::X_connection> connection(_conn.lock());

  if (connection)
  {
    ret_val = connection->table_insert(_data);
  }

  return ret_val;
}

boost::shared_ptr<shcore::Object_bridge> TableInsert::create(const shcore::Argument_list &args)
{
  args.ensure_count(3, 4, "TableInsert()");
  return boost::shared_ptr<shcore::Object_bridge>(new TableInsert(args));
}

#include "shellcore/object_factory.h"
namespace {
  static struct Auto_register {
    Auto_register()
    {
      shcore::Object_factory::register_factory("mysqlx", "Connection", &X_connection::create);
      shcore::Object_factory::register_factory("mysqlx", "TableInsert", &TableInsert::create);
    }
  } Mysqlx_register;
};
