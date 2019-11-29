/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/libs/utils/utils_string.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"

namespace mysqlsh {
namespace mysqlx {

using shcore::Value;
using std::placeholders::_1;

// Documentation of CollectionRemove class
REGISTER_HELP_CLASS(CollectionRemove, mysqlx);
REGISTER_HELP(COLLECTIONREMOVE_BRIEF,
              "Operation to delete documents on a Collection.");
REGISTER_HELP(COLLECTIONREMOVE_DETAIL,
              "A CollectionRemove object represents an operation to remove "
              "documents on a Collection, it is created through the "
              "<b>remove</b> function on the <b>Collection</b> class.");
CollectionRemove::CollectionRemove(std::shared_ptr<Collection> owner)
    : Collection_crud_definition(
          std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::DOCUMENT);

  auto limit_id = F::limit;
  auto bind_id = F::bind;
  // Exposes the methods available for chaining
  add_method("remove", std::bind(&CollectionRemove::remove, this, _1), "data");
  add_method("sort", std::bind(&CollectionRemove::sort, this, _1), "data");
  add_method("limit",
             std::bind(&CollectionRemove::limit, this, _1, limit_id, false),
             "data");
  add_method("bind", std::bind(&CollectionRemove::bind_, this, _1, bind_id),
             "data");

  // Registers the dynamic function behavior
  register_dynamic_function(
      F::remove, F::sort | F::limit | F::bind | F::execute | F::__shell_hook__);
  register_dynamic_function(F::sort);
  register_dynamic_function(F::limit, K_ENABLE_NONE, F::sort);
  register_dynamic_function(F::bind, K_ENABLE_NONE, F::limit | F::sort,
                            K_ALLOW_REUSE);
  register_dynamic_function(F::execute, F::limit, K_DISABLE_NONE,
                            K_ALLOW_REUSE);

  // Initial function update
  enable_function(F::remove);
}

shcore::Value CollectionRemove::this_object() {
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of remove function
REGISTER_HELP_FUNCTION(remove, CollectionRemove);
REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_BRIEF,
    "Sets the search condition to filter the documents to be deleted from "
    "the owner Collection.");
REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_PARAM,
    "@param searchCondition An expression to filter the documents to be "
    "deleted.");
REGISTER_HELP(COLLECTIONREMOVE_REMOVE_RETURNS,
              "@returns This CollectionRemove object.");

REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_DETAIL,
    "Creates a handler for the deletion of documents on the collection.");

REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_DETAIL1,
    "A condition must be provided to this function, all the documents "
    "matching the condition will be removed from the collection.");

REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_DETAIL2,
    "To delete all the documents, set a condition that always evaluates to "
    "true, for example '1'.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL3,
              "The searchCondition supports parameter binding.");

REGISTER_HELP(COLLECTIONREMOVE_REMOVE_DETAIL4,
              "This function is called automatically when "
              "Collection.remove(searchCondition) is called.");

