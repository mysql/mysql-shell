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

#include "modules/devapi/mod_mysqlx_collection.h"
#include <mysqld_error.h>
#include <memory>
#include <string>
#include "modules/devapi/mod_mysqlx_schema.h"

#include "modules/devapi/mod_mysqlx_collection_add.h"
#include "modules/devapi/mod_mysqlx_collection_find.h"
#include "modules/devapi/mod_mysqlx_collection_modify.h"
#include "modules/devapi/mod_mysqlx_collection_remove.h"
#include "modules/devapi/mod_mysqlx_resultset.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
namespace mysqlsh {
namespace mysqlx {
using namespace shcore;

REGISTER_HELP_SUB_CLASS(Collection, mysqlx, DatabaseObject);
REGISTER_HELP(COLLECTION_BRIEF,
              "A Collection is a container that may be used to store Documents "
              "in a MySQL database.");
REGISTER_HELP(COLLECTION_DETAIL,
              "A Document is a set of key and value pairs, as represented by a "
              "JSON object.");
REGISTER_HELP(COLLECTION_DETAIL1,
              "A Document is represented internally using the MySQL binary "
              "JSON object, through the JSON MySQL datatype.");
REGISTER_HELP(COLLECTION_DETAIL2,
              "The values of fields can contain other documents, arrays, and "
              "lists of documents.");
REGISTER_HELP(COLLECTION_UPPER_CLASS, "DatabaseObject");
Collection::Collection(std::shared_ptr<Schema> owner, const std::string &name)
    : DatabaseObject(owner->_session.lock(),
                     std::static_pointer_cast<DatabaseObject>(owner), name) {
  init();
}
void Collection::init() {
  add_method("add", std::bind(&Collection::add_, this, _1), "searchCriteria",
             shcore::String);
  add_method("modify", std::bind(&Collection::modify_, this, _1),
             "searchCriteria", shcore::String);
  add_method("find", std::bind(&Collection::find_, this, _1), "searchCriteria",
             shcore::String);
  add_method("remove", std::bind(&Collection::remove_, this, _1),
             "searchCriteria", shcore::String);
  add_method("createIndex", std::bind(&Collection::create_index_, this, _1),
             "searchCriteria", shcore::String);
  add_method("dropIndex", std::bind(&Collection::drop_index_, this, _1),
             "searchCriteria", shcore::String);
  add_method("replaceOne", std::bind(&Collection::replace_one_, this, _1), "id",
             shcore::String, "doc", shcore::Map);
  add_method("addOrReplaceOne",
             std::bind(&Collection::add_or_replace_one, this, _1), "id",
             shcore::String, "doc", shcore::Map);
  add_method("getOne", std::bind(&Collection::get_one, this, _1), "id",
             shcore::String);
  add_method("removeOne", std::bind(&Collection::remove_one, this, _1), "id",
             shcore::String);
}

Collection::~Collection() {}

REGISTER_HELP_FUNCTION(add, Collection);
REGISTER_HELP(COLLECTION_ADD_BRIEF,
              "Inserts one or more documents into a collection.");
REGISTER_HELP(COLLECTION_ADD_CHAINED, "CollectionAdd.add.[execute]");

/**
 * $(COLLECTION_ADD_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Collection</td><td>.add(...)</td></tr>
 *     <tr><td></td><td>execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .add(...)
 *
 * ##### Overloads
 *
 * @li add$(COLLECTIONADD_ADD_SIGNATURE)
 * @li add$(COLLECTIONADD_ADD_SIGNATURE1)
 * @li add$(COLLECTIONADD_ADD_SIGNATURE2)
 *
 * $(COLLECTIONADD_ADD_DETAIL)
 *
 * $(COLLECTIONADD_ADD_DETAIL1)
 *
 * $(COLLECTIONADD_ADD_DETAIL2)
 * $(COLLECTIONADD_ADD_DETAIL3)
 *
 * $(COLLECTIONADD_ADD_DETAIL4)
 *
 * $(COLLECTIONADD_ADD_DETAIL5)
 * $(COLLECTIONADD_ADD_DETAIL6)
 * $(COLLECTIONADD_ADD_DETAIL7)
 *
 * $(COLLECTIONADD_ADD_DETAIL8)
 *
 * #### .execute()
 *
 * $(COLLECTIONADD_EXECUTE_BRIEF)
 *
 * ### Examples
 */
#if DOXYGEN_JS
/**
 * \snippet collection_add.js CollectionAdd: Chained Calls
 *
 * $(COLLECTIONADD_ADD_DETAIL9)
 *
 * $(COLLECTIONADD_ADD_DETAIL10)
 * \snippet collection_add.js CollectionAdd: Using an Expression
 *
 * #### Using a Document List
 * Adding document using an existing document list
 * \snippet collection_add.js CollectionAdd: Document List
 *
 * #### Multiple Parameters
 * Adding document using a separate parameter for each document on a single call
 * to add(...)
 * \snippet collection_add.js CollectionAdd: Multiple Parameters
 */
CollectionAdd Collection::add(...) {}
#elif DOXYGEN_PY
/**
 * Adding documents using chained calls to add(...)
 * \snippet collection_add.py CollectionAdd: Chained Calls
 *
 * $(COLLECTIONADD_ADD_DETAIL9)
 *
 * $(COLLECTIONADD_ADD_DETAIL10)
 * \snippet collection_add.py CollectionAdd: Using an Expression
 *
 * #### Using a Document List
 * Adding document using an existing document list
 * \snippet collection_add.py CollectionAdd: Document List
 *
 * #### Multiple Parameters
 * Adding document using a separate parameter for each document on a single call
 * to add(...)
 * \snippet collection_add.py CollectionAdd: Multiple Parameters
 */
CollectionAdd Collection::add(...) {}
#endif
shcore::Value Collection::add_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionAdd> collectionAdd(
      new CollectionAdd(shared_from_this()));

