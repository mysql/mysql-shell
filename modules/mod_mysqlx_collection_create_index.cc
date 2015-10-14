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
#include "mod_mysqlx_collection_create_index.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_constant.h"
#include "uuid_gen.h"
#include "mysqlx_parser.h"

#include <iomanip>
#include <sstream>
#include <boost/format.hpp>

using namespace mysh::mysqlx;
using namespace shcore;

CollectionCreateIndex::CollectionCreateIndex(boost::shared_ptr<Collection> owner)
:_owner(owner)
{
  // Exposes the methods available for chaining
  add_method("createIndex", boost::bind(&CollectionCreateIndex::create_index, this, _1), "data");
  add_method("field", boost::bind(&CollectionCreateIndex::field, this, _1), "data");
  add_method("execute", boost::bind(&CollectionCreateIndex::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("createIndex", "");
  register_dynamic_function("field", "createIndex, field");
  register_dynamic_function("execute", "field");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Sets the name for the creation of a non unique index on the collection.
* \param String The name of the index to be created.
* \return This CollectionCreateIndex object.
*/
CollectionCreateIndex CollectionCreateIndex::createIndex(String indexName){}

/**
* Sets the name for the creation of a unique index on the collection.
* \param String The name of the index to be created.
* \param IndexType The type of the index to be created, only supported type for the moment is mysqlx.IndexUnique.
* \return This CollectionCreateIndex object.
*/
CollectionCreateIndex CollectionCreateIndex::createIndex(String indexName, IndexType type){}
#endif
shcore::Value CollectionCreateIndex::create_index(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, 2, "CollectionCreateIndex.createIndex");

  try
  {
    // Does nothing with the values, but just calling the proper getter performs
    // standard data type validation.
    args.string_at(0);

    Value unique;
    if (args.size() == 2)
    {
      if (args[1].type == shcore::Object)
      {
        boost::shared_ptr <Constant> constant = boost::dynamic_pointer_cast<Constant>(args.object_at(1));
        if (constant && constant->group() == "IndexTypes")
            unique = constant->data();
      }

      if (!unique)
        throw shcore::Exception::argument_error("Argument #2 is expected to be mysqlx.IndexUnique");
    }
    else
      unique = Value::False();

    boost::shared_ptr<Collection> raw_owner(_owner.lock());

    if (raw_owner)
    {
      Value schema = raw_owner->get_member("schema");
      _create_index_args.push_back(schema.as_object()->get_member("name"));
      _create_index_args.push_back(raw_owner->get_member("name"));
      _create_index_args.push_back(args[0]);
      _create_index_args.push_back(unique);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionCreateIndex.createIndex");

  update_functions("createIndex");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Adds column to be part of the collection index being created .
* \param DocPath The document path to the field to be added into the index.
* \param FieldType The type of the field to be added into the index.
* \param Bool a flag that indicates whether the field is required or not.
* \return A Result object.
*
* This function can be invoked many times, every time it is called the received information will be added to the index definition.
*
* For the type parameter, the Data Type definitions on the mysqlx module should be used, or in the case of Varchar, Char and Decimal
* data types, use the Varchar, Char and Decimal functions defined on the mysqlx module.
*/
CollectionCreateIndex CollectionCreateIndex::field(DocPath documentPath, IndexColumnType type, Bool isRequired){}
#endif
shcore::Value CollectionCreateIndex::field(const shcore::Argument_list &args)
{
  args.ensure_count(3, "CollectionCreateIndex.field");

  try
  {
    // Data Type Validation
    std::string path = args.string_at(0);

    // This parsing is required to validate the fiven document path
    //Mysqlx::Expr::Expr *docpath = ::mysqlx::parser::parse_column_identifier(path);

    //::mysqlx::Expr_unparser unparser;
    //std::string doc_path = unparser.expr_to_string(*docpath);

    Value data;
    if (args[1].type == shcore::Object)
    {
      boost::shared_ptr <Constant> constant = boost::dynamic_pointer_cast<Constant>(args.object_at(1));
      if (constant && constant->group() == "DataTypes")
        data = constant->data();
    }

    if (!data)
      throw shcore::Exception::argument_error("Argument #2 is expected to be a Data Type constant");

    // Validates the data type
    args.bool_at(2);

    _create_index_args.push_back(Value("." + path));
    _create_index_args.push_back(data);
    _create_index_args.push_back(args[2]);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionCreateIndex.field");

  update_functions("field");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
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
Result CollectionCreateIndex::execute(){}
#endif
shcore::Value CollectionCreateIndex::execute(const shcore::Argument_list &args)
{
  Value result;

  args.ensure_count(0, "CollectionCreateIndex.execute");

  boost::shared_ptr<Collection> raw_owner(_owner.lock());

  if (raw_owner)
  {
    Value session = raw_owner->get_member("session");
    boost::shared_ptr<BaseSession> session_obj = boost::static_pointer_cast<BaseSession>(session.as_object());
    result = session_obj->executeAdminCommand("create_collection_index", false, _create_index_args);
  }

  update_functions("execute");

  return result;
}