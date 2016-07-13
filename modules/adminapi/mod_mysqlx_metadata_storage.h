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

#ifndef _MOD_MYSQLX_METADATA_STORAGE_H_
#define _MOD_MYSQLX_METADATA_STORAGE_H_

#include <boost/enable_shared_from_this.hpp>

#include "mod_mysqlx_admin_session.h"
#include "mod_mysqlx_farm.h"

namespace mysh
{
  namespace mysqlx
  {
    /**
    * Represents a Session to a Metadata Storage
    */
    class MetadataStorage : public boost::enable_shared_from_this<MetadataStorage>
    {
    public:
      MetadataStorage(boost::shared_ptr<AdminSession> admin_session);
      ~MetadataStorage();

      bool metadata_schema_exists();
      void create_metadata_schema();
      uint64_t get_farm_id(std::string farm_name);
      bool farm_exists(std::string farm_name);
      void insert_farm(boost::shared_ptr<Farm> farm);
      void drop_farm(std::string farm_name);
      bool farm_has_default_replicaset_only(std::string farm_name);
      void drop_default_replicaset(std::string farm_name);

      uint64_t get_farm_default_rs_id(std::string farm_name);
      boost::shared_ptr<Farm> get_farm(std::string farm_name);

      std::string get_replicaset_name(uint64_t rs_id);
      boost::shared_ptr<ReplicaSet> get_replicaset(uint64_t rs_id);

    private:
      boost::shared_ptr<AdminSession> _admin_session;
    };
  }
}

#endif  // _MOD_MYSQLX_METADATA_STORAGE_H_