  return collectionAdd->add(args);
}

REGISTER_HELP_FUNCTION(modify, Collection);
REGISTER_HELP(COLLECTION_MODIFY_BRIEF, "Creates a collection update handler.");
REGISTER_HELP(COLLECTION_MODIFY_CHAINED,
              "CollectionModify.modify.[set].[unset].[merge].[patch]."
              "[<<<arrayInsert>>>].[<<<arrayAppend>>>].[<<<arrayDelete>>>]."
              "[sort].[limit].[bind].[execute]");

/**
 * $(COLLECTION_MODIFY_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Collection</td><td>.modify(searchCondition)</td></tr>
 *     <tr><td></td><td>[.set(attribute, value)]</td></tr>
 *     <tr><td></td><td>[.unset(...)]</td></tr>
 *     <tr><td></td><td>[.merge(document)]</td></tr>
 *     <tr><td></td><td>[.patch(document)]</td></tr>
 */
#if DOXYGEN_JS
/**
 *     <tr><td></td><td>[.arrayInsert(docPath, value)]</td></tr>
 *     <tr><td></td><td>[.arrayAppend(docPath, value)]</td></tr>
 *     <tr><td></td><td>[.arrayDelete(docPath)]</td></tr>
 */
#elif DOXYGEN_PY
/**
 *     <tr><td></td><td>[.array_insert(docPath, value)]</td></tr>
 *     <tr><td></td><td>[.array_append(docPath, value)]</td></tr>
 *     <tr><td></td><td>[.array_delete(docPath)]</td></tr>
 */
#endif
/**
 *     <tr><td></td><td>[.sort(...)]</td></tr>
 *     <tr><td></td><td>[.limit(numberOfDocs)]</td></tr>
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>[.execute()]</td></tr>
 *   </table>
 * </code>
 *
 * #### .modify(searchCondition)
 *
 * $(COLLECTIONMODIFY_MODIFY_PARAM)
 *
 * $(COLLECTIONMODIFY_MODIFY_DETAIL)
 *
 * $(COLLECTIONMODIFY_MODIFY_DETAIL1)
 *
 * $(COLLECTIONMODIFY_MODIFY_DETAIL2)
 *
 * #### .set(attribute, value)
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
 * #### .unset(...)
 *
 * ##### Overloads
 *
 * @li unset$(COLLECTIONMODIFY_UNSET_SIGNATURE)
 * @li unset$(COLLECTIONMODIFY_UNSET_SIGNATURE1)
 *
 * $(COLLECTIONMODIFY_UNSET_BRIEF)
 *
 * $(COLLECTIONMODIFY_UNSET_DETAIL)
 *
 * #### .merge(document)
 *
 * $(COLLECTIONMODIFY_MERGE_DETAIL)
 *
 * $(COLLECTIONMODIFY_MERGE_DETAIL1)
 *
 * $(COLLECTIONMODIFY_MERGE_DETAIL2)
 *
 * #### .patch(document)
 *
 * $(COLLECTIONMODIFY_PATCH_BRIEF)
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
 * #### .arrayInsert(docPath, value)
 *
 * $(COLLECTIONMODIFY_ARRAYINSERT_DETAIL)
 *
 * $(COLLECTIONMODIFY_ARRAYINSERT_DETAIL1)
 *
 * #### .arrayAppend(docPath, value)
 *
 * $(COLLECTIONMODIFY_ARRAYAPPEND_DETAIL)
 *
 * #### .arrayDelete(docPath)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_DETAIL)
 *
 * $(COLLECTIONMODIFY_ARRAYDELETE_DETAIL1)
 *
 * #### .sort(...)
 *
 * ##### Overloads
 *
 * @li sort$(COLLECTIONMODIFY_SORT_SIGNATURE)
 * @li sort$(COLLECTIONMODIFY_SORT_SIGNATURE1)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL)
 * $(COLLECTIONMODIFY_SORT_DETAIL1)
 *
 * $(COLLECTIONMODIFY_SORT_DETAIL2)
 *
 * #### .limit(numberOfDocs)
 *
 * $(COLLECTIONMODIFY_LIMIT_DETAIL)
 *
 * #### .bind(name, value)
 *
 * $(COLLECTIONMODIFY_BIND_BRIEF)
 *
 * #### .execute()
 *
 * $(COLLECTIONMODIFY_EXECUTE_BRIEF)
 */
#if DOXYGEN_JS
/**
 *
 * #### Examples
 * \dontinclude "mysqlx_collection_modify.js"
 * \skip //@# CollectionModify: Set Execution
 * \until //@ CollectionModify: sorting and limit Execution - 4
 * \until print(dir(doc));
 */
#elif DOXYGEN_PY
/**
 *
 * #### Examples
 * \dontinclude "mysqlx_collection_modify.py"
 * \skip #@# CollectionModify: Set Execution
 * \until #@ CollectionModify: sorting and limit Execution - 4
 * \until print dir(doc)
 */
#endif
#if DOXYGEN_JS
CollectionModify Collection::modify(String searchCondition) {}
#elif DOXYGEN_PY
CollectionModify Collection::modify(str searchCondition) {}
#endif
shcore::Value Collection::modify_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionModify> collectionModify(
      new CollectionModify(shared_from_this()));

