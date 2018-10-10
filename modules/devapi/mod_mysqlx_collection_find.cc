/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/devapi/mod_mysqlx_collection_find.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "modules/devapi/base_constants.h"
#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "scripting/common.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
using namespace shcore;
namespace mysqlsh {
namespace mysqlx {

REGISTER_HELP_CLASS(CollectionFind, mysqlx);
REGISTER_HELP(COLLECTIONFIND_BRIEF,
              "Operation to retrieve documents from a Collection.");
REGISTER_HELP(COLLECTIONFIND_DETAIL,
              "A CollectionFind object represents an operation to retreve "
              "documents from a Collection, it is created through the "
              "<b>find</b> function on the <b>Collection</b> class.");
CollectionFind::CollectionFind(std::shared_ptr<Collection> owner)
    : Collection_crud_definition(
          std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::DOCUMENT);

  auto limit_id = F::limit;
  auto offset_id = F::offset;
  auto bind_id = F::bind;
  // Exposes the methods available for chaining
  add_method("find", std::bind(&CollectionFind::find, this, _1), "data");
  add_method("fields", std::bind(&CollectionFind::fields, this, _1), "data");
  add_method("groupBy", std::bind(&CollectionFind::group_by, this, _1), "data");
  add_method("having", std::bind(&CollectionFind::having, this, _1), "data");
  add_method("sort", std::bind(&CollectionFind::sort, this, _1), "data");
  add_method("skip",
             std::bind(&CollectionFind::offset, this, _1, offset_id, "skip"),
             "data");
  add_method("offset",
             std::bind(&CollectionFind::offset, this, _1, offset_id, "offset"),
             "data");
  add_method("limit",
             std::bind(&CollectionFind::limit, this, _1, limit_id, true),
             "data");
  add_method("lockShared", std::bind(&CollectionFind::lock_shared, this, _1));
  add_method("lockExclusive",
             std::bind(&CollectionFind::lock_exclusive, this, _1));
  add_method("bind", std::bind(&CollectionFind::bind_, this, _1, bind_id),
             "data");

  // Registers the dynamic function behavior
  register_dynamic_function(
      F::find, F::fields | F::groupBy | F::sort | F::limit | F::lockShared |
                   F::lockExclusive | F::bind | F::execute | F::__shell_hook__);
  register_dynamic_function(F::fields);
  register_dynamic_function(F::groupBy, F::having, F::fields);
  register_dynamic_function(F::having);
  register_dynamic_function(F::sort, K_ENABLE_NONE,
                            F::fields | F::groupBy | F::having);
  register_dynamic_function(F::limit, F::skip | F::offset,
                            F::fields | F::groupBy | F::having | F::sort);
  register_dynamic_function(F::offset, K_ENABLE_NONE, F::limit | F::skip);
  register_dynamic_function(F::skip, K_ENABLE_NONE, F::limit | F::offset);
  register_dynamic_function(F::lockShared, K_ENABLE_NONE,
                            F::lockExclusive | F::fields | F::groupBy |
                                F::having | F::sort | F::limit | F::offset |
                                F::skip);
  register_dynamic_function(F::lockExclusive, K_ENABLE_NONE,
                            F::lockShared | F::fields | F::groupBy | F::having |
                                F::sort | F::limit | F::offset | F::skip);
  register_dynamic_function(F::bind, K_ENABLE_NONE,
                            F::lockExclusive | F::lockShared | F::fields |
                                F::groupBy | F::having | F::sort | F::limit |
                                F::offset | F::skip,
                            K_ALLOW_REUSE);
  register_dynamic_function(F::execute, F::limit, K_DISABLE_NONE,
                            K_ALLOW_REUSE);

