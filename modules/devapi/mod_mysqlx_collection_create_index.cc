/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/mod_mysqlx_collection_create_index.h"
#include "modules/devapi/base_constants.h"
#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "uuid_gen.h"
#include "db/mysqlx/mysqlx_parser.h"
#include "shellcore/utils_help.h"
#include "uuid_gen.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of CollectionCreateIndex class
REGISTER_HELP(COLLECTIONCREATEINDEX_BRIEF,
              "Handler for index creation on a Collection.");
REGISTER_HELP(
    COLLECTIONCREATEINDEX_DETAIL,
    "This object provides the necessary functions to allow adding an index into a collection.");
REGISTER_HELP(
    COLLECTIONCREATEINDEX_DETAIL1,
    "This object should only be created by calling any of the createIndex functions on the collection object where the index will be created.");

CollectionCreateIndex::CollectionCreateIndex(std::shared_ptr<Collection> owner)
    : _owner(owner) {
  // Exposes the methods available for chaining
  add_method("createIndex",
             std::bind(&CollectionCreateIndex::create_index, this, _1), "data");
  add_method("field", std::bind(&CollectionCreateIndex::field, this, _1),
             "data");
  add_method("execute", std::bind(&CollectionCreateIndex::execute, this, _1),
             "data");

  // Registers the dynamic function behavior
  register_dynamic_function("createIndex", "");
  register_dynamic_function("field", "createIndex, field");
  register_dynamic_function("execute", "field");

  // Initial function update
  update_functions("");
}

// Documentation of createIndex function
REGISTER_HELP(
    COLLECTIONCREATEINDEX_CREATEINDEX_BRIEF,
    "Sets the name for the creation of a non unique index on the collection.");
REGISTER_HELP(COLLECTIONCREATEINDEX_CREATEINDEX_SYNTAX,
              "createIndex(String indexName)");
REGISTER_HELP(COLLECTIONCREATEINDEX_CREATEINDEX_SYNTAX1,
              "createIndex(String indexName, IndexType type)");
REGISTER_HELP(COLLECTIONCREATEINDEX_CREATEINDEX_PARAM,
              "@param indexName The name of the index to be created.");
REGISTER_HELP(COLLECTIONCREATEINDEX_CREATEINDEX_RETURNS,
              "@returns This CollectionCreateIndex object.");

/**
* $(COLLECTIONCREATEINDEX_CREATEINDEX_BRIEF)
*
* $(COLLECTIONCREATEINDEX_CREATEINDEX_PARAM)
*
* $(COLLECTIONCREATEINDEX_CREATEINDEX_RETURNS)
*/
#if DOXYGEN_JS
CollectionCreateIndex CollectionCreateIndex::createIndex(String indexName) {}
#elif DOXYGEN_PY
CollectionCreateIndex CollectionCreateIndex::create_index(str indexName) {}
#endif

REGISTER_HELP(
    COLLECTIONCREATEINDEX_CREATEINDEX_BRIEF1,
    "Sets the name for the creation of a unique index on the collection.");
REGISTER_HELP(
    COLLECTIONCREATEINDEX_CREATEINDEX_PARAM1,
    "@param type The type of the index to be created, only supported type for the moment is mysqlx.IndexUnique.");

/**
* $(COLLECTIONCREATEINDEX_CREATEINDEX_BRIEF1)
*
* $(COLLECTIONCREATEINDEX_CREATEINDEX_PARAM)
* $(COLLECTIONCREATEINDEX_CREATEINDEX_PARAM1)
*
* $(COLLECTIONCREATEINDEX_CREATEINDEX_RETURNS)
*
* #### Method Chaining
*
*/
#if DOXYGEN_JS
CollectionCreateIndex CollectionCreateIndex::createIndex(String indexName,
                                                         IndexType type) {}
#elif DOXYGEN_PY
CollectionCreateIndex CollectionCreateIndex::create_index(str indexName,
                                                          IndexType type) {}
#endif

