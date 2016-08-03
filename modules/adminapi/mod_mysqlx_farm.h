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

#ifndef _MOD_MYSQLX_ADMIN_FARM_H_
#define _MOD_MYSQLX_ADMIN_FARM_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "mod_mysqlx_replicaset.h"

namespace mysh
{
  namespace mysqlx
  {
    class MetadataStorage;
    /**
    * Represents a Farm on an AdminSession
    */
    class Farm : public std::enable_shared_from_this<Farm>, public shcore::Cpp_object_bridge
    {
    public:
      Farm(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage);
      virtual ~Farm();

      virtual std::string class_name() const { return "Farm"; }
      virtual bool operator == (const Object_bridge &other) const;

      virtual shcore::Value get_member(const std::string &prop) const;

      const uint64_t get_id() { return _id; }
      void set_id(uint64_t id) { _id = id; }
      std::shared_ptr<ReplicaSet> get_default_replicaset() { return _default_replica_set; }
      void set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs)
      {
        _default_replica_set = default_rs;
      };

#if DOXYGEN_JS
      String getName();
      String getAdminType();
      Undefined addSeedInstance(String conn);
      Undefined addSeedInstance(Document doc);
      Undefined addInstance(String conn);
      Undefined addInstance(Document doc);
      Undefined removeInstance(String name);
      Undefined removeInstance(Document doc);

#elif DOXYGEN_PY
      str get_name();
      str get_admin_type();
      None add_seed_instance(str conn);
      None add_seed_instance(Document doc);
      None add_instance(str conn);
      None add_instance(Document doc);
      None remove_instance(str name);
      None remove_instance(Document doc);
#endif

      shcore::Value add_seed_instance(const shcore::Argument_list &args);
      shcore::Value add_instance(const shcore::Argument_list &args);
      shcore::Value remove_instance(const shcore::Argument_list &args);
      shcore::Value get_replicaset(const shcore::Argument_list &args);

    protected:
      uint64_t _id;
      std::string _name;
      std::string _admin_type;
      std::shared_ptr<ReplicaSet> _default_replica_set;

    private:
      void init();
      std::shared_ptr<MetadataStorage> _metadata_storage;
    };
  }
}

#endif  // _MOD_MYSQLX_ADMIN_FARM_H_
