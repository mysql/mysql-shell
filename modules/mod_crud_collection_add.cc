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
#include "mod_crud_collection_add.h"
#include "mod_mysqlx_resultset.h"

using namespace mysh::mysqlx;
using namespace shcore;

CollectionAdd::CollectionAdd(const ::mysqlx::AddStatement &add_)
: _add(new ::mysqlx::AddStatement(add_))
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

  _add->add(::mysqlx::Document(args.string_at(0)));

  // Updates the exposed functions
  update_functions("add");

  return Value(Object_bridge_ref(this));
}

shcore::Value CollectionAdd::execute(const shcore::Argument_list &args)
{
  args.ensure_count(0, "CollectionAdd::execute");

  return shcore::Value::wrap(new mysqlx::Collection_resultset(boost::shared_ptr< ::mysqlx::Result>(_add->execute())));
}