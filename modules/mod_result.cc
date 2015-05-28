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

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <iostream>
#include <iomanip>
#include "mod_result.h"

using namespace mysh;
using namespace shcore;

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

X_resultset::X_resultset(boost::shared_ptr<mysh::X_connection> owner,
                         bool has_data,
                         int cursor_id,
                         uint64_t affected_rows,
                         uint64_t last_insert_id,
                         int warning_count,
                         const char *info,
                         int next_mid,
                         Message* next_message,
                         bool expect_metadata,
                         boost::shared_ptr<shcore::Value::Map_type> options) :
Base_resultset(owner, affected_rows, last_insert_id, warning_count, info, options),
  _xowner(owner),
  _cursor_id(cursor_id),
  _next_mid(next_mid),
  _next_message(next_message),
  _current_fetch_done(!has_data),
  _all_fetch_done(!has_data),
  _expect_metadata(expect_metadata)
{
  _has_resultset = has_data;
}

X_resultset::~X_resultset(){}

Base_row* X_resultset::next_row()
{
  Base_row* ret_val = NULL;

  if (has_resultset())
  {
    boost::shared_ptr<mysh::X_connection> owner = _xowner.lock();

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
          flush_messages(true);
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

// Used to consume all the remaining messages of the current resultset that are coming from the
// server.
// If complete false it will only flush the messages about the current result in the resultset
// If it is true it will flush all the messages for the result set
void X_resultset::flush_messages(bool complete)
{
  Message *msg;
  int mid;
  bool done_flushing = false;
  try
  {
    boost::shared_ptr<mysh::X_connection> owner = _xowner.lock();
    if (owner)
    {
      do
      {
        // Handles any error properly
        if (_next_mid == Mysqlx::ServerMessages::ERROR)
          owner->get_protobuf()->handle_wrong_response(mid, msg, "flushing result");
        else
        {
          // Complete flush ends on fetch done or stmt execute ok
          // note that stmt execute ok is only received by results
          // with resultset metadata, so is not always present.
          if (complete)
          {
            if (_expect_metadata)
              done_flushing = _next_mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK;
            else
              done_flushing = _next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE;

            _all_fetch_done = done_flushing;
            _current_fetch_done = done_flushing;
          }
          else
          {
            done_flushing = (_next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE ||
                             _next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS ||
                             _next_mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);

            // On simple resultset request, if this is the last resultset then we need to continue reading if
            // metadata is expected
            if (_next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE && _expect_metadata)
              done_flushing = false;

            _current_fetch_done = done_flushing;
          }

          reset(-1, _next_mid, _next_message);

          if (done_flushing && _expect_metadata)
            set_result_metadata(_next_message);

          delete _next_message;
          _next_message = NULL;

          if (!done_flushing)
            _next_message = owner->get_protobuf()->read_response(_next_mid);
        }
      } while (!done_flushing);
    }
  }
  CATCH_AND_TRANSLATE();
}

int X_resultset::fetch_metadata()
{
  // Fetch the metadata
  int ret_val = 0;
  boost::shared_ptr<mysh::X_connection> owner = _xowner.lock();

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
    if (_expect_metadata)
      _all_fetch_done = (_next_mid == Mysqlx::ServerMessages::SQL_STMT_EXECUTE_OK);
    else
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

Collection_resultset::Collection_resultset(boost::shared_ptr<X_connection> owner,
                     bool has_data,
                     int cursor_id,
                     uint64_t affected_rows,
                     uint64_t last_insert_id,
                     int warning_count,
                     const char *info,
                     int next_mid,
                     Message* next_message,
                     bool expect_metadata,
                     boost::shared_ptr<shcore::Value::Map_type> options) :
                     X_resultset(owner, has_data, cursor_id, affected_rows, last_insert_id, warning_count,
                     info, next_mid, next_message, expect_metadata, options)
{
}

shcore::Value Collection_resultset::next(const shcore::Argument_list &args)
{
  Value ret_val = Value::Null();

  std::string function = class_name() + "::next";
  bool raw = false;

  args.ensure_count(0, function.c_str());

  // Gets the next row
  std::auto_ptr<Base_row> row(next_row());

  if (row.get())
  {
    // Now parses the document and returns it
    Value document = row->get_value(0);

    ret_val = Value::parse(document.as_string());
  }

  return ret_val;
}

shcore::Value Collection_resultset::fetch_all(const shcore::Argument_list &args)
{
  Value::Array_type_ref array(new Value::Array_type());

  std::string function = class_name() + "::all";
  bool raw = false;

  args.ensure_count(0, function.c_str());

  // Gets the next row
  Value record = next(args);
  while (record)
  {
    array->push_back(record);
    record = next(args);
  }

  return Value(array);
}