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

shcore::Value Collection::add_(const shcore::Argument_list &args)
{
  args.ensure_count(1, "Collection::add");

  boost::shared_ptr<CollectionAdd> collectionAdd(new CollectionAdd(shared_from_this()));

  return collectionAdd->add(args);
}

shcore::Value Collection::modify_(const shcore::Argument_list &args)
{
  std::string searchCriteria;
  args.ensure_count(1, "Collection::modify");
  searchCriteria = args.string_at(0);
  // return shcore::Value::wrap(new CollectionFind(_coll->find(searchCriteria)));
  return shcore::Value();
}

shcore::Value Collection::remove_(const shcore::Argument_list &args)
{
  std::string searchCriteria;
  args.ensure_count(1, "Collection::remove");
  searchCriteria = args.string_at(0);
  // return shcore::Value::wrap(new CollectionFind(_coll->find(searchCriteria)));
  return shcore::Value();
}

shcore::Value Collection::find_(const shcore::Argument_list &args)
{
  std::string searchCriteria;
  args.ensure_count(1, "Collection::find");
  searchCriteria = args.string_at(0);
  // return shcore::Value::wrap(new CollectionFind(_coll->find(searchCriteria)));
  return shcore::Value();
}