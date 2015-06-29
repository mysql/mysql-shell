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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <iomanip>
#include "mod_mysql_resultset.h"
#include "mysql.h"

using namespace mysh;
using namespace shcore;
using namespace mysh::mysql;

Resultset::Resultset(boost::shared_ptr< ::mysql::Result> result)
: _result(result)
{
}

shcore::Value Resultset::next(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::next");
  Row *inner_row = _result->next();

  if (inner_row)
  {
    mysh::Row *value_row = new mysh::Row();

    std::vector<Field> metadata(_result->get_metadata());

    for (size_t index = 0; index < metadata.size(); index++)
      value_row->add_item(metadata[index].name(), inner_row->get_value(index));

    return shcore::Value::wrap(value_row);
  }

  return shcore::Value::Null();
}

shcore::Value Resultset::next_result(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::nextResult");

  return shcore::Value(_result->next_result());
}

shcore::Value Resultset::all(const shcore::Argument_list &args)
{
  args.ensure_count(0, "Resultset::all");

  boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

  shcore::Value record = next(args);

  while (record)
  {
    array->push_back(record);
    record = next(args);
  }

  return shcore::Value(array);
}

shcore::Value Resultset::get_member(const std::string &prop) const
{
  if (prop == "fetchedRowCount")
    return shcore::Value((int64_t)_result->fetched_row_count());

  if (prop == "affectedRows")
    return shcore::Value((int64_t)((_result->affected_rows() == ~(my_ulonglong)0) ? 0 : _result->affected_rows()));

  if (prop == "warningCount")
    return shcore::Value(_result->warning_count());

  if (prop == "warnings")
  {
    Result* inner_warnings = _result->query_warnings();
    boost::shared_ptr<Resultset> warnings(new Resultset(boost::shared_ptr<Result>(inner_warnings)));
    return warnings->all(shcore::Argument_list());
  }

  if (prop == "info")
    return shcore::Value(_result->info());

  if (prop == "executionTime")
    return shcore::Value(MySQL_timer::format_legacy(_result->execution_time(), true));

  if (prop == "lastInsertId")
    return shcore::Value((int)_result->last_insert_id());

  if (prop == "info")
    return shcore::Value(_result->info());

  if (prop == "hasData")
    return Value(_result->has_resultset());

  if (prop == "columnMetadata")
  {
    std::vector<Field> metadata(_result->get_metadata());

    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    int num_fields = metadata.size();

    for (int i = 0; i < num_fields; i++)
    {
      boost::shared_ptr<shcore::Value::Map_type> map(new shcore::Value::Map_type);

      (*map)["catalog"] = shcore::Value(metadata[i].catalog());
      (*map)["db"] = shcore::Value(metadata[i].db());
      (*map)["table"] = shcore::Value(metadata[i].table());
      (*map)["org_table"] = shcore::Value(metadata[i].org_table());
      (*map)["name"] = shcore::Value(metadata[i].name());
      (*map)["org_name"] = shcore::Value(metadata[i].org_name());
      (*map)["charset"] = shcore::Value(int(metadata[i].charset()));
      (*map)["length"] = shcore::Value(int(metadata[i].length()));
      (*map)["type"] = shcore::Value(int(metadata[i].type()));
      (*map)["flags"] = shcore::Value(int(metadata[i].flags()));
      (*map)["decimal"] = shcore::Value(int(metadata[i].decimals()));
      (*map)["max_length"] = shcore::Value(int(metadata[i].max_length()));
      (*map)["name_length"] = shcore::Value(int(metadata[i].name_length()));

      // Temporal hack to identify numeric values
      (*map)["is_numeric"] = shcore::Value(IS_NUM(metadata[i].type()));

      array->push_back(shcore::Value(map));
    }

    return shcore::Value(array);
  }

  return BaseResultset::get_member(prop);
}

#if 0
// Used to consume all the remaining messages of the current resultset that are coming from the
// server.
// If complete false it will only flush the messages about the current result in the resultset
// If it is true it will flush all the messages for the result set
void Resultset::flush_messages(bool complete)
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

int Resultset::fetch_metadata()
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

void Resultset::reset(unsigned long duration, int next_mid, ::google::protobuf::Message* next_message)
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
                           _next_mid == Mysqlx::ServerMessages::SQL_CURSOR_FETCH_DONE_MORE_RESULTSETS ||
                           _all_fetch_done);
  }

  if (duration != (unsigned long)-1)
    _raw_duration = duration;
}

void Resultset::set_result_metadata(Message *msg)
{
  Mysqlx::Sql::StmtExecuteOk *stmt_exec_ok = dynamic_cast<Mysqlx::Sql::StmtExecuteOk*>(msg);

  if (stmt_exec_ok)
  {
    if (stmt_exec_ok->has_rows_affected())
      _affected_rows = stmt_exec_ok->rows_affected();

    if (stmt_exec_ok->has_last_insert_id())
      _last_insert_id = stmt_exec_ok->last_insert_id();
  }
  else
    throw shcore::Exception::logic_error("Unexpected message to set resultset metadata.");
}
#endif