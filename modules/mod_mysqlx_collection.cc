/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_collection.h"

#include "mod_mysqlx_collection_add.h"
#include "mod_mysqlx_collection_find.h"
#include "mod_mysqlx_collection_remove.h"
#include "mod_mysqlx_collection_modify.h"
#include "mod_mysqlx_collection_create_index.h"
#include "mod_mysqlx_collection_drop_index.h"
#include "utils/utils_help.h"

using namespace std::placeholders;
using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Collection::Collection(std::shared_ptr<Schema> owner, const std::string &name)
  : DatabaseObject(owner->_session.lock(), std::static_pointer_cast<DatabaseObject>(owner), name),
  _collection_impl(owner->_schema_impl->getCollection(name)) {
  init();
}

Collection::Collection(std::shared_ptr<const Schema> owner, const std::string &name) :
DatabaseObject(owner->_session.lock(), std::const_pointer_cast<Schema>(owner), name) {
  _collection_impl = owner->_schema_impl->getCollection(name);
  init();
}

void Collection::init() {
  add_method("add", std::bind(&Collection::add_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("modify", std::bind(&Collection::modify_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("find", std::bind(&Collection::find_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("remove", std::bind(&Collection::remove_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("createIndex", std::bind(&Collection::create_index_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("dropIndex", std::bind(&Collection::drop_index_, this, _1), "searchCriteria", shcore::String, NULL);
}

Collection::~Collection() {}

REGISTER_HELP(COLLECTION_ADD_BRIEF, "Inserts one or more documents into a collection.");
REGISTER_HELP(COLLECTION_ADD_CHAINED, "CollectionAdd.add.[execute]");

/**
* $(COLLECTIONADD_ADD_BRIEF)
*
* ### Full Syntax
*
* <code>
*   <table border = "0">
*     <tr><td>Collection</td><td>$(COLLECTIONADD_ADD_SYNTAX)</td></tr>
*     <tr><td></td><td>$(COLLECTIONADD_ADD_SYNTAX1)</td></tr>
*     <tr><td></td><td>$(COLLECTIONADD_EXECUTE_SYNTAX)</td></tr>
*   </table>
* </code>
*
* #### .add(...)
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
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Chained Calls
*
* $(COLLECTIONADD_ADD_DETAIL9)
*
* $(COLLECTIONADD_ADD_DETAIL10)
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Using an Expression
*
* #### Using a Document List
* Adding document using an existing document list
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Document List
*
* #### Multiple Parameters
* Adding document using a separate parameter for each document on a single call to add(...)
* \snippet js_devapi/scripts/mysqlx_collection_add.js CollectionAdd: Multiple Parameters
*/
CollectionAdd Collection::add(DocDefinition document[, DocDefinition document, ...]) {}
#elif DOXYGEN_PY
/**
* Adding documents using chained calls to add(...)
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Chained Calls
*
* $(COLLECTIONADD_ADD_DETAIL9)
*
* $(COLLECTIONADD_ADD_DETAIL10)
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Using an Expression
*
* #### Using a Document List
* Adding document using an existing document list
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Document List
*
* #### Multiple Parameters
* Adding document using a separate parameter for each document on a single call to add(...)
* \snippet py_devapi/scripts/mysqlx_collection_add.py CollectionAdd: Multiple Parameters
*/
CollectionAdd Collection::add(DocDefinition document[, DocDefinition document, ...]) {}
#endif
shcore::Value Collection::add_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionAdd> collectionAdd(new CollectionAdd(shared_from_this()));

  return collectionAdd->add(args);
}

//! Creates a collection update handler.
#if DOXYGEN_CPP
//! \param args may contain an optional string with the filter expression of the documents to be modified.
#else
//! \param searchCondition An optional string with the filter expression of the documents to be modified.
#endif
/**
* \return A CollectionFind object.
*
* This function creates a CollectionModify object which is a document update handler.
*
* The CollectionModify class has several functions that allow specifying the way the update occurs, if a searchCondition was specified, it will be set on the handler.
*
* The update is done when the execute function is called on the handler.
*
* \sa CollectionModify
*/
#if DOXYGEN_JS
CollectionModify Collection::modify(String searchCondition) {}
#elif DOXYGEN_PY
CollectionModify Collection::modify(str searchCondition) {}
#endif
shcore::Value Collection::modify_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionModify> collectionModify(new CollectionModify(shared_from_this()));

  return collectionModify->modify(args);
}

//! Creates a document deletion handler.
#if DOXYGEN_CPP
//! \param args may contain an optional string with the filter expression of the documents to be deleted.
#else
//! \param searchCondition An optional string with the filter expression of the documents to be deleted.
#endif
/**
* \return A CollectionRemove object.
*
* This function creates a CollectionRemove object which is a document deletion handler.
*
* The CollectionRemove class has several functions that allow specifying what should be deleted and how, if a searchCondition was specified, it will be set on the handler.
*
* The deletion is done when the execute function is called on the handler.
*
* \sa CollectionRemove
*/
#if DOXYGEN_JS
CollectionRemove Collection::remove(String searchCondition) {}
#elif DOXYGEN_PY
CollectionRemove Collection::remove(str searchCondition) {}
#endif
shcore::Value Collection::remove_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionRemove> collectionRemove(new CollectionRemove(shared_from_this()));

  return collectionRemove->remove(args);
}

//! Retrieves documents from a collection.
#if DOXYGEN_CPP
//! \param args may contain an optional string with the filter expression of the documents to be retrieved.
#else
//! \param searchCriteria An optional string with the filter expression of the documents to be retrieved.
#endif
/**
* \return A CollectionFind object.
*
* This function creates a CollectionFind object which is a document selection handler.
*
* The CollectionFind class has several functions that allow specifying what should be retrieved from the collection, if a searchCondition was specified, it will be set on the handler.
*
* The selection will be returned when the execute function is called on the handler.
*
* \sa CollectionFind
*/
#if DOXYGEN_JS
CollectionFind Collection::find(String searchCriteria) {}
#elif DOXYGEN_PY
CollectionFind Collection::find(str searchCriteria) {}
#endif
shcore::Value Collection::find_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionFind> collectionFind(new CollectionFind(shared_from_this()));

  return collectionFind->find(args);
}

#if DOXYGEN_CPP
/**
 * Creates an index on a collection.
 * \param args should contain the name and optionally the type of index to be created.
 * \return A CollectionCreateIndex object.
 *
 * This function creates a CollectionCreateIndex object which is an index creation handler.
 *
 * The CollectionCreateIndex class has a function to define the fields to be included on the index.
 *
 * The index will be created when the execute function is called on the index creation handler.
 *
 * The function will create a non unique index unless mysqlx.IndexType.IndexUnique is passed as the second element on args.
 *
 * \sa CollectionCreateIndex
 */
#else
/**
* Creates a non unique index on a collection.
* \param name The name of the index to be created.
* \return A CollectionCreateIndex object.
*
* This function creates a CollectionCreateIndex object which is an index creation handler.
*
* The CollectionCreateIndex class has a function to define the fields to be included on the index.
*
* The index will be created when the execute function is called on the index creation handler.
*
* \sa CollectionCreateIndex
*/
#if DOXYGEN_JS
CollectionCreateIndex Collection::createIndex(String name) {}
#elif DOXYGEN_PY
CollectionCreateIndex Collection::create_index(str name) {}
#endif

/**
* Creates a unique index on a collection.
* \param name The name of the index to be created.
* \param type The type of index to be created.
* \return A CollectionCreateIndex object.
*
* This function creates a CollectionCreateIndex object which is an index creation handler.
*
* The CollectionCreateIndex class has a function to define the fields to be included on the index.
*
* The index will be created when the execute function is called on the index creation handler.
*
* The only available index type at the moment is mysqlx.IndexType.IndexUnique.
*
* \sa CollectionCreateIndex
*/
#if DOXYGEN_JS
CollectionCreateIndex Collection::createIndex(String name, IndexType type) {}
#elif DOXYGEN_PY
CollectionCreateIndex Collection::create_index(str name, IndexType type) {}
#endif
#endif

shcore::Value Collection::create_index_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionCreateIndex> createIndex(new CollectionCreateIndex(shared_from_this()));

  auto ss = createIndex->set_scoped_naming_style(naming_style);

  return createIndex->create_index(args);
}

//! Drops an index from a collection.
#if DOXYGEN_CPP
//! \param args should contain the name of the index to be dropped.
#else
//! \param name The name of the index to be dropped.
#endif
/**
* \return A CollectionDropIndex object.
*
* This function creates a CollectionDropIndex object.
*
* The index will be dropped when the execute function is called on the index dropping handler.
*
* \sa CollectionDropIndex
*/
#if DOXYGEN_JS
CollectionDropIndex Collection::dropIndex(String name) {}
#elif DOXYGEN_PY
CollectionDropIndex Collection::drop_index(str name) {}
#endif
shcore::Value Collection::drop_index_(const shcore::Argument_list &args) {
  std::shared_ptr<CollectionDropIndex> dropIndex(new CollectionDropIndex(shared_from_this()));

  return dropIndex->drop_index(args);
}
