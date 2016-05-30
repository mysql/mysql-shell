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
  are stored in the farm_metadata_schema database.
*/

CREATE DATABASE farm_metadata_schema;
USE farm_metadata_schema;

/*
  This table contain information about the metadata and is used to identify
  basic information about the farm.
*/
CREATE TABLE farms (
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
  This table contains all the user accounts that are used while working with
  a farm.
*/
CREATE TABLE accounts (
  /* Associates the user account with a farm definition. */
  `farm_uuid` VARCHAR(40) NOT NULL,
  /* The user name associated with the account */
  `user` VARCHAR(64) NOT NULL,
  /* The encrypted password to be used for the account */
  `password` TEXT NOT NULL,
  /*
     Random data to defend against dictionary attacks and against pre-computed
     rainbow table attacks.
  */
  salt VARCHAR(32),
  /* The role for which the usr account is created in the farm */
  role VARCHAR(64),
  PRIMARY KEY(farm_uuid, role),
  FOREIGN KEY (farm_uuid) REFERENCES farms(farm_uuid) ON DELETE RESTRICT
) CHARSET = utf8;

/*
  The high-availability Replica Sets are the containers of the application data.
  Each group is composed of a number of MySQL servers in an HA configuration.
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
  `replicaset_name` VARCHAR(40) UNIQUE NOT NULL,
  /* Associates the replicaset with a farm definition. */
  `farm_uuid` VARCHAR(40) NOT NULL,
  /*
    State of the rs. Either active, in which case traffic can be directed at
    the replicaset, or inactive, in which case no traffic should be directed to
    it.
  */
  `active` BIT(1) NOT NULL,
  /* Custom properties. */
  `attributes` JSON,
  /* An optional brief description of the replicaset. */
  `description` TEXT,
  PRIMARY KEY (replicaset_uuid),
  FOREIGN KEY (farm_uuid) REFERENCES farms(farm_uuid) ON DELETE RESTRICT
) CHARSET = utf8;

ALTER TABLE farms ADD FOREIGN KEY (default_replicaset) REFERENCES replicasets(replicaset_uuid) ON DELETE RESTRICT;

/*
  This table contains a list of all the hosts in the farm.
*/
CREATE TABLE hosts (
  /*
    The UUID of the host instance. The host UUID is used internally for
    farm management.
  */
  `host_uuid` VARCHAR(40) NOT NULL,
  /*
    unique name of the host
  */
  `host_name` VARCHAR(40) UNIQUE NOT NULL,
  /* A string representing the location. */
  `location` VARCHAR(256) NOT NULL,
  /*
    A JSON blob with the addresses available for the server instance. The
    protocols and addresses are further described in the Protocol section below.
  */
  `addresses` JSON NOT NULL,
  PRIMARY KEY(host_uuid)
) CHARSET = utf8;

/*
  This table contain a list of all server instances that are tracked by the
  farm.
*/
CREATE TABLE instances (
  /*
    The UUID of the server instance and is a unique identifer of the server
    instance.
  */
  `instance_uuid` VARCHAR(40) NOT NULL,
  /*
    The UUID of the host in which the server is running.
  */
  `host_uuid` VARCHAR(40) NOT NULL,
  /*
    Replicaset UUID that the server belongs to. Can be NULL if it does not
    belong to any.
  */
  `replicaset_uuid` VARCHAR(40),
  /* unique, user specified name for the server */
  `instance_name` VARCHAR(40) UNIQUE NOT NULL,
  /* The role of the server instance in the setup e.g. scale-out, master etc. */
  `role` VARCHAR(30) NOT NULL,
  /*
    The mode of the server instance : if it accept read queries, write queries,
    neither, or both.
  */
  `mode` VARCHAR(30) NOT NULL,
  /*
    The weight of the server instance for load balancing purposes. The relative
    proportion of traffic it should receive.
  */
  `weight` FLOAT,
  /*
    Contain attributes assigned to the server and is a JSON data type with
    key-value pair. The attributes can be used to tag the servers with custom
    attributes.
  */
  `attributes` JSON,
  /*
    Server version token in effect for the server instance. The version token
    changes whenever there is a change of the role of the server instance and
    is used to force cache invalidation when topology changes.
  */
  `version_token` INTEGER UNSIGNED,
  /* An optional brief description of the group. */
  `description` TEXT,
  PRIMARY KEY (instance_uuid),
  FOREIGN KEY (host_uuid) REFERENCES hosts(host_uuid) ON DELETE RESTRICT,
  FOREIGN KEY (replicaset_uuid) REFERENCES replicasets(replicaset_uuid) ON DELETE SET NULL
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