  return collectionModify->modify(args);
}

REGISTER_HELP_FUNCTION(remove, Collection);
REGISTER_HELP(COLLECTION_REMOVE_BRIEF, "Creates a document deletion handler.");
REGISTER_HELP(COLLECTION_REMOVE_CHAINED,
              "CollectionRemove.remove.[sort].[limit].[bind].[execute]");

/**
 * $(COLLECTION_REMOVE_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Collection</td><td>.remove(searchCondition)</td></tr>
 *     <tr><td></td><td>[.sort(...)]</td></tr>
 *     <tr><td></td><td>[.limit(numberOfDocs)]</td></tr>
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>[.execute()]</td></tr>
 *   </table>
 * </code>
 *
 * #### .remove(searchCondition)
 *
 * $(COLLECTIONREMOVE_REMOVE_DETAIL)
 *
 * $(COLLECTIONREMOVE_REMOVE_DETAIL1)
 *
 * $(COLLECTIONREMOVE_REMOVE_DETAIL2)
 *
 * $(COLLECTIONREMOVE_REMOVE_DETAIL3)
 *
 * #### .sort(...)
 *
 * ##### Overloads
 *
 * @li sort$(COLLECTIONREMOVE_SORT_SIGNATURE)
 * @li sort$(COLLECTIONREMOVE_SORT_SIGNATURE1)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL1)
 *
 * $(COLLECTIONREMOVE_SORT_DETAIL2)
 *
 * #### .limit(numberOfDocs)
 *
 * $(COLLECTIONREMOVE_LIMIT_BRIEF)
 *
 * $(COLLECTIONREMOVE_LIMIT_DETAIL)
 *
 * #### .bind(name, value)
 *
 * $(COLLECTIONREMOVE_BIND_DETAIL)
 *
 * $(COLLECTIONREMOVE_BIND_DETAIL1)
 *
 * #### .execute()
 *
 * $(COLLECTIONREMOVE_EXECUTE_BRIEF)
 *
 * \sa CollectionRemove
 *
 * #### Examples
 */
