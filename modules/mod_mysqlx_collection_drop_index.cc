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
#include "mod_mysqlx_collection_drop_index.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_resultset.h"
#include "uuid_gen.h"
#include "shellcore/utils_help.h"

#include <iomanip>
#include <sstream>

using namespace std::placeholders;
using namespace mysqlsh::mysqlx;
using namespace shcore;

// Documentation of CollectionDropIndex class
REGISTER_HELP(COLLECTIONDROPINDEX_BRIEF, "Handler for index dropping handler on a Collection.");
REGISTER_HELP(COLLECTIONDROPINDEX_DETAIL, "This object allows dropping an index from a collection.");
REGISTER_HELP(COLLECTIONDROPINDEX_DETAIL1, "This object should only be created by calling the dropIndex function on the collection object where the index will be removed.");

CollectionDropIndex::CollectionDropIndex(std::shared_ptr<Collection> owner)
  :_owner(owner) {
  // Exposes the methods available for chaining
  add_method("dropIndex", std::bind(&CollectionDropIndex::drop_index, this, _1), "data");
  add_method("execute", std::bind(&CollectionDropIndex::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("dropIndex", "");
  register_dynamic_function("execute", "dropIndex");
  register_dynamic_function("__shell_hook__", "dropIndex");

  // Initial function update
  update_functions("");
}

// Documentation of dropIndex function
REGISTER_HELP(COLLECTIONDROPINDEX_DROPINDEX_BRIEF, "Drops an index from a collection.");
REGISTER_HELP(COLLECTIONDROPINDEX_DROPINDEX_PARAM, "@param indexName The name of the index to be dropped.");
REGISTER_HELP(COLLECTIONDROPINDEX_DROPINDEX_RETURN, "@return This CollectionDropIndex object.");

/**
* $(COLLECTIONDROPINDEX_DROPINDEX_BRIEF)
*
* $(COLLECTIONDROPINDEX_DROPINDEX_PARAM)
*
* $(COLLECTIONDROPINDEX_DROPINDEX_RETURN)
*
* #### Method Chaining
*
* This function can be invoked only once but the operation will be performed when the execute() function is invoked.
*/
//@{
#if DOXYGEN_JS
CollectionDropIndex CollectionDropIndex::dropIndex(String indexName) {}
#elif DOXYGEN_PY
CollectionDropIndex CollectionDropIndex::drop_index(str indexName) {}
#endif
//@}
shcore::Value CollectionDropIndex::drop_index(const shcore::Argument_list &args) {
  // Each method validates the received parameters
  args.ensure_count(1, 2, get_function_name("dropIndex").c_str());

  try {
    // Does nothing with the values, but just calling the proper getter performs
    // standard data type validation.
    args.string_at(0);

    std::shared_ptr<Collection> raw_owner(_owner.lock());

    if (raw_owner) {
      Value schema = raw_owner->get_member("schema");
      _drop_index_args.push_back(schema.as_object()->get_member("name"));
      _drop_index_args.push_back(raw_owner->get_member("name"));
      _drop_index_args.push_back(args[0]);
    }
  }
  CATCH_AND_TRANSLATE_CRUD_EXCEPTION(get_function_name("dropIndex"));

  update_functions("dropIndex");

  return Value(std::static_pointer_cast<Object_bridge>(shared_from_this()));
}

// Documentation of execute function
REGISTER_HELP(COLLECTIONDROPINDEX_EXECUTE_BRIEF, "Executes the drop index operation for the index indicated in dropIndex.");
REGISTER_HELP(COLLECTIONDROPINDEX_EXECUTE_RETURN, "@return A Result object.");

/**
* $(COLLECTIONDROPINDEX_EXECUTE_BRIEF)
*
* $(COLLECTIONDROPINDEX_EXECUTE_RETURN)
*
* This function can be invoked once after:
*/
#if DOXYGEN_JS
Result CollectionDropIndex::execute() {}
#elif DOXYGEN_PY
Result CollectionDropIndex::execute() {}
#endif
shcore::Value CollectionDropIndex::execute(const shcore::Argument_list &args) {
  Value result;

  args.ensure_count(0, get_function_name("execute").c_str());

  std::shared_ptr<Collection> raw_owner(_owner.lock());

  try {
    if (raw_owner) {
      Value session = raw_owner->get_member("session");
      std::shared_ptr<BaseSession> session_obj = std::static_pointer_cast<BaseSession>(session.as_object());
      result = session_obj->executeAdminCommand("drop_collection_index", false, _drop_index_args);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("execute"));

  update_functions("execute");

  return result;
}
