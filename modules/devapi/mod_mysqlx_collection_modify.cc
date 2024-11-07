/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "modules/devapi/mod_mysqlx_collection.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace mysqlx {

using shcore::Value;
using shcore::Value_type::Array;
using std::placeholders::_1;

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
  auto limit_id = F::limit;
  auto bind_id = F::bind;
  // Exposes the methods available for chaining
  expose("modify", &CollectionModify::modify, "searchCondition");
  expose("set", &CollectionModify::set, "attribute", "value");
  add_method("unset", std::bind(&CollectionModify::unset, this, _1), "data");
  add_method("patch", std::bind(&CollectionModify::patch, this, _1), "data");
  expose("arrayInsert", &CollectionModify::array_insert, "docPath", "value");
  expose("arrayAppend", &CollectionModify::array_append, "docPath", "value");
  add_method("sort", std::bind(&CollectionModify::sort, this, _1), "data");
  add_method("limit",
             std::bind(&CollectionModify::limit, this, _1, limit_id, false),
             "data");
  add_method("bind", std::bind(&CollectionModify::bind_, this, _1, bind_id),
             "data");

  // Registers the dynamic function behavior
  Allowed_function_mask operations =
      F::set | F::unset | F::patch | F::arrayInsert | F::arrayAppend;
  register_dynamic_function(F::modify, operations);
  register_dynamic_function(F::operation,
                            F::sort | F::limit | F::bind | F::execute);
  register_dynamic_function(F::sort, K_ENABLE_NONE, operations);
  register_dynamic_function(F::limit, K_ENABLE_NONE, operations | F::sort);
  register_dynamic_function(F::bind, K_ENABLE_NONE,
                            operations | F::sort | F::limit, K_ALLOW_REUSE);
  register_dynamic_function(F::execute, F::limit, K_DISABLE_NONE,
                            K_ALLOW_REUSE);

  // Initial function update
  enable_function(F::modify);
}

