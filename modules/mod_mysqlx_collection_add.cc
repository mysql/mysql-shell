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
#include "mod_mysqlx_collection_add.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "uuid_gen.h"
#include "mysqlxtest/common/expr_parser.h"
#include "mod_mysqlx_expression.h"

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
* The document parameter could be either a native representation of a document or an expression representing a document.
*
* Expressions are generated using the mysqlx.expr(expression) function.
*
* To create an expression that reoresents a document, the expression parameter must be a valid JSON string.
*
* The values on this JSON string could be literal values but it is possible to also use the functions available at the MySQL server.
*
* To be added, the document must have a property named '_id' with a universal unique identifier (UUID), if this property is missing, it is set with an auto generated UUID.
*
* This method can be called many times, every time it is called the received document will be cached into an internal list.
*
* The actual addition into the collection will occur only when the execute method is called.
*
* Example:
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getSession("myuser@localhost", mypwd);
*
* // creates a collection and adds a document into it
* var collection = mysession.sampledb.createCollection('sample');
* var result = collection.add({ name: 'jhon', last_name: 'doe'}).execute();
*
*
* // adds a document using an expression
* result = collection.add(mysqlx.expr('{"name":"jack", "register_date":current_date()}')).execute();
* \endcode
*/
CollectionAdd CollectionAdd::add(Document document){}

/**
* Adds a list of documents into a collection.
* \param documents A list of documents to be added into the collection.
* \return This CollectionAdd object.
*
* Each document on the list could be either a native representation of a document or an expression representing a document.
*
* Expressions are generated using the mysqlx.expr(expression) function.
*
* To create an expression that reoresents a document, the expression parameter must be a valid JSON string.
*
* The values on this JSON string could be literal values but it is possible to also use the functions available at the MySQL server.
* To be added, each document must have a property named '_id' with a universal unique identifier (UUID), if this property is missing, it is set with an auto generated UUID.
*
* This method can be called many times, every time it is called the received documents will be cached into an internal list.
*
* The actual addition into the collection will occur only when the execute method is called.
*
* Example:
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getSession("myuser@localhost", mypwd);
*
* // creates a collection and adds documents into it
* var collection = mysession.sampledb.createCollection('sample');
* var result = collection.add([{ name: 'john', last_name: 'doe'}, mysqlx.expr('{"name":"jane", "last_name":"doe"}')]).execute();
* \endcode
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
          size_t index, size = shell_docs->size();
          for (index = 0; index < size; index++)
          {
            ::mysqlx::Document inner_doc;
            Value element = shell_docs->at(index);
            if (element.type == Map)
            {
              Value::Map_type_ref shell_doc = element.as_map();

              if (!shell_doc->has_key("_id"))
                (*shell_doc)["_id"] = Value(get_new_uuid());

              // Stores the last document id
              _last_document_id = (*shell_doc)["_id"].as_string();

              inner_doc.reset(element.json());
            }
            else if (element.type == Object && element.as_object()->class_name() == "Expression")
            {
              boost::shared_ptr<mysqlx::Expression> expression = boost::static_pointer_cast<mysqlx::Expression>(element.as_object());
              ::mysqlx::Expr_parser parser(expression->get_data());
              std::auto_ptr<Mysqlx::Expr::Expr> expr_obj(parser.expr());

              // Parsing is done here to identify if a new ID must be generated for the object
              if (expr_obj->type() == Mysqlx::Expr::Expr_Type_OBJECT)
              {
                bool found = false;
                int size = expr_obj->object().fld_size();
                int index = 0;
                while (index < size && !found)
                  found = expr_obj->object().fld(index++).key() == "_id";

                std::string id;
                if (!found)
                  id = get_new_uuid();

                inner_doc.reset(expression->get_data(), true, id);
              }
              else
                throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a JSON expression") % (index + 1)).str());
            }
            else
              throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a document or a JSON expression") % (index + 1)).str());

            // We have a document so it gets added!
            if (!_add_statement.get())
            {
              _add_statement.reset(new ::mysqlx::AddStatement(collection->_collection_impl->add(inner_doc)));

              // Updates the exposed functions (since a document has been added)
              update_functions("add");
            }
            else
              _add_statement->add(inner_doc);
          }
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
* This function can be invoked once after:
* \sa add(Document document)
* \sa add(List documents)
*/
Result CollectionAdd::execute(){}
#endif
shcore::Value CollectionAdd::execute(const shcore::Argument_list &args)
{
  mysqlx::Result *result = NULL;

  try
  {
    args.ensure_count(0, "CollectionAdd.execute");

    result = new mysqlx::Result(boost::shared_ptr< ::mysqlx::Result>(_add_statement->execute()));

    result->set_last_document_id(_last_document_id);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionAdd.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}