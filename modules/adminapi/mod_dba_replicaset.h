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

#ifndef _MOD_DBA_ADMIN_REPLICASET_H_
#define _MOD_DBA_ADMIN_REPLICASET_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include <set>
namespace mysh
{
  namespace mysqlx
  {
    class MetadataStorage;

    /**
    * Represents a ReplicaSet
    */
    class ReplicaSet : public std::enable_shared_from_this<ReplicaSet>, public shcore::Cpp_object_bridge
    {
    public:
      ReplicaSet(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage);
      virtual ~ReplicaSet();

      virtual std::string class_name() const { return "ReplicaSet"; }
      virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
      virtual bool operator == (const Object_bridge &other) const;

      virtual void append_json(shcore::JSON_dumper& dumper) const;

      virtual shcore::Value get_member(const std::string &prop) const;

      void set_id(uint64_t id) { _id = id; }
      uint64_t get_id() { return _id; }

      void set_name(std::string name) { _name = name; }

      std::string get_replication_user() { return _replication_user; };
      void set_replication_user(std::string user) { _replication_user = user; };

#if DOXYGEN_JS
      String getName();
      Undefined addInstance(String conn);
      Undefined addInstance(Document doc);
      Undefined removeInstance(String name);
      Undefined removeInstance(Document doc);

#elif DOXYGEN_PY
      str get_name();
      None add_instance(str conn);
      None add_instance(Document doc);
      None remove_instance(str name);
      None remove_instance(Document doc);
#endif

      shcore::Value add_instance_(const shcore::Argument_list &args);
      shcore::Value add_instance(const shcore::Argument_list &args);
      shcore::Value remove_instance(const shcore::Argument_list &args);

      static std::set<std::string> get_invalid_attributes(const std::set<std::string> input);

    protected:
      uint64_t _id;
      std::string _name;
      // TODO: add missing fields, rs_type, etc

    private:
      void init();

      static std::set<std::string> _valid_attributes;

      std::shared_ptr<MetadataStorage> _metadata_storage;
      std::string _replication_user;

    protected:
      virtual int get_default_port() { return 33060; };
    };
  }
}

#endif  // _MOD_DBA_ADMIN_REPLICASET_H_