REGISTER_HELP(
    COLLECTIONREMOVE_REMOVE_DETAIL5,
    "The actual deletion of the documents will occur only when the execute() "
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
 */
#if DOXYGEN_JS
/**
 * - sort(List sortCriteria),
 *   <a class="el" href="#a63a0fc78136e8465253f81b16ab6d2bb">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - sort(list sortCriteria),
 *   <a class="el" href="#a4bcce795b4a78d999cfb00284512bc03">
 *   sort(str sortCriterion[, str sortCriterion, ...])</a>
 * - limit(int numberOfRows)
 * - bind(str name, Value value)
 */
#endif
/**
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
      std::string search_condition = shcore::str_strip(args.string_at(0));

      if (search_condition.empty())
        throw shcore::Exception::argument_error("Requires a search condition.");

      set_filter(search_condition);

      // Updates the exposed functions
      update_functions(F::remove);
      reset_prepared_statement();
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("remove"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionRemove &CollectionRemove::set_filter(const std::string &filter) {
  message_.set_allocated_criteria(
      ::mysqlx::parser::parse_collection_filter(filter, &_placeholders));

  return *this;
}

// Documentation of sort function
REGISTER_HELP_FUNCTION(sort, CollectionRemove);
REGISTER_HELP(COLLECTIONREMOVE_SORT_BRIEF,
              "Sets the order in which the deletion should be done.");
REGISTER_HELP(COLLECTIONREMOVE_SORT_SIGNATURE, "(sortCriteriaList)");
REGISTER_HELP(COLLECTIONREMOVE_SORT_SIGNATURE1,
              "(sortCriterion[, sortCriterion, ...])");
REGISTER_HELP(COLLECTIONREMOVE_SORT_RETURNS,
              "@returns This CollectionRemove object.");
REGISTER_HELP(COLLECTIONREMOVE_SORT_DETAIL,
              "Every defined sort criterion follows the format:");
REGISTER_HELP(COLLECTIONREMOVE_SORT_DETAIL1, "name [ ASC | DESC ]");
REGISTER_HELP(COLLECTIONREMOVE_SORT_DETAIL2,
              "ASC is used by default if the sort order is not specified.");
REGISTER_HELP(COLLECTIONREMOVE_SORT_DETAIL3,
              "This method is usually used in combination with limit to fix "
              "the amount of documents to be deleted.");

/**
 * $(COLLECTIONREMOVE_SORT_BRIEF)
 *
 * $(COLLECTIONREMOVE_SORT_RETURNS)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL1)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL2)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL3)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 */
#if DOXYGEN_JS
/**
 * - remove(String searchCondition)
 */
#elif DOXYGEN_PY
/**
 * - remove(str searchCondition)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - limit(int numberOfRows)
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
CollectionRemove CollectionRemove::sort(List sortCriteria) {}
CollectionRemove CollectionRemove::sort(
    String sortCriterion[, String sortCriterion, ...]) {}
#elif DOXYGEN_PY
CollectionRemove CollectionRemove::sort(list sortCriteria) {}
CollectionRemove CollectionRemove::sort(
    str sortCriterion[, str sortCriterion, ...]) {}
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

    update_functions(F::sort);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("sort"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of limit function
REGISTER_HELP_FUNCTION(limit, CollectionRemove);
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_BRIEF,
              "Sets a limit for the documents to be deleted.");
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_PARAM,
              "@param numberOfDocs the number of documents to affect in the "
              "remove execution.");
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_RETURNS,
              "@returns This CollectionRemove object.");
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_DETAIL,
              "This method is usually used in combination with sort to fix the "
              "amount of documents to be deleted.");
REGISTER_HELP(COLLECTIONREMOVE_LIMIT_DETAIL1, "${LIMIT_EXECUTION_MODE}");

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
 */
#if DOXYGEN_JS
/**
 * - remove(String searchCondition)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a63a0fc78136e8465253f81b16ab6d2bb">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 */
#elif DOXYGEN_PY
/**
 * - remove(str searchCondition)
 * - sort(list sortCriteria),
 *   <a class="el" href="#a4bcce795b4a78d999cfb00284512bc03">
 *   sort(str sortCriterion[, str sortCriterion, ...])</a>
 */
#endif
/**
 *
 * $(LIMIT_EXECUTION_MODE)
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - bind(str name, Value value)
 */
#endif
/**
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

// Documentation of function
REGISTER_HELP_FUNCTION(bind, CollectionRemove);
REGISTER_HELP(COLLECTIONREMOVE_BIND_BRIEF,
              "Binds a value to a specific placeholder used on this "
              "CollectionRemove object.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_PARAM,
              "@param name The name of the placeholder to which the value "
              "will be bound.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_RETURNS,
              "@returns This CollectionRemove object.");
REGISTER_HELP(
    COLLECTIONREMOVE_BIND_DETAIL,
    "Binds the given value to the placeholder with the specified name.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_DETAIL1,
              "An error will be raised if the placeholder indicated by name "
              "does not exist.");
REGISTER_HELP(COLLECTIONREMOVE_BIND_DETAIL2,
              "This function must be called once for each used placeholder or "
              "an error will be raised when the execute() method is called.");

/**
 * $(COLLECTIONREMOVE_BIND_BRIEF)
 *
 * $(COLLECTIONREMOVE_BIND_PARAM)
 * $(COLLECTIONREMOVE_BIND_PARAM1)
 *
 * $(COLLECTIONREMOVE_BIND_RETURNS)
 *
 * $(COLLECTIONREMOVE_BIND_DETAIL)
 *
 * $(COLLECTIONREMOVE_BIND_DETAIL1)
 *
 * $(COLLECTIONREMOVE_BIND_DETAIL2)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times right before calling execute().
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - bind(str name, Value value)
 */
#endif
/**
 * - execute()
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
CollectionRemove CollectionRemove::bind(String name, Value value) {}
#elif DOXYGEN_PY
CollectionRemove CollectionRemove::bind(str name, Value value) {}
#endif
//@}

// Documentation of function
REGISTER_HELP_FUNCTION(execute, CollectionRemove);
REGISTER_HELP(
    COLLECTIONREMOVE_EXECUTE_BRIEF,
    "Executes the document deletion with the configured filter and limit.");
REGISTER_HELP(COLLECTIONREMOVE_EXECUTE_RETURNS,
              "@returns Result A Result object that can be used to retrieve "
              "the results of the deletion operation.");

/**
 * $(COLLECTIONREMOVE_EXECUTE_BRIEF)
 *
 * $(COLLECTIONREMOVE_EXECUTE_RETURNS)
 *
 * #### Method Chaining
 *
 * This function can be invoked after any other function on this class.
 *
 * ### Examples
 */
//@{
#if DOXYGEN_JS
/**
 *
 * #### Remove under condition
 *
 * \snippet mysqlx_collection_remove.js CollectionRemove: remove under condition
 *
 * #### Remove with binding
 *
 * \snippet mysqlx_collection_remove.js CollectionRemove: remove with binding
 *
 * #### Full remove
 *
 * \snippet mysqlx_collection_remove.js CollectionRemove: full remove
 */
Result CollectionRemove::execute() {}
#elif DOXYGEN_PY
/**
 *
 * #### Remove under condition
 *
 * \snippet mysqlx_collection_remove.py CollectionRemove: remove under condition
 *
 * #### Remove with binding
 *
 * \snippet mysqlx_collection_remove.py CollectionRemove: remove with binding
 *
 * #### Full remove
 *
 * \snippet mysqlx_collection_remove.py CollectionRemove: full remove
 */
Result CollectionRemove::execute() {}
#endif
//@}
shcore::Value CollectionRemove::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());
  shcore::Value ret_val;
  try {
    ret_val = execute();
    update_functions(F::execute);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return ret_val;
}

void CollectionRemove::set_prepared_stmt() {
  m_prep_stmt.mutable_stmt()->set_type(
      Mysqlx::Prepare::Prepare_OneOfMessage_Type_DELETE);
  message_.clear_args();
  update_limits();
  *m_prep_stmt.mutable_stmt()->mutable_delete_() = message_;
}

#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
shcore::Value CollectionRemove::execute() {
  std::unique_ptr<mysqlsh::mysqlx::Result> result;

  result.reset(new mysqlx::Result(safe_exec([this]() {
    update_limits();
    insert_bound_values(message_.mutable_args());
    return session()->session()->execute_crud(message_);
  })));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
#endif

}  // namespace mysqlx
}  // namespace mysqlsh
