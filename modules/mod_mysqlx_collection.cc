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

#include "mod_mysqlx_schema.h"
#include "mod_mysqlx_collection.h"
#include <boost/bind.hpp>

#include "mod_mysqlx_collection_add.h"
#include "mod_mysqlx_collection_find.h"
#include "mod_mysqlx_collection_remove.h"
#include "mod_mysqlx_collection_modify.h"
#include "mod_mysqlx_collection_create_index.h"
#include "mod_mysqlx_collection_drop_index.h"

using namespace mysh;
using namespace mysh::mysqlx;
using namespace shcore;

Collection::Collection(boost::shared_ptr<Schema> owner, const std::string &name)
: DatabaseObject(owner->_session.lock(), boost::static_pointer_cast<DatabaseObject>(owner), name),
  _collection_impl(owner->_schema_impl->getCollection(name))
{
  init();
}

Collection::Collection(boost::shared_ptr<const Schema> owner, const std::string &name) :
DatabaseObject(owner->_session.lock(), boost::const_pointer_cast<Schema>(owner), name)
{
  init();
}

void Collection::init()
{
  add_method("add", boost::bind(&Collection::add_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("modify", boost::bind(&Collection::modify_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("find", boost::bind(&Collection::find_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("remove", boost::bind(&Collection::remove_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("createIndex", boost::bind(&Collection::create_index_, this, _1), "searchCriteria", shcore::String, NULL);
  add_method("dropIndex", boost::bind(&Collection::drop_index_, this, _1), "searchCriteria", shcore::String, NULL);
}

Collection::~Collection()
{
}

#ifdef DOXYGEN
/**
* Adds a document to a collection.
* \param document The document to be added into the collection.
* \return A CollectionAdd object.
*
* To be added, the document must have a property named '_id' with a universal unique identifier (UUID). If the property is missing, it is set with an auto generated UUID.
*
* This function creates a CollectionAdd object which is a document addition handler, the received document is added into this handler.
*
* The CollectionAdd class has other functions that allow specifying the way the addition occurs.
*
* The addition is done when the execute function is called on the handler.
*
* \sa CollectionAdd
*/
CollectionAdd Collection::add(Document document){}

/**
* Adds a list of documents to a collection.
* \param documents The document list to be added into the collection.
* \return A CollectionAdd object.
*
* Every document to be added must have a property named '_id' with a universal unique identifier (UUID). If the property is missing, it is set with an auto generated UUID.
*
* This function creates a CollectionAdd object which is a document addition handler, the received documents are added into this handler.
*
* The CollectionAdd class has other functions that allow specifying the way the addition occurs.
*
* The addition is done when the execute function is called on the handler.
*
* \sa CollectionAdd
*/
CollectionAdd Collection::add(List documents){}
#endif
shcore::Value Collection::add_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionAdd> collectionAdd(new CollectionAdd(shared_from_this()));

  return collectionAdd->add(args);
}

#ifdef DOXYGEN
/**
* Creates a collection update handler.
* \param searchCondition An optional string with the filter expression of the documents to be modified.
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
CollectionModify Collection::modify(String searchCondition){}
#endif
shcore::Value Collection::modify_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionModify> collectionModify(new CollectionModify(shared_from_this()));

  return collectionModify->modify(args);
}

#ifdef DOXYGEN
/**
* Creates a document deletion handler.
* \param searchCondition An optional string with the filter expression of the documents to be deleted.
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
CollectionRemove Collection::remove(String searchCondition){}
#endif
shcore::Value Collection::remove_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionRemove> collectionRemove(new CollectionRemove(shared_from_this()));

  return collectionRemove->remove(args);
}

#ifdef DOXYGEN
/**
* Retrieves documents from a collection.
* \param searchCriteria An optional string with the filter expression of the documents to be retrieved.
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
CollectionFind Collection::find(String searchCriteria){}
#endif
shcore::Value Collection::find_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionFind> collectionFind(new CollectionFind(shared_from_this()));

  return collectionFind->find(args);
}

#ifdef DOXYGEN
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
CollectionCreateIndex Collection::createIndex(String name){}

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
* The only available index type at the moment is mysqlx.IndexUnique.
*
* \sa CollectionCreateIndex
*/
CollectionCreateIndex Collection::createIndex(String name, IndexType type){}
#endif

shcore::Value Collection::create_index_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionCreateIndex> createIndex(new CollectionCreateIndex(shared_from_this()));

  return createIndex->create_index(args);
}

#ifdef DOXYGEN
/**
* Drops an index from a collection.
* \param name The name of the index to be dropped.
* \return A CollectionDropIndex object.
*
* This function creates a CollectionDropIndex object.
*
* The index will be dropped when the execute function is called on the index dropping handler.
*
* \sa CollectionDropIndex
*/
CollectionDropIndex Collection::dropIndex(String name){}
#endif
shcore::Value Collection::drop_index_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionDropIndex> dropIndex(new CollectionDropIndex(shared_from_this()));

  return dropIndex->drop_index(args);
}