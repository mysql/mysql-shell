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
  The major and minor version of the schema representing the semantic
  version of the schema that is in use
*/
CREATE VIEW schema_version (major, minor) AS SELECT 1, 0;

/*
  This table contain information about the metadata and is used to identify
  basic information about the farm.
*/
CREATE TABLE farms (
  /* unique ID used to distinguish the farm from other farms */
  `farm_id` INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  /* unique, user specified name for the farm */
  `farm_name` VARCHAR(40) UNIQUE NOT NULL,
  /* Points to the default replicaset. */
  `default_replicaset` INT UNSIGNED,
  /* Brief description of the farm. */
  `description` TEXT,
  /*
    Stores default mysql user accounts for automated management.
    The passwords are encrypted using the FarmAdminPassword.
    {
      -- User for Read-Only access to the Farm (Router metadata cache access)
      mysqlRoUserName: <..>,
      mysqlRoUserPassword: <..>
    }
  */
  `mysql_user_accounts` JSON,
  /*
    Stores all management options in the JSON format.
    {
      farmAdminType: < "manual" | "ssh" >,
      defaultSshUserName: < ... >,
      ...
    }
  */
  `options` JSON,
   /*
    Contain attributes assigned to each farm and is a JSON data type with
    key-value pair. The attributes can be used to tag the farms with custom
    attributes.
   */
  `attributes` JSON

) CHARSET = utf8;

/*
  The high-availability Replica Sets are the containers of the application data.
  Each group is composed of a number of MySQL servers in an HA configuration.
*/
CREATE TABLE replicasets (
  /*
    The replicaset_id is generated on creation of the replicaset and does not
    change over its lifetime.
  */
  `replicaset_id` INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  /* Associates the replicaset with a farm definition. */
  `farm_id` INT UNSIGNED NOT NULL,
  /*
    Specifies the type of a replicaset, for now this needs to be set to
    Group Replication.
  */
  `replicaset_type` ENUM('gr') NOT NULL,
  /*
    A replicaset can be assigned a name and this can be used to refer to it.
    The name of the replicaset can change over time and can even be NULL.
  */
  `replicaset_name` VARCHAR(40) NOT NULL,
  /*
    State of the rs. Either active, in which case traffic can be directed at
    the replicaset, or inactive, in which case no traffic should be directed
    to it.
  */
  `active` BIT(1) NOT NULL,
  /*
    Custom properties.
    {
      groupReplicationChannelName: "254616cc-fb47-11e5-aac5"
    }
  */
  `attributes` JSON,
  /* An optional brief description of the replicaset. */
  `description` TEXT,
  FOREIGN KEY (farm_id) REFERENCES farms(farm_id) ON DELETE RESTRICT
) CHARSET = utf8;

ALTER TABLE farms ADD FOREIGN KEY (default_replicaset) REFERENCES replicasets(replicaset_id) ON DELETE RESTRICT;

/*
  This table contains a list of all the hosts in the farm.
*/
CREATE TABLE hosts (
  /*
    The ID of the host instance. The host UUID is used internally for farm
    management.
  */
  `host_id` INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  /* network host name address of the host */
  `host_name` VARCHAR(128),
  /* network ip address of the host */
  `ip_address` VARCHAR(45),
  /* A string representing the location (e.g. datacenter name). */
  `location` VARCHAR(256) NOT NULL,
  /*
    Contain attributes assigned to the server host and is a JSON data type with
        key-value pair. The attributes can be used to tag the servers with custom
        attributes.
  */
  attributes JSON,
  /*
    Stores the admin user accounts information (e.g. for SSH) for automated
    management.
    {
      -- SSH User for Administrative access to the Host
      sshUserName: <..>
    }
  */
  `admin_user_account` JSON
) CHARSET = utf8;

/*
  This table contain a list of all server instances that are tracked by the farm.
*/
CREATE TABLE instances (
  /*
    The ID of the server instance and is a unique identifier of the server
    instance.
  */
  `instance_id` INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  /* The ID of the host in which the server is running. */
  `host_id` INT UNSIGNED NOT NULL,
  /*
    Replicaset ID that the server belongs to. Can be NULL if it does not belong
    to any.
  */
  `replicaset_id` INT UNSIGNED,
  /* MySQL generated server_uuid for the instance */
  `mysql_server_uuid` VARCHAR(40) UNIQUE NOT NULL,
  /* unique, user specified name for the server */
  `instance_name` VARCHAR(40) UNIQUE NOT NULL,
  /* The role of the server instance in the setup e.g. scale-out, master etc. */
  `role` ENUM('master', 'scaleOut', 'hotSpare') NOT NULL,
  /*
    The mode of the server instance : if it accept read queries, read and write
    queries, or neither.
  */
  `mode` ENUM('rw', 'ro', 'none') NOT NULL,
  /*
    The weight of the server instance for load balancing purposes. The relative
    proportion of traffic it should receive.
  */
  `weight` FLOAT,
  /*
    A JSON blob with the addresses available for the server instance. The
    protocols and addresses are further described in the Protocol section below.
    {
      mysqlClassic: "mysql://host.foo.com:3306",
      mysqlX: "mysqlx://host.foo.com:33060",
      localClassic: "mysql://localhost:/tmp/mysql.sock",
      localX: "mysqlx://localhost:/tmp/mysqlx.sock",
      mysqlXcom: "mysqlXcom://host.foo.com:49213?channelName=<..>"
    }
  */
  `addresses` JSON NOT NULL,
  /*
    Contain attributes assigned to the server and is a JSON data type with
    key-value pair. The attributes can be used to tag the servers with custom
    attributes.
  */
  `attributes` JSON,
  /*
    Server version token in effect for the server instance. The version token
    changes whenever there is a change of the role of the server instance and is
    used to force cache invalidation when topology changes.
  */
  `version_token` INTEGER UNSIGNED,
  /* An optional brief description of the group. */
  `description` TEXT,
  FOREIGN KEY (host_id) REFERENCES hosts(host_id) ON DELETE RESTRICT,
  FOREIGN KEY (replicaset_id) REFERENCES replicasets(replicaset_id) ON DELETE SET NULL
) CHARSET = utf8;
