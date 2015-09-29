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
#include "mod_mysqlx_collection_modify.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"
#include <sstream>
#include <boost/format.hpp>

using namespace mysh::mysqlx;
using namespace shcore;

CollectionModify::CollectionModify(boost::shared_ptr<Collection> owner)
:Collection_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("modify", boost::bind(&CollectionModify::modify, this, _1), "data");
  add_method("set", boost::bind(&CollectionModify::set, this, _1), "data");
  add_method("unset", boost::bind(&CollectionModify::unset, this, _1), "data");
  add_method("merge", boost::bind(&CollectionModify::merge, this, _1), "data");
  add_method("arrayInsert", boost::bind(&CollectionModify::array_insert, this, _1), "data");
  add_method("arrayAppend", boost::bind(&CollectionModify::array_append, this, _1), "data");
  add_method("arrayDelete", boost::bind(&CollectionModify::array_delete, this, _1), "data");
  add_method("sort", boost::bind(&CollectionModify::sort, this, _1), "data");
  add_method("limit", boost::bind(&CollectionModify::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionModify::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("modify", "");
  register_dynamic_function("set", "modify, operation");
  register_dynamic_function("unset", "modify, operation");
  register_dynamic_function("merge", "modify, operation");
  register_dynamic_function("arrayInsert", "modify, operation");
  register_dynamic_function("arrayAppend", "modify, operation");
  register_dynamic_function("arrayDelete", "modify, operation");
  register_dynamic_function("sort", "operation");
  register_dynamic_function("limit", "operation, sort");
  register_dynamic_function("bind", "operation, sort, limit, bind");
  register_dynamic_function("execute", "operation, sort, limit, bind");
  register_dynamic_function("__shell_hook__", "operation, sort, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Sets the search condition to identify the Documents to be updated on the owner Collection.
* \param searchCondition: An optional expression to identify the documents to be updated;
* if not specified all the documents will be updated on the collection unless a limit is set.
* \return This CollectionModify object.
*
* The searchCondition supports using placeholders instead of raw values, example:
*
* \code{.js}
* // Setting adult flag on records
* collection.modify("age > 21").set('adult', 'yes').execute()
*
* // Equivalent code using bound values
* collection.modify("age > :adultAge").set('adult', 'yes').bind('adultAge', 21).execute()
* \endcode
*
* On the previous example, adultAge is a placeholder for a value that will be set by calling the bind() function
* right before calling execute().
*
* Note that if placeholders are used, a value must be bounded on each of them or the operation will fail.
*
* This function is called automatically when Collection.modify(searchCondition) is called.
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* \sa Usage examples at execute(ExecuteOptions options).
* \sa Collection
*/
CollectionModify CollectionModify::modify(String searchCondition){}
#endif
shcore::Value CollectionModify::modify(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionModify.modify");

  boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

  if (collection)
  {
    try
    {
      std::string search_condition;
      if (args.size())
        search_condition = args.string_at(0);

      _modify_statement.reset(new ::mysqlx::ModifyStatement(collection->_collection_impl->modify(search_condition)));

      // Updates the exposed functions
      update_functions("modify");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.modify");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets or updates attributes on documents in a collection.
* \param attribute A string with the document path of the item to be set.
* \param value The value to be set on the specified attribute.
* \return This CollectionModify object.
*
* Adds an opertion into the modify handler to set an attribute on the documents that were included on the selection filter and limit.
* - If the attribute is not present on the document, it will be added with the given value.
* - If the attribute already exists on the document, it will be updated with the given value.
*
* The attribute addition will be done on the collection's documents once the execute method is called.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::set(String attribute, Value value){}
#endif
shcore::Value CollectionModify::set(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(2, "CollectionModify.set");

  try
  {
    std::string field = args.string_at(0);
    _modify_statement->set(field, map_document_value(args[1]));

    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.set");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Removes attributes from documents in a collection.
* \param attribute A string with the document path of the attribute to be removed.
* \return This CollectionModify object.
*
* This function can receive either one or more attributes, for each received attribute adds an opertion into the modify handler
* to remove an attribute or attributes on the documents that were included on the selection filter and limit.
*
* The attribute removal will be done on the collection's documents once the execute method is called.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::unset(String attribute){}

/**
* Removes attributes from documents in a collection.
* \param attributes A string with the document path of the attributes to be removed.
* \return This CollectionModify object.
*
* For each attribute on the attributes list, adds an opertion into the modify handler
* to remove the attribute on the documents that were included on the selection filter and limit.
*
* The attribute removal will be done on the collection's documents once the execute method is called.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::unset(List attributes){}
#endif
shcore::Value CollectionModify::unset(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_at_least(1, "CollectionModify.unset");

  try
  {
    size_t unset_count = 0;

    // Could receive either a List or many strings
    if (args.size() == 1 && args[0].type == Array)
    {
      shcore::Value::Array_type_ref items = args.array_at(0);
      shcore::Value::Array_type::iterator index, end = items->end();

      int int_index = 0;
      for (index = items->begin(); index != end; index++)
      {
        int_index++;
        if (index->type == shcore::String)
          _modify_statement->remove(index->as_string());
        else
          throw shcore::Exception::type_error((boost::format("Element #%1% is expected to be a string") % (int_index)).str());
      }

      unset_count = items->size();
    }
    else
    {
      for (size_t index = 0; index < args.size(); index++)
      {
        if (args[index].type == shcore::String)
          _modify_statement->remove(args.string_at(index));
        else
        {
          std::string error;

          if (args.size() == 1)
            error = (boost::format("Argument #%1% is expected to be either string or list of strings") % (index + 1)).str();
          else
            error = (boost::format("Argument #%1% is expected to be a string") % (index + 1)).str();

          throw shcore::Exception::type_error(error);
        }
      }

      unset_count = args.size();
    }

    // Updates the exposed functions
    if (unset_count)
      update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.unset");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Adds attributes taken from a document into the documents in a collection.
* \param document The document from which the attributes will be merged.
* \return This CollectionModify object.
*
* This function adds an operation to add into the documents of a collection, all the attribues defined in document that do not exist on the collection's documents.
* \todo Define what happens when document contains attributes that arelady exist on the collection's documents.
*
* The attribute addition will be done on the collection's documents once the execute method is called.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::merge(Document document){}
#endif
shcore::Value CollectionModify::merge(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionModify.merge");

  try
  {
    shcore::Value::Map_type_ref items = args.map_at(0);

    Value object_as_json(args[0].json());

    _modify_statement->merge(map_document_value(object_as_json));

    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.merge");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Inserts a value into a specific position in an array attribute in documents of a collection.
* \param path A document path that identifies the array attribute and position where the value will be inserted.
* \param value The value to be inserted.
* \return This CollectionModify object.
*
* Adds an opertion into the modify handler to insert a value into an array attribute on the documents that were included on the selection filter and limit.
*
* The insertion of the value will be done on the collection's documents once the execute method is called.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::arrayInsert(String path, Value value){}
#endif
shcore::Value CollectionModify::array_insert(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(2, "CollectionModify.arrayInsert");

  try
  {
    _modify_statement->arrayInsert(args.string_at(0), map_document_value(args[1]));

    // Updates the exposed functions
    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.arrayInsert");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Appends a value into an array attribute in documents of a collection.
* \param path A document path that identifies the array attribute where the value will be appended.
* \param value The value to be appended.
* \return This CollectionModify object.
*
* Adds an opertion into the modify handler to append a value into an array attribute on the documents that were included on the selection filter and limit.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* The attribute addition will be done on the collection's documents once the execute method is called.
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::arrayAppend(String path, Value value){}
#endif
shcore::Value CollectionModify::array_append(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(2, "CollectionModify.arrayAppend");

  try
  {
    std::string field = args.string_at(0);

    _modify_statement->arrayAppend(field, map_document_value(args[1]));

    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.arrayAppend");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Deletes the value at a specific position in an array attribute in documents of a collection.
* \param path A document path that identifies the array attribute and position of the value to be deleted.
* \return This CollectionModify object.
*
* Adds an opertion into the modify handler to delete a value from an array attribute on the documents that were included on the selection filter and limit.
*
* The attribute deletion will be done on the collection's documents once the execute method is called.
*
* This function can be invoked multiple times after:
*
* - modify(String searchCondition)
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::arrayDelete(String path, Value value){}
#endif
shcore::Value CollectionModify::array_delete(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionModify.arrayDelete");

  try
  {
    _modify_statement->arrayDelete(args.string_at(0));

    // Updates the exposed functions
    update_functions("operation");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.arrayDelete");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the document order in which the update operations added to the handler should be done.
* \param sortExprStr: A list of expression strings defining a collection sort criteria.
* \return This CollectionModify object.
*
* The elements of sortExprStr list are usually strings defining the attribute name on which the collection sorting will be based. Each criterion could be followed by asc or desc to indicate ascending
* or descending order respectivelly. If no order is specified, ascending will be used by default.
*
* This method is usually used in combination with limit to fix the amount of documents to be updated.
*
* This function can be invoked only once after:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::sort(List sortExprStr){}
#endif
shcore::Value CollectionModify::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionModify.sort");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    _modify_statement->sort(fields);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.sort");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a limit for the documents to be updated by the operations added to the handler.
* \param numberOfDocs the number of documents to affect on the update operations.
* \return This CollectionModify object.
*
* This method is usually used in combination with sort to fix the amount of documents to be updated.
*
* This function can be invoked only once after:
*
* - set(String attribute, Value value)
* - unset(String attribute)
* - unset(List attributes)
* - merge(Document document)
* - arrayAppend(String path, Value value)
* - arrayInsert(String path, Value value)
* - arrayDelete(String path)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute(ExecuteOptions opt)
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionModify CollectionModify::limit(Integer numberOfDocs){}
#endif
shcore::Value CollectionModify::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionModify.limit");

  try
  {
    _modify_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Binds a value to a specific placeholder used on this CollectionModify object.
* \param name: The name of the placeholder to which the value will be bound.
* \param value: The value to be bound on the placeholder.
* \return This CollectionModify object.
*
* This function can be invoked multiple times right before calling execute:
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute(ExecuteOptions options)
*
* An error will be raised if the placeholder indicated by name does not exist.
*
* This function must be called once for each used placeohlder or an error will be
* raised when the execute method is called.
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionFind CollectionModify::bind(String name, Value value){}
#endif
shcore::Value CollectionModify::bind(const shcore::Argument_list &args)
{
  args.ensure_count(2, "CollectionModify.bind");

  try
  {
    _modify_statement->bind(args.string_at(0), map_document_value(args[1]));

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.bind");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the update operations added to the handler with the configured filter and limit.
* \return CollectionResultset A Collection resultset object that can be used to retrieve the results of the update operation.
*
* This function can be invoked after any other function on this class except modify().
*
* The update operation will be executed in the order they were added.
*
* \code{.js}
* // open a connection
* var mysqlx = require('mysqlx').mysqlx;
* var mysession = mysqlx.getSession("myuser@localhost", mypwd);
*
* // Assuming a collection named friends exists on the test schema
* var collection = mysession.test.friends;
*
* // create some initial data
* collection.add([{name: 'jack', last_name = 'black', age: 17, gender: 'male'},
*                 {name: 'adam', last_name = 'sandler', age: 15, gender: 'male'},
*                 {name: 'brian', last_name = 'adams', age: 14, gender: 'male'},
*                 {name: 'alma', last_name = 'lopez', age: 13, gender: 'female'},
*                 {name: 'carol', last_name = 'shiffield', age: 14, gender: 'female'},
*                 {name: 'donna', last_name = 'summers', age: 16, gender: 'female'},
*                 {name: 'angel', last_name = 'down', age: 14, gender: 'male'}]).execute();
*
* // Adds a likes and unlikes attributes into the documents in friends
* var res_set = collection.modify().set('likes', 0).set('unlikes', 0).execute();
*
* // Merges additional attributes from a document into the collection documents
* var res_merge = collection.modify().merge({hobbies:[], pages:[], active:True}).execute();
*
* // Renames unlikes to dislikes
* var res_unset = collection.modify().set('dislikes', '$.unlikes').unset('unlikes').execute();
*
* // Appends hobbies
* var res_males = collection.modify('gender="male"').arrayAppend('hobbies', 'wrestling').execute();
* var res_males = collection.modify('gender=:heorshe').arrayAppend('hobbies', 'dolls').bind('heorshe', 'female').execute();
*
* // Updates hobbies for the youngest
* var res_toons = collection.modify().arrayAppend('hobbies', 'cartoons').sort(['age']).limit(1).execute();*
*
* // The youngest prefers the bike over the cartoons
* var res_bike = collection.modify().arrayInsert('hobbies[1]', 'bike').sort(['age']).limit(1).execute();*
*
* // The youngest doesn't have interest in cars at all
* var res_car = collection.modify().arrayDelete('hobbies[0]').sort(['age']).limit(1).execute();*
* \endcode
*/
CollectionResultset CollectionModify::execute(ExecuteOptions opt){}
#endif
shcore::Value CollectionModify::execute(const shcore::Argument_list &args)
{
  mysqlx::CollectionResultset *result = NULL;

  try
  {
    args.ensure_count(0, "CollectionModify.execute");

    result = new mysqlx::CollectionResultset(boost::shared_ptr< ::mysqlx::Result>(_modify_statement->execute()));
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionModify.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}