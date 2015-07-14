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

#include "simple_shell_client.h"

#include <boost/shared_ptr.hpp>
#include "modules/base_resultset.h"


void print_table_result_set(Table_result_set* tbl)
{
  // a table result set is an array of shcore::Value's each of them in turn is an array (vector) of shcore::Value's (escalars)
  std::vector<Result_set_metadata> *metadata = tbl->get_metadata().get();
  std::vector<shcore::Value> *dataset = tbl->get_data().get();

  std::cout << std::endl;
  for (int i = 0; i < metadata->size(); ++i)
  {
    std::cout << (*metadata)[i].get_name() << '\t';
  }
  std::cout << std::endl;

  for (int i = 0; i < dataset->size(); ++i)
  {
    shcore::Value& v_row = (*dataset)[i];
    boost::shared_ptr<mysh::Row> row = v_row.as_object<mysh::Row>();   
    for (size_t i = 0; i < metadata->size(); i++)
    {
      shcore::Value& val = row->values[i];
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
    std::cout << std::endl;
  }
  std::cout << std::endl << std::endl;
  std::cout << "warning count: " << tbl->get_warning_count() << std::endl;
  std::cout << "affected rows: " << tbl->get_affected_rows() << std::endl;
  std::cout << "execution time: " << tbl->get_execution_time() << std::endl;
  std::cout << std::endl << std::endl;
}

void print_doc_result_set(Document_result_set *doc)
{
  boost::shared_ptr<std::vector<shcore::Value> > data = doc->get_data();
  std::vector<shcore::Value>::const_iterator myend = data->end();
  // a document is an array of objects of type mysh::Row
  int i = 0;
  std::cout << "{";
  for (std::vector<shcore::Value>::const_iterator it = data->begin(); it != myend; ++it)
  {
if (i++ != 0)
      std::cout << ",\n";
    boost::shared_ptr<mysh::Row> row = it->as_object<mysh::Row>();
    row->values.size();
    std::cout << "[" << std::endl;
    std::map<std::string, int>::const_iterator it2, myend = row->keys.end();
    size_t j = 0;
    for (it2 = row->keys.begin(); it2 != myend; ++it2)
    {
      std::cout << "\t\"" << it2->first << " : ";
shcore::Value& val = row->values[it2->second];
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
      if (++j >= row->keys.size())
        std::cout << std::endl;
      else
        std::cout << "," << std::endl;
    }
    std::cout << "]";
  }
  std::cout << "}" << std::endl;
}


std::string get_query_from_prompt(const std::string prompt)
{
  std::cout << prompt;
  std::string input;
  if(std::getline(std::cin, input))
    return input;
  else
    return "";
}

// Sample usage model, override any of the methods print, print_error, input, password, source
class MyShell : public Simple_shell_client
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
  // CHANGEME: Set up connection string here as required
  shell.make_connection("root:123@localhost:3305");

  bool empty_result = false;
  // Run shell in SQL mode
  shell.switch_mode(shcore::IShell_core::Mode_SQL);
  std::string input;
  while (!(input = get_query_from_prompt("sql> ")).empty())
  {
    if (input == "\\quit") break;
    boost::shared_ptr<Result_set> result = shell.execute(input);
    Result_set* res = result.get();
    Table_result_set* tbl = dynamic_cast<Table_result_set*>(res);

    if (tbl != NULL)
    {
      print_table_result_set(tbl);
    }
    else
    {
      // Empty result...
      empty_result = true;
    }
  }

  // Run shell in JS mode
  shell.switch_mode(shcore::IShell_core::Mode_JScript);
  while (!(input = get_query_from_prompt("js> ")).empty())
  {
    if (input == "\\quit") break;
    boost::shared_ptr<Result_set> result = shell.execute(input);
    Result_set* res = result.get();
    Document_result_set *doc = dynamic_cast<Document_result_set*>(res);

    if (doc != NULL)
    {
      print_doc_result_set(doc);
    }
    else
    {
      // Empty result...
      empty_result = true;
    }
  }

  shell.switch_mode(shcore::IShell_core::Mode_Python);
  while (!(input = get_query_from_prompt("py> ")).empty())
  {
    if (input == "\\quit") break;
    boost::shared_ptr<Result_set> result = shell.execute(input);
    Result_set* res = result.get();
    Document_result_set *doc = dynamic_cast<Document_result_set*>(res);

    if (doc != NULL)
    {
      print_doc_result_set(doc);
    }
    else
    {
      // Empty result...
      empty_result = true;
    }
  }
	return 0;
}