#if DOXYGEN_JS
/**
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
 *
 */
#elif DOXYGEN_PY
/**
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
 *
 */
#endif
#if DOXYGEN_JS
CollectionRemove Collection::remove(String searchCondition) {}
#elif DOXYGEN_PY
CollectionRemove Collection::remove(str searchCondition) {}
#endif
shcore::Value Collection::remove_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionRemove> collectionRemove(
      new CollectionRemove(shared_from_this()));

  return collectionRemove->remove(args);
}

REGISTER_HELP_FUNCTION(find, Collection);
REGISTER_HELP(
    COLLECTION_FIND_BRIEF,
    "Retrieves documents from a collection, matching a specified criteria.");
REGISTER_HELP(COLLECTION_FIND_CHAINED,
              "CollectionFind.find.[fields].[<<<groupBy>>>->[having]].[sort]."
              "[limit->[skip]].[<<<lockShared>>>].[<<<lockExclusive>>>].[bind]."
              "[execute]");

/**
 * $(COLLECTION_FIND_BRIEF)
 *
 * ### Full Syntax
 *
 * <code>
 *   <table border = "0">
 *     <tr><td>Collection</td><td>.find([searchCondition])</td></tr>
 *     <tr><td></td><td>[.fields(...)]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.groupBy(...)[.having(condition)]]</td></tr>*/
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.group_by(...)[.having(condition)]]</td></tr>*/
#endif
/**
 *     <tr><td></td><td>[.sort(...)]</td></tr>
 *     <tr><td></td><td>[.limit(numberOfDocs)[.skip(numberOfDocs)]]</td></tr>
 */
#if DOXYGEN_JS
/**
 * <tr><td></td><td>[.lockShared([lockContention])|.lockExclusive([lockContention])]</td></tr>*/
#elif DOXYGEN_PY
/**
 * <tr><td></td><td>[.lock_shared(lockContention)|.lock_exclusive(lockContention)]</td></tr>*/
#endif
/**
 *     <tr><td></td><td>[.bind(name, value)]</td></tr>
 *     <tr><td></td><td>.execute()</td></tr>
 *   </table>
 * </code>
 *
 * #### .find([searchCondition])
 *
 * $(COLLECTIONFIND_FIND_DETAIL)
 *
 * $(COLLECTIONFIND_FIND_DETAIL1)
 *
 * #### .fields(...)
 *
 * ##### Overloads
 *
 * @li $(COLLECTIONFIND_FIELDS_SIGNATURE)
 * @li $(COLLECTIONFIND_FIELDS_SIGNATURE1)
 * @li $(COLLECTIONFIND_FIELDS_SIGNATURE2)
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
 */

#if DOXYGEN_JS
/**
 *
 * #### .groupBy(...)
 *
 * ##### Overloads
 *
 * @li groupBy$(COLLECTIONFIND_GROUPBY_SIGNATURE)
 * @li groupBy$(COLLECTIONFIND_GROUPBY_SIGNATURE1)
 */
#elif DOXYGEN_PY
/**
 *
 * #### .group_by(...)
 *
 * ##### Overloads
 *
 * @li group_by$(COLLECTIONFIND_GROUPBY_SIGNATURE)
 * @li group_by$(COLLECTIONFIND_GROUPBY_SIGNATURE1)
 */
#endif

/**
 * $(COLLECTIONFIND_GROUPBY_DETAIL)
 *
 * #### .having(condition)
 *
 * $(COLLECTIONFIND_HAVING_DETAIL)
 *
 * #### .sort(...)
 *
 * ##### Overloads
 *
 * @li sort$(COLLECTIONFIND_SORT_SIGNATURE)
 * @li sort$(COLLECTIONFIND_SORT_SIGNATURE1)
 *
 * $(COLLECTIONFIND_SORT_DETAIL)
 *
 * $(COLLECTIONFIND_SORT_DETAIL1)
 *
 * $(COLLECTIONFIND_SORT_DETAIL2)
 *
 * $(COLLECTIONFIND_SORT_DETAIL3)
 *
 * #### .limit(numberOfDocs)
 *
 * $(COLLECTIONFIND_LIMIT_DETAIL)
 *
 * #### .skip(numberOfDocs)
 *
 * $(COLLECTIONFIND_SKIP_DETAIL)
 */

