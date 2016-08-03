/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
    * its Tables are loaded from the database and a cache is filled with the corresponding objects.
    *
    * This cache is used to allow the user accessing the Tables as Schema properties. These Dynamic Properties are named
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
    * Note that dynamic properties for Tables are available only if the next conditions are met:
    *
    * - The object name is a valid identifier.
    * - The object name is different from any member of the ClassicSchema class.
    * - The object is in the cache.
    *
    * The object cache is updated every time getTables() is called.
    *
    * To retrieve an object that is not available through a Dynamic Property use getTable(name).
    *
    * \b View \b Support
    *
    * MySQL Views are stored queries that when executed produce a result set.
    *
    * MySQL supports the concept of Updatable Views: in specific conditions are met, Views can be used not only to retrieve data from them but also to update, add and delete records.
    *
    * For the purpose of this API, Views behave similar to a Table, and so they are threated as Tables.
    */
    class SHCORE_PUBLIC ClassicSchema : public DatabaseObject, public std::enable_shared_from_this<ClassicSchema>
    {
    public:
      ClassicSchema(std::shared_ptr<ClassicSession> owner, const std::string &name);
      ClassicSchema(std::shared_ptr<const ClassicSession> owner, const std::string &name);
      ~ClassicSchema();

      virtual std::string class_name() const{ return "ClassicSchema"; };

      virtual shcore::Value get_member(const std::string &prop) const;

      void update_cache();
      void _remove_object(const std::string& name, const std::string& type);

      virtual std::string get_object_type() { return "Schema"; }

      friend class ClassicTable;

#if DOXYGEN_JS
      ClassicTable getTable(String name);
      List getTables();
#elif DOXYGEN_PY
      ClassicTable get_table(str name);
      list get_tables();
#endif
    public:
      shcore::Value get_table(const shcore::Argument_list &args);
      shcore::Value get_tables(const shcore::Argument_list &args);

    private:
      void init();

      // Object cache
      std::shared_ptr<shcore::Value::Map_type> _tables;
      std::shared_ptr<shcore::Value::Map_type> _views;

      std::function<void(const std::string&, bool exists)> update_table_cache, update_view_cache;
      std::function<void(const std::vector<std::string>&)> update_full_table_cache, update_full_view_cache;
    };
  };
};

#endif
