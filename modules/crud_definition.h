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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_CRUD_DEFINITION_H_
#define _MOD_CRUD_DEFINITION_H_

#include "shellcore/types_cpp.h"
#include "shellcore/common.h"
#include "boost/weak_ptr.hpp"
#include "boost/enable_shared_from_this.hpp"

#include <set>

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/
static void ATTR_UNUSED translate_crud_exception(const std::string& operation, const std::string& param_type)
{
  try
  {
    throw;
  }
  catch (shcore::Exception &e)
  {
    if (e.is_type())
      throw shcore::Exception::argument_error(operation + ": " + param_type + " parameter required.");
    else
      throw shcore::Exception::argument_error(operation + ": " + e.what());
  }
  catch (std::runtime_error &e)
  {
    throw shcore::Exception::runtime_error(operation + ": " + e.what());
  }
  catch (...)
  {
    throw;
  }
}

#define CATCH_AND_TRANSLATE_CRUD_EXCEPTION(operation,param_type)   \
  catch (...)                   \
{ translate_crud_exception(operation, param_type); }

namespace mysh
{
  class DatabaseObject;
  namespace mysqlx
  {
    class Crud_definition : public shcore::Cpp_object_bridge
    {
    public:
      Crud_definition(boost::shared_ptr<DatabaseObject> owner);
      // T the moment will put these since we don't really care about them
      virtual bool operator == (const Object_bridge &) const { return false; }

      // Overrides to consider enabled/disabled status
      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;
      virtual bool has_member(const std::string &prop) const;
      virtual shcore::Value call(const std::string &name, const shcore::Argument_list &args);

      // The last step on CRUD operations
      virtual shcore::Value execute(const shcore::Argument_list &args) = 0;
    protected:
      boost::weak_ptr<DatabaseObject> _owner;
      // The CRUD operations will use "dynamic" functions to control the method chaining.
      // A dynamic function is one that will be enabled/disabled based on the method
      // chain sequence.

      // Holds the dynamic functions enable/disable status
      std::map<std::string, bool> _enabled_functions;

      // Holds relation of 'enabled' states for every dynamic function
      std::map<std::string, std::set<std::string> > _enable_paths;

      // Registers a dynamic function and it's associated 'enabled' states
      void register_dynamic_function(const std::string& name, const std::string& enable_after);

      // Enable/disable functions based on the received and registered states
      void update_functions(const std::string& state);
      void enable_function(const char *name, bool enable);

      void set_filter(const std::string &source, const std::string &field, shcore::Value value, bool collection);
      void set_columns(const std::string &source, const std::string &field, shcore::Value value, bool collection, bool with_alias);
      void set_order(const std::string &source, const std::string &field, shcore::Value value);
    };
  }
}

#endif
