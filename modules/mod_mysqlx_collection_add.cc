/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "mod_mysqlx_collection_add.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "uuid_gen.h"
#include "mysqlxtest/common/expr_parser.h"
#include "mod_mysqlx_expression.h"
#include "utils/utils_time.h"

#include <iomanip>
#include <sstream>
#include <boost/format.hpp>

using namespace mysh::mysqlx;
using namespace shcore;

CollectionAdd::CollectionAdd(boost::shared_ptr<Collection> owner)
  :Collection_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("add", boost::bind(&CollectionAdd::add, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("add", ",add");
  register_dynamic_function("execute", "add");
  register_dynamic_function("__shell_hook__", "add");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Adds a document into a collection.
* \param document The document to be added into the collection.
* \return This CollectionAdd object.
*
* To be added, the document must have a property named '_id' with a universal unique identifier (UUID), if this property is missing, it is set with an auto generated UUID.
*
* #### Using Expressions for Documents
*
* The document parameter could be either a native representation of a document or an expression representing a document.
*
* To define an expression use:
* \code{.py}
* mysqlx.expr(expression)
* \endcode
*
* To create an expression that represents a document, the expression parameter must be a valid JSON string.
*
* The values on this JSON string could be literal values but it is possible to also use the functions available at the MySQL server.
*
* #### Method Chaining
*
* This method can be called many times, every time it is called the received document will be cached into an internal list.
*
* The actual addition into the collection will occur only when the execute method is called.
*
* \sa Usage examples at execute().
*/
CollectionAdd CollectionAdd::add(Document document){}

/**
* Adds a list of documents into a collection.
* \param documents A list of documents to be added into the collection.
* \return This CollectionAdd object.
*
* To be added, each document must have a property named '_id' with a universal unique identifier (UUID), if this property is missing, it is set with an auto generated UUID.
*
* #### Using Expressions for Documents
*
* Each document on the list could be either a native representation of a document or an expression representing a document.
*
* To define an expression use:
* \code{.py}
* mysqlx.expr(expression)
* \endcode
*
* To create an expression that represents a document, the expression parameter must be a valid JSON string.
*
* The values on this JSON string could be literal values but it is possible to also use the functions available at the MySQL server.
*
* #### Method Chaining
*
* This method can be called many times, every time it is called the received documents will be cached into an internal list.
*
* The actual addition into the collection will occur only when the execute method is called.
*
* \sa Usage examples at execute().
*/
CollectionAdd CollectionAdd::add(List documents){}
#endif
shcore::Value CollectionAdd::add(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionAdd.add");

  boost::shared_ptr<DatabaseObject> raw_owner(_owner.lock());

  if (raw_owner)
  {
    boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(raw_owner));

    if (collection)
    {
      try
      {
        shcore::Value::Array_type_ref shell_docs;

        if (args[0].type == Map || (args[0].type == Object && args[0].as_object()->class_name() == "Expression"))
        {
          // On a single document parameter, creates an array and processes it as a list of
          // documents, only advantage of this is avoid duplicating validation and setup logic
          shell_docs.reset(new Value::Array_type());
          shell_docs->push_back(args[0]);
        }
        else if (args[0].type == Array)
          shell_docs = args[0].as_array();
        else
          throw shcore::Exception::argument_error("Argument is expected to be either a document or a list of documents");

        if (shell_docs)
        {
          if (!_add_statement.get())
            _add_statement.reset(new ::mysqlx::AddStatement(collection->_collection_impl));

          size_t index, size = shell_docs->size();
          for (index = 0; index < size; index++)
          {
            Value element = shell_docs->at(index);

            ::mysqlx::Document inner_doc;
            Value::Map_type_ref shell_doc;

            // Validation of the incoming parameter
            if (element.type == Map)
               shell_doc = element.as_map();
            else if (element.type == Object && element.as_object()->class_name() == "Expression")
            {
              boost::shared_ptr<mysqlx::Expression> expression = boost::static_pointer_cast<mysqlx::Expression>(element.as_object());
              shcore::Value document = shcore::Value::parse(expression->get_data());
              if (document.type == Map)
                shell_doc = document.as_map();
              else
                throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a JSON expression") % (index + 1)).str());
            }
            else
              throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a document or a JSON expression") % (index + 1)).str());

            // Verification of the _id existence
            if (shell_doc)
            {
              if (!shell_doc->has_key("_id"))
                (*shell_doc)["_id"] = Value(get_new_uuid());

              // No matter how the document was received, gets passed as expression to the
              // backend
              inner_doc.reset(shcore::Value(shell_doc).json(), true, (*shell_doc)["_id"].as_string());
            }

            // We have a document so it gets added!
            _add_statement->add(inner_doc);
          }

          // Updates the exposed functions (since a document has been added)
          update_functions("add");
        }
      }
      CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionAdd.add");
    }
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

std::string CollectionAdd::get_new_uuid()
{
  uuid_type uuid;
  generate_uuid(uuid);

  std::stringstream str;
  str << std::hex << std::noshowbase << std::setfill('0') << std::setw(2);
  //*
  str << (int)uuid[0] << std::setw(2) << (int)uuid[1] << std::setw(2) << (int)uuid[2] << std::setw(2) << (int)uuid[3];
  str << std::setw(2) << (int)uuid[4] << std::setw(2) << (int)uuid[5];
  str << std::setw(2) << (int)uuid[6] << std::setw(2) << (int)uuid[7];
  str << std::setw(2) << (int)uuid[8] << std::setw(2) << (int)uuid[9];
  str << std::setw(2) << (int)uuid[10] << std::setw(2) << (int)uuid[11]
    << std::setw(2) << (int)uuid[12] << std::setw(2) << (int)uuid[13]
    << std::setw(2) << (int)uuid[14] << std::setw(2) << (int)uuid[15];

  return str.str();
}

#ifdef DOXYGEN
/**
* Executes the document addition for the documents cached on this object.
* \return A Result object.
*
* #### Method Chaining
*
* This function can be invoked once after:
*
* - add(Document document)
* - add(List documents)
*
* #### JavaScript Examples
*
* \dontinclude "js_devapi/scripts/mysqlx_collection_add.js"
* \skip //@ Collection.add execution
* \until print("Affected Rows Mixed List:", result.affectedItemCount, "\n")
*
* #### Python Examples
*
* \dontinclude "py_devapi/scripts/mysqlx_collection_add.py"
* \skip #@ Collection.add execution
* \until print "Affected Rows Mixed List:", result.affectedItemCount, "\n"
*/
Result CollectionAdd::execute(){}
#endif
shcore::Value CollectionAdd::execute(const shcore::Argument_list &args)
{
  mysqlx::Result *result = NULL;

  try
  {
    args.ensure_count(0, "CollectionAdd.execute");

    MySQL_timer timer;
    timer.start();
    result = new mysqlx::Result(boost::shared_ptr< ::mysqlx::Result>(_add_statement->execute()));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionAdd.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}