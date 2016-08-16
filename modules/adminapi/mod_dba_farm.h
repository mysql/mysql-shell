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

#ifndef _MOD_DBA_ADMIN_FARM_H_
#define _MOD_DBA_ADMIN_FARM_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "mod_dba_replicaset.h"

namespace mysh
{
  namespace mysqlx
  {
    class MetadataStorage;
    /**
    * Represents a Farm
    */
    class Farm : public std::enable_shared_from_this<Farm>, public shcore::Cpp_object_bridge
    {
    public:
      Farm(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage);
      virtual ~Farm();

      virtual std::string class_name() const { return "Farm"; }
      virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
      virtual bool operator == (const Object_bridge &other) const;

      virtual void append_json(shcore::JSON_dumper& dumper) const;

      virtual shcore::Value get_member(const std::string &prop) const;

      const uint64_t get_id() { return _id; }
      void set_id(uint64_t id) { _id = id; }
      std::shared_ptr<ReplicaSet> get_default_replicaset() { return _default_replica_set; }
      void set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs)
      {
        _default_replica_set = default_rs;
      };
      std::string get_admin_type() { return _admin_type; }
      void set_admin_type(std::string admin_type) { _admin_type = admin_type; }
      std::string get_password() { return _password; }
      void set_password(std::string farm_password) { _password = farm_password; }
      std::string get_description() { return _description; }
      void set_description(std::string description) { _description = description; };
      std::string get_instance_admin_user() { return _instance_admin_user; };
      void set_instance_admin_user(std::string user) { _instance_admin_user = user; };
      std::string get_instance_admin_user_password() { return _instance_admin_user_password; };
      void set_instance_admin_user_password(std::string password) { _instance_admin_user_password = password; };
      std::string get_farm_reader_user() { return _farm_reader_user; };
      void set_farm_reader_user(std::string user) { _farm_reader_user = user; };
      std::string get_farm_reader_user_password() { return _farm_reader_user_password; }
      void set_farm_reader_user_password(std::string password) { _farm_reader_user_password = password; };
      std::string get_replication_user() { return _farm_replication_user; };
      void set_replication_user(std::string user) { _farm_replication_user = user; };
      std::string get_replication_user_password() { return _farm_reader_user_password; };
      void set_replication_user_password(std::string password) { _farm_reader_user_password = password; };

#if DOXYGEN_JS
      String getName();
      String getAdminType();
      Undefined addSeedInstance(String conn);
      Undefined addSeedInstance(Document doc);
      Undefined addInstance(String conn);
      Undefined addInstance(Document doc);
      Undefined removeInstance(String name);
      Undefined removeInstance(Document doc);
      String describe();
#elif DOXYGEN_PY
      str get_name();
      str get_admin_type();
      None add_seed_instance(str conn);
      None add_seed_instance(Document doc);
      None add_instance(str conn);
      None add_instance(Document doc);
      None remove_instance(str name);
      None remove_instance(Document doc);
      str describe();
#endif

      shcore::Value add_seed_instance(const shcore::Argument_list &args);
      shcore::Value add_instance(const shcore::Argument_list &args);
      shcore::Value remove_instance(const shcore::Argument_list &args);
      shcore::Value get_replicaset(const shcore::Argument_list &args);
      shcore::Value describe(const shcore::Argument_list &args);
      shcore::Value status(const shcore::Argument_list &args);

    protected:
      uint64_t _id;
      std::string _name;
      std::string _admin_type;
      std::shared_ptr<ReplicaSet> _default_replica_set;
      std::string _password;
      std::string _description;

    private:
      // This flag will be used to determine what should be included on the JSON output for the object
      // 0 standard
      // 1 means status
      // 2 means describe
      int _json_mode;
      void init();

      std::shared_ptr<MetadataStorage> _metadata_storage;
      std::string _instance_admin_user;
      std::string _instance_admin_user_password;
      std::string _farm_reader_user;
      std::string _farm_reader_user_password;
      std::string _farm_replication_user;
      std::string _farm_replication_user_password;
    };
  }
}

#endif  // _MOD_DBA_ADMIN_FARM_H_
