/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "shell_client.h"

#include <boost/shared_ptr.hpp>
#include "modules/base_resultset.h"
#include "modules/mod_mysqlx_resultset.h"

void print_tab_value(const shcore::Value& val)
{
  switch (val.type)
  {
    case shcore::Integer:
      std::cout << val.as_int() << '\t';
      break;
    case shcore::String:
      std::cout << val.as_string() << '\t';
      break;
    case shcore::Bool:
      std::cout << val.as_bool() << '\t';
      break;
    case shcore::Float:
      std::cout << val.as_double() << '\t';
      break;
    default:
      std::cout << val.descr() << "\t";
  }
}

void print_doc_value(const shcore::Value& val)
{
  switch (val.type)
  {
    case shcore::Integer:
      std::cout << "\"" << val.as_int() << "\"";
      break;
    case shcore::String:
      std::cout << "\"" << val.as_string() << "\"";
      break;
    case shcore::Bool:
      std::cout << "\"" << val.as_bool() << "\"";
      break;
    case shcore::Float:
      std::cout << "\"" << val.as_double() << "\"";
      break;
    default:
      std::cout << "\"" << val.descr() << "\"";
  }
}

void print_base_result(const shcore::Value& value)
{
  boost::shared_ptr<mysh::mysqlx::BaseResult> result = boost::static_pointer_cast<mysh::mysqlx::BaseResult>(value.as_object());

  std::cout << "Execution Time: " << result->get_execution_time() << std::endl;
  std::cout << "Warning Count: " << result->get_warning_count() << std::endl;

  if (result->get_warning_count())
  {
    shcore::Value::Array_type_ref warnings = result->get_member("warnings").as_array();
    for (size_t index = 0; index < warnings->size(); index++)
    {
      boost::shared_ptr<mysh::Row> row = boost::static_pointer_cast<mysh::Row>(warnings->at(index).as_object());
      std::cout << row->get_member("Level").as_string() << "(" << row->get_member("Code").descr(true) << "): " << row->get_member("Message").descr(true) << std::endl;
    }
  }
}

void print_result(const shcore::Value& value)
{
  boost::shared_ptr<mysh::mysqlx::Result> result = boost::static_pointer_cast<mysh::mysqlx::Result>(value.as_object());

  std::cout << "Affected Items: " << result->get_affected_item_count() << std::endl;
  std::cout << "Last Insert Id: " << result->get_last_insert_id() << std::endl;
  std::cout << "Last Document Id: " << result->get_last_document_id() << std::endl;

  print_base_result(value);
}

void print_row_result(const shcore::Value& value)
{
  boost::shared_ptr<mysh::mysqlx::RowResult> result = boost::static_pointer_cast<mysh::mysqlx::RowResult>(value.as_object());

  // a table result set is an array of shcore::Value's each of them in turn is an array (vector) of shcore::Value's (escalars)
  std::vector<std::string> columns = result->get_column_names();

  std::cout << std::endl;
  for (size_t i = 0; i < columns.size(); ++i)
    std::cout << columns[i] << '\t';
  std::cout << std::endl;

  shcore::Argument_list args;
  shcore::Value raw_record;

  while (raw_record = result->fetch_one(args))
  {
    boost::shared_ptr<mysh::Row> record = boost::static_pointer_cast<mysh::Row>(raw_record.as_object());

    for (size_t i = 0; i < columns.size(); i++)
    {
      const shcore::Value& val = record->get_member(i);
      print_tab_value(val);
    }
    std::cout << std::endl;
  }

  print_base_result(value);
}

void print_sql_result(const shcore::Value& value)
{
  boost::shared_ptr<mysh::mysqlx::SqlResult> result = boost::static_pointer_cast<mysh::mysqlx::SqlResult>(value.as_object());

  if (result->has_data(shcore::Argument_list()))
    print_row_result(value);

  std::cout << "Affected Rows: " << result->get_affected_row_count() << std::endl;
  std::cout << "Last Insert Id: " << result->get_last_insert_id() << std::endl;
}

void print_doc_result(const shcore::Value& value)
{
  boost::shared_ptr<mysh::mysqlx::DocResult> result = boost::static_pointer_cast<mysh::mysqlx::DocResult>(value.as_object());

  shcore::Value document;
  shcore::Argument_list args;
  while (document = result->fetch_one(args))
  {
    std::cout << document.json(true) << std::endl;
  }

  print_base_result(value);
}

std::string get_query_from_prompt(const std::string prompt)
{
  std::cout << prompt;
  std::string input;
  if (std::getline(std::cin, input))
    return input;
  else
    return "";
}

// Sample usage model, override any of the methods print, print_error, input, password, source
class MyShell : public Shell_client
{
public:
  MyShell() { }
  virtual ~MyShell() { }
  virtual void print(const char *text) { std::cout << text << std::endl; }
  virtual void print_error(const char *text) { std::cerr << text << std::endl; }
};

int main(int argc, char* argv[])
{
  // ...
  MyShell shell;
  std::string prompt = "js> ";
  // CHANGEME: Set up connection string here as required
  shell.make_connection("root:toor@localhost:33060");
  // ... or to open an SSL connection
  //shell.make_connection("root:123@localhost:33060?ssl_ca=C:\\MySQL\\xplugin-16293675.mysql-advanced-5.7.9-winx64\\data\\ca.pem&ssl_cert=C:\\MySQL\\xplugin-16293675.mysql-advanced-5.7.9-winx64\\data\\server-cert.pem&ssl_key=C:\\MySQL\\xplugin-16293675.mysql-advanced-5.7.9-winx64\\data\\server-key.pem");

  std::cout << "Change mode entering either \\sql, \\js, \\py" << std::endl;

  // Run shell in SQL mode
  shell.switch_mode(shcore::IShell_core::Mode_JScript);
  std::string input;
  while (!(input = get_query_from_prompt(prompt)).empty())
  {
    shcore::Value result;
    if (input == "\\quit") break;
    else if (input == "\\sql")
    {
      prompt = "sql> ";
      shell.switch_mode(shcore::IShell_core::Mode_SQL);
      continue;
    }
    else if (input == "\\js")
    {
      prompt = "js> ";
      shell.switch_mode(shcore::IShell_core::Mode_JScript);
      continue;
    }
    else if (input == "\\py")
    {
      prompt = "py>";
      shell.switch_mode(shcore::IShell_core::Mode_Python);
      continue;
    }
    else
    {
      //result.reset(NULL);
      result = shell.execute(input);
    }

    if (result.type == shcore::Object)
    {
      std::string class_name = result.as_object()->class_name();
      if (class_name == "Result")
        print_result(result);
      else if (class_name == "DocResult")
        print_doc_result(result);
      else if (class_name == "RowResult")
        print_row_result(result);
      else if (class_name == "SqlResult")
        print_sql_result(result);
      else
        std::cout << result.descr(true) << std::endl;
    }
    else
      std::cout << result.descr(true) << std::endl;
  }
  return 0;
}