  enable_function(F::find);
}

shcore::Value CollectionFind::this_object() {
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(find, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_FIND_BRIEF,
              "Sets the search condition to identify the Documents to be "
              "retrieved from the owner Collection.");
REGISTER_HELP(COLLECTIONFIND_FIND_PARAM,
              "@param searchCondition Optional String expression defining the "
              "condition to be used on the selection.");
REGISTER_HELP(COLLECTIONFIND_FIND_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_FIND_DETAIL,
              "Sets the search condition to identify the Documents to be "
              "retrieved from the owner Collection. "
              "If the search condition is not specified the find operation "
              "will be executed over all the documents in the collection.");
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
 */
#if DOXYGEN_JS
/**
 * - groupBy(List searchExprStr)
 */
#elif DOXYGEN_PY
/**
 * - group_by(List searchExprStr)
 */
#endif
/**
 * - sort(List sortExprStr)
 * - limit(Integer numberOfDocs)
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
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
      std::static_pointer_cast<Collection>(_owner));

  if (collection) {
    try {
      std::string search_condition;
      if (args.size()) {
        search_condition = args.string_at(0);

        if (!search_condition.empty()) set_filter(search_condition);
      }
      // Updates the exposed functions
      update_functions(F::find);
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.find");
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionFind &CollectionFind::set_filter(const std::string &filter) {
  message_.set_allocated_criteria(
      ::mysqlx::parser::parse_collection_filter(filter, &_placeholders));

  return *this;
}

REGISTER_HELP_FUNCTION(fields, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_FIELDS_BRIEF,
              "Sets the fields to be retrieved from each document matching the "
              "criteria on this find operation.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_PARAM,
    "@param fieldDefinition Definition of the fields to be retrieved.");
REGISTER_HELP(COLLECTIONFIND_FIELDS_SIGNATURE, "(fieldList)");
REGISTER_HELP(COLLECTIONFIND_FIELDS_SIGNATURE1, "(field[, field, ...])");
REGISTER_HELP(COLLECTIONFIND_FIELDS_SIGNATURE2, "(mysqlx.expr(...))");
REGISTER_HELP(COLLECTIONFIND_FIELDS_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_FIELDS_DETAIL,
              "This function sets the fields to be retrieved from each "
              "document matching the criteria on this find operation.");
REGISTER_HELP(COLLECTIONFIND_FIELDS_DETAIL1,
              "A field is defined as a string value containing an expression "
              "defining the field to be retrieved.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL2,
    "The fields to be retrieved can be set using any of the next methods:");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL3,
    "@li Passing each field definition as an individual string parameter.");
REGISTER_HELP(
    COLLECTIONFIND_FIELDS_DETAIL4,
    "@li Passing a list of strings containing the field definitions.");
REGISTER_HELP(COLLECTIONFIND_FIELDS_DETAIL5,
              "@li Passing a JSON expression representing a document "
              "projection to be generated.");

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
 */
#if DOXYGEN_JS
/**
 * - groupBy(List searchExprStr)
 */
#elif DOXYGEN_PY
/**
 * - group_by(List searchExprStr)
 */
#endif
/**
 * - sort(List sortExprStr)
 * - limit(Integer numberOfDocs)
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
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
      if (args[0].type == Array) {
        std::vector<std::string> fields;

        parse_string_list(args, fields);

        for (auto &field : fields)
          ::mysqlx::parser::parse_collection_column_list_with_alias(
              *message_.mutable_projection(), field);

        if (message_.projection().size() == 0)
          throw shcore::Exception::argument_error(
              "Field selection criteria can not be empty");

      } else if (args[0].type == Object &&
                 args[0].as_object()->class_name() == "Expression") {
        auto expression =
            std::static_pointer_cast<mysqlx::Expression>(args[0].as_object());
        std::unique_ptr<Mysqlx::Expr::Expr> expr_obj(
            ::mysqlx::parser::parse_collection_filter(expression->get_data(),
                                                      &_placeholders));

        // Parsing is done just to validate it is a valid JSON expression
        if (expr_obj->type() == Mysqlx::Expr::Expr_Type_OBJECT) {
          message_.mutable_projection()->Add()->set_allocated_source(
              expr_obj.release());
        } else {
          throw shcore::Exception::argument_error(
              "Argument #1 is expected to be a JSON expression");
        }
      } else {
        throw shcore::Exception::argument_error(
            "Argument #1 is expected to be a string, array of strings or a "
            "JSON expression");
      }
    } else {
      std::vector<std::string> fields;
      parse_string_list(args, fields);

      for (auto &field : fields) {
        ::mysqlx::parser::parse_collection_column_list_with_alias(
            *message_.mutable_projection(), field);
      }
    }

    update_functions(F::fields);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.fields");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(groupBy, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_GROUPBY_BRIEF,
              "Sets a grouping criteria for the resultset.");
REGISTER_HELP(COLLECTIONFIND_GROUPBY_SIGNATURE, "(fieldList)");
REGISTER_HELP(COLLECTIONFIND_GROUPBY_SIGNATURE1, "(field[, field, ...])");
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
 * - limit(Integer numberOfDocs)
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
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

    for (auto &field : fields)
      message_.mutable_grouping()->AddAllocated(
          ::mysqlx::parser::parse_collection_filter(field));

    update_functions(F::groupBy);

    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("groupBy"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(having, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_HAVING_BRIEF,
              "Sets a condition for records to be considered in agregate "
              "function operations.");
REGISTER_HELP(COLLECTIONFIND_HAVING_PARAM,
              "@param condition A condition on the agregate functions "
              "used on the grouping criteria.");
REGISTER_HELP(COLLECTIONFIND_HAVING_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_HAVING_DETAIL,
              "Sets a condition for records to be considered in agregate "
              "function operations.");

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
 */
#if DOXYGEN_JS
/**
 * - groupBy(List searchExprStr)
 */
#elif DOXYGEN_PY
/**
 * - group_by(List searchExprStr)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - sort(List sortExprStr)
 * - limit(Integer numberOfDocs)
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 */
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::having(String condition) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::having(str condition) {}
#endif
//@}
shcore::Value CollectionFind::having(const shcore::Argument_list &args) {
  args.ensure_count(1, "CollectionFind.having");

  try {
    message_.set_allocated_grouping_criteria(
        ::mysqlx::parser::parse_collection_filter(args.string_at(0),
                                                  &_placeholders));

    update_functions(F::having);

    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.having");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(sort, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_SORT_BRIEF,
              "Sets the sorting criteria to be used on the DocResult.");
REGISTER_HELP(
    COLLECTIONFIND_SORT_PARAM,
    "@param sortCriteria The sort criteria for the returned documents.");
REGISTER_HELP(COLLECTIONFIND_SORT_SIGNATURE, "(sortCriteriaList)");
REGISTER_HELP(COLLECTIONFIND_SORT_SIGNATURE1,
              "(sortCriteria[, sortCriteria, ...])");
REGISTER_HELP(COLLECTIONFIND_SORT_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_SORT_DETAIL,
              "If used the CollectionFind operation will return the records "
              "sorted with the defined criteria.");
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
 */
#if DOXYGEN_JS
/**
 * - groupBy(List searchExprStr)
 */
#elif DOXYGEN_PY
/**
 * - group_by(List searchExprStr)
 */
#endif
/**
 * - having(String searchCondition)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - limit(Integer numberOfDocs)
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
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

    for (auto &field : fields)
      ::mysqlx::parser::parse_collection_sort_column(*message_.mutable_order(),
                                                     field);

    update_functions(F::sort);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionFind.sort");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(limit, CollectionFind);
REGISTER_HELP(
    COLLECTIONFIND_LIMIT_BRIEF,
    "Sets the maximum number of documents to be returned by the operation.");
REGISTER_HELP(
    COLLECTIONFIND_LIMIT_PARAM,
    "@param numberOfDocs The maximum number of documents to be retrieved.");
REGISTER_HELP(COLLECTIONFIND_LIMIT_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_LIMIT_DETAIL,
              "If used, the operation will return at most <b>numberOfDocs</b> "
              "documents.");
REGISTER_HELP(COLLECTIONFIND_LIMIT_DETAIL1, "${LIMIT_EXECUTION_MODE}");

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
 */
#if DOXYGEN_JS
/**
 * - groupBy(List searchExprStr)
 */
#elif DOXYGEN_PY
/**
 * - group_by(List searchExprStr)
 */
#endif
/**
 * - having(String searchCondition)
 * - sort(List sortExprStr)
 *
 * $(LIMIT_EXECUTION_MODE)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - offset(Integer limitOffset)
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 */
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::limit(Integer numberOfDocs) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::limit(int numberOfDocs) {}
#endif
//@}

REGISTER_HELP_FUNCTION(skip, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_SKIP_BRIEF,
              "Sets number of documents to skip on the resultset when a limit "
              "has been defined.");
REGISTER_HELP(
    COLLECTIONFIND_SKIP_PARAM,
    "@param numberOfDocs The number of documents to skip before start "
    "including them on the DocResult.");
REGISTER_HELP(COLLECTIONFIND_SKIP_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_SKIP_DETAIL,
              "If used, the first <b>numberOfDocs</b>' records will not be "
              "included on the result.");
REGISTER_HELP(COLLECTIONFIND_SKIP_DETAIL1, "${COLLECTIONFIND_SKIP_DEPRECATED}");
REGISTER_HELP(COLLECTIONFIND_SKIP_DEPRECATED,
              "@attention This function will be removed in a future release, "
              "use the <b>offset</b> function instead.");

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
 * - limit(Integer numberOfDocs)
 *
 * After this function invocation, the following functions can be invoked:
 *
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 */
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::skip(Integer numberOfDocs) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::skip(int numberOfDocs) {}
#endif
//@}

REGISTER_HELP(COLLECTIONFIND_OFFSET_BRIEF,
              "Sets number of documents to skip on the resultset when a limit "
              "has been defined.");
REGISTER_HELP(COLLECTIONFIND_OFFSET_PARAM,
              "@param quantity The number of documents to skip before start "
              "including them on the DocResult.");
REGISTER_HELP(COLLECTIONFIND_OFFSET_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_OFFSET_SYNTAX, "offset(quantity)");
REGISTER_HELP(
    COLLECTIONFIND_OFFSET_DETAIL,
    "If used, the first <b>quantity</b> records will not be included on the "
    "result.");

/**
 * $(COLLECTIONFIND_OFFSET_BRIEF)
 *
 * $(COLLECTIONFIND_OFFSET_PARAM)
 *
 * $(COLLECTIONFIND_OFFSET_RETURNS)
 *
 * $(COLLECTIONFIND_OFFSET_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - limit(Integer numberOfRows)
 *
 * After this function invocation, the following functions can be invoked:
 *
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 */
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::offset(Integer quantity) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::offset(int quantity) {}
#endif
//@}

void CollectionFind::set_lock_contention(const shcore::Argument_list &args) {
  std::string lock_contention;
  if (args.size() == 1) {
    if (args[0].type == shcore::Object) {
      std::shared_ptr<Constant> constant =
          std::dynamic_pointer_cast<Constant>(args.object_at(0));
      if (constant && constant->group() == "LockContention")
        lock_contention = constant->data().get_string();
    } else if (args[0].type == shcore::String) {
      lock_contention = args.string_at(0);
    }

    if (!shcore::str_casecmp(lock_contention.c_str(), "nowait")) {
      message_.set_locking_options(Mysqlx::Crud::Find_RowLockOptions_NOWAIT);
    } else if (!shcore::str_casecmp(lock_contention.c_str(), "skip_locked")) {
      message_.set_locking_options(
          Mysqlx::Crud::Find_RowLockOptions_SKIP_LOCKED);
    } else if (shcore::str_casecmp(lock_contention.c_str(), "default")) {
      throw shcore::Exception::argument_error(
          "Argument #1 is expected to be one of DEFAULT, NOWAIT or "
          "SKIP_LOCKED");
    }
  }
}

REGISTER_HELP_FUNCTION(lockShared, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_BRIEF,
              "Instructs the server to acquire shared row locks in documents "
              "matched by this find operation.");
REGISTER_HELP(
    COLLECTIONFIND_LOCKSHARED_PARAM,
    "@param lockContention optional parameter to indicate how to handle "
    "documents that are already locked.");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL,
              "When this function is called, the selected documents will be"
              "locked for write operations, they may be retrieved on a "
              "different session, but no updates will be allowed.");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is commited or rolled back.");

REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL2,
              "The lockContention parameter defines the behavior of the "
              "operation if another session contains an exlusive lock to "
              "matching documents.");

REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL3,
              "The lockContention can be specified using the following "
              "constants:");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL4,
              "@li mysqlx.LockContention.DEFAULT");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL5,
              "@li mysqlx.LockContention.NOWAIT");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL6,
              "@li mysqlx.LockContention.SKIP_LOCKED");

REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL7,
              "The lockContention can also be specified using the following "
              "string literals (no case sensitive):");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL8, "@li 'DEFAULT'");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL9, "@li 'NOWAIT'");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL10, "@li 'SKIP_LOCKED'");

REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL11,
              "If no lockContention or the default is specified, the operation "
              "will block if another session already holds an exclusive lock "
              "on matching documents until the lock is released.");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL12,
              "If lockContention is set to NOWAIT and another session "
              "already holds an exclusive lock on matching documents, the "
              "operation will not block and an error will be generated.");
REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL13,
              "If lockContention is set to SKIP_LOCKED and another session "
              "already holds an exclusive lock on matching documents, the "
              "operation will not block and will return only those documents "
              "not having an exclusive lock.");

REGISTER_HELP(COLLECTIONFIND_LOCKSHARED_DETAIL14,
              "This operation only makes sense within a transaction.");

/**
 * $(COLLECTIONFIND_LOCKSHARED_BRIEF)
 *
 * $(COLLECTIONFIND_LOCKSHARED_PARAM)
 *
 * $(COLLECTIONFIND_LOCKSHARED_RETURNS)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL1)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL2)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL3)
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL4)
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL5)
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL6)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL7)
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL8)
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL9)
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL10)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL11)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL12)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL13)
 *
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL14) *
 * #### Method Chaining
 *
 * This function can be invoked at any time before bind or execute are called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 */
