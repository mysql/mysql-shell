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

/*
  All the tables that contain information about the topology of MySQL servers
  are stored in the metadata_schema database.
*/

CREATE DATABASE metadata_schema;
USE metadata_schema;

/*
  This table contain information about the metadata and is used to identify
  basic information about the farm/cluster.
*/
CREATE TABLE farm (
  /* unique ID used to distinguish the farm from other farms */
  `farm_uuid` VARCHAR(40) NOT NULL,
  /* unique, user specified name for the farm */
  `farm_name` VARCHAR(40) UNIQUE NOT NULL,
  /*
    The major version of the schema version field representing the semantic
    version of the schema that is in use.
  */
  `major_version` INT UNSIGNED NOT NULL,
  /*
    The minor version of the schema version field representing the semantic
    version of the schema that is in use.
  */
  `minor_version` INT UNSIGNED NOT NULL,
  /* Points to the default replicaset. */
  `default_replicaset` VARCHAR(40),
  /* Brief description of the farm. */
  `description` TEXT,
  PRIMARY KEY (farm_uuid)
) CHARSET = utf8;

/*
  The high-availability replicasets (or groups) are the containers of the
  application data. Each group is composed of a number of MySQL servers in an
  HA configuration.
*/
CREATE TABLE replicasets (
  /*
    The replicaset UUID is generated on creation of the replicaset and does not
    change over its lifetime.
  */
  `replicaset_uuid` VARCHAR(40) NOT NULL,
  /*
    Specifies the type of a replicaset, for example, Group Replication,
    Primary Backup, MySQL Cluster etc. A string is selected to allow
    custom types to be defined without having to change the schema.
  */
  `replicaset_type` VARCHAR(20),
  /*
    A replicaset can be assigned a name and this can be used to refer to it.
    The name of the replicaset can change over time and can even be NULL.
  */
  `replicaset_name` VARCHAR(40),
  /* Associates the replicaset with a farm definition. */
  `farm_uuid` VARCHAR(40) NOT NULL,
  /*
    State of the rs. Either active, in which case traffic can be directed at
    the replicaset, or inactive, in which case no traffic should be directed to the
    it.
  */
  `active` BIT(1) NOT NULL,
  /* Custom properties. */
  `attributes` JSON,
  /* An optional brief description of the replicaset. */
  `description` TEXT,
  PRIMARY KEY (replicaset_uuid),
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE RESTRICT
) CHARSET = utf8;

ALTER TABLE farm ADD FOREIGN KEY (default_replicaset) REFERENCES replicasets(replicaset_uuid) ON DELETE RESTRICT;

/*
  This table contain a list of all server that are tracked by the farm,
  regardless of whether they are part of a replicaset or not. The reason for tracking
  all servers is that some servers may be temporarily moved out of groups for
  different purposes but should still be tracked and possible to locate as part
  of administrative procedures.
*/
CREATE TABLE servers (
  /* The UUID of the server and is a unique identifer of the server. */
  `server_uuid` VARCHAR(40),
  /*
    Replicaset UUID that the server belongs to. Can be NULL if it does not
    belong to any.
  */
  `replicaset_uuid` VARCHAR(40),
  /* The role of the server in the setup e.g. scale-out, master etc. */
  `role` VARCHAR(30) NOT NULL,
  /*
    The mode of the server: if it accept read queries, write queries, neither,
    or both.
  */
  `mode` VARCHAR(30) NOT NULL,
  /*
    The weight of the server for load balancing purposes. The relative
    proportion of traffic it should receive.
  */
  `weight` FLOAT,
  /* A string representing the location. */
  `location` VARCHAR(256) NOT NULL,
  /*
    A JSON blob with the addresses available for the server. The protocols and
    addresses are further described in the Protocol section below.
  */
  `addresses` JSON NOT NULL,
  /*
    Contain attributes assigned to the server and is a JSON data type with
    key-value pair. The attributes can be used to tag the servers with custom
    attributes.
  */
  `attributes` JSON,
  /*
    Server version token in effect for the server. The version token changes
    whenever there is a change of the role of the server and is used to force
    cache invalidation when topology changes.
  */
  `version_token` INTEGER UNSIGNED,
  /* An optional brief description of the group. */
  `description` TEXT,
  PRIMARY KEY (server_uuid),
  FOREIGN KEY (replicaset_uuid) REFERENCES replicasets(replicaset_uuid) ON DELETE SET NULL
) CHARSET = utf8;

