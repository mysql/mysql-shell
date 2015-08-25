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
    class Table;

    /**
    * Represents a Schema retrieved with a session created using the MySQL Protocol.
    */
    class SHCORE_PUBLIC Schema : public DatabaseObject, public boost::enable_shared_from_this<Schema>
    {
    public:
      Schema(boost::shared_ptr<ClassicSession> owner, const std::string &name);
      Schema(boost::shared_ptr<const ClassicSession> owner, const std::string &name);
      ~Schema();

      virtual std::string class_name() const{ return "Schema"; };

      virtual std::vector<std::string> get_members() const;
      virtual shcore::Value get_member(const std::string &prop) const;

      void cache_table_objects();

      friend class Table;
      friend class View;

#ifdef DOXYGEN

      /**
      * Returns a list of tables for this schema.
      * This method is run against a local cache of objects, if you want to see lastest changes by other sessions you may need to create a new copy the schema object with session.getSchema().
      * \sa Table      
      * \return The list of tables as a Map(String, Table).
      */
      Map getTables()
      {}

      /**
      * Returns a list of views for this schema.
      * This method is run against a local cache of objects, if you want to see lastest changes by other sessions you may need to create a new copy the schema object with session.getSchema().
      * \sa View
      * \return The list of views as a Map(String, View).
      */
      Map getViews()
      {}

      Table getTable(String name);
      View getView(String name);
#endif

    private:
      boost::shared_ptr<shcore::Value::Map_type> _tables;
      boost::shared_ptr<shcore::Value::Map_type> _views;

      shcore::Value _load_object(const std::string& name, const std::string& type = "") const;
      shcore::Value find_in_collection(const std::string& name, boost::shared_ptr<shcore::Value::Map_type>source) const;
      shcore::Value getTable(const shcore::Argument_list &args);
      shcore::Value getView(const shcore::Argument_list &args);

      void init();
    };
  };
};

#endif
