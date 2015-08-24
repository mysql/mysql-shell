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

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Add a document to the collection. The document doc must have a property named '_id' with the universal unique identifier (uuid) that uniquely identifies the document.
* If the property is not there, it is added with an auto generated uuid.
* This method must be invoked before execute, add can be invoked as many times as required, after it, the following methods can be invoked: execute.
* \sa execute()
* \param document: the document to add
* \return the same instance of the collection the method was invoked with.
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getNodeSession("root:123@localhost:33060");
* // create some initial data
* var collection = mysession.js_shell_test.getCollection('collection1');
* var result = collection.add({ name: 'my first', passed: 'document', count: 1}).execute();
* var result = collection.add([{name: 'my second', passed: 'again', count: 2}, {name: 'my third', passed: 'once again', count: 3}]).execute();
* // check results
* var crud = collection.find();
* crud.execute();
* \endcode
*/
CollectionAdd CollectionAdd::add({ document })
{}

/**
* Adds a set of documents to the collection. Each document must have a property named '_id' with the universal unique identifier (uuid) that uniquely identifies the document.
* If the property is not there, it is added with an auto generated uuid.
* This method must be invoked before execute, add can be invoked as many times as required, after it, the following methods can be invoked: execute.
* \sa execute()
* \param {document, document, ...}: the document(s) to add
* \return the same instance of the collection the method was invoked with.
*/
CollectionAdd CollectionAdd::add({document, document, ...})
{}
#endif
shcore::Value CollectionAdd::add(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionAdd::add");

  boost::shared_ptr<DatabaseObject> raw_owner(_owner.lock());

  if (raw_owner)
  {
    boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(raw_owner));

    if (collection)
    {
      try
      {
        shcore::Value::Array_type_ref shell_docs;

        if (args[0].type == Map)
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
            Value element = shell_docs->at(index);
            if (element.type == Map)
            {
              Value::Map_type_ref shell_doc = element.as_map();

              if (!shell_doc->has_key("_id"))
                (*shell_doc)["_id"] = Value(get_new_uuid());

              //TODO: we are assumming that repr returns a valid JSON document
              //      we should introduce a routine that vensures that is correct.
              ::mysqlx::Document inner_doc(element.json());

              if (!_add_statement.get())
                _add_statement.reset(new ::mysqlx::AddStatement(collection->_collection_impl->add(inner_doc)));
              else
                _add_statement->add(inner_doc);
            }
            else
              throw shcore::Exception::argument_error((boost::format("Element #%1% is expected to be a document") % (index + 1)).str());
          }
        }
      }
      CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionAdd::add");
    }
  }

  // Updates the exposed functions
  update_functions("add");

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
* Execute the batch of insert document statements. Returning a result object.
* This method must be invoked after the method add has been invoked at least once.
* \sa add()
* \return the result of the batch insert.
*/
Result CollectionAdd::execute()
{}
#endif
shcore::Value CollectionAdd::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionAdd::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_add_statement->execute())));
}