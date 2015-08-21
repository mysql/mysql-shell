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
}

Collection::~Collection()
{
}

#ifdef DOXYGEN
/**
* Adds a document to the collection. The document doc must have a property named '_id' with the universal unique identifier (uuid) that uniquely identifies the document.
* If the property is not there, it is added with an auto generated uuid.
* This method must be invoked before execute method, add can be invoked as many times as required.
* \sa add()
* \param doc the document to add.
* \return the same instance of the collection the method was invoked with.
*/
Collection Collection::add(document doc)
{}

/**
* Adds a set of documents to the collection. Each document must have a property named '_id' with the universal unique identifier (uuid) that uniquely identifies the document.
* If the property is not there, it is added with an auto generated uuid.
* This method must be invoked before execute, add can be invoked as many times as required.
* \sa add()
* \param document the array of documents to add.
* \return the same instance of the collection the method was invoked with.
*/
Collection Collection::add({ document }, { document }, ...)
{}
#endif
shcore::Value Collection::add_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionAdd> collectionAdd(new CollectionAdd(shared_from_this()));

  return collectionAdd->add(args);
}

#ifdef DOXYGEN
/**
* Sets the search condition of the statement to modify documents.
* This method needs to be invoked first, and can be invoked once, after it, the folllowing methods can be invoked: set, unset, arrayInsert, arrayAppend, arrayDelete,
* sort, limit, bind, execute.
* \param searchCondition An optional string with the filter expression of the documents to be modified.
* \return the same instance of the collection the method was invoked with.
*/
Collection Collection::modify(String searchCondition)
{}
#endif
shcore::Value Collection::modify_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionModify> collectionModify(new CollectionModify(shared_from_this()));

  return collectionModify->modify(args);
}

#ifdef DOXYGEN
/**
* Sets the search condition of the statement to move documents.
* This method needs to be invoked first, and can be invoked once, after it the following methods can be invoked: sort, limit, bind, execute
* \param searchCondition An optional string with the filter expression of the documents to be removed.
* \return the same instance of the collection the method was invoked with.
*/
Collection Collection::remove(String searchCondition)
{}
#endif
shcore::Value Collection::remove_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionRemove> collectionRemove(new CollectionRemove(shared_from_this()));

  return collectionRemove->remove(args);
}

#ifdef DOXYGEN
/**
* Sets the search condition to identify the Documents to be retrieved from the owner Collection.
* This method needs to be invoked first, and can be invoked once, after it, the following methods can be invoked: fields, groupBy, sort, limit, bind, execute.
* \param searchCondition: An optional expression to identify the documents to be retrieved, if not specified all the documents will be included on the result.
* \return itself (the collection instance) with it's state updated as the searchCondition was established.
*/
Collection Collection::find(String searchCriteria)
{}
#endif
shcore::Value Collection::find_(const shcore::Argument_list &args)
{
  boost::shared_ptr<CollectionFind> collectionFind(new CollectionFind(shared_from_this()));

  return collectionFind->find(args);
}