/*
  The table shard_sets identifies the set of application tables that are
  sharded using a specific sharding key.
*/
CREATE TABLE shard_sets (
  /* This column is a unique integer denoting the sharding set. */
  `shard_set_id` INT,
  /* Associates the group with a farm definition */
  `farm_uuid` VARCHAR(40),

  /*
    Type of the shard_set. If global, all replicasets have a copy of the table,
    but there is a single shard defined for it, which point to the master
    where global data is replicated from.
  */
  `type` ENUM ('local', 'sharded', 'global'),
  /*
    The sharding type is stored as a string to be possible to use when sharding
    on a string, integer, or arbitrary column.
  */
  `sharding_data_type` VARCHAR(40) NOT NULL,

  `master_schema_name` VARCHAR(64),
  `master_table_name` VARCHAR(64),
  PRIMARY KEY (shard_set_id),
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE RESTRICT
) CHARSET = utf8;

/*
  The table shard_tables identifies tables that are sharded and whether there
  should be mechanisms in place (e.g. triggers) to check whether data is being
  inserted into the right shard.
*/
CREATE TABLE shard_tables (
  /* This column is a unique integer denoting the sharding set. */
  `shard_set_id` INT NOT NULL,
  /* The schema in which the table being sharded is present */
  `schema_name` VARCHAR(64) NOT NULL,
  /* The table being sharded */
  `table_name` VARCHAR(64),
  /*
    Name of the column that contains the sharding key. If the column is a field
    in a JSON blob, a generated column need to be constructed.
  */
  `field_ref` VARCHAR(256) NOT NULL,
  PRIMARY KEY (`schema_name`, `table_name`),
  FOREIGN KEY (shard_set_id) REFERENCES shard_sets(shard_set_id) ON DELETE RESTRICT
) CHARSET = utf8;

/*
  The shards table table identifies the data stored in each shard and the
  high-availability group of the shards. Note that with this definition of the
  shards table, it is possible to co-locate different shards in the same
  high-availability group.
*/
CREATE TABLE shards (
  /*
    This is the identifier for the shard and uniquely identify the partition of
    the data within the shard set.
  */
  `shard_id` INT,
  /* The shard set the shard belong to. */
  `shard_set_id` INT,
  /*
    The replicaset the shard belongs to. In the first iteration we only have one
    shard per replicaset, but this scheme allow several shards per replicaset.
  */
  `replicaset_uuid` VARCHAR(40) NOT NULL,
  /*
    Lower bound for the shard, or NULL if there is no lower bound. The value
    have to be converted to the correct type to be used.
  */
  `lower_bound` VARBINARY(256),
  /* An optional brief description of the group. */
  `description` VARCHAR(120),
  PRIMARY KEY (shard_set_id, shard_id),
  FOREIGN KEY (shard_set_id) REFERENCES shard_sets(shard_set_id),
  FOREIGN KEY (replicaset_uuid) REFERENCES replicasets(replicaset_uuid) ON DELETE RESTRICT
) CHARSET = utf8;



/*
  This table provide the default port to use for a protocol given by a symbol.
  In the schema, we are using a string for the protocol name since this will
  allow custom definitions of protocols.
*/
CREATE TABLE protocol_defaults (
  /* String describing the protocol e.g. mysql.classic */
  `protocol` VARCHAR(40) NOT NULL,
  /*
    Default address JSON object for the protocol without the address field
    filled in.
  */
  `default_address` JSON NOT NULL,
  PRIMARY KEY (protocol)
) CHARSET = utf8;

INSERT INTO protocol_defaults VALUES ('mysql.classic', '{"port": 3306}');
INSERT INTO protocol_defaults VALUES ('mysql.x', '{"port": 33060}');