#if DOXYGEN_JS
/**
 * - lockExclusive(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_exclusive(str lockContention)
 */
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 *
 * If lockExclusive() is called, it will override the lock type to be used on
 * on the selected documents.
 */
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::lockShared(String lockContention) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::lock_shared(str lockContention) {}
#endif
//@}
shcore::Value CollectionFind::lock_shared(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("lockShared").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_SHARED_LOCK);

    set_lock_contention(args);

    update_functions(F::lockShared);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("lockShared"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(lockExclusive, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_BRIEF,
              "Instructs the server to acquire an exclusive lock on documents "
              "matched by this find operation.");
REGISTER_HELP(
    COLLECTIONFIND_LOCKEXCLUSIVE_PARAM,
    "@param lockContention optional parameter to indicate how to handle "
    "documents that are already locked.");

REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL,
              "When this function is called, the selected documents will be "
              "locked for read operations, they will not be retrievable by "
              "other session.");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL1,
              "The acquired locks will be released when the current "
              "transaction is commited or rolled back.");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL2,
              "The lockContention parameter defines the behavior of the "
              "operation if another session contains a lock to matching "
              "documents.");

REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL3,
              "The lockContention can be specified using the following "
              "constants:");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL4,
              "@li mysqlx.LockContention.DEFAULT");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL5,
              "@li mysqlx.LockContention.NOWAIT");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL6,
              "@li mysqlx.LockContention.SKIP_LOCKED");

REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL7,
              "The lockContention can also be specified using the following "
              "string literals (no case sensitive):");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL8, "@li 'DEFAULT'");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL9, "@li 'NOWAIT'");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL10, "@li 'SKIP_LOCKED'");

REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL11,
              "If no lockContention or the default is specified, the operation "
              "will block if another session already holds a lock on matching "
              "documents.");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL12,
              "If lockContention is set to NOWAIT and another session "
              "already holds a lock on matching documents, the operation will "
              "not block and an error will be generated.");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL13,
              "If lockContention is set to SKIP_LOCKED and  another session "
              "already holds a lock on matching documents, the operation will "
              "not block and will return only those documents not having a "
              "lock.");
REGISTER_HELP(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL14,
              "This operation only makes sense within a transaction.");

/**
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_BRIEF)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_PARAM)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_RETURNS)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL1)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL2)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL3)
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL4)
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL5)
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL6)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL7)
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL8)
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL9)
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL10)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL11)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL12)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL13)
 *
 * $(COLLECTIONFIND_LOCKEXCLUSIVE_DETAIL14)
 *
 * #### Method Chaining
 *
 * This function can be invoked at any time before bind or execute are called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 */
#if DOXYGEN_JS
/**
 * - lockShared(String lockContention)
 */
#elif DOXYGEN_PY
/**
 * - lock_shared(str lockContention)
 */
#endif
/**
 * - bind(String name, Value value)
 * - execute()
 *
 * If lockShared() is called, it will override the lock type to be used on
 * on the selected documents.
 */
//@{
#if DOXYGEN_JS
CollectionFind CollectionFind::lockExclusive(String lockContention) {}
#elif DOXYGEN_PY
CollectionFind CollectionFind::lock_exclusive(str lockContention) {}
#endif
//@}
shcore::Value CollectionFind::lock_exclusive(
    const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("lockExclusive").c_str());

  try {
    message_.set_locking(Mysqlx::Crud::Find_RowLock_EXCLUSIVE_LOCK);

    set_lock_contention(args);

    update_functions(F::lockExclusive);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("lockExclusive"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

REGISTER_HELP_FUNCTION(bind, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_BIND_BRIEF,
              "Binds a value to a specific placeholder used on this "
              "CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_BIND_PARAM,
              "@param name The name of the placeholder to which the value will "
              "be bound.");
REGISTER_HELP(COLLECTIONFIND_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(COLLECTIONFIND_BIND_RETURNS,
              "@returns This CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_BIND_SYNTAX,
              "bind(placeHolder, value)[.bind(...)]");
REGISTER_HELP(COLLECTIONFIND_BIND_DETAIL,
              "Binds a value to a specific placeholder used on this "
              "CollectionFind object.");
REGISTER_HELP(COLLECTIONFIND_BIND_DETAIL1,
              "An error will be raised if the placeholder indicated by name "
              "does not exist.");
REGISTER_HELP(COLLECTIONFIND_BIND_DETAIL2,
              "This function must be called once for each used placeholder or "
              "an error will be "
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

REGISTER_HELP_FUNCTION(execute, CollectionFind);
REGISTER_HELP(COLLECTIONFIND_EXECUTE_BRIEF,
              "Executes the find operation with all the configured options.");
REGISTER_HELP(COLLECTIONFIND_EXECUTE_RETURNS,
              "@returns A DocResult object that can be used to traverse the "
              "documents returned by this operation.");

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
 * \snippet mysqlx_collection_find.js CollectionFind: All Records
 *
 * #### Filtering
 * \snippet mysqlx_collection_find.js CollectionFind: Filtering
 *
 * #### Field Selection
 * Using a field selection list
 * \snippet mysqlx_collection_find.js CollectionFind: Field Selection List
 *
 * Using separate field selection parameters
 * \snippet mysqlx_collection_find.js CollectionFind: Field Selection Parameters
 *
 * Using a projection expression
 * \snippet mysqlx_collection_find.js CollectionFind: Field Selection Projection
 *
 * #### Sorting
 * \snippet mysqlx_collection_find.js CollectionFind: Sorting
 *
 * #### Using Limit and Offset
 * \snippet mysqlx_collection_find.js CollectionFind: Limit and Offset
 *
 * #### Parameter Binding
 * \snippet mysqlx_collection_find.js CollectionFind: Parameter Binding
 */
DocResult CollectionFind::execute() {}
#elif DOXYGEN_PY
/**
 * #### Retrieving All Documents
 * \snippet mysqlx_collection_find.py CollectionFind: All Records
 *
 * #### Filtering
 * \snippet mysqlx_collection_find.py CollectionFind: Filtering
 *
 * #### Field Selection
 * Using a field selection list
 * \snippet mysqlx_collection_find.py CollectionFind: Field Selection List
 *
 * Using separate field selection parameters
 * \snippet mysqlx_collection_find.py CollectionFind: Field Selection Parameters
 *
 * Using a projection expression
 * \snippet mysqlx_collection_find.py CollectionFind: Field Selection Projection
 *
 * #### Sorting
 * \snippet mysqlx_collection_find.py CollectionFind: Sorting
 *
 * #### Using Limit and Offset
 * \snippet mysqlx_collection_find.py CollectionFind: Limit and Offset
 *
 * #### Parameter Binding
 * \snippet mysqlx_collection_find.py CollectionFind: Parameter Binding
 */
DocResult CollectionFind::execute() {}
#endif
//@}
shcore::Value CollectionFind::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());
  std::unique_ptr<DocResult> result;
  try {
    result = execute();
    update_functions(F::execute);
    if (!m_limit.is_null()) {
      enable_function(F::offset);
      enable_function(F::skip);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}

void CollectionFind::set_prepared_stmt() {
  m_prep_stmt.mutable_stmt()->set_type(
      Mysqlx::Prepare::Prepare_OneOfMessage_Type_FIND);
  message_.clear_args();
  update_limits();
  *m_prep_stmt.mutable_stmt()->mutable_find() = message_;
}

std::unique_ptr<DocResult> CollectionFind::execute() {
  std::unique_ptr<DocResult> result;

  result.reset(new DocResult(safe_exec([this]() {
    update_limits();
    insert_bound_values(message_.mutable_args());
    return session()->session()->execute_crud(message_);
  })));

  return result;
}
}  // namespace mysqlx
}  // namespace mysqlsh
