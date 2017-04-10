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
#include "mod_mysqlx_collection_add.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "uuid_gen.h"
#include "mysqlxtest/common/expr_parser.h"
#include "mod_mysqlx_expression.h"
#include "utils/utils_time.h"
#include "utils/utils_string.h"
#include "shellcore/utils_help.h"

#include <iomanip>
#include <sstream>
#include <boost/random.hpp>

struct Init_uuid_gen {
  Init_uuid_gen() {
    // Use a random seed for UUIDs
    std::time_t now = std::time(NULL);
    boost::uniform_int<> dist(INT_MIN, INT_MAX);
    boost::mt19937 gen;
    boost::variate_generator<boost::mt19937 &, boost::uniform_int<> > vargen(gen, dist);
    gen.seed(now);
    init_uuid(vargen());
  }
};
static Init_uuid_gen __init_uuid;


using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

CollectionAdd::CollectionAdd(std::shared_ptr<Collection> owner)
  :Collection_crud_definition(std::static_pointer_cast<DatabaseObject>(owner)) {
  // Exposes the methods available for chaining
  add_method("add", std::bind(&CollectionAdd::add, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("add", ",add");
  register_dynamic_function("execute", "add");
  register_dynamic_function("__shell_hook__", "add");

  // Initial function update
  update_functions("");
}

REGISTER_HELP(COLLECTIONADD_ADD_BRIEF, "Adds documents into a collection.");
REGISTER_HELP(COLLECTIONADD_ADD_SYNTAX, "add(document[, document, ...])[.add(...)]");
REGISTER_HELP(COLLECTIONADD_ADD_SYNTAX1, "add(documentList)[.add(...)]");
REGISTER_HELP(COLLECTIONADD_ADD_RETURNS, "@return This CollectionAdd object.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL, "This function receives one or more document definitions to be added into a collection.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL1, "A document definition may be provided in two ways:");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL2, "@li Using a dictionary containing the document fields.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL3, "@li Using A JSON string as a document expression.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL4, "There are three ways to add multiple documents:");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL5, "@li Passing several parameters to the function, each parameter should be a document definition.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL6, "@li Passing a list of document definitions.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL7, "@li Calling this function several times before calling execute().");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL8, "To be added, every document must have a string property named '_id' ideally with a universal unique identifier (UUID) as value. "\
"If the '_id' property is missing, it is automatically set with an internally generated UUID.");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL9, "<b>JSON as Document Expressions</b>");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL10, "A document can be represented as a JSON expression as follows:");
REGISTER_HELP(COLLECTIONADD_ADD_DETAIL11, "mysqlx.expr(<JSON String>)");

