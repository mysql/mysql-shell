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
#include "mod_mysqlx_collection_create_index.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "mod_mysqlx_session.h"
#include "base_constants.h"
#include "uuid_gen.h"
#include "mysqlx_parser.h"

#include <iomanip>
#include <sstream>
#include <boost/format.hpp>

using namespace std::placeholders;
using namespace mysh::mysqlx;
using namespace shcore;

CollectionCreateIndex::CollectionCreateIndex(std::shared_ptr<Collection> owner)
  :_owner(owner) {
  // Exposes the methods available for chaining
  add_method("createIndex", std::bind(&CollectionCreateIndex::create_index, this, _1), "data");
  add_method("field", std::bind(&CollectionCreateIndex::field, this, _1), "data");
  add_method("execute", std::bind(&CollectionCreateIndex::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("createIndex", "");
  register_dynamic_function("field", "createIndex, field");
  register_dynamic_function("execute", "field");

  // Initial function update
  update_functions("");
}

#if DOXYGEN_CPP
/**
 * Creates an index on a collection.
 * \param args should contain the name and optionally the type of index to be created.
 * \return A CollectionCreateIndex object.
 *
 * This function creates a CollectionCreateIndex object which is an index creation handler.
 *
 * The CollectionCreateIndex class has a function to define the fields to be included on the index.
 *
 * The index will be created when the execute function is called on the index creation handler.
 *
 * The function will create a non unique index unless mysqlx.IndexType.IndexUnique is passed as the second element on args.
 *
 * \sa CollectionCreateIndex
 */
#else
/**
* Sets the name for the creation of a non unique index on the collection.
* \param indexName The name of the index to be created.
* \return This CollectionCreateIndex object.
*/
#if DOXYGEN_JS
CollectionCreateIndex CollectionCreateIndex::createIndex(String indexName) {}
#elif DOXYGEN_PY
CollectionCreateIndex CollectionCreateIndex::create_index(str indexName) {}
#endif

/**
* Sets the name for the creation of a unique index on the collection.
* \param indexName The name of the index to be created.
* \param type The type of the index to be created, only supported type for the moment is mysqlx.IndexUnique.
* \return This CollectionCreateIndex object.
*
* #### Method Chaining
*
*/
#if DOXYGEN_JS
CollectionCreateIndex CollectionCreateIndex::createIndex(String indexName, IndexType type) {}
#elif DOXYGEN_PY
CollectionCreateIndex CollectionCreateIndex::create_index(str indexName, IndexType type) {}
#endif
#endif
shcore::Value CollectionCreateIndex::create_index(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, 2, get_function_name("createIndex").c_str());

  try {
    // Does nothing with the values, but just calling the proper getter performs
    // standard data type validation.
    args.string_at(0);

    Value unique;
    if (args.size() == 2) {
      if (args[1].type == shcore::Object) {
        std::shared_ptr <Constant> constant = std::dynamic_pointer_cast<Constant>(args.object_at(1));
        if (constant && constant->group() == "IndexType")
            unique = constant->data();
      }

      if (!unique)
        throw shcore::Exception::argument_error("Argument #2 is expected to be mysqlx.IndexType.UNIQUE");
    } else
      unique = Value::False();

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

//! Adds column to be part of the collection index being created.
#if DOXYGEN_CPP
//! \param args : Should contain three elements described below.
//!
//! \li A string with the document path to the field to be added into the index.
//! \li A string defining a valid MySQL data type.
//! \li A boolean a flag that indicates whether the field is required or not.
#else
//! \param documentPath The document path to the field to be added into the index.
//! \param type A string defining a valid MySQL data type.
//! \param isRequired a flag that indicates whether the field is required or not.
#endif
/**
* \return A Result object.
*
* #### Method Chaining
*
* This function can be invoked many times, every time it is called the received information will be added to the index definition.
*
* After this function invocation, the following functions can be invoked:
*
* - execute()
*/
#if DOXYGEN_JS
CollectionCreateIndex CollectionCreateIndex::field(DocPath documentPath, IndexColumnType type, Bool isRequired) {}
#elif DOXYGEN_PY
CollectionCreateIndex CollectionCreateIndex::field(DocPath documentPath, IndexColumnType type, bool isRequired) {}
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

/**
* Executes the document addition for the documents cached on this object.
* \return A Result object.
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
shcore::Value CollectionCreateIndex::execute(const shcore::Argument_list &args) {
  Value result;

  args.ensure_count(0, get_function_name("execute").c_str());

  std::shared_ptr<Collection> raw_owner(_owner.lock());

  if (raw_owner) {
    Value session = raw_owner->get_member("session");
    std::shared_ptr<BaseSession> session_obj = std::static_pointer_cast<BaseSession>(session.as_object());
    result = session_obj->executeAdminCommand("create_collection_index", false, _create_index_args);
  }

  update_functions("execute");

  return result;
}