shcore::Value CollectionModify::this_object() {
  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
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
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * \sa Usage examples at execute().
 */
//@{
#if DOXYGEN_JS
CollectionModify CollectionModify::modify(String searchCondition) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::modify(str searchCondition) {}
#endif
//@}
std::shared_ptr<CollectionModify> CollectionModify::modify(
    const std::string &condition) {
  std::shared_ptr<Collection> collection(
      std::static_pointer_cast<Collection>(_owner));

  if (collection) {
    auto search_condition = shcore::str_strip(condition);
    if (search_condition.empty())
      throw shcore::Exception::argument_error("Requires a search condition.");

    set_filter(search_condition);

    // Updates the exposed functions
    update_functions(F::modify);
    reset_prepared_statement();
  }

  return shared_from_this();
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
REGISTER_HELP(
    COLLECTIONMODIFY_SET_DETAIL,
    "Adds an operation into the modify handler to set an attribute on "
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
 * The <b>expression</b> supports \ref param_binding.
 *
 * The attribute addition will be done on the collection's documents once the
 * execute() method is called.
 *
 * #### Method Chaining
 *
 * This function can be invoked multiple times after:
 */
#if DOXYGEN_JS
/**
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - modify(str searchCondition)
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
CollectionModify CollectionModify::set(String attribute, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::set(str attribute, Value value) {}
#endif
//@}
std::shared_ptr<CollectionModify> CollectionModify::set(
    const std::string &attribute, shcore::Value value) {
  if (attribute.empty()) {
    throw std::logic_error("Invalid document path");
  }

  set_operation(Mysqlx::Crud::UpdateOperation::ITEM_SET, attribute, value);

  update_functions(F::operation);
  reset_prepared_statement();

  return shared_from_this();
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
              "documents once the execute() method is called.");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_DETAIL1,
              "For each attribute on the attributes list, adds an operation "
              "into the modify handler");
REGISTER_HELP(COLLECTIONMODIFY_UNSET_DETAIL2,
              "to remove the attribute on the documents that were included on "
              "the selection filter and limit.");
//@{
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
 */
#if DOXYGEN_JS
/**
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - modify(str searchCondition)
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
#if DOXYGEN_JS
CollectionModify CollectionModify::unset(
    String attribute[, String attribute, ...]) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::unset(str attribute[, str attribute, ...]) {}
#endif

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
 */
#if DOXYGEN_JS
/**
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - modify(str searchCondition)
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
#if DOXYGEN_JS
CollectionModify CollectionModify::unset(List attributes) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::unset(list attributes) {}
#endif
//@}
shcore::Value CollectionModify::unset(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_at_least(1, get_function_name("unset").c_str());

  try {
    size_t unset_count = 0;

    // Could receive either a List or many strings
    if (args.size() == 1 && args[0].get_type() == Array) {
      shcore::Value::Array_type_ref items = args.array_at(0);
      shcore::Value::Array_type::iterator index, end = items->end();

      int int_index = 0;
      for (index = items->begin(); index != end; index++) {
        int_index++;
        if (index->get_type() == shcore::String) {
          set_operation(Mysqlx::Crud::UpdateOperation::ITEM_REMOVE,
                        index->get_string(), shcore::Value());
        } else {
          throw shcore::Exception::type_error(shcore::str_format(
              "Element #%d is expected to be a string", int_index));
        }
      }

      unset_count = items->size();
    } else {
      for (size_t index = 0; index < args.size(); index++) {
        if (args[index].get_type() == shcore::String) {
          set_operation(Mysqlx::Crud::UpdateOperation::ITEM_REMOVE,
                        args.string_at(index), shcore::Value());
        } else {
          std::string error;

          if (args.size() == 1)
            error = shcore::str_format(
                "Argument #%u is expected to be either string or list of "
                "strings",
                static_cast<uint32_t>(index + 1));
          else
            error =
                shcore::str_format("Argument #%u is expected to be a string",
                                   static_cast<uint32_t>(index + 1));

          throw shcore::Exception::type_error(error);
        }
      }

      unset_count = args.size();
    }

    // Updates the exposed functions
    if (unset_count) update_functions(F::operation);
    reset_prepared_statement();
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("unset"));

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of patch function
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
    "@li The _id of the documents is immutable, so it will not be affected by "
    "the patch operation even if it is included on the patch JSON object.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL8,
    "@li The patch JSON object accepts expression objects as values. If used "
    "they will be evaluated at the server side.");
REGISTER_HELP(
    COLLECTIONMODIFY_PATCH_DETAIL9,
    "The patch operations will be done on the collection's documents once the "
    "execute() method is called.");
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
 */
#if DOXYGEN_JS
/**
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - modify(str searchCondition)
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
CollectionModify CollectionModify::patch(Document document) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::patch(Document document) {}
#endif
//@}
shcore::Value CollectionModify::patch(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, get_function_name("patch").c_str());

  try {
    set_operation(Mysqlx::Crud::UpdateOperation::MERGE_PATCH, "", args[0]);

    update_functions(F::operation);
    reset_prepared_statement();
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
REGISTER_HELP(
    COLLECTIONMODIFY_ARRAYINSERT_DETAIL,
    "Adds an operation into the modify handler to insert a value into "
    "an array attribute on the documents that were included on the "
    "selection filter and limit.");
REGISTER_HELP(COLLECTIONMODIFY_ARRAYINSERT_DETAIL1,
              "The insertion of the value will be done on the collection's "
              "documents once the execute() method is called.");

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
 */
#if DOXYGEN_JS
/**
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - modify(str searchCondition)
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
CollectionModify CollectionModify::arrayInsert(String docPath, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::array_insert(str docPath, Value value) {}
#endif
//@}
std::shared_ptr<CollectionModify> CollectionModify::array_insert(
    const std::string &doc_path, shcore::Value value) {
  set_operation(Mysqlx::Crud::UpdateOperation::ARRAY_INSERT, doc_path, value,
                true);

  // Updates the exposed functions
  update_functions(F::operation);
  reset_prepared_statement();

  return shared_from_this();
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
REGISTER_HELP(
    COLLECTIONMODIFY_ARRAYAPPEND_DETAIL,
    "Adds an operation into the modify handler to append a value into "
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
 */
#if DOXYGEN_JS
/**
 * - modify(String searchCondition)
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - modify(str searchCondition)
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 */
#endif
/**
 *
 * After this function invocation, the following functions can be invoked:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 * - limit(Integer numberOfRows)
 * - bind(String name, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
CollectionModify CollectionModify::arrayAppend(String docPath, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::array_append(str docPath, Value value) {}
#endif
//@}
std::shared_ptr<CollectionModify> CollectionModify::array_append(
    const std::string &doc_path, shcore::Value value) {
  set_operation(Mysqlx::Crud::UpdateOperation::ARRAY_APPEND, doc_path, value);

  update_functions(F::operation);

  reset_prepared_statement();
  return shared_from_this();
}

// Documentation of sort function
REGISTER_HELP_FUNCTION(sort, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_SORT_BRIEF,
              "Sets the document order in which the update operations added to "
              "the handler should be done.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_SIGNATURE, "(sortCriteriaList)");
REGISTER_HELP(COLLECTIONMODIFY_SORT_SIGNATURE1,
              "(sortCriterion[, sortCriterion, ...])");
REGISTER_HELP(COLLECTIONMODIFY_SORT_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL,
              "Every defined sort criterion follows the format:");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL1, "name [ ASC | DESC ]");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL2,
              "ASC is used by default if the sort order is not specified.");
REGISTER_HELP(COLLECTIONMODIFY_SORT_DETAIL3,
              "This method is usually used in combination with limit to fix "
              "the amount of documents to be updated.");

/**
 * $(COLLECTIONMODIFY_SORT_BRIEF)
 *
 * $(COLLECTIONMODIFY_SORT_RETURNS)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL1)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL2)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL3)
 *
 * #### Method Chaining
 *
 * This function can be invoked only once after:
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
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
CollectionModify CollectionModify::sort(List sortCriteria) {}
CollectionModify CollectionModify::sort(
    String sortCriterion[, String sortCriterion, ...]) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::sort(list sortCriteria) {}
CollectionModify CollectionModify::sort(
    str sortCriterion[, str sortCriterion, ...]) {}
#endif
//@}
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
    reset_prepared_statement();
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
REGISTER_HELP(COLLECTIONMODIFY_LIMIT_DETAIL1, "${LIMIT_EXECUTION_MODE}");

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
 */
#if DOXYGEN_JS
/**
 * - set(String attribute, Value value)
 * - <a class="el" href="#a9d437bacdafe2a8d6f8926383799e00a">
 *   unset(String attribute[, String attribute, ...])</a>,
 *   unset(List attributes)
 * - patch(Document document)
 * - arrayAppend(String docPath, Value value)
 * - arrayInsert(String docPath, Value value)
 * - sort(List sortCriteria),
 *   <a class="el" href="#a66972f83287da6ec22c6c5f62fbbd1dd">
 *   sort(String sortCriterion[, String sortCriterion, ...])</a>
 */
#elif DOXYGEN_PY
/**
 * - set(str attribute, Value value)
 * - <a class="el" href="#ab5519a8a8522e370fe4cb79ebf267f5e">
 *   unset(str attribute[, str attribute, ...])</a>,
 *   unset(list attributes)
 * - patch(Document document)
 * - array_append(str docPath, Value value)
 * - array_insert(str docPath, Value value)
 * - sort(list sortCriteria),
 *   <a class="el" href="#ab5f363eb8790616578b118502bbdf2e7">
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
CollectionModify CollectionModify::limit(Integer numberOfDocs) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::limit(int numberOfDocs) {}
#endif
//@}

// Documentation of bind function
REGISTER_HELP_FUNCTION(bind, CollectionModify);
REGISTER_HELP(COLLECTIONMODIFY_BIND_BRIEF,
              "Binds a value to a specific placeholder used on this "
              "CollectionModify object.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_PARAM,
              "@param name The name of the placeholder to which the value "
              "will be bound.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_PARAM1,
              "@param value The value to be bound on the placeholder.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_RETURNS,
              "@returns This CollectionModify object.");
REGISTER_HELP(
    COLLECTIONMODIFY_BIND_DETAIL,
    "Binds the given value to the placeholder with the specified name.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_DETAIL1,
              "An error will be raised if the placeholder indicated by name "
              "does not exist.");
REGISTER_HELP(COLLECTIONMODIFY_BIND_DETAIL2,
              "This function must be called once for each used placeholder or "
              "an error will be raised when the execute() method is called.");

/**
 * $(COLLECTIONMODIFY_BIND_BRIEF)
 *
 * $(COLLECTIONMODIFY_BIND_PARAM)
 *
 * $(COLLECTIONMODIFY_BIND_PARAM1)
 *
 * $(COLLECTIONMODIFY_BIND_RETURNS)
 *
 * $(COLLECTIONMODIFY_BIND_DETAIL)
 *
 * $(COLLECTIONMODIFY_BIND_DETAIL1)
 *
 * $(COLLECTIONMODIFY_BIND_DETAIL2)
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
CollectionModify CollectionModify::bind(String name, Value value) {}
#elif DOXYGEN_PY
CollectionModify CollectionModify::bind(str name, Value value) {}
#endif
//@}

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
 *
 * ### Examples
 */
//@{
#if DOXYGEN_JS
/**
 * #### Modify multiple attributes
 * \snippet mysqlx_collection_modify.js CollectionModify: Set Execution
 *
 * #### Modify an attribute with an array value
 * \snippet mysqlx_collection_modify.js CollectionModify: Set Binding Array
 *
 * #### Unset an attribute
 * \snippet mysqlx_collection_modify.js CollectionModify: Simple Unset Execution
 *
 * #### Unset multiple attributes using an array
 * \snippet mysqlx_collection_modify.js CollectionModify: List Unset Execution
 *
 * #### Patch multiple attributes
 * \snippet mysqlx_collection_modify.js CollectionModify: Patch Execution
 *
 * #### Append to an array attribute
 * \snippet mysqlx_collection_modify.js CollectionModify: arrayAppend Execution
 *
 * #### Insert into an array attribute
 * \snippet mysqlx_collection_modify.js CollectionModify: arrayInsert Execution
 *
 * #### Sorting and setting a limit
 * \snippet mysqlx_collection_modify.js CollectionModify: sorting and limit
 */
Result CollectionModify::execute() {}
#elif DOXYGEN_PY
/**
 * #### Modify multiple attributes
 * \snippet mysqlx_collection_modify.py CollectionModify: Set Execution
 *
 * #### Modify an attribute with an array value
 * \snippet mysqlx_collection_modify.py CollectionModify: Set Binding Array
 *
 * #### Unset an attribute
 * \snippet mysqlx_collection_modify.py CollectionModify: Simple Unset Execution
 *
 * #### Unset multiple attributes using an array
 * \snippet mysqlx_collection_modify.py CollectionModify: List Unset Execution
 *
 * #### Patch multiple attributes
 * \snippet mysqlx_collection_modify.py CollectionModify: Patch Execution
 *
 * #### Append to an array attribute
 * \snippet mysqlx_collection_modify.py CollectionModify: array_append Execution
 *
 * #### Insert into an array attribute
 * \snippet mysqlx_collection_modify.py CollectionModify: array_insert Execution
 *
 * #### Sorting and setting a limit
 * \snippet mysqlx_collection_modify.py CollectionModify: sorting and limit
 */
Result CollectionModify::execute() {}
#endif
//@}
shcore::Value CollectionModify::execute(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("execute").c_str());
  shcore::Value ret_val;
  try {
    ret_val = execute();
    update_functions(F::execute);
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("execute"));

  return ret_val;
}

void CollectionModify::set_prepared_stmt() {
  m_prep_stmt.mutable_stmt()->set_type(
      Mysqlx::Prepare::Prepare_OneOfMessage_Type_UPDATE);
  message_.clear_args();
  update_limits();
  *m_prep_stmt.mutable_stmt()->mutable_update() = message_;
}

#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
shcore::Value CollectionModify::execute() {
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
