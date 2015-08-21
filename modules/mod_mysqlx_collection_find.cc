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
#include <boost/format.hpp>
#include "mod_mysqlx_collection_find.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

CollectionFind::CollectionFind(boost::shared_ptr<Collection> owner)
: Collection_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("find", boost::bind(&CollectionFind::find, this, _1), "data");
  add_method("fields", boost::bind(&CollectionFind::fields, this, _1), "data");
  add_method("groupBy", boost::bind(&CollectionFind::group_by, this, _1), "data");
  add_method("having", boost::bind(&CollectionFind::having, this, _1), "data");
  add_method("sort", boost::bind(&CollectionFind::sort, this, _1), "data");
  add_method("skip", boost::bind(&CollectionFind::skip, this, _1), "data");
  add_method("limit", boost::bind(&CollectionFind::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionFind::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("find", "");
  register_dynamic_function("fields", "find");
  register_dynamic_function("groupBy", "find, fields");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("sort", "find, fields, groupBy, having");
  register_dynamic_function("limit", "find, fields, groupBy, having, sort");
  register_dynamic_function("skip", "limit");
  register_dynamic_function("bind", "find, fields, groupBy, having, sort, skip, limit");
  register_dynamic_function("execute", "find, fields, groupBy, having, sort, skip, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Sets the search condition to identify the Documents to be retrieved from the owner Collection.
* This method is called automatically when Collection.find(searchCondition) is called.
* Calling this method is allowed only for the first time, after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After this method invocation, the following methods can be invoked: fields, groupBy, sort, limit, bind, execute.
* 
* \sa fields(), groupBy(), sort(), limit(), bind(), execute()
* \param searchCondition: An optional expression to identify the documents to be retrieved, if not specified all the documents will be included on the result.
* \return CollectionFindRef returns itself with it's state updated as the searchCondition was established.
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
CollectionFindRef CollectionFind::find(String searchCondition)
{}
#endif
shcore::Value CollectionFind::find(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionFind::find");

  boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

  if (collection)
  {
    try
    {
      std::string search_condition;
      if (args.size())
        search_condition = args.string_at(0);

      _find_statement.reset(new ::mysqlx::FindStatement(collection->_collection_impl->find(search_condition)));

      // Updates the exposed functions
      update_functions("find");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::find");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a field filter, if used the CollectionFind operation will only return the fields that were included.
* Calling this method is allowed only for the first time and only if the search criteria has been set by calling CollectionFind.find(searchCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After this method invocation the following methods can be invoked: groupBy, sort, limit, bind, execute.
*
* \sa groupBy(), sort(), limit(), bind(), execute()
* \param projectedSearchExprStr: A list of string expressions identifying the fields to be extracted, alias support is suported on these fields.
* \return CollectionFindRef returns itself with it's state updated as the field list has been stablished.
*/
CollectionFindRef CollectionFind::fields(List projectedSearchExprStr)
{}
#endif
shcore::Value CollectionFind::fields(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::fields");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Field selection criteria can not be empty");

    _find_statement->fields(fields);

    update_functions("fields");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::fields");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a grouping criteria for the resultset, if used the CollectionFind operation will group the records using the stablished criteria.
* Calling this method is allowed only for the first time and only if the search criteria has been set by calling CollectionFind.find(searchCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After method groupBy invocation the following methods can be invoked: having, sort, limit, bind, execute.
* 
* \sa find(), fields(), having(), sort(), limit(), bind(), execute().
* \param searchExprStr: A list of string expressions identifying the grouping criteria.
* \return CollectionFindRef returns itself with it's state updated as the grouping criteria has been set.
*/
CollectionFindRef CollectionFind::groupBy(List searchExprStr)
{}
#endif
shcore::Value CollectionFind::group_by(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::groupBy");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Grouping criteria can not be empty");

    _find_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::groupBy");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets a condition for records to be considered in agregate function operations, if used the CollectionFind operation will only consider the records matching the stablished criteria.
* Calling this method is allowed only for the first time and only if the grouping criteria has been set by calling CollectionFind.groupBy(groupCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After this method invocation the following methods can be invoked: sort, limit, bind, execute.
* 
* \sa groupBy(), sort(), limit(), bind(), execute().
* \param searchCondition: A condition on the agregate functions used on the grouping criteria.
* \return CollectionFindRef returns itself with it's state updated as the grouping condition has been set.
*/
CollectionFindRef CollectionFind::having(String searchCondition)
{}
#endif
shcore::Value CollectionFind::having(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::having");

  try
  {
    _find_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::having");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the sorting criteria to be used on the Resultset, if used the CollectionFind operation will return the records sorted with the defined criteria.
* Calling this method is allowed only for the first time and only if the search criteria has been set by calling CollectionFind.find(searchCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After this method invocation the following methods can be invoked: limit, bind, execute.
* 
* \sa find(), fields(), groupBy(), having(), limit(), bind(), execute()
* \param sortExprStr: A list containing the sort criteria expressions to be used on the operation.
* \return CollectionFindRef returns itself with it's state updated as the grouping condition has been set.
*/
CollectionFindRef CollectionFind::sort(List sortExprStr)
{}
#endif
shcore::Value CollectionFind::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::sort");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    _find_statement->sort(fields);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::sort");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the maximum number of documents to be returned on the find operation, if used the CollectionFind operation will return at most numberOfRows documents.
* Calling this method is allowed only for the first time and only if the search criteria has been set by calling CollectionFind.find(searchCriteria), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After this method invocation the following methods can be invoked: skip, bind, execute.
*
* \sa find(), fields(), groupBy(), having(), sort(), skip(), bind(), execute()
* \param numberOfRows: The maximum number of documents to be retrieved.
* \return CollectionFindRef returns itself with it's state updated as the grouping condition has been set.
*/
CollectionFindRef CollectionFind::limit(Integer numberOfRows)
{}
#endif
shcore::Value CollectionFind::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::limit");

  try
  {
    _find_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets number of records to skip on the resultset when a limit has been defined.
* Calling this method is allowed only for the first time and only if a limit has been set by calling CollectionFind.limit(numberOfRows), after that its usage is forbidden since the internal class state has been updated to handle the rest of the Find operation.
* After this method, the following methods can be invoked: bind, execute.
*
* \sa bind(), execute()
* \param limitOffset: The number of documents to skip before start including them on the Resultset.
* \return CollectionFindRef returns itself with it's state updated as the grouping condition has been set.
*/
CollectionFindRef CollectionFind::skip(Integer limitOffset)
{}
#endif
shcore::Value CollectionFind::skip(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionFind::skip");

  try
  {
    _find_statement->skip(args.uint_at(0));

    update_functions("skip");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind::skip");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionFind::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionFind::bind: not yet implemented.");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the Find operation with all the configured options and returns.
* This method can be invoked after any of the following methods: find, fields, groupBy, having, sort, skip, limit, bind.
*
* \return Collection_resultset A Collection resultset object that can be used to retrieve the results of the find operation.
*/
Collection_resultset CollectionFind::execute(ExecuteOptions opt)
{}
#endif
shcore::Value CollectionFind::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionFind::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_find_statement->execute())));
}