/**
* $(COLLECTIONADD_ADD_BRIEF)
*
* #### Parameters
*
* @li \b document The definition of a document to be added.
* @li \b documents A list of documents to be added.
*
* $(COLLECTIONADD_ADD_RETURNS)
*
* $(COLLECTIONADD_ADD_DETAIL)
*
* $(COLLECTIONADD_ADD_DETAIL1)
*
* $(COLLECTIONADD_ADD_DETAIL2)
* $(COLLECTIONADD_ADD_DETAIL3)
*
* $(COLLECTIONADD_ADD_DETAIL4)
*
* $(COLLECTIONADD_ADD_DETAIL5)
* $(COLLECTIONADD_ADD_DETAIL6)
* $(COLLECTIONADD_ADD_DETAIL7)
*
* $(COLLECTIONADD_ADD_DETAIL8)
*
* #### Method Chaining
*
* This method can be called many times, every time it is called the received document(s) will be cached into an internal list.
* The actual addition into the collection will occur only when the execute method is called.
*/
//@{
#if DOXYGEN_JS
CollectionAdd CollectionAdd::add(DocDefinition document[, DocDefinition document, ...]) {}
CollectionAdd CollectionAdd::add(List documents) {}
#elif DOXYGEN_PY
CollectionAdd CollectionAdd::add(DocDefinition document[, DocDefinition document, ...]) {}
CollectionAdd CollectionAdd::add(list documents) {}
#endif
//@}
shcore::Value CollectionAdd::add(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_at_least(1, get_function_name("add").c_str());

  std::shared_ptr<DatabaseObject> raw_owner(_owner.lock());

  if (raw_owner) {
    std::shared_ptr<Collection> collection(std::static_pointer_cast<Collection>(raw_owner));

    if (collection) {
      try {
        shcore::Value::Array_type_ref shell_docs;
        std::string error_prefix = "Argument";
        if (args.size() == 1) {
          if (args[0].type == Map || (args[0].type == Object && args[0].as_object()->class_name() == "Expression")) {
            // On a single document parameter, creates an array and processes it as a list of
            // documents, only advantage of this is avoid duplicating validation and setup logic
            shell_docs.reset(new Value::Array_type());
            shell_docs->push_back(args[0]);
          } else if (args[0].type == Array) {
            shell_docs = args[0].as_array();
            error_prefix = "Element";
          } else
            throw shcore::Exception::argument_error("Argument is expected to be either a document or a list of documents");
        } else {
          // Individual document parameters support
          shell_docs.reset(new Value::Array_type());
          for (auto document : args)
            shell_docs->push_back(document);
        }

        if (shell_docs) {
          if (!_add_statement.get())
            _add_statement.reset(new ::mysqlx::AddStatement(collection->_collection_impl));

          size_t index, size = shell_docs->size();
          for (index = 0; index < size; index++) {
            Value element = shell_docs->at(index);

            ::mysqlx::Document inner_doc;
            Value::Map_type_ref shell_doc;

            // Validation of the incoming parameter
            if (element.type == Map)
               shell_doc = element.as_map();
            else if (element.type == Object && element.as_object()->class_name() == "Expression") {
              std::shared_ptr<mysqlx::Expression> expression = std::static_pointer_cast<mysqlx::Expression>(element.as_object());
              shcore::Value document = shcore::Value::parse(expression->get_data());
              if (document.type == Map)
                shell_doc = document.as_map();
              else
                throw shcore::Exception::argument_error(str_format("%s #%zu is expected to be a JSON expression", error_prefix.c_str(), (index + 1)));
            } else
              throw shcore::Exception::argument_error(str_format("%s #%zu is expected to be a document or a JSON expression", error_prefix.c_str(), (index + 1)));

            // Verification of the _id existence
            if (shell_doc) {
              if (!shell_doc->has_key("_id"))
                (*shell_doc)["_id"] = Value(get_new_uuid());
              else if ((*shell_doc)["_id"].type != shcore::String)
                throw shcore::Exception::argument_error("Invalid data type for _id field, should be a string");

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
      CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("add").c_str());
    }
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

std::string CollectionAdd::get_new_uuid() {
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

REGISTER_HELP(COLLECTIONADD_EXECUTE_BRIEF, "Executes the add operation, the documents are added to the target collection.");
REGISTER_HELP(COLLECTIONADD_EXECUTE_RETURN, "@return A Result object.");
REGISTER_HELP(COLLECTIONADD_EXECUTE_SYNTAX, "execute()");

/**
* $(COLLECTIONADD_EXECUTE_BRIEF)
*
* $(COLLECTIONADD_EXECUTE_RETURN)
*
* #### Method Chaining
*
* This function can be invoked once after:
*
* - add(DocDefinition document[, DocDefinition document, ...])
*
* ### Examples
*/
//@{
#if DOXYGEN_JS
/**
* #### Using a Document List
* Adding document using an existing document list
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Document List
*
* #### Multiple Parameters
* Adding document using a separate parameter for each document on a single call to add(...)
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Multiple Parameters
*
* #### Chaining Addition
* Adding documents using chained calls to add(...)
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Chained Calls
*
* $(COLLECTIONADD_ADD_DETAIL9)
*
* $(COLLECTIONADD_ADD_DETAIL10)
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Using an Expression
*/
Result CollectionAdd::execute() {}
#elif DOXYGEN_PY
/**
* #### Using a Document List
* Adding document using an existing document list
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Document List
*
* #### Multiple Parameters
* Adding document using a separate parameter for each document on a single call to add(...)
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Multiple Parameters
*
* #### Chaining Addition
* Adding documents using chained calls to add(...)
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Chained Calls
*
* $(COLLECTIONADD_ADD_DETAIL9)
*
* $(COLLECTIONADD_ADD_DETAIL10)
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Using an Expression
*/
Result CollectionAdd::execute() {}
#endif
//@}
shcore::Value CollectionAdd::execute(const shcore::Argument_list &args) {
  mysqlx::Result *result = NULL;

  try {
    args.ensure_count(0, get_function_name("execute").c_str());

    MySQL_timer timer;
    timer.start();
    result = new mysqlx::Result(std::shared_ptr< ::mysqlx::Result>(_add_statement->execute()));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute").c_str());

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}