shcore::Value CollectionCreateIndex::create_index(
    const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, 2, get_function_name("createIndex").c_str());

  try {
    // Does nothing with the values, but just calling the proper getter performs
    // standard data type validation.
    args.string_at(0);

    Value unique;
    if (args.size() == 2) {
      if (args[1].type == shcore::Object) {
        std::shared_ptr<Constant> constant =
            std::dynamic_pointer_cast<Constant>(args.object_at(1));
        if (constant && constant->group() == "IndexType")
          unique = constant->data();
      }

      if (!unique)
        throw shcore::Exception::argument_error(
            "Argument #2 is expected to be mysqlx.IndexType.UNIQUE");
    } else {
      unique = Value::False();
    }

    std::shared_ptr<Collection> raw_owner(_owner.lock());

    if (raw_owner) {
      Value schema = raw_owner->get_member("schema");
      _create_index_args.push_back(schema.as_object()->get_member("name"));
      _create_index_args.push_back(raw_owner->get_member("name"));
      _create_index_args.push_back(args[0]);
      _create_index_args.push_back(unique);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("createIndex").c_str());

  update_functions("createIndex");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of field function
REGISTER_HELP(COLLECTIONCREATEINDEX_FIELD_BRIEF,
              "Adds column to be part of the collection index being created.");
REGISTER_HELP(
    COLLECTIONCREATEINDEX_FIELD_PARAM,
    "@param documentPath The document path to the field to be added into the index.");
REGISTER_HELP(COLLECTIONCREATEINDEX_FIELD_PARAM1,
              "@param type A string defining a valid MySQL data type.");
REGISTER_HELP(
    COLLECTIONCREATEINDEX_FIELD_PARAM2,
    "@param isRequired a flag that indicates whether the field is required or not.");
REGISTER_HELP(COLLECTIONCREATEINDEX_FIELD_RETURNS, "@returns A Result object.");

/**
* $(COLLECTIONCREATEINDEX_FIELD_BRIEF)
*
* $(COLLECTIONCREATEINDEX_FIELD_PARAM)
* $(COLLECTIONCREATEINDEX_FIELD_PARAM1)
* $(COLLECTIONCREATEINDEX_FIELD_PARAM2)
*
* $(COLLECTIONCREATEINDEX_FIELD_RETURNS)
*
* #### Method Chaining
*
* This function can be invoked many times, every time it is called the received
* information will be added to the index definition.
*
* After this function invocation, the following functions can be invoked:
*
* - execute()
*/
#if DOXYGEN_JS
CollectionCreateIndex CollectionCreateIndex::field(DocPath documentPath,
                                                   IndexColumnType type,
                                                   Bool isRequired) {}
#elif DOXYGEN_PY
CollectionCreateIndex CollectionCreateIndex::field(DocPath documentPath,
                                                   IndexColumnType type,
                                                   bool isRequired) {}
#endif
shcore::Value CollectionCreateIndex::field(const shcore::Argument_list &args) {
  args.ensure_count(3, get_function_name("field").c_str());

  try {
    // Data Type Validation
    std::string path = args.string_at(0);
    std::string type = args.string_at(1);

    // Validates the data type
    args.bool_at(2);

    _create_index_args.push_back(Value("$." + path));
    _create_index_args.push_back(args[1]);
    _create_index_args.push_back(args[2]);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("field").c_str());

  update_functions("field");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of execute function
REGISTER_HELP(
    COLLECTIONCREATEINDEX_EXECUTE_BRIEF,
    "Executes the document addition for the documents cached on this object.");
REGISTER_HELP(COLLECTIONCREATEINDEX_EXECUTE_RETURNS,
              "@returns A Result object.");

/**
* $(COLLECTIONCREATEINDEX_EXECUTE_BRIEF)
*
* $(COLLECTIONCREATEINDEX_EXECUTE_RETURNS)
*
* #### Method Chaining
*
* This function can be invoked once after:
* - add(Document document)
* - add(List documents)
*/
#if DOXYGEN_JS
Result CollectionCreateIndex::execute() {}
#elif DOXYGEN_PY
Result CollectionCreateIndex::execute() {}
#endif
shcore::Value CollectionCreateIndex::execute(
    const shcore::Argument_list &args) {
  Value result;

  args.ensure_count(0, get_function_name("execute").c_str());

  std::shared_ptr<Collection> raw_owner(_owner.lock());

  try {
    if (raw_owner) {
      Value session = raw_owner->get_member("session");
      auto session_obj =
          std::static_pointer_cast<NodeSession>(session.as_object());
      result = session_obj->executeAdminCommand("create_collection_index",
                                                false, _create_index_args);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  update_functions("execute");

  return result;
}