#if DOXYGEN_JS
/**
 *
 * #### .lockShared([lockContention])
 *
 */
#elif DOXYGEN_PY
/**
 *
 * #### .lock_shared([lockContention])
 *
 */
#endif
/**
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
 * $(COLLECTIONFIND_LOCKSHARED_DETAIL14)
 */
#if DOXYGEN_JS
/**
 *
 * #### .lockExclusive([lockContention])
 *
 */
#elif DOXYGEN_PY
/**
 *
 * #### .lock_exclusive([lockContention])
 *
 */
#endif

/**
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
 * #### .bind(name, value)
 *
 * $(COLLECTIONFIND_BIND_DETAIL)
 *
 * $(COLLECTIONFIND_BIND_DETAIL1)
 *
 * $(COLLECTIONFIND_BIND_DETAIL2)
 *
 * #### .execute()
 *
 * $(COLLECTIONFIND_EXECUTE_BRIEF)
 *
 * ### Examples
 */
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
CollectionFind Collection::find(...) {}
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
CollectionFind Collection::find(...) {}
#endif
shcore::Value Collection::find_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionFind> collectionFind(
      new CollectionFind(shared_from_this()));

  return collectionFind->find(args);
}

REGISTER_HELP_FUNCTION(createIndex, Collection);
REGISTER_HELP(COLLECTION_CREATEINDEX_BRIEF,
              "Creates an index on a collection.");
REGISTER_HELP(COLLECTION_CREATEINDEX_PARAM,
              "@param name the name of the index to be created.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_PARAM1,
    "@param indexDefinition a JSON document with the index information.");
REGISTER_HELP(COLLECTION_CREATEINDEX_RETURNS, "@returns a Result object.");
REGISTER_HELP(COLLECTION_CREATEINDEX_DETAIL,
              "This function will create an index on the collection using the "
              "information "
              "provided in indexDefinition.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL1,
    "The indexDefinition is a JSON document with the next information:");
REGISTER_HELP(COLLECTION_CREATEINDEX_DETAIL2,
              "<code>{\n"
              "&nbsp;&nbsp;fields : [@<index_field@>, ...],\n"
              "&nbsp;&nbsp;type   : @<type@>\n"
              "}</code>");
REGISTER_HELP(COLLECTION_CREATEINDEX_DETAIL3,
              "@li fields array of index_field objects, each describing a "
              "single document "
              "member to be included in the index.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL4,
    "@li type string, (optional) the type of index. One of INDEX or SPATIAL. "
    "Default is INDEX and may be omitted.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL5,
    "A single index_field description consists of the following fields:");
REGISTER_HELP(COLLECTION_CREATEINDEX_DETAIL6,
              "<code>{\n"
              "&nbsp;&nbsp;field    : @<field@>,\n"
              "&nbsp;&nbsp;type     : @<type@>,\n"
              "&nbsp;&nbsp;required : @<boolean@>\n"
              "&nbsp;&nbsp;options  : @<uint@>,\n"
              "&nbsp;&nbsp;srid     : @<uint@>\n"
              "}</code>");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL7,
    "@li field: string, the full document path to the document member or field "
    "to be indexed.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL8,
    "@li type: string, one of the supported SQL column types to map the field "
    "into. For numeric types, the optional UNSIGNED keyword may follow. For "
    "the "
    "TEXT type, the length to consider for indexing may be added.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL9,
    "@li required: bool, (optional) true if the field is required to exist in "
    "the document. defaults to false, except for GEOJSON where it defaults to "
    "true.");
REGISTER_HELP(
    COLLECTION_CREATEINDEX_DETAIL10,
    "@li options: uint, (optional) special option flags for use when decoding "
    "GEOJSON data");
REGISTER_HELP(COLLECTION_CREATEINDEX_DETAIL11,
              "@li srid: uint, (optional) srid value for use when decoding "
              "GEOJSON data.");
REGISTER_HELP(COLLECTION_CREATEINDEX_DETAIL12,
              "The 'options' and 'srid' fields in IndexField can and must be "
              "present only "
              "if 'type' is set to 'GEOJSON'.");

