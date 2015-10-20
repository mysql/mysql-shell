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
#include <boost/bind.hpp>
#include "mod_mysqlx_collection_drop_index.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_resultset.h"
#include "uuid_gen.h"

#include <iomanip>
#include <sstream>
#include <boost/format.hpp>

using namespace mysh::mysqlx;
using namespace shcore;

CollectionDropIndex::CollectionDropIndex(boost::shared_ptr<Collection> owner)
:_owner(owner)
{
  // Exposes the methods available for chaining
  add_method("dropIndex", boost::bind(&CollectionDropIndex::drop_index, this, _1), "data");
  add_method("execute", boost::bind(&CollectionDropIndex::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("dropIndex", "");
  register_dynamic_function("execute", "dropIndex");
  register_dynamic_function("__shell_hook__", "dropIndex");

  // Initial function update
  update_functions("");
}

#ifdef DOXYGEN
/**
* Drops a collection index based on its name.
* \param indexName The name of the index to be dropped.
* \return This CollectionDropIndex object.
*/
CollectionDropIndex CollectionDropIndex::dropIndex(String indexName){}
#endif
shcore::Value CollectionDropIndex::drop_index(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, 2, "CollectionCreateIndex.dropIndex");

  try
  {
    // Does nothing with the values, but just calling the proper getter performs
    // standard data type validation.
    args.string_at(0);

    boost::shared_ptr<Collection> raw_owner(_owner.lock());

    if (raw_owner)
    {
      Value schema = raw_owner->get_member("schema");
      _drop_index_args.push_back(schema.as_object()->get_member("name"));
      _drop_index_args.push_back(raw_owner->get_member("name"));
      _drop_index_args.push_back(args[0]);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION("CollectionCreateIndex.createIndex");

  update_functions("dropIndex");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

#ifdef DOXYGEN
/**
* Executes the drop index operation for the index indicated in dropIndex.
* \return A Result object.
*
* This function can be invoked once after:
* \sa dropIndex(String indexName)
*/
Result CollectionDropIndex::execute(){}
#endif
shcore::Value CollectionDropIndex::execute(const shcore::Argument_list &args)
{
  Value result;

  args.ensure_count(0, "CollectionDropIndex.execute");

  boost::shared_ptr<Collection> raw_owner(_owner.lock());

  if (raw_owner)
  {
    Value session = raw_owner->get_member("session");
    boost::shared_ptr<BaseSession> session_obj = boost::static_pointer_cast<BaseSession>(session.as_object());
    result = session_obj->executeAdminCommand("drop_collection_index", false, _drop_index_args);
  }

  update_functions("execute");

  return result;
}