/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "modules/devapi/mod_mysqlx_collection_remove.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_time.h"
#include "db/mysqlx/mysqlx_parser.h"
#include "mysqlshdk/libs/utils/utils_time.h"

using std::placeholders::_1;
using namespace shcore;

namespace mysqlsh {
namespace mysqlx {

// Documentation of CollectionRemove class
REGISTER_HELP(COLLECTIONREMOVE_BRIEF,
              "Handler for document removal from a Collection.");
REGISTER_HELP(
    COLLECTIONREMOVE_DETAIL,
    "This object provides the necessary functions to allow removing documents from a collection.");
REGISTER_HELP(
    COLLECTIONREMOVE_DETAIL1,
    "This object should only be created by calling the remove function on the collection object "
    "from which the documents will be removed.");

CollectionRemove::CollectionRemove(std::shared_ptr<Collection> owner)
    : Collection_crud_definition(
          std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::DOCUMENT);

  // Exposes the methods available for chaining
  add_method("remove", std::bind(&CollectionRemove::remove, this, _1), "data");
  add_method("sort", std::bind(&CollectionRemove::sort, this, _1), "data");
  add_method("limit", std::bind(&CollectionRemove::limit, this, _1), "data");
  add_method("bind", std::bind(&CollectionRemove::bind_, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("remove", "");
  register_dynamic_function("sort", "remove");
  register_dynamic_function("limit", "remove, sort");
  register_dynamic_function("bind", "remove, sort, limit, bind");
  register_dynamic_function("execute", "remove, sort, limit, bind");
  register_dynamic_function("__shell_hook__", "remove, sort, limit, bind");

  // Initial function update
  update_functions("");
}

// Documentation of remove function
REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_BRIEF,
    "Sets the search condition to filter the documents to be deleted from "
    "the owner Collection.");
REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_PARAM,
    "@param searchCondition: An expression to filter the documents to be "
    "deleted.");
REGISTER_HELP(COLLECTIONREMOVE_REMOVE_RETURNS,
              "@returns This CollectionRemove object.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL,
    "Creates a handler for the deletion of documents on the collection.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL1,
    "A condition must be provided to this function, all the documents "
    "matching the condition will be removed from the collection.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL2,
    "To delete all the documents, set a condition that always evaluates to "
    "true, for example '1'.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL3,
    "The searchCondition supports parameter binding.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL4,
    "This function is called automatically when "
    "Collection.remove(searchCondition) is called.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL5,
    "The actual deletion of the documents will occur only when the execute "
    "method is called.");

/**
* $(COLLECTIONREMOVE_REMOVE_BRIEF)
*
* $(COLLECTIONREMOVE_REMOVE_PARAM)
*
* $(COLLECTIONREMOVE_REMOVE_RETURNS)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL1)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL2)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL3)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL4)
*
* $(COLLECTIONREMOVE_REMOVE_DETAIL5)
*
* #### Method Chaining
*
* After this function invocation, the following functions can be invoked:
*
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
//@{
#if DOXYGEN_JS
CollectionRemove CollectionRemove::remove(String searchCondition) {}
#elif DOXYGEN_PY
CollectionRemove CollectionRemove::remove(str searchCondition) {}
#endif
//@}
shcore::Value CollectionRemove::remove(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("remove").c_str());

  auto collection(std::static_pointer_cast<Collection>(_owner));

  if (collection) {
    try {
      std::string search_condition = str_strip(args.string_at(0));

      if (search_condition.empty())
        throw shcore::Exception::argument_error("Requires a search condition.");

      set_filter(search_condition);

      // Updates the exposed functions
      update_functions("remove");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("remove"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionRemove &CollectionRemove::set_filter(const std::string& filter) {
  message_.set_allocated_criteria(
      ::mysqlx::parser::parse_collection_filter(filter, &_placeholders));

  return *this;
}

// Documentation of sort function
REGISTER_HELP(COLLECTIONREMOVE_SORT_BRIEF,
              "Sets the order in which the deletion should be done.");
REGISTER_HELP(
    COLLECTIONREMOVE_SORT_PARAM,
    "@param sortExprStr: A list of expression strings defining a sort criteria, "
    "the deletion will be done following the order defined by this criteria.");
REGISTER_HELP(COLLECTIONREMOVE_SORT_RETURNS,
              "@returns This CollectionRemove object.");
REGISTER_HELP(
    COLLECTIONREMOVE_SORT_DETAIL,
    "The elements of sortExprStr list are strings defining the column name on which "
    "the sorting will be based in the form of 'columnIdentifier [ ASC | DESC ]'.");
REGISTER_HELP(
    COLLECTIONREMOVE_SORT_DETAIL1,
    "If no order criteria is specified, ascending will be used by default.");
REGISTER_HELP(
    COLLECTIONREMOVE_SORT_DETAIL2,
    "This method is usually used in combination with limit to fix the amount of documents to be deleted.");

/**
* $(COLLECTIONREMOVE_SORT_BRIEF)
*
* $(COLLECTIONREMOVE_SORT_PARAM)
*
* $(COLLECTIONREMOVE_SORT_RETURNS)
*
* $(COLLECTIONREMOVE_SORT_DETAIL)
*
* $(COLLECTIONREMOVE_SORT_DETAIL1)
*
* $(COLLECTIONREMOVE_SORT_DETAIL2)
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - remove(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionRemove CollectionRemove::sort(List sortExprStr) {}
#elif DOXYGEN_PY
CollectionRemove CollectionRemove::sort(list sortExprStr) {}
#endif
//@}
shcore::Value CollectionRemove::sort(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("sort").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    for (auto &field : fields)
      ::mysqlx::parser::parse_collection_sort_column(*message_.mutable_order(),
                                                     field);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("sort"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of limit function
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_BRIEF,
              "Sets a limit for the documents to be deleted.");
REGISTER_HELP(
    COLLECTIONREMOVE_LIMIT_PARAM,
    "@param numberOfDocs the number of documents to affect in the remove execution.");
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_RETURNS,
              "@returns This CollectionRemove object.");
REGISTER_HELP(
    COLLECTIONREMOVE_LIMIT_DETAIL,
    "This method is usually used in combination with sort to fix the amount of documents to be deleted.");

/**
* $(COLLECTIONREMOVE_LIMIT_BRIEF)
*
* $(COLLECTIONREMOVE_LIMIT_PARAM)
*
* $(COLLECTIONREMOVE_LIMIT_RETURNS)
*
* $(COLLECTIONREMOVE_LIMIT_DETAIL)
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - remove(String searchCondition)
* - sort(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
//@{
#if DOXYGEN_JS
CollectionRemove CollectionRemove::limit(Integer numberOfDocs) {}
#elif DOXYGEN_PY
CollectionRemove CollectionRemove::limit(int numberOfDocs) {}
#endif
//@}
shcore::Value CollectionRemove::limit(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("limit").c_str());

  try {
    message_.mutable_limit()->set_row_count(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("limit"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of function
REGISTER_HELP(
    COLLECTIONREMOVE_BIND_BRIEF,
    "Binds a value to a specific placeholder used on this CollectionRemove object.");
REGISTER_HELP(
    COLLECTIONREMOVE_BIND_PARAM1,
    "@param name: The name of the placeholder to which the value will be bound.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_PARAM2,
              "@param value: The value to be bound on the placeholder.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_RETURNS,
              "@returns This CollectionRemove object.");
REGISTER_HELP(
    COLLECTIONREMOVE_BIND_DETAIL,
    "An error will be raised if the placeholder indicated by name does not exist.");
REGISTER_HELP(
    COLLECTIONREMOVE_BIND_DETAIL1,
    "This function must be called once for each used placeohlder or an error will be raised when the execute method is called.");

/**
* $(COLLECTIONREMOVE_BIND_BRIEF)
*
* $(COLLECTIONREMOVE_BIND_PARAM1)
* $(COLLECTIONREMOVE_BIND_PARAM2)
*
* $(COLLECTIONREMOVE_BIND_RETURNS)
*
* $(COLLECTIONREMOVE_BIND_DETAIL)
*
* $(COLLECTIONREMOVE_BIND_DETAIL1)
*
* #### Method Chaining
*
* This function can be invoked multiple times right before calling execute:
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*
* \sa Usage examples at execute().
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionRemove::bind(String name, Value value) {}
#elif DOXYGEN_PY
CollectionFind CollectionRemove::bind(str name, Value value) {}
#endif
//@}
shcore::Value CollectionRemove::bind_(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("bind").c_str());

  try {
    bind_value(args.string_at(0), args[1]);

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("bind"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionRemove &CollectionRemove::bind(const std::string &name,
                                         shcore::Value value) {
  bind_value(name, value);
  return *this;
}

// Documentation of function
REGISTER_HELP(
    COLLECTIONREMOVE_EXECUTE_BRIEF,
    "Executes the document deletion with the configured filter and limit.");
REGISTER_HELP(
    COLLECTIONREMOVE_EXECUTE_RETURNS,
    "@returns Result A Result object that can be used to retrieve the results of the deletion operation.");

/**
* $(COLLECTIONREMOVE_EXECUTE_BRIEF)
*
* $(COLLECTIONREMOVE_EXECUTE_RETURNS)
*
* #### Method Chaining
*
* This function can be invoked after any other function on this class.
*/
//@{
#if DOXYGEN_JS
/**
*
* #### Examples
* \dontinclude "js_devapi/scripts/mysqlx_collection_remove.js"
* \skip //@ CollectionRemove: remove under condition
* \until print('Records Left:', docs.length, '\n');
* \until print('Records Left:', docs.length, '\n');
* \until print('Records Left:', docs.length, '\n');
*/
Result CollectionRemove::execute() {}
#elif DOXYGEN_PY
/**
*
* #### Examples
* \dontinclude "py_devapi/scripts/mysqlx_collection_remove.py"
* \skip #@ CollectionRemove: remove under condition
* \until print 'Records Left:', len(docs), '\n'
* \until print 'Records Left:', len(docs), '\n'
* \until print 'Records Left:', len(docs), '\n'
*/
Result CollectionRemove::execute() {}
#endif
//@}
shcore::Value CollectionRemove::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());
  shcore::Value ret_val;
  try {
    ret_val = execute();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return ret_val;
}

shcore::Value CollectionRemove::execute() {
  std::unique_ptr<mysqlsh::mysqlx::Result> result;

  MySQL_timer timer;
  insert_bound_values(message_.mutable_args());
  timer.start();
  result.reset(new mysqlx::Result(
      safe_exec([this]() { return session()->session()->execute_crud(message_); })));
  timer.end();
  result->set_execution_time(timer.raw_duration());

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
}  // namespace mysqlx
}  // namespace mysqlsh
