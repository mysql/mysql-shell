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

#ifndef _MOD_DBA_METADATA_STORAGE_H_
#define _MOD_DBA_METADATA_STORAGE_H_

#include "mod_dba.h"
#include "mod_dba_farm.h"
#include "mod_dba_replicaset.h"
#include "modules/base_resultset.h"

namespace mysh
{
  namespace mysqlx
  {
    /**
    * Represents a Session to a Metadata Storage
    */
    class MetadataStorage : public std::enable_shared_from_this<MetadataStorage>
    {
    public:
      MetadataStorage(Dba* admin_session);
      ~MetadataStorage();

      bool metadata_schema_exists();
      void create_metadata_schema();
      void drop_metadata_schema();
      uint64_t get_farm_id(const std::string &farm_name);
      uint64_t get_farm_id(uint64_t rs_id);
      bool farm_exists(const std::string &farm_name);
      void insert_farm(const std::shared_ptr<Farm> &farm);
      void insert_default_replica_set(const std::shared_ptr<Farm> &farm);
      std::shared_ptr<ShellBaseResult> insert_host(const shcore::Argument_list &args);
      void insert_instance(const shcore::Argument_list &args, uint64_t host_id, uint64_t rs_id);
      void drop_farm(const std::string &farm_name);
      bool farm_has_default_replicaset_only(const std::string &farm_name);
      void drop_default_replicaset(const std::string &farm_name);

      uint64_t get_farm_default_rs_id(const std::string &farm_name);
      std::shared_ptr<Farm> get_farm(const std::string &farm_name);
      bool has_default_farm();
      std::string get_default_farm_name();

      std::string get_replicaset_name(uint64_t rs_id);
      std::shared_ptr<ReplicaSet> get_replicaset(uint64_t rs_id);
      bool is_replicaset_empty(uint64_t rs_id);
      bool is_instance_on_replicaset(uint64_t rs_id, std::string address);

      std::string get_instance_admin_user(uint64_t rs_id);
      std::string get_instance_admin_user_password(uint64_t rs_id);
      std::string get_replication_user_password(uint64_t rs_id);
      std::string get_seed_instance(uint64_t rs_id);

      Dba* get_dba() { return _dba; };

      std::shared_ptr<ShellBaseResult> execute_sql(const std::string &sql) const;

    private:
      Dba* _dba;
    };
  }
}

#endif  // _MOD_DBA_METADATA_STORAGE_H_
