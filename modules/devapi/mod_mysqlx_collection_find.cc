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
#include "modules/devapi/mod_mysqlx_collection_find.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "mysqlxtest/common/expr_parser.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"
#include "utils/utils_time.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

CollectionFind::CollectionFind(std::shared_ptr<Collection> owner)
    : Collection_crud_definition(
          std::static_pointer_cast<DatabaseObject>(owner)) {
  // Exposes the methods available for chaining
  add_method("find", std::bind(&CollectionFind::find, this, _1), "data");
  add_method("fields", std::bind(&CollectionFind::fields, this, _1), "data");
  add_method("groupBy", std::bind(&CollectionFind::group_by, this, _1), "data");
  add_method("having", std::bind(&CollectionFind::having, this, _1), "data");
  add_method("sort", std::bind(&CollectionFind::sort, this, _1), "data");
  add_method("skip", std::bind(&CollectionFind::skip, this, _1), "data");
  add_method("limit", std::bind(&CollectionFind::limit, this, _1), "data");
  add_method("bind", std::bind(&CollectionFind::bind, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("find", "");
  register_dynamic_function("fields", "find");
  register_dynamic_function("groupBy", "find, fields");
  register_dynamic_function("having", "groupBy");
  register_dynamic_function("sort", "find, fields, groupBy, having");
  register_dynamic_function("limit", "find, fields, groupBy, having, sort");
  register_dynamic_function("skip", "limit");
  register_dynamic_function(
      "bind", "find, fields, groupBy, having, sort, skip, limit, bind");
  register_dynamic_function(
      "execute", "find, fields, groupBy, having, sort, skip, limit, bind");
  register_dynamic_function(
      "__shell_hook__",
      "find, fields, groupBy, having, sort, skip, limit, bind");

  // Initial function update
  update_functions("");
}

REGISTER_HELP(
    COLLECTIONFIND_FIND_BRIEF,
    "Sets the search condition to identify the Documents to be retrieved from the owner Collection.");
REGISTER_HELP(
    COLLECTIONFIND_FIND_PARAM,
    "@param searchCondition Optional String expression defining the condition to be used on the selection.");
REGISTER_HELP(COLLECTIONFIND_FIND_SYNTAX, "find()");
REGISTER_HELP(COLLECTIONFIND_FIND_SYNTAX1, "find(searchCondition)");
REGISTER_HELP(COLLECTIONFIND_FIND_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(
    COLLECTIONFIND_FIND_DETAIL,
    "Sets the search condition to identify the Documents to be retrieved from the owner Collection. "
    "If the search condition is not specified the find operation will be executed over all the documents in the collection.");
REGISTER_HELP(COLLECTIONFIND_FIND_DETAIL1,
              "The search condition supports parameter binding.");

/**
* $(COLLECTIONFIND_FIND_BRIEF)
*
* $(COLLECTIONFIND_FIND_PARAM)
*
* $(COLLECTIONFIND_FIND_RETURNS)
*
* $(COLLECTIONFIND_FIND_DETAIL)
*
* $(COLLECTIONFIND_FIND_DETAIL1)
*
* #### Method Chaining
*
* After this function invocation, the following functions can be invoked:
*
* - fields(List projectedSearchExprStr)
* - groupBy(List searchExprStr)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::find(String searchCondition) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::find(str searchCondition) {}
#endif
//@}
shcore::Value CollectionFind::find(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(0, 1, "CollectionFind.find");

  std::shared_ptr<Collection> collection(
      std::static_pointer_cast<Collection>(_owner.lock()));

  if (collection) {
    try {
      std::string search_condition;
      if (args.size())
        search_condition = args.string_at(0);

      _find_statement.reset(new ::mysqlx::FindStatement(
          collection->_collection_impl->find(search_condition)));

      // Updates the exposed functions
      update_functions("find");
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.find");
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(
    COLLECTIONFIND_FIELDS_BRIEF,
    "Sets the fields to be retrieved from each document matching the criteria on this find operation.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_PARAM,
    "@param fieldDefinition Definition of the fields to be retrieved.");
REGISTER_HELP(COLLECTIONFIND_FIELDS_SYNTAX, "fields(field[, field, ...])");
REGISTER_HELP(COLLECTIONFIND_FIELDS_SYNTAX1, "fields(fieldList)");
REGISTER_HELP(COLLECTIONFIND_FIELDS_SYNTAX2, "fields(projectionExpression)");
REGISTER_HELP(COLLECTIONFIND_FIELDS_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL,
    "This function sets the fields to be retrieved from each document matching the criteria on this find operation.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL1,
    "A field is defined as a string value containing an expression defining the field to be retrieved.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL2,
    "The fields to be retrieved can be set using any of the next methods:");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL3,
    "@li Passing each field definition as an individual string parameter.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL4,
    "@li Passing a list of strings containing the field definitions.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL5,
    "@li Passing a JSON expression representing a document projection to be generated.");

/**
* $(COLLECTIONFIND_FIELDS_BRIEF)
*
* $(COLLECTIONFIND_FIELDS_PARAM)
*
* $(COLLECTIONFIND_FIELDS_RETURNS)
*
* $(COLLECTIONFIND_FIELDS_DETAIL)
*
* $(COLLECTIONFIND_FIELDS_DETAIL1)
*
* $(COLLECTIONFIND_FIELDS_DETAIL2)
*
* $(COLLECTIONFIND_FIELDS_DETAIL3)
* $(COLLECTIONFIND_FIELDS_DETAIL4)
* $(COLLECTIONFIND_FIELDS_DETAIL5)
*
* #### Method Chaining
*
* This function can be invoked only once after:
* - find(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - groupBy(List searchExprStr)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::fields(
    String fieldDefinition[, String fieldDefinition, ...]) {}
CollectionFind CollectionFind::fields(List fieldDefinition) {}
CollectionFind CollectionFind::fields(DocExpression fieldDefinition);
#elif DOXYGEN_PY
CollectionFind CollectionFind::fields(
    str fieldDefinition[, str fieldDefinition, ...]) {}
CollectionFind CollectionFind::fields(list fieldDefinition) {}
CollectionFind CollectionFind::fields(DocExpression fieldDefinition);
#endif
//@}

shcore::Value CollectionFind::fields(const shcore::Argument_list &args) {
  args.ensure_at_least(1, "CollectionFind.fields");

  try {
    if (args.size() == 1 && args[0].type != String) {
      if (args[0].type == String || args[0].type == Array) {
        std::vector<std::string> fields;

        if (args[0].type == Array)
          parse_string_list(args, fields);
        else
          fields.push_back(args.string_at(0));

        if (fields.size() == 0)
          throw shcore::Exception::argument_error(
              "Field selection criteria can not be empty");

        _find_statement->fields(fields);
      } else if (args[0].type == Object &&
                 args[0].as_object()->class_name() == "Expression") {
        std::shared_ptr<mysqlx::Expression> expression =
            std::static_pointer_cast<mysqlx::Expression>(args[0].as_object());
        ::mysqlx::Expr_parser parser(expression->get_data());
        std::unique_ptr<Mysqlx::Expr::Expr> expr_obj(parser.expr());

        // Parsing is done just to validate it is a valid JSON expression
        if (expr_obj->type() == Mysqlx::Expr::Expr_Type_OBJECT)
          _find_statement->fields(expression->get_data());
        else
          throw shcore::Exception::argument_error(
              "Argument #1 is expected to be a JSON expression");
      } else {
        throw shcore::Exception::argument_error(
            "Argument #1 is expected to be a string, array of strings or a JSON expression");
      }

      update_functions("fields");
    } else {
      std::vector<std::string> fields;
      parse_string_list(args, fields);
      _find_statement->fields(fields);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.fields");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(COLLECTIONFIND_GROUPBY_BRIEF,
              "Sets a grouping criteria for the resultset.");
REGISTER_HELP(
    COLLECTIONFIND_GROUPBY_PARAM,
    "@param groupCriteria A list of string expressions defining the grouping criteria.");
REGISTER_HELP(COLLECTIONFIND_GROUPBY_SYNTAX, "groupBy(fieldList)");
REGISTER_HELP(COLLECTIONFIND_GROUPBY_SYNTAX1, "groupBy(field[, field, ...])");
REGISTER_HELP(COLLECTIONFIND_GROUPBY_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_GROUPBY_DETAIL,
              "Sets a grouping criteria for the resultset.");
/**
* $(COLLECTIONFIND_GROUPBY_BRIEF)
*
* $(COLLECTIONFIND_GROUPBY_PARAM)
*
* $(COLLECTIONFIND_GROUPBY_RETURNS)
*
* $(COLLECTIONFIND_GROUPBY_DETAIL)
*
* #### Method Chaining
*
* This function can be only once invoked after:
* - find(String searchCondition)
* - fields(List projectedSearchExprStr)
*
* After this function invocation the following functions can be invoked:
*
* - having(String searchCondition)
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::groupBy(List groupCriteria) {}
CollectionFind CollectionFind::groupBy(
    String groupCriteria[, String groupCriteria, ...]) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::group_by(list groupCriteria) {}
CollectionFind CollectionFind::group_by(
    str groupCriteria[, str groupCriteria, ...]) {}
#endif
//@}
shcore::Value CollectionFind::group_by(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("groupBy").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error(
          "Grouping criteria can not be empty");

    _find_statement->groupBy(fields);

    update_functions("groupBy");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("groupBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(
    COLLECTIONFIND_HAVING_BRIEF,
    "Sets a condition for records to be considered in agregate function operations.");
REGISTER_HELP(
    COLLECTIONFIND_HAVING_PARAM,
    "@param searchCondition A condition on the agregate functions used on the grouping criteria.");
REGISTER_HELP(COLLECTIONFIND_HAVING_SYNTAX, "having(searchCondition)");
REGISTER_HELP(COLLECTIONFIND_HAVING_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(
    COLLECTIONFIND_HAVING_DETAIL,
    "Sets a condition for records to be considered in agregate function operations.");

/**
* $(COLLECTIONFIND_HAVING_BRIEF)
*
* $(COLLECTIONFIND_HAVING_PARAM)
*
* $(COLLECTIONFIND_HAVING_RETURNS)
*
* $(COLLECTIONFIND_HAVING_DETAIL)
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - groupBy(List searchExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - sort(List sortExprStr)
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::having(String searchCondition) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::having(str searchCondition) {}
#endif
//@}
shcore::Value CollectionFind::having(const shcore::Argument_list &args) {
  args.ensure_count(1, "CollectionFind.having");

  try {
    _find_statement->having(args.string_at(0));

    update_functions("having");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.having");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(COLLECTIONFIND_SORT_BRIEF,
              "Sets the sorting criteria to be used on the DocResult.");
REGISTER_HELP(
    COLLECTIONFIND_SORT_PARAM,
    "@param sortCriteria The sort criteria for the returned documents.");
REGISTER_HELP(COLLECTIONFIND_SORT_SYNTAX,
              "sort(sortCriterion[, sortCriterion, ...])");
REGISTER_HELP(COLLECTIONFIND_SORT_SYNTAX1, "sort(sortCritera)");
REGISTER_HELP(COLLECTIONFIND_SORT_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(
    COLLECTIONFIND_SORT_DETAIL,
    "If used the CollectionFind operation will return the records sorted with the defined criteria.");
REGISTER_HELP(COLLECTIONFIND_SORT_DETAIL1,
              "Every defined sort criterion sollows the next format:");
REGISTER_HELP(COLLECTIONFIND_SORT_DETAIL2, "name [ ASC | DESC ]");
REGISTER_HELP(COLLECTIONFIND_SORT_DETAIL3,
              "ASC is used by default if the sort order is not specified.");

/**
* $(COLLECTIONFIND_SORT_BRIEF)
*
* $(COLLECTIONFIND_SORT_PARAM)
*
* $(COLLECTIONFIND_SORT_RETURNS)
*
* $(COLLECTIONFIND_SORT_DETAIL)
*
* $(COLLECTIONFIND_SORT_DETAIL1)
*
* $(COLLECTIONFIND_SORT_DETAIL2)
*
* $(COLLECTIONFIND_SORT_DETAIL3)
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - find(String searchCondition)
* - fields(List projectedSearchExprStr)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
*
* After this function invocation, the following functions can be invoked:
*
* - limit(Integer numberOfRows)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::sort(List sortCriteria) {}
CollectionFind CollectionFind::sort(
    String sortCriteria[, String sortCriteria, ...]) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::sort(list sortCriteria) {}
CollectionFind CollectionFind::sort(str sortCriteria[, str sortCriteria, ...]) {
}
#endif
//@}
shcore::Value CollectionFind::sort(const shcore::Argument_list &args) {
  args.ensure_at_least(1, "CollectionFind.sort");

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    _find_statement->sort(fields);

    update_functions("sort");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.sort");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(
    COLLECTIONFIND_LIMIT_BRIEF,
    "Sets the maximum number of documents to be returned on the find operation.");
REGISTER_HELP(
    COLLECTIONFIND_LIMIT_PARAM,
    "@param numberOfRows The maximum number of documents to be retrieved.");
REGISTER_HELP(COLLECTIONFIND_LIMIT_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_LIMIT_SYNTAX, "limit(numberOfRows)");
REGISTER_HELP(
    COLLECTIONFIND_LIMIT_DETAIL,
    "If used, the CollectionFind operation will return at most numberOfRows documents.");

/**
* $(COLLECTIONFIND_LIMIT_BRIEF)
*
* $(COLLECTIONFIND_LIMIT_PARAM)
*
* $(COLLECTIONFIND_LIMIT_RETURNS)
*
* $(COLLECTIONFIND_LIMIT_DETAIL)
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - find(String searchCondition)
* - fields(List projectedSearchExprStr)
* - groupBy(List searchExprStr)
* - having(String searchCondition)
* - sort(List sortExprStr)
*
* After this function invocation, the following functions can be invoked:
*
* - skip(Integer limitOffset)
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::limit(Integer numberOfRows) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::limit(int numberOfRows) {}
#endif
//@}
shcore::Value CollectionFind::limit(const shcore::Argument_list &args) {
  args.ensure_count(1, "CollectionFind.limit");

  try {
    _find_statement->limit(args.uint_at(0));

    update_functions("limit");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.limit");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(
    COLLECTIONFIND_SKIP_BRIEF,
    "Sets number of documents to skip on the resultset when a limit has been defined.");
REGISTER_HELP(
    COLLECTIONFIND_SKIP_PARAM,
    "@param offset The number of documents to skip before start including them on the DocResult.");
REGISTER_HELP(COLLECTIONFIND_SKIP_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_SKIP_SYNTAX, "skip(offset)");
REGISTER_HELP(
    COLLECTIONFIND_SKIP_DETAIL,
    "If used, the first 'offset' records will not be included on the result.");

/**
* $(COLLECTIONFIND_SKIP_BRIEF)
*
* $(COLLECTIONFIND_SKIP_PARAM)
*
* $(COLLECTIONFIND_SKIP_RETURNS)
*
* $(COLLECTIONFIND_SKIP_DETAIL)
*
* #### Method Chaining
*
* This function can be invoked only once after:
*
* - limit(Integer numberOfRows)
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::skip(Integer offset) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::skip(int offset) {}
#endif
//@}
shcore::Value CollectionFind::skip(const shcore::Argument_list &args) {
  args.ensure_count(1, "CollectionFind.skip");

  try {
    _find_statement->skip(args.uint_at(0));

    update_functions("skip");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.skip");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(
    COLLECTIONFIND_BIND_BRIEF,
    "Binds a value to a specific placeholder used on this CollectionFind object.");
REGISTER_HELP(
    COLLECTIONFIND_BIND_PARAM,
    "@param name The name of the placeholder to which the value will be bound.");
REGISTER_HELP(COLLECTIONFIND_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(COLLECTIONFIND_BIND_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_BIND_SYNTAX,
              "bind(placeHolder, value)[.bind(...)]");
REGISTER_HELP(
    COLLECTIONFIND_BIND_DETAIL,
    "Binds a value to a specific placeholder used on this CollectionFind object.");
REGISTER_HELP(
    COLLECTIONFIND_BIND_DETAIL1,
    "An error will be raised if the placeholder indicated by name does not exist.");
REGISTER_HELP(
    COLLECTIONFIND_BIND_DETAIL2,
    "This function must be called once for each used placeohlder or an error will be "
    "raised when the execute method is called.");

/**
* $(COLLECTIONFIND_BIND_BRIEF)
*
* $(COLLECTIONFIND_BIND_PARAM)
* $(COLLECTIONFIND_BIND_PARAM1)
*
* $(COLLECTIONFIND_BIND_RETURNS)
*
* $(COLLECTIONFIND_BIND_DETAIL)
*
* $(COLLECTIONFIND_BIND_DETAIL1)
*
* $(COLLECTIONFIND_BIND_DETAIL2)
*
* #### Method Chaining
*
* This function can be invoked multiple times right before calling execute:
*
* After this function invocation, the following functions can be invoked:
*
* - bind(String name, Value value)
* - execute()
*/
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::bind(String name, Value value) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::bind(str name, Value value) {}
#endif
//@}
shcore::Value CollectionFind::bind(const shcore::Argument_list &args) {
  args.ensure_count(2, "CollectionFind.bind");

  try {
    _find_statement->bind(args.string_at(0), map_document_value(args[1]));

    update_functions("bind");
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.bind");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP(COLLECTIONFIND_EXECUTE_BRIEF,
              "Executes the find operation with all the configured options.");
REGISTER_HELP(
    COLLECTIONFIND_EXECUTE_RETURNS,
    "@returns A DocResult object that can be used to traverse the documents returned by this operation.");
REGISTER_HELP(COLLECTIONFIND_EXECUTE_SYNTAX, "execute()");

/**
* $(COLLECTIONFIND_EXECUTE_BRIEF)
*
* $(COLLECTIONFIND_EXECUTE_RETURNS)
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
* #### Retrieving All Documents
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: All
* Records
*
* #### Filtering
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Filtering
*
* #### Field Selection
* Using a field selection list
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Field
* Selection List
*
* Using separate field selection parameters
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Field
* Selection Parameters
*
* Using a projection expression
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Field
* Selection Projection
*
* #### Sorting
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Sorting
*
* #### Using Limit and Skip
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Limit and
* Skip
*
* #### Parameter Binding
* \snippet js_devapi/scripts/mysqlx_collection_find.js CollectionFind: Parameter
* Binding
*/
DocResult CollectionFind::execute() {}
#elif DOXYGEN_PY
/**
* #### Retrieving All Documents
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: All
* Records
*
* #### Filtering
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Filtering
*
* #### Field Selection
* Using a field selection list
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Field
* Selection List
*
* Using separate field selection parameters
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Field
* Selection Parameters
*
* Using a projection expression
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Field
* Selection Projection
*
* #### Sorting
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Sorting
*
* #### Using Limit and Skip
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Limit and
* Skip
*
* #### Parameter Binding
* \snippet py_devapi/scripts/mysqlx_collection_find.py CollectionFind: Parameter
* Binding
*/
DocResult CollectionFind::execute() {}
#endif
//@}
shcore::Value CollectionFind::execute(const shcore::Argument_list &args) {
  mysqlx::DocResult *result = NULL;

  try {
    args.ensure_count(0, "CollectionFind.execute");
    MySQL_timer timer;
    timer.start();
    result = new mysqlx::DocResult(
        std::shared_ptr< ::mysqlx::Result>(_find_statement->execute()));
    timer.end();
    result->set_execution_time(timer.raw_duration());
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.execute");

  return result ? shcore::Value::wrap(result) : shcore::Value::Null();
}
