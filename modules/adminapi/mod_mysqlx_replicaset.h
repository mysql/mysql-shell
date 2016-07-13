/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_MYSQLX_ADMIN_REPLICASET_H_
#define _MOD_MYSQLX_ADMIN_REPLICASET_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include <boost/enable_shared_from_this.hpp>

namespace mysh
{
  namespace mysqlx
  {
    /**
    * Represents a ReplicaSet
    */
    class ReplicaSet : public boost::enable_shared_from_this<ReplicaSet>, public shcore::Cpp_object_bridge
    {
    public:
      ReplicaSet(const std::string &name);
      virtual ~ReplicaSet();

      virtual std::string class_name() const { return "ReplicaSet"; }
      virtual bool operator == (const Object_bridge &other) const;

      virtual shcore::Value get_member(const std::string &prop) const;

      void set_id(uint64_t id) { _id = id; }
      uint64_t get_id() { return _id;}

      void set_name(std::string name) { _name = name; }

#if DOXYGEN_JS
      String getName();
      Undefined addNode(String conn);
      Undefined addNode(Document doc);
      Undefined removeNode(String name);
      Undefined removeNode(Document doc);

#elif DOXYGEN_PY
      str get_name();
      None add_node(str conn);
      None add_node(Document doc);
      None remove_node(str name);
      None remove_node(Document doc);
#endif

      shcore::Value add_node(const shcore::Argument_list &args);
      shcore::Value remove_node(const shcore::Argument_list &args);

    protected:
      uint64_t _id;
      std::string _name;
      // TODO: add missing fields, rs_type, etc

    private:
      void init();

    protected:
      virtual int get_default_port() { return 33060; };
    };
  }
}

#endif  // _MOD_MYSQLX_ADMIN_REPLICASET_H_
