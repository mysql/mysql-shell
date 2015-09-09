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
  register_dynamic_function("__shell_hook__", "remove, sort, limit, bind");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Sets the search condition to filter the Documents to be deleted from the owner Collection.
* \param searchCondition: An optional expression to filter the documents to be deleted;
* if not specified all the documents will be deleted from the collection unless a limit is set.
* \return This CollectionRemove object.
*
* This function is called automatically when Collection.remove(searchCondition) is called.
*
* The actual deletion of the documents will occur only when the execute method is called.
*
* After this function invocation, the following functions can be invoked:
*
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - execute(ExecuteOptions options).
*
* \sa Usage examples at execute(ExecuteOptions options).
*/
CollectionRemove CollectionRemove::remove(String searchCondition){}
#endif
shcore::Value CollectionRemove::remove(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionRemove.remove");

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
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove.remove");
  }

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Sets the order in which the deletion should be done.
* \param sortExprStr: A list of expression strings defining a sort criteria, the deletion will be done following the order defined by this criteria.
* \return This CollectionRemove object.
*
* The elements of sortExprStr list are strings defining the column name on which the sorting will be based in the form of "columnIdentifier [ ASC | DESC ]".
* If no order criteria is specified, ascending will be used by default.
*
* This method is usually used in combination with limit to fix the amount of documents to be deleted.
*
* This function can be invoked only once after:
*
* - remove(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - execute(ExecuteOptions options).
*/
CollectionRemove CollectionRemove::sort(List sortExprStr){}
#endif
shcore::Value CollectionRemove::sort(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove.sort");

  try
  {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    _remove_statement->sort(fields);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove.sort");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN

/**
* Sets a limit for the documents to be deleted.
* \param numberOfDocs the number of documents to affect in the remove execution.
* \return This CollectionRemove object.
*
* This method is usually used in combination with sort to fix the amount of documents to be deleted.
*
* This function can be invoked only once after:
*
* - remove(String searchCondition)
* - sort(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - execute(ExecuteOptions options).
*/
CollectionRemove CollectionRemove::limit(Integer numberOfDocs){}
#endif
shcore::Value CollectionRemove::limit(const shcore::Argument_list &args)
{
  args.ensure_count(1, "CollectionRemove.limit");

  try
  {
    _remove_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionRemove.limit");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

shcore::Value CollectionRemove::bind(const shcore::Argument_list &UNUSED(args))
{
  throw shcore::Exception::logic_error("CollectionRemove.bind: not yet implemented.");

  return Value(Object_bridge_ref(this));
}

#ifdef DOXYGEN
/**
* Executes the document deletion with the configured filter and limit.
* \return Collection_resultset A Collection resultset object that can be used to retrieve the results of the deletion operation.
*
* This function can be invoked after any other function on this class.
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
* // Remove the youngest
* var res_youngest = collection.remove().sort(['age', 'name']).limit(1).execute();
*
* // Remove the males
* var res_males = collection.remove('gender="male"').execute();
*
* // Removes all the documents
* var res_all = collection.remove().execute();
* \endcode
*/
Collection_resultset CollectionRemove::execute(ExecuteOptions opt){}
#endif
shcore::Value CollectionRemove::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionRemove.execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_remove_statement->execute())));
}