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
#include "modules/devapi/mod_mysqlx_collection_modify.h"
#include "db/mysqlx/mysqlx_parser.h"
#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "shellcore/utils_help.h"
#include "utils/utils_string.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "scripting/common.h"

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of CollectionModify class
REGISTER_HELP_CLASS(CollectionModify, mysqlx);
REGISTER_HELP(COLLECTIONMODIFY_BRIEF,
              "Operation to update documents on a Collection.");
REGISTER_HELP(COLLECTIONMODIFY_DETAIL,
              "A CollectionModify object represents an operation to update "
              "documents on a Collection, it is created through the "
              "<b>modify</b> function on the <b>Collection</b> class.");
CollectionModify::CollectionModify(std::shared_ptr<Collection> owner)
    : Collection_crud_definition(
          std::static_pointer_cast<DatabaseObject>(owner)) {
  message_.mutable_collection()->set_schema(owner->schema()->name());
  message_.mutable_collection()->set_name(owner->name());
  message_.set_data_model(Mysqlx::Crud::DOCUMENT);
  // Exposes the methods available for chaining
  add_method("modify", std::bind(&CollectionModify::modify, this, _1), "data");
  add_method("set", std::bind(&CollectionModify::set, this, _1), "data");
  add_method("unset", std::bind(&CollectionModify::unset, this, _1), "data");
  add_method("merge", std::bind(&CollectionModify::merge, this, _1), "data");
  add_method("patch", std::bind(&CollectionModify::patch, this, _1), "data");
  add_method("arrayInsert",
             std::bind(&CollectionModify::array_insert, this, _1), "data");
  add_method("arrayAppend",
             std::bind(&CollectionModify::array_append, this, _1), "data");
  add_method("arrayDelete",
             std::bind(&CollectionModify::array_delete, this, _1), "data");
  add_method("sort", std::bind(&CollectionModify::sort, this, _1), "data");
  add_method("limit", std::bind(&CollectionModify::limit, this, _1), "data");
  add_method("bind", std::bind(&CollectionModify::bind_, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function(F::modify, F::_empty);
  register_dynamic_function(F::set, F::modify | F::operation);
  register_dynamic_function(F::unset, F::modify | F::operation);
  register_dynamic_function(F::merge, F::modify | F::operation);
  register_dynamic_function(F::patch, F::modify | F::operation);
  register_dynamic_function(F::arrayInsert, F::modify | F::operation);
  register_dynamic_function(F::arrayAppend, F::modify | F::operation);
  register_dynamic_function(F::arrayDelete, F::modify | F::operation);
  register_dynamic_function(F::sort, F::operation);
  register_dynamic_function(F::limit, F::operation | F::sort);
  register_dynamic_function(F::bind,
                            F::operation | F::sort | F::limit | F::bind);
  register_dynamic_function(F::execute,
                            F::operation | F::sort | F::limit | F::bind);
  register_dynamic_function(F::__shell_hook__,
                            F::operation | F::sort | F::limit | F::bind);

  // Initial function update
  update_functions(F::_empty);
}

void CollectionModify::set_operation(int type, const std::string &path,
                                     const shcore::Value &value,
                                     bool validate_array) {
  // Sets the operation
  Mysqlx::Crud::UpdateOperation *operation =
      message_.mutable_operation()->Add();
  operation->set_operation(Mysqlx::Crud::UpdateOperation_UpdateType(type));

  std::unique_ptr<Mysqlx::Expr::Expr> docpath(
      ::mysqlx::parser::parse_column_identifier(path.empty() ? "$" : path));
  Mysqlx::Expr::ColumnIdentifier identifier(docpath->identifier());

  // Validates the source is an array item
  int size = identifier.document_path().size();
  if (size) {
    if (validate_array) {
      if (identifier.document_path().Get(size - 1).type() !=
          Mysqlx::Expr::DocumentPathItem::ARRAY_INDEX)
        throw std::logic_error("An array document path must be specified");
    }
  } else if (type != Mysqlx::Crud::UpdateOperation::ITEM_MERGE &&
             type != Mysqlx::Crud::UpdateOperation::ITEM_SET &&
             type != Mysqlx::Crud::UpdateOperation::MERGE_PATCH) {
    throw std::logic_error("Invalid document path");
  }

  // Sets the source
  operation->mutable_source()->CopyFrom(identifier);

  Mysqlx::Expr::Expr *expr_value = nullptr;
  if (value) {
    expr_value = operation->mutable_value();
    encode_expression_value(expr_value, value);
  }

  if ((type == Mysqlx::Crud::UpdateOperation::ITEM_MERGE ||
       type == Mysqlx::Crud::UpdateOperation::MERGE_PATCH) &&
      (!expr_value || expr_value->type() != Mysqlx::Expr::Expr::Expr::OBJECT)) {
    throw std::invalid_argument("Argument expected to be a JSON object");
  }
}

// Documentation of modify function
REGISTER_HELP_FUNCTION(modify, CollectionModify);
REGISTER_HELP(
    COLLECTIONMODIFY_MODIFY_BRIEF,
    "Sets the search condition to identify the Documents to be updated on the "
    "owner Collection.");

REGISTER_HELP(
    COLLECTIONMODIFY_MODIFY_PARAM,
    "@param searchCondition An expression to identify the documents to be "
    "updated.");

REGISTER_HELP(COLLECTIONMODIFY_MODIFY_RETURNS,
              "@returns This CollectionModify object.");

REGISTER_HELP(COLLECTIONMODIFY_MODIFY_DETAIL,
              "Creates a handler to update documents in the collection.");

REGISTER_HELP(
    COLLECTIONMODIFY_MODIFY_DETAIL1,
    "A condition must be provided to this function, all the documents "
    "matching the condition will be updated.");

REGISTER_HELP(
    COLLECTIONMODIFY_MODIFY_DETAIL2,
    "To update all the documents, set a condition that always evaluates to "
    "true, for example '1'.");

/**
 * $(COLLECTIONMODIFY_MODIFY_BRIEF)
 *
 * $(COLLECTIONMODIFY_MODIFY_PARAM)
 *
 * $(COLLECTIONMODIFY_MODIFY_DETAIL)
 *
 * $(COLLECTIONMODIFY_MODIFY_RETURNS)
 *
 * $(COLLECTIONMODIFY_MODIFY_DETAIL1)
 *
 * $(COLLECTIONMODIFY_MODIFY_DETAIL2)
 *
 * #### Method Chaining
 *
 * This function is called automatically when Collection.modify(searchCondition)
 * is called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * \sa Usage examples at execute().
 * \sa Collection
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::modify(String searchCondition) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::modify(str searchCondition) {}
#endif
shcore::Value CollectionModify::modify(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("modify").c_str());

  std::shared_ptr<Collection> collection(
      std::static_pointer_cast<Collection>(_owner));

  if (collection) {
    try {
      std::string search_condition = str_strip(args.string_at(0));

      if (search_condition.empty())
        throw shcore::Exception::argument_error("Requires a search condition.");

      set_filter(search_condition);

      // Updates the exposed functions
      update_functions(F::modify);
    }
    CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("modify"));
  }

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionModify &CollectionModify::set_filter(const std::string &filter) {
  message_.set_allocated_criteria(
      ::mysqlx::parser::parse_collection_filter(filter, &_placeholders));

  return *this;
}

// Documentation of set function
REGISTER_HELP_FUNCTION(set, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_SET_BRIEF,
              "Sets or updates attributes on documents in a collection.");
REGISTER_HELP(
    COLLECTIONMODIFY_SET_PARAM,
    "@param attribute A string with the document path of the item to be set.");
REGISTER_HELP(COLLECTIONMODIFY_SET_PARAM1,
              "@param value The value to be set on the specified attribute.");
REGISTER_HELP(COLLECTIONMODIFY_SET_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_SET_DETAIL,
              "Adds an opertion into the modify handler to set an attribute on "
              "the documents that were included on the selection filter and "
              "limit.");
REGISTER_HELP(COLLECTIONMODIFY_SET_DETAIL1,
              "@li If the attribute is not present on the document, it will be "
              "added with the given value.");
REGISTER_HELP(COLLECTIONMODIFY_SET_DETAIL2,
              "@li If the attribute already exists on the document, it will be "
              "updated with the given value.");
REGISTER_HELP(COLLECTIONMODIFY_SET_DETAIL3,
              "<b>Using Expressions for Values</b>");
REGISTER_HELP(
    COLLECTIONMODIFY_SET_DETAIL4,
    "The received values are set into the document in a literal way unless an "
    "expression is used.");
REGISTER_HELP(
    COLLECTIONMODIFY_SET_DETAIL5,
    "When an expression is used, it is evaluated on the server and the "
    "resulting value is set into the document.");

/**
 * $(COLLECTIONMODIFY_SET_BRIEF)
 *
 * $(COLLECTIONMODIFY_SET_PARAM)
 *
 * $(COLLECTIONMODIFY_SET_PARAM1)
 *
 * $(COLLECTIONMODIFY_SET_RETURNS)
 *
 * $(COLLECTIONMODIFY_SET_DETAIL)
 * $(COLLECTIONMODIFY_SET_DETAIL1)
 * $(COLLECTIONMODIFY_SET_DETAIL2)
 *
 * $(COLLECTIONMODIFY_SET_DETAIL3)
 *
 * $(COLLECTIONMODIFY_SET_DETAIL4)
 *
 * $(COLLECTIONMODIFY_SET_DETAIL5)
 *
 * To define an expression use:
 * \code{.py}
 * mysqlx.expr(expression)
 * \endcode
 *
 * The expression also can be used for \a [Parameter
 * Binding](param_binding.html).
 *
 * The attribute addition will be done on the collection's documents once the
 * execute method is called.
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::set(String attribute, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::set(str attribute, Value value) {}
#endif
shcore::Value CollectionModify::set(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(2, get_function_name("set").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::ITEM_SET, args.string_at(0),
                  args[1]);

    update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("set"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of unset function
REGISTER_HELP_FUNCTION(unset, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_UNSET_BRIEF,
              "Removes attributes from documents in a collection.");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_SIGNATURE, "(attributeList)");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_SIGNATURE1,
              "(attribute[, attribute, ...])");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_PARAM,
              "@param attribute A string with the document path of the "
              "attribute to be removed.");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_PARAM1,
              "@param attributes A list with the document paths of the "
              "attributes to be removed.");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_DETAIL,
              "The attribute removal will be done on the collection's "
              "documents once the execute method is called.");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_DETAIL1,
              "For each attribute on the attributes list, adds an opertion "
              "into the modify handler");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_DETAIL2,
              "to remove the attribute on the documents that were included on "
              "the selection filter and limit.");

/**
 * $(COLLECTIONMODIFY_UNSET_BRIEF)
 *
 * $(COLLECTIONMODIFY_UNSET_PARAM)
 *
 * $(COLLECTIONMODIFY_UNSET_RETURNS)
 *
 * $(COLLECTIONMODIFY_UNSET_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
CollectionModify CollectionModify::unset(
    String attribute[, String attribute, ...]) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::unset(str attribute[, str attribute, ...]) {}
#endif
//@}

/**
 * $(COLLECTIONMODIFY_UNSET_BRIEF)
 *
 * $(COLLECTIONMODIFY_UNSET_PARAM1)
 *
 * $(COLLECTIONMODIFY_UNSET_RETURNS)
 *
 * $(COLLECTIONMODIFY_UNSET_DETAIL1)
 * $(COLLECTIONMODIFY_UNSET_DETAIL2)
 *
 * $(COLLECTIONMODIFY_UNSET_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::unset(List attributes) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::unset(list attributes) {}
#endif

shcore::Value CollectionModify::unset(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_at_least(1, get_function_name("unset").c_str());

  try {
    size_t unset_count = 0;

    // Could receive either a List or many strings
    if (args.size() == 1 && args[0].type == Array) {
      shcore::Value::Array_type_ref items = args.array_at(0);
      shcore::Value::Array_type::iterator index, end = items->end();

      int int_index = 0;
      for (index = items->begin(); index != end; index++) {
        int_index++;
        if (index->type == shcore::String) {
          set_operation(Mysqlx::Crud::UpdateOperation::ITEM_REMOVE,
                        index->get_string(), shcore::Value());
        } else {
          throw shcore::Exception::type_error(
              str_format("Element #%d is expected to be a string", int_index));
        }
      }

      unset_count = items->size();
    } else {
      for (size_t index = 0; index < args.size(); index++) {
        if (args[index].type == shcore::String) {
          set_operation(Mysqlx::Crud::UpdateOperation::ITEM_REMOVE,
                        args.string_at(index), shcore::Value());
        } else {
          std::string error;

          if (args.size() == 1)
            error = str_format(
                "Argument #%u is expected to be either string or list of "
                "strings",
                static_cast<uint32_t>(index + 1));
          else
            error = str_format("Argument #%u is expected to be a string",
                               static_cast<uint32_t>(index + 1));

          throw shcore::Exception::type_error(error);
        }
      }

      unset_count = args.size();
    }

    // Updates the exposed functions
    if (unset_count) update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("unset"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of merge function
REGISTER_HELP_FUNCTION(merge, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_MERGE_BRIEF,
              "Adds attributes taken from a document into the documents in a "
              "collection.");
REGISTER_HELP(
    COLLECTIONMODIFY_MERGE_PARAM,
    "@param document The document from which the attributes will be merged.");
REGISTER_HELP(COLLECTIONMODIFY_MERGE_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_MERGE_DETAIL,
              "This function adds an operation to add into the documents of a "
              "collection, all the attribues defined in document that do not "
              "exist on the collection's documents.");
REGISTER_HELP(COLLECTIONMODIFY_MERGE_DETAIL1,
              "The attribute addition will be done on the collection's "
              "documents once the execute method is called.");
REGISTER_HELP(COLLECTIONMODIFY_MERGE_DETAIL2,
              "${COLLECTIONMODIFY_MERGE_DEPRECATED}");
REGISTER_HELP(COLLECTIONMODIFY_MERGE_DEPRECATED,
              "@attention This function will be removed in a future release, "
              "use the <b>patch</b> function instead.");

/**
 * $(COLLECTIONMODIFY_MERGE_BRIEF)
 *
 * $(COLLECTIONMODIFY_MERGE_PARAM)
 *
 * $(COLLECTIONMODIFY_MERGE_RETURNS)
 *
 * $(COLLECTIONMODIFY_MERGE_DETAIL)
 *
 * $(COLLECTIONMODIFY_MERGE_DETAIL1)
 *
 * $(COLLECTIONMODIFY_MERGE_DEPRECATED)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::merge(Document document) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::merge(Document document) {}
#endif
shcore::Value CollectionModify::merge(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("merge").c_str());

  log_warning("'%s' is deprecated, use '%s' instead.",
              get_function_name("merge").c_str(),
              get_function_name("patch").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::ITEM_MERGE, "", args[0]);

    update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("merge"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of merge function
REGISTER_HELP_FUNCTION(patch, CollectionModify);
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_BRIEF,
    "Performs modifications on a document based on a patch JSON object.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_PARAM,
    "@param document The JSON object to be used on the patch process.");
REGISTER_HELP(COLLECTIONMODIFY_PATCH_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL,
    "This function adds an operation to update the documents of a collection, "
    "the patch operation follows the algorithm described on the JSON Merge "
    "Patch RFC7386.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL1,
    "The patch JSON object will be used to either add, update or remove fields "
    "from documents in the collection that match the filter specified on the "
    "call to the modify() function.");
REGISTER_HELP(COLLECTIONMODIFY_PATCH_DETAIL2,
              "The operation to be performed "
              "depends on the attributes defined at the patch JSON object:");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL3,
    "@li Any attribute with value equal to null will be removed if exists.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL4,
    "@li Any attribute with value different than null will be updated if "
    "exists.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL5,
    "@li Any attribute with value different than null will be added if "
    "does not exists.");
REGISTER_HELP(COLLECTIONMODIFY_PATCH_DETAIL6, "Special considerations:");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL7,
    "@li The _id of the documents is inmutable, so it will not be affected by "
    "the patch operation even if it is included on the patch JSON object.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL8,
    "@li The patch JSON object accepts expression objects as values. If used "
    "they will be evaluated at the server side.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL9,
    "The patch operations will be done on the collection's documents once the "
    "execute method is called.");
/**
 * $(COLLECTIONMODIFY_PATCH_BRIEF)
 *
 * $(COLLECTIONMODIFY_PATCH_PARAM)
 *
 * $(COLLECTIONMODIFY_PATCH_RETURNS)
 *
 * $(COLLECTIONMODIFY_PATCH_DETAIL)
 *
 * $(COLLECTIONMODIFY_PATCH_DETAIL1)
 *
 * $(COLLECTIONMODIFY_PATCH_DETAIL2)
 * $(COLLECTIONMODIFY_PATCH_DETAIL3)
 * $(COLLECTIONMODIFY_PATCH_DETAIL4)
 * $(COLLECTIONMODIFY_PATCH_DETAIL5)
 *
 * $(COLLECTIONMODIFY_PATCH_DETAIL6)
 * $(COLLECTIONMODIFY_PATCH_DETAIL7)
 * $(COLLECTIONMODIFY_PATCH_DETAIL8)
 *
 * $(COLLECTIONMODIFY_PATCH_DETAIL9)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::patch(Document document) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::patch(Document document) {}
#endif
shcore::Value CollectionModify::patch(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("patch").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::MERGE_PATCH, "", args[0]);

    update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("patch"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of arrayInsert function
REGISTER_HELP_FUNCTION(arrayInsert, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_ARRAYINSERT_BRIEF,
              "Inserts a value into a specific position in an array attribute "
              "in documents of a collection.");
REGISTER_HELP(
    COLLECTIONMODIFY_ARRAYINSERT_PARAM,
    "@param docPath A document path that identifies the array attribute "
    "and position where the value will be inserted.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYINSERT_PARAM1,
              "@param value The value to be inserted.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYINSERT_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYINSERT_DETAIL,
              "Adds an opertion into the modify handler to insert a value into "
              "an array attribute on the documents that were included on the "
              "selection filter and limit.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYINSERT_DETAIL1,
              "The insertion of the value will be done on the collection's "
              "documents once the execute method is called.");

/**
 * $(COLLECTIONMODIFY_ARRAYINSERT_BRIEF)
 *
 * $(COLLECTIONMODIFY_ARRAYINSERT_PARAM)
 * $(COLLECTIONMODIFY_ARRAYINSERT_PARAM1)
 *
 * $(COLLECTIONMODIFY_ARRAYINSERT_RETURNS)
 *
 * $(COLLECTIONMODIFY_ARRAYINSERT_DETAIL)
 *
 * $(COLLECTIONMODIFY_ARRAYINSERT_DETAIL1)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::arrayInsert(String docPath, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::array_insert(str docPath, Value value) {}
#endif
shcore::Value CollectionModify::array_insert(
    const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(2, get_function_name("arrayInsert").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::ARRAY_INSERT,
                  args.string_at(0), args[1], true);

    // Updates the exposed functions
    update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("arrayInsert"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of arrayAppend function
REGISTER_HELP_FUNCTION(arrayAppend, CollectionModify);
REGISTER_HELP(
    COLLECTIONMODIFY_ARRAYAPPEND_BRIEF,
    "Appends a value into an array attribute in documents of a collection.");
REGISTER_HELP(
    COLLECTIONMODIFY_ARRAYAPPEND_PARAM,
    "@param docPath A document path that identifies the array attribute "
    "where the value will be appended.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYAPPEND_PARAM1,
              "@param value The value to be appended.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYAPPEND_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYAPPEND_DETAIL,
              "Adds an opertion into the modify handler to append a value into "
              "an array attribute on the documents that were included on the "
              "selection filter and limit.");

/**
 * $(COLLECTIONMODIFY_ARRAYAPPEND_BRIEF)
 *
 * $(COLLECTIONMODIFY_ARRAYAPPEND_PARAM)
 * $(COLLECTIONMODIFY_ARRAYAPPEND_PARAM1)
 *
 * $(COLLECTIONMODIFY_ARRAYAPPEND_RETURNS)
 *
 * $(COLLECTIONMODIFY_ARRAYAPPEND_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * The attribute addition will be done on the collection's documents once the
 * execute method is called.
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::arrayAppend(String docPath, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::array_append(str docPath, Value value) {}
#endif
shcore::Value CollectionModify::array_append(
    const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(2, get_function_name("arrayAppend").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::ARRAY_APPEND,
                  args.string_at(0), args[1]);

    update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("arrayAppend"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of arrayDelete function
REGISTER_HELP_FUNCTION(arrayDelete, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_ARRAYDELETE_BRIEF,
              "Deletes the value at a specific position in an array attribute "
              "in documents of a collection.");
REGISTER_HELP(
    COLLECTIONMODIFY_ARRAYDELETE_PARAM,
    "@param docPath A document path that identifies the array attribute "
    "and position of the value to be deleted.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYDELETE_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYDELETE_DETAIL,
              "Adds an opertion into the modify handler to delete a value from "
              "an array attribute on the documents that were included on the "
              "selection filter and limit.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYDELETE_DETAIL1,
              "The attribute deletion will be done on the collection's "
              "documents once the execute method is called.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYDELETE_DETAIL2,
              "${COLLECTIONMODIFY_ARRAYDELETE_DEPRECATED}");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYDELETE_DEPRECATED,
              "@attention This function will be removed in a future release, "
              "use the <b>unset</b> function instead.");
/**
 * $(COLLECTIONMODIFY_ARRAYDELETE_BRIEF)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_PARAM)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_RETURNS)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_DETAIL)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_DETAIL1)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_DEPRECATED)
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 *
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 * - sort(List sortExprStr)
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::arrayDelete(String docPath) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::array_delete(str docPath) {}
#endif
shcore::Value CollectionModify::array_delete(
    const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("arrayDelete").c_str());

  log_warning("'%s' is deprecated, use '%s' instead.",
              get_function_name("arrayDelete").c_str(),
              get_function_name("unset").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::ITEM_REMOVE, args.string_at(0),
                  shcore::Value(), true);

    // Updates the exposed functions
    update_functions(F::operation);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("arrayDelete"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of sort function
REGISTER_HELP_FUNCTION(sort, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_SORT_BRIEF,
              "Sets the document order in which the update operations added to "
              "the handler should be done.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_SIGNATURE, "(sortDataList)");
REGISTER_HELP(COLLECTIONMODIFY_SORT_SIGNATURE1, "(sortData[, sortData, ...])");
// REGISTER_HELP(COLLECTIONMODIFY_SORT_PARAM,
//              "@param sortExprStr: A list of expression strings defining a "
//              "collection sort criteria.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL,
              "The elements of sortExprStr list are usually strings defining "
              "the attribute name on which the collection sorting will be "
              "based. Each criterion could be followed by asc or desc to "
              "indicate ascending");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL1,
              "or descending order respectivelly. If no order is specified, "
              "ascending will be used by default.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL2,
              "This method is usually used in combination with limit to fix "
              "the amount of documents to be updated.");

/**
 * $(COLLECTIONMODIFY_SORT_BRIEF)
 *
 * $(COLLECTIONMODIFY_SORT_PARAM)
 *
 * $(COLLECTIONMODIFY_SORT_RETURNS)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL)
 * $(COLLECTIONMODIFY_SORT_DETAIL1)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL2)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::sort(List sortExprStr) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::sort(list sortExprStr) {}
#endif
shcore::Value CollectionModify::sort(const shcore::Argument_list &args) {
  args.ensure_at_least(1, get_function_name("sort").c_str());

  try {
    std::vector<std::string> fields;

    parse_string_list(args, fields);

    if (fields.size() == 0)
      throw shcore::Exception::argument_error("Sort criteria can not be empty");

    for (auto &f : fields)
      ::mysqlx::parser::parse_collection_sort_column(*message_.mutable_order(),
                                                     f);

    update_functions(F::sort);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("sort"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of limit function
REGISTER_HELP_FUNCTION(limit, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_LIMIT_BRIEF,
              "Sets a limit for the documents to be updated by the operations "
              "added to the handler.");
REGISTER_HELP(COLLECTIONMODIFY_LIMIT_PARAM,
              "@param numberOfDocs the number of documents to affect on the "
              "update operations.");
REGISTER_HELP(COLLECTIONMODIFY_LIMIT_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_LIMIT_DETAIL,
              "This method is usually used in combination with sort to fix the "
              "amount of documents to be updated.");

/**
 * $(COLLECTIONMODIFY_LIMIT_BRIEF)
 *
 * $(COLLECTIONMODIFY_LIMIT_PARAM)
 *
 * $(COLLECTIONMODIFY_LIMIT_RETURNS)
 *
 * $(COLLECTIONMODIFY_LIMIT_DETAIL)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 *
 * - set(String attribute, Value value)
 * - unset(String attribute)
 * - unset(List attributes)
 * - merge(Document document)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - arrayDelete(String docPath)
 *
 * After this function invocation, the following functions can be invoked:
 *
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 * - execute(ExecuteOptions opt)
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionModify CollectionModify::limit(Integer numberOfDocs) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::limit(int numberOfDocs) {}
#endif
shcore::Value CollectionModify::limit(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("limit").c_str());

  try {
    message_.mutable_limit()->set_row_count(args.uint_at(0));

    update_functions(F::limit);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("limit"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of bind function
REGISTER_HELP_FUNCTION(bind, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_BIND_BRIEF,
              "Binds a value to a specific placeholder used on this "
              "CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_PARAM,
              "@param name: The name of the placeholder to which the value "
              "will be bound.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_PARAM1,
              "@param value: The value to be bound on the placeholder.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_RETURNS,
              "@returns This CollectionModify object.");

/**
 * $(COLLECTIONMODIFY_BIND_BRIEF)
 *
 * $(COLLECTIONMODIFY_BIND_PARAM)
 *
 * $(COLLECTIONMODIFY_BIND_PARAM1)
 *
 * $(COLLECTIONMODIFY_BIND_RETURNS)
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
 * An error will be raised if the placeholder indicated by name does not exist.
 *
 * This function must be called once for each used placeholder or an error will
 * be
 * raised when the execute method is called.
 *
 * \sa Usage examples at execute().
 */
#if DOXYGEN_JS
CollectionFind CollectionModify::bind(String name, Value value) {}
#elif DOXYGEN_PY
CollectionFind CollectionModify::bind(str name, Value value) {}
#endif
shcore::Value CollectionModify::bind_(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("bind").c_str());

  try {
    bind_value(args.string_at(0), args[1]);

    update_functions(F::bind);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("bind"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

CollectionModify &CollectionModify::bind(const std::string &name,
                                         shcore::Value value) {
  bind_value(name, value);
  return *this;
}

// Documentation of execute function
REGISTER_HELP_FUNCTION(execute, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_EXECUTE_BRIEF,
              "Executes the update operations added to the handler with the "
              "configured filter and limit.");
REGISTER_HELP(COLLECTIONMODIFY_EXECUTE_RETURNS,
              "@returns CollectionResultset A Result object that can be used "
              "to retrieve the results of the update operation.");

/**
 * $(COLLECTIONMODIFY_EXECUTE_BRIEF)
 *
 * $(COLLECTIONMODIFY_EXECUTE_RETURNS)
 *
 * #### Method Chaining
 *
 * This function can be invoked after any other function on this class except
 * modify().
 *
 * The update operation will be executed in the order they were added.
 */
#if DOXYGEN_JS
/**
 *
 * #### Examples
 * \dontinclude "js_devapi/scripts/mysqlx_collection_modify.js"
 * \skip //@# CollectionModify: Set Execution
 * \until //@ CollectionModify: sorting and limit Execution - 4
 * \until print(dir(doc));
 */
Result CollectionModify::execute() {}
#elif DOXYGEN_PY
/**
 *
 * #### Examples
 * \dontinclude "py_devapi/scripts/mysqlx_collection_modify.py"
 * \skip #@# CollectionModify: Set Execution
 * \until #@ CollectionModify: sorting and limit Execution - 4
 * \until print dir(doc)
 */
Result CollectionModify::execute() {}
#endif
shcore::Value CollectionModify::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());
  shcore::Value ret_val;
  try {
    ret_val = execute();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return ret_val;
}

shcore::Value CollectionModify::execute() {
  std::unique_ptr<mysqlsh::mysqlx::Result> result;
  mysqlshdk::utils::Profile_timer timer;
  insert_bound_values(message_.mutable_args());
  timer.stage_begin("CollectionModify::execute");
  result.reset(new mysqlx::Result(safe_exec(
      [this]() { return session()->session()->execute_crud(message_); })));
  timer.stage_end();
  result->set_execution_time(timer.total_seconds_ellapsed());

  return result ? shcore::Value::wrap(result.release()) : shcore::Value::Null();
}
