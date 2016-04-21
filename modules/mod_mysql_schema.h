/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _MOD_DB_H_
#define _MOD_DB_H_

#include "mod_common.h"
#include "base_database_object.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

namespace shcore
{
  class Proxy_object;
};

namespace mysh
{
  namespace mysql
  {
    class ClassicSession;
    class ClassicTable;

    /**
    * Represents a Schema retrieved with a session created using the MySQL Protocol.
    *
    * \b Dynamic \b Properties
    *
    * In addition to the properties documented above, when a schema object is retrieved from the session,
    * its Tables and Views are loaded from the database and a cache is filled with the corresponding objects.
    *
    * This cache is used to allow the user accessing the Tables and Views as Schema properties. These Dynamic Properties are named
    * as the object name, so, if a Schema has a table named *customers* this table can be accessed in two forms:
    *
    * \code{.js}
    * // Using the standard getTable() function.
    * var table = mySchema.getTable('customers');
    *
    * // Using the corresponding dynamic property
    * var table = mySchema.customers;
    * \endcode
    *
    * Note that dynamic properties for Schema objects (Tables, Views) are available only if the next conditions are met:
    *
    * - The object name is a valid identifier.
    * - The object name is different from any member of the ClassicSchema class.
    * - The object is in the cache.
    *
    * The object cache is updated every time getTables() or getViews() are called.
    *
    * To retrieve an object that is not available through a Dynamic Property use getTable(name) or getView(name).
    */
    class SHCORE_PUBLIC ClassicSchema : public DatabaseObject, public boost::enable_shared_from_this<ClassicSchema>
    {
    public:
      ClassicSchema(boost::shared_ptr<ClassicSession> owner, const std::string &name);
      ClassicSchema(boost::shared_ptr<const ClassicSession> owner, const std::string &name);
      ~ClassicSchema();

      virtual std::string class_name() const{ return "ClassicSchema"; };

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      void update_cache();
      void _remove_object(const std::string& name, const std::string& type);

      friend class ClassicTable;
      friend class ClassicView;

#ifdef DOXYGEN
      ClassicTable getTable(String name);
      List getTables();
      ClassicView getView(String name);
      List getViews();
#endif
    public:
      shcore::Value get_table(const shcore::Argument_list &args);
      shcore::Value get_tables(const shcore::Argument_list &args);
      shcore::Value get_view(const shcore::Argument_list &args);
      shcore::Value get_views(const shcore::Argument_list &args);

    private:
      void init();

      // Object cache
      boost::shared_ptr<shcore::Value::Map_type> _tables;
      boost::shared_ptr<shcore::Value::Map_type> _views;

      std::function<void(const std::string&, bool exists)> update_table_cache, update_view_cache;
      std::function<void(const std::vector<std::string>&)> update_full_table_cache, update_full_view_cache;
    };
  };
};

#endif
