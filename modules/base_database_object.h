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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _MOD_DB_OBJECT_H_
#define _MOD_DB_OBJECT_H_

#include "mod_common.h"
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
  class ShellBaseSession;
  class CoreSchema;
  /**
  * Provides base functionality for database objects.
  * \todo Document name and getName()
  * \todo Document session and getSession()
  * \todo Document schema and getSchema()
  */
  class MOD_PUBLIC DatabaseObject : public shcore::Cpp_object_bridge
  {
  public:
    DatabaseObject(boost::shared_ptr<ShellBaseSession> session, boost::shared_ptr<DatabaseObject> schema, const std::string &name);
    ~DatabaseObject();

    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;
    virtual void append_json(const shcore::JSON_dumper& dumper) const;

    virtual bool has_member(const std::string &prop) const;
    virtual std::vector<std::string> get_members() const;
    virtual shcore::Value get_member(const std::string &prop) const;

    virtual bool operator == (const Object_bridge &other) const;

    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);

  protected:
    boost::weak_ptr<ShellBaseSession> _session;
    boost::weak_ptr<DatabaseObject> _schema;
    std::string _name;
  };
};

#endif