/**
 * $(COLLECTION_CREATEINDEX_BRIEF)
 *
 * $(COLLECTION_CREATEINDEX_PARAM)
 * $(COLLECTION_CREATEINDEX_PARAM1)
 *
 * $(COLLECTION_CREATEINDEX_RETURNS)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL)
 * $(COLLECTION_CREATEINDEX_DETAIL1)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL2)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL3)
 * $(COLLECTION_CREATEINDEX_DETAIL4)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL5)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL6)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL7)
 * $(COLLECTION_CREATEINDEX_DETAIL8)
 * $(COLLECTION_CREATEINDEX_DETAIL9)
 * $(COLLECTION_CREATEINDEX_DETAIL10)
 * $(COLLECTION_CREATEINDEX_DETAIL11)
 *
 * $(COLLECTION_CREATEINDEX_DETAIL12)
 */
//@{
#if DOXYGEN_JS
Result Collection::createIndex(String name, JSON indexDefinition) {}
#elif DOXYGEN_PY
Result Collection::create_index(str name, JSON indexDefinition) {}
#endif
//@}

shcore::Value Collection::create_index_(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("createIndex").c_str());
  shcore::Value ret_val;
  try {
    auto index_name = args.string_at(0);
    auto index = args.map_at(1);

    std::shared_ptr<ShellBaseSession> session_obj = session();
    std::shared_ptr<DatabaseObject> schema_obj = schema();

    if (session_obj && schema_obj) {
      (*index)["schema"] = shcore::Value(schema_obj->name());
      (*index)["collection"] = shcore::Value(_name);
      (*index)["name"] = shcore::Value(index_name);

      // Moves "fields" to "constraint"
      if (index->has_key("fields")) {
        std::swap((*index)["constraint"], (*index)["fields"]);
        index->erase("fields");

        if ((*index)["constraint"].type == shcore::Array) {
          for (auto &field_val : *index->get_array("constraint")) {
            if (field_val.type == shcore::Map) {
              auto field = field_val.as_map();
              if (field->has_key("field")) {
                // Moves "field" to "member"
                std::swap((*field)["member"], (*field)["field"]);
                field->erase("field");

                // The options and srid values must be converted to UINT
                // or the plugin will fail with an invalid data type error
                if (field->has_key("options")) {
                  uint64_t options = (*field)["options"].as_uint();
                  (*field)["options"] = shcore::Value(options);
                }
                if (field->has_key("srid")) {
                  uint64_t srid = (*field)["srid"].as_uint();
                  (*field)["srid"] = shcore::Value(srid);
                }
              }

              bool is_geojson = false;
              if (field->has_key("type") &&
                  (*field)["type"].type == shcore::String)
                is_geojson = shcore::str_caseeq(
                    (*field)["type"].get_string().c_str(), "GEOJSON");

              if (!field->has_key("required")) {
                (*field)["required"] = shcore::Value(is_geojson);
              }
            }
          }
        }
      }

      // Sets the default for unique
      if (!index->has_key("unique"))
        (*index)["unique"] = shcore::Value::False();

      // Default index type is INDEX
      if (!index->has_key("type")) (*index)["type"] = shcore::Value("index");

      auto session = std::dynamic_pointer_cast<Session>(session_obj);

      if (session) {
        // This inner try/catch is just to translate plugin errors to
        // devapi errors
        try {
          auto x_result =
              session->execute_mysqlx_stmt("create_collection_index", index);
          SqlResult *sql_result = new mysqlsh::mysqlx::SqlResult(x_result);
          ret_val = shcore::Value::wrap(sql_result);
        } catch (const mysqlshdk::db::Error &e) {
          std::string error = e.what();
          error = shcore::str_replace(error, "'constraint'", "'fields'");
          error = shcore::str_replace(error, "'member'", "'field'");
          error = shcore::str_replace(error, "constraint.required",
                                      "field.required");
          throw mysqlshdk::db::Error(error.c_str(), e.code(), e.sqlstate());
        }
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("createIndex"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(dropIndex, Collection);
REGISTER_HELP(COLLECTION_DROPINDEX_BRIEF, "Drops an index from a collection.");

/**
 * $(COLLECTION_DROPINDEX_BRIEF)
 */
#if DOXYGEN_JS
Undefined Collection::dropIndex(String name) {}
#elif DOXYGEN_PY
None Collection::drop_index(str name) {}
#endif
shcore::Value Collection::drop_index_(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("dropIndex").c_str());

  try {
    args.string_at(0);

    shcore::Dictionary_t drop_index_args = shcore::make_dict();
    Value schema = this->get_member("schema");
    (*drop_index_args)["schema"] = schema.as_object()->get_member("name");
    (*drop_index_args)["collection"] = this->get_member("name");
    (*drop_index_args)["name"] = args[0];

    Value session = this->get_member("session");
    auto session_obj = std::static_pointer_cast<Session>(session.as_object());
    try {
      session_obj->_execute_mysqlx_stmt("drop_collection_index",
                                        drop_index_args);
    } catch (const mysqlshdk::db::Error &e) {
      if (e.code() != ER_CANT_DROP_FIELD_OR_KEY) throw;
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dropIndex"));
  return shcore::Value();
}

REGISTER_HELP_FUNCTION(replaceOne, Collection);
REGISTER_HELP(COLLECTION_REPLACEONE_BRIEF,
              "Replaces an existing document with a new document.");
REGISTER_HELP(COLLECTION_REPLACEONE_PARAM,
              "@param id identifier of the document to be replaced.");
REGISTER_HELP(COLLECTION_REPLACEONE_PARAM1, "@param doc the new document.");
REGISTER_HELP(
    COLLECTION_REPLACEONE_RETURNS,
    "@returns A Result object containing the number of affected rows.");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL,
              "Replaces the document identified with the given id. If no "
              "document is found "
              "matching the given id the returned Result will indicate 0 "
              "affected items.");
REGISTER_HELP(COLLECTION_REPLACEONE_DETAIL1,
              "Only one document will be affected by this operation.");
REGISTER_HELP(
    COLLECTION_REPLACEONE_DETAIL2,
    "The id of the document remain inmutable, if the new document contains a "
    "different id, it will be ignored.");
REGISTER_HELP(
    COLLECTION_REPLACEONE_DETAIL3,
    "Any constraint (unique key) defined on the collection is applicable:");
REGISTER_HELP(
    COLLECTION_REPLACEONE_DETAIL4,
    "The operation will fail if the new document contains a unique key which "
    "is "
    "already defined for any document in the collection except the one being "
    "replaced.");

/**
 * $(COLLECTION_REPLACEONE_BRIEF)
 *
 * $(COLLECTION_REPLACEONE_PARAM)
 * $(COLLECTION_REPLACEONE_PARAM1)
 *
 * $(COLLECTION_REPLACEONE_RETURNS)
 *
 * $(COLLECTION_REPLACEONE_DETAIL)
 *
 * $(COLLECTION_REPLACEONE_DETAIL1)
 *
 * $(COLLECTION_REPLACEONE_DETAIL2)
 *
 * $(COLLECTION_REPLACEONE_DETAIL3)
 *
 * $(COLLECTION_REPLACEONE_DETAIL4)
 */
#if DOXYGEN_JS
Result Collection::replaceOne(String id, Document doc) {}
#elif DOXYGEN_PY
Result Collection::replace_one(str id, document doc) {}
#endif
shcore::Value Collection::replace_one_(const Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(2, get_function_name("replaceOne").c_str());
  try {
    auto id = args.string_at(0);
    auto document = args.map_at(1);

    CollectionModify modify_op(shared_from_this());
    modify_op.set_filter("_id = :id").bind("id", args[0]);
    modify_op.set_operation(Mysqlx::Crud::UpdateOperation::ITEM_SET, "",
                            args[1]);
    ret_val = modify_op.execute();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("replaceOne"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(addOrReplaceOne, Collection);
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_BRIEF,
              "Replaces or adds a document in a collection.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_PARAM,
              "@param id the identifier of the document to be replaced.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_PARAM1,
              "@param doc the new document.");
REGISTER_HELP(
    COLLECTION_ADDORREPLACEONE_RETURNS,
    "@returns A Result object containing the number of affected rows.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL,
              "Replaces the document identified with the given id. If no "
              "document is found matching the given id the given document will "
              "be added to the collection.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL1,
              "Only one document will be affected by this operation.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL2,
              "The id of the document remains inmutable, if the new document "
              "contains a different id, it will be ignored.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL3,
              "Any constraint (unique key) defined on the collection is "
              "applicable on both the replace and add operations:");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL4,
              "@li The replace operation will fail if the new document "
              "contains a unique key which is already defined for any document "
              "in the collection except the one being replaced.");
REGISTER_HELP(COLLECTION_ADDORREPLACEONE_DETAIL5,
              "@li The add operation will fail if the new document contains a "
              "unique key which is already defined for any document in the "
              "collection.");
/**
 * $(COLLECTION_ADDORREPLACEONE_BRIEF)
 *
 * $(COLLECTION_ADDORREPLACEONE_PARAM)
 * $(COLLECTION_ADDORREPLACEONE_PARAM1)
 *
 * $(COLLECTION_ADDORREPLACEONE_RETURNS)
 *
 * $(COLLECTION_ADDORREPLACEONE_DETAIL)
 *
 * $(COLLECTION_ADDORREPLACEONE_DETAIL1)
 *
 * $(COLLECTION_ADDORREPLACEONE_DETAIL2)
 *
 * $(COLLECTION_ADDORREPLACEONE_DETAIL3)
 *
 * $(COLLECTION_ADDORREPLACEONE_DETAIL4)
 *
 * $(COLLECTION_ADDORREPLACEONE_DETAIL5)
 */
#if DOXYGEN_JS
Result Collection::addOrReplaceOne(String id, Document doc) {}
#elif DOXYGEN_PY
Result Collection::add_or_replace_one(str id, document doc) {}
#endif
shcore::Value Collection::add_or_replace_one(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(2, get_function_name("addOrReplaceOne").c_str());
  try {
    auto id = args.string_at(0);
    auto document = args.map_at(1);

    // The document gets updated with given id
    (*document)["_id"] = shcore::Value(id);

    CollectionAdd add_op(shared_from_this());
    add_op.add_one_document(shcore::Value(document), "Parameter #1");
    ret_val = add_op.execute(true);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addOrReplaceOne"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(getOne, Collection);
REGISTER_HELP(COLLECTION_GETONE_BRIEF,
              "Fetches the document with the given _id from the collection.");
REGISTER_HELP(COLLECTION_GETONE_PARAM,
              "@param id The identifier of the document to be retrieved.");
REGISTER_HELP(COLLECTION_GETONE_RETURNS,
              "@returns The Document object matching the given id or NULL if "
              "no match is found.");
/**
 * $(COLLECTION_GETONE_BRIEF)
 *
 * $(COLLECTION_GETONE_PARAM)
 *
 * $(COLLECTION_GETONE_RETURNS)
 */
#if DOXYGEN_JS
Document Collection::getOne(String id) {}
#elif DOXYGEN_PY
document Collection::get_one(str id) {}
#endif
shcore::Value Collection::get_one(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("getOne").c_str());
  shcore::Value ret_val = shcore::Value::Null();
  try {
    auto id = args.string_at(0);

    CollectionFind find_op(shared_from_this());
    find_op.set_filter("_id = :id").bind("id", args[0]);
    auto result = find_op.execute();
    if (result) ret_val = result->fetch_one({});
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getOne"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(removeOne, Collection);
REGISTER_HELP(COLLECTION_REMOVEONE_BRIEF,
              "Removes document with the given _id value.");
REGISTER_HELP(COLLECTION_REMOVEONE_PARAM,
              "@param id The id of the document to be removed.");
REGISTER_HELP(
    COLLECTION_REMOVEONE_RETURNS,
    "@returns A Result object containing the number of affected rows.");
REGISTER_HELP(COLLECTION_REMOVEONE_DETAIL,
              "If no document is found matching the given id, the Result "
              "object will indicate 0 as the number of affected rows.");
/**
 * $(COLLECTION_REMOVEONE_BRIEF)
 *
 * $(COLLECTION_REMOVEONE_PARAM)
 *
 * $(COLLECTION_REMOVEONE_RETURNS)
 *
 * $(COLLECTION_REMOVEONE_DETAIL)
 */
#if DOXYGEN_JS
Result Collection::removeOne(String id) {}
#elif DOXYGEN_PY
Result Collection::remove_one(str id) {}
#endif
shcore::Value Collection::remove_one(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("removeOne").c_str());
  shcore::Value ret_val;
  try {
    auto id = args.string_at(0);

    CollectionRemove remove_op(shared_from_this());
    remove_op.set_filter("_id = :id").bind("id", args[0]);
    ret_val = remove_op.execute();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeOne"));

  return ret_val;
}

}  // namespace mysqlx
}  // namespace mysqlsh
