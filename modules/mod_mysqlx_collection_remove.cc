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
#include "mod_mysqlx_collection_remove.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "shellcore/common.h"

using namespace mysh::mysqlx;
using namespace shcore;

CollectionRemove::CollectionRemove(boost::shared_ptr<Collection> owner)
:Collection_crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("remove", boost::bind(&CollectionRemove::remove, this, _1), "data");
  add_method("sort", boost::bind(&CollectionRemove::sort, this, _1), "data");
  add_method("limit", boost::bind(&CollectionRemove::limit, this, _1), "data");
  add_method("bind", boost::bind(&CollectionRemove::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("remove", "");
  register_dynamic_function("sort", "remove");
  register_dynamic_function("limit", "remove, sort");
  register_dynamic_function("bind", "remove, sort, limit");
  register_dynamic_function("execute", "remove, sort, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Sets the search condition for the to be executed remove operation.
* This method returns a new CollectionRemove object each time is invoked.
* The method must be invoked before any other, and after at least one invocation the following methods can be executed:
* sort, limit, bind, execute.
*
* \sa sort(), limit(), bind(), execute()
* \param searchCondition an optional string specifying the filter condition to use.
* \return the same instance collection where the method was invoked.
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
* // delete one doc
* var crud = collection.remove("name like 'my first'");
* crud.execute();
* // check results
* var crud = collection.find();
* crud.execute();
* \endcode
*/
CollectionRemove CollectionRemove::remove([String searchCondition])
{}
#endif
shcore::Value CollectionRemove::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionRemove::remove");

  boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

  if (collection)
  {
    try
    {
      std::string search_condition;
      if (args.size())
        search_condition = args.string_at(0);

      _remove_statement.reset(new ::mysqlx::RemoveStatement(collection->_collection_impl->remove(search_condition)));

      // Updates the exposed functions
      update_functions("remove");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove::remove");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the fields names use to order the result set in the to be executed remove operation.
* This method is usually used in combination with limit to fix the amount of documents to be removed.
* The method must be invoked after the following methods: modify.
* And after at least one invocation the following methods can be executed: limit, bind, execute.
*
* \sa limit(), bind(), execute()
* \param sortExprStr a list of strings with the field names to order by. The list can include the ASC/DESC for ascending/descending order for each field, 
*   for example "mycol1 asc, mycol2 desc" if order is not specified the default is ASC.
* \return the same instance collection where the method was invoked.
*/
CollectionRemove CollectionRemove::sort(List sortExprStr)
{}
#endif
shcore::Value CollectionRemove::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove::sort");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    _remove_statement->sort(fields);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove::sort");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the limit of the number of documents to affect by this remove operation.
* This method is used in combination with order to set a fixed amount of documents to affect by a given criteria.
* The method must be invoked after the following methods: sort.
* And after at least one invocation the following methods can be executed: bind, execute.
*
* \sa  bind(), execute()
* \param numberOfDocs the number of documents to affect in the remove execution.
* \return the same instance collection where the method was invoked.
*/
CollectionRemove CollectionRemove::limit(Integer numberOfDocs)
{}
#endif
shcore::Value CollectionRemove::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove::limit");

  try
  {
    _remove_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove::limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionRemove::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionRemove::bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

#ifdef DOXYGEN
/**
* Excutes the modify statement against a MySQLX server returning the a Result set of the operation.
*
* \sa modify(), sort(), limit(), bind()
* \param opt the execution options, currently ignored.
* \return a collection resultset describing the effects of the operation.
*/
Collection_resultset CollectionRemove::execute(ExecuteOptions opt)
{}
#endif
shcore::Value CollectionRemove::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionRemove::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_remove_statement->execute())));
}