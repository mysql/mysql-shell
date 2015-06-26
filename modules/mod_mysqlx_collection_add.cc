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
#include "mod_mysqlx_collection_add.h"
#include "mod_mysqlx_collection.h"
#include "mod_mysqlx_resultset.h"
#include "uuid_gen.h"

#include <iomanip>
#include <sstream>

using namespace mysh::mysqlx;
using namespace shcore;

CollectionAdd::CollectionAdd(boost::shared_ptr<Collection> owner)
:Crud_definition(boost::static_pointer_cast<DatabaseObject>(owner))
{
  // Exposes the methods available for chaining
  add_method("add", boost::bind(&CollectionAdd::add, this, _1), "data");
  add_method("execute", boost::bind(&Crud_definition::execute, this, _1), "data");

  // Registers the dynamic function behavior
  register_dynamic_function("add", "");
  register_dynamic_function("execute", "add");

  // Initial function update
  update_functions("");
}

shcore::Value CollectionAdd::add(const shcore::Argument_list &args)
{
  // Each method validates the received parameters
  args.ensure_count(1, "CollectionAdd::add");

  boost::shared_ptr<DatabaseObject> raw_owner(_owner.lock());

  if (raw_owner)
  {
    boost::shared_ptr<Collection> collection(boost::static_pointer_cast<Collection>(_owner.lock()));

    if (collection)
    {
      shcore::Value::Array_type_ref shell_docs;

      if (args[0].type == Map)
      {
        // On a single document parameter, creates an array and processes it as a list of
        // documents, only advantage of this is avoid duplicating validation and setup logic
        shell_docs.reset(new Value::Array_type());
        shell_docs->push_back(args[0]);
      }
      else if (args[0].type == Array)
        shell_docs = args[0].as_array();
      else
        throw shcore::Exception::argument_error("Invalid document specified on add operation.");

      if (shell_docs)
      {
        size_t index, size = shell_docs->size();
        for (index = 0; index < size; index++)
        {
          Value element = shell_docs->at(index);
          if (element.type == Map)
          {
            Value::Map_type_ref shell_doc = element.as_map();

            if (!shell_doc->has_key("_id"))
              (*shell_doc)["_id"] = Value(get_new_uuid());

            ::mysqlx::Document inner_doc(element.repr());

            if (!index)
              _add_statement.reset(new ::mysqlx::AddStatement(collection->_collection_impl->add(inner_doc)));
            else
              _add_statement->add(inner_doc);
          }
          else
            throw shcore::Exception::argument_error("Invalid document specified on list for add operation.");
        }
      }
    }
  }

  // Updates the exposed functions
  update_functions("add");

  return Value(boost::static_pointer_cast<Object_bridge>(shared_from_this()));
}

std::string CollectionAdd::get_new_uuid()
{
  uuid_type uuid;
  generate_uuid(uuid);

  std::stringstream str;
  str << std::hex << std::noshowbase << std::setfill('0') << std::setw(2);
  //*
  str << (int)uuid[0] << std::setw(2) << (int)uuid[1] << std::setw(2) << (int)uuid[2] << std::setw(2) << (int)uuid[3];
  str << std::setw(2) << (int)uuid[4] << std::setw(2) << (int)uuid[5];
  str << std::setw(2) << (int)uuid[6] << std::setw(2) << (int)uuid[7];
  str << std::setw(2) << (int)uuid[8] << std::setw(2) << (int)uuid[9];
  str << std::setw(2) << (int)uuid[10] << std::setw(2) << (int)uuid[11]
    << std::setw(2) << (int)uuid[12] << std::setw(2) << (int)uuid[13]
    << std::setw(2) << (int)uuid[14] << std::setw(2) << (int)uuid[15];

  return str.str();
}

shcore::Value CollectionAdd::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionAdd::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_add_statement->execute())));
}