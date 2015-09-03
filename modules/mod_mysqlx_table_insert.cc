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
#include <boost/bind.hpp>
#include "mod_mysqlx_table_insert.h"
#include "mod_mysqlx_table.h"
#include "shellcore/common.h"
#include "mod_mysqlx_resultset.h"
#include <boost/format.hpp>

using namespace mysh::mysqlx;
using namespace shcore;

/*
* Class constructor represents the call to the first method on the
* call chain, on this case insert.
* It will reveive not only the parameter documented on the insert function
* but also other initialization data for the object:
* - The connection represents the intermediate class in charge of actually
*   creating the message.
* - Message information that is not provided through the different functions
*/
TableInsert::TableInsert(boost::shared_ptr<Table> owner)
:Table_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // The values function should not be enabled if values were already given
  add_method("insert", boost::bind(&TableInsert::insert, this, _1), "data");
  add_method("values", boost::bind(&TableInsert::values, this, _1), "data");
  add_method("bind", boost::bind(&TableInsert::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("insert", "");
  register_dynamic_function("values", "insert, insertFields, values");
  register_dynamic_function("bind", "insertFieldsAndValues, values");
  register_dynamic_function("execute", "insertFieldsAndValues, values, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Initializes the record insertion handler.
* \return This TableInsert object.
*
* This function is called automatically when Table.insert() is called.
*
* After this function invocation, the following functions can be invoked:
*
* - values(Value value1, Value value2, ...)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableInsert TableInsert::insert(){}

/**
* Initializes the record insertion handler with the received column list.
* \return This TableInsert object.
*
* This function is called automatically when Table.insert(List columns) is called.
*
* After this function invocation, the following functions can be invoked:
*
* - values(Value value1, Value value2, ...)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableInsert TableInsert::insert(List columns){}

/**
* Initializes the record insertion handler with the received column list.
* \param col1 The first column name.
* \param col2 The second column name.
* \return This TableInsert object.
*
* This function is called automatically when Table.insert(String col1, String col2, ...) is called.
*
* A string parameter should be specified for each column to be included on the insertion process.
*
* After this function invocation, the following functions can be invoked:
*
* - values(Value value1, Value value2, ...)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
TableInsert TableInsert::insert(String col1, String col2, ...){}
#endif
shcore::Value TableInsert::insert(const shcore::Argument_list &args)
{
  boost::shared_ptr<Table> table(boost::static_pointer_cast<Table>(_owner.lock()));

  std::string path;

  if (table)
  {
    try
    {
      _insert_statement.reset(new ::mysqlx::InsertStatement(table->_table_impl->insert()));

      if (args.size())
      {
        shcore::Value::Map_type_ref sh_columns_and_values;

        std::vector < std::string > columns;

        // An array with the fields was received as parameter
        if (args.size() == 1 && args[0].type == Array)
        {
          path = "Fields";

          parse_string_list(args, columns);

          _insert_statement->insert(columns);
        }

        // A map with fields and values was received as parameter
        else if (args.size() == 1 && args[0].type == Map)
        {
          std::vector < ::mysqlx::TableValue > values;
          path = "FieldsAndValues";
          sh_columns_and_values = args[0].as_map();
          shcore::Value::Map_type::iterator index = sh_columns_and_values->begin(),
                                            end = sh_columns_and_values->end();

          for (; index != end; index++)
          {
            columns.push_back(index->first);
            values.push_back(map_table_value(index->second));
          }

          _insert_statement->insert(columns);
          _insert_statement->values(values);
        }

        // Each parameter is a field
        else
        {
          for (unsigned int index = 0; index < args.size(); index++)
          {
            if (args[index].type == shcore::String)
              columns.push_back(args.string_at(index));
            else
            {
              std::string error;

              if (args.size() == 1)
                error = (boost::format("Argument #%1% is expected to be either string, a list of strings or a map with fields and values") % (index + 1)).str();
              else
                error = (boost::format("Argument #%1% is expected to be a string") % (index + 1)).str();

              throw shcore::Exception::type_error(error);
            }
          }

          _insert_statement->insert(columns);

          path = "Fields";
        }
      }
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableInsert.insert");
  }

  // Updates the exposed functions
  update_functions("insert" + path);

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the values for a row to be inserted.
* \param value1 The value for the first column.
* \param value2 The value for the second column.
* \return This TableInsert object.
*
* Each column value comes as a parameter on this function call.
*
* The number of parameters must match the length of the column list defined on the called insert function.
* If no column list was defined the number of parameters must match the number of columns defined on the Table where the records will be inserted.
*
* The values must be positioned on the list in a way they match the column list defined on the called insert function.
* If no column list was defined the fields must match the column definition of the Table where the records will be inserted.
*
* This function can be invoked multiple times after:
* - insert()
* - insert(List columns)
* - insert(String col1, String col2, ...)
* - values(Value value1, Value value2, ...)
*
* After this function invocation, the following functions can be invoked:
*
* - execute(ExecuteOptions options).
*/
TableInsert TableInsert::values(Value value1, Value value2, ...){}
#endif
shcore::Value TableInsert::values(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_at_least(1, "TableInsert.values");

  try
  {
    std::vector < ::mysqlx::TableValue > values;

    for (size_t index = 0; index < args.size(); index++)
      values.push_back(map_table_value(args[index]));

    _insert_statement->values(values);

    // Updates the exposed functions
    update_functions("values");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("TableInsert.values");

  // Returns the same object
  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
TableInsert TableInsert::bind({ var:val, var : val, ... }){}
#endif
shcore::Value TableInsert::bind(const shcore::Argument_list &UNUSED(args))
{
  // TODO: Logic to determine the kind of parameter passed
  //       Should end up adding one of the next to the data dictionary:
  //       - ValuesAndSubQueries
  //       - ParamsValuesAndSubQueries
  //       - IteratorObject

  // Updates the exposed functions
  update_functions("bind");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the record insertion.
* \return Resultset A resultset object that can be used to retrieve the results of the insertion operation.
*
* This function can be invoked after:
* - values(Value value1, Value value2, ...)
*
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getNodeSession("myuser@localhost", mypwd);
*
* // Creates a table named friends on the test schema
* mysession.sql('create table test.friends (name varchar(50), age integer, gender varchar(20));').execute();
*
* var table = mysession.test.friends;
*
* // create some initial data
* table.insert('name','last_name','age','gender')
*      .values('jack','black', 17, 'male')
*      .values('adam', 'sandler', 15, 'male')
*      .values('brian', 'adams', 14, 'male')
*      .values('alma', 'lopez', 13, 'female').execute();

* table.insert(['name','last_name','age','gender'])
*      .values('carol', 'shiffield', 14, 'female')
*      .values('donna', 'summers', 16, 'female')
*      .values('angel', 'down', 14, 'male').execute();
*
* \endcode
*/
Resultset TableInsert::execute(ExecuteOptions options){}
#endif
shcore::Value TableInsert::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "TableInsert.execute");

  return shcore::Value::wrap(new mysqlx::Resultset(boost::shared_ptr< ::mysqlx::Result>(_insert_statement->execute())));
}