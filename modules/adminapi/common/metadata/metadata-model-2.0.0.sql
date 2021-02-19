/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
  Metadata Schema Version Change Log
  ----------------------------------
  * 1.0.1 - Initial version with support for InnoDB Clusters based on Group
            Replication technology.
  * 2.0.0 - Added support for ReplicaSets and a public interface for
            components like MySQL Router. Cleanup and removal of unused items.

  Description
  -----------
  All the tables that contain information about the topology of MySQL servers
  in a MySQL InnoDB Cluster setup are stored in the
  mysql_innodb_cluster_metadata database.

  Consumers of the metadata (i.e. MySQL Router) are required to query the
  public interface items (VIEWs and STORED PROCEDUREs) instead of querying the
  actual metadata tables. The public interface items are prefixed with the
  corresponding major version of the metadata, e.g. 'v2_clusters'.
*/

CREATE DATABASE IF NOT EXISTS mysql_innodb_cluster_metadata DEFAULT CHARACTER SET utf8mb4;
USE mysql_innodb_cluster_metadata;

SET names utf8mb4;

--  Metadata Schema Version
--  -----------------------

/*
  View that holds the current version of the metadata schema.

  PLEASE NOTE:
  During the upgrade process of the metadata schema the schema_version is
  set to 0, 0, 0. This behavior is used for other components to detect a
  schema upgrade process and hold back metadata refreshes during upgrades.
*/
DROP VIEW IF EXISTS schema_version;
CREATE SQL SECURITY INVOKER VIEW schema_version (major, minor, patch) AS SELECT 2, 0, 0;


--  GR Cluster Tables
--  -----------------

/*
  Basic information about clusters in general.

  Both InnoDB clusters and replicasets are represented as clusters in
  this table, with different cluster_type values.
*/
CREATE TABLE IF NOT EXISTS clusters (
  /*
    unique ID used to distinguish the cluster from other clusters
  */
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci,
  /*
    user specified name for the cluster. cluster_name must be unique
  */
  `cluster_name` VARCHAR(40) NOT NULL,
  /*
    Brief description of the cluster.
  */
  `description` TEXT,
  /*
    Cluster options explicitly set by the user from the Shell.
   */
  `options` JSON,
   /*
    Contain attributes assigned to each cluster.
    The attributes can be used to tag the clusters with custom attributes.
    {
      group_replication_group_name: "254616cc-fb47-11e5-aac5"
    }
   */
  `attributes` JSON,
  /*
   Whether this is a GR or AR "cluster" (that is, an InnoDB cluster or an
   Async ReplicaSet).
   */
  `cluster_type` ENUM('gr', 'ar') NOT NULL,
  /*
    Specifies the type of topology of a cluster as last known by the shell.
    This is just a snapshot of the GR configuration and should be automatically
    refreshed by the shell.
  */
  `primary_mode` ENUM('pm', 'mm') NOT NULL DEFAULT 'pm',

  /*
    Default options for Routers.
   */
  `router_options` JSON,

  PRIMARY KEY(cluster_id)
) ENGINE = InnoDB CHARSET = utf8mb4 ROW_FORMAT = DYNAMIC;


/*
  Managed MySQL server instances and basic information about them.
*/
CREATE TABLE IF NOT EXISTS instances (
  /*
    The ID of the server instance and is a unique identifier of the server
    instance.
  */
  `instance_id` INT UNSIGNED AUTO_INCREMENT,
  /*
    Cluster ID that the server belongs to.
  */
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL,
  /*
    network address of the host of the instance.
    Taken from @@report_host or @@hostname
  */
  `address` VARCHAR(265) CHARACTER SET ascii COLLATE ascii_general_ci,
  /*
    MySQL generated server_uuid for the instance
  */
  `mysql_server_uuid` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci
    NOT NULL,
  /*
    Unique, user specified name for the server.
    Default is address:port (must fit at least 255 chars of address + port)
   */
  `instance_name` VARCHAR(265) NOT NULL,
  /*
    A JSON document with the addresses available for the server instance. The
    protocols and addresses are further described in the Protocol section below.
    {
      mysqlClassic: "host.foo.com:3306",
      mysqlX: "host.foo.com:33060",
      grLocal: "host.foo.com:49213"
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
    An optional brief description of the group.
  */
  `description` TEXT,

  PRIMARY KEY(instance_id, cluster_id),
  FOREIGN KEY (cluster_id) REFERENCES clusters(cluster_id)
) ENGINE = InnoDB CHARSET = utf8mb4 ROW_FORMAT = DYNAMIC;


--  AR ReplicaSet Tables
--  ---------------------

/*
  A "view" of the topology of a replicaset at a given point in time.
  Every time topology of the replicaset changes (added and removed instances
  and failovers), a new record is added here (and in async_cluster_members).

  The most current view will be the one with the highest view_id.

  This table maintains a history of replicaset configurations, but older
  records may get deleted.
 */
CREATE TABLE IF NOT EXISTS async_cluster_views (
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL,

  `view_id` INT UNSIGNED NOT NULL,

  /*
    The type of the glboal topology between clusters.
    Changing the topology_type is not currently supported, so this value
    must always be copied when a view changes.
   */
  `topology_type` ENUM('SINGLE-PRIMARY-TREE'),

  /*
    What caused the view change. Possible values are pre-defined by the shell.
   */
  `view_change_reason` VARCHAR(32) NOT NULL,

  /*
    Timestamp of the change.
   */
  `view_change_time` TIMESTAMP(6) NOT NULL,

  /*
    Information about the cause for a view change.
   */
  `view_change_info` JSON NOT NULL,

  `attributes` JSON NOT NULL,

  PRIMARY KEY (cluster_id, view_id),
  INDEX (view_id),

  FOREIGN KEY (cluster_id)
    REFERENCES clusters (cluster_id)
) ENGINE = InnoDB CHARSET = utf8mb4 ROW_FORMAT = DYNAMIC;


/*
  The instances that are part of a given view, along with their replication
  master. The PRIMARY will have a NULL master.
*/
CREATE TABLE IF NOT EXISTS async_cluster_members (
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL,

  `view_id` INT UNSIGNED NOT NULL,

  /*
    Reference to an instance in the instances table.
    This reference may become invalid for older views, for instances that are
    removed from the replicaset.
  */
  `instance_id` INT UNSIGNED NOT NULL,

  /*
    id of the master of this instance. NULL if this is the PRIMARY.
   */
  `master_instance_id` INT UNSIGNED,

  /*
    TRUE if this is the PRIMARY of the replicaset.
   */
  `primary_master` BOOL NOT NULL,

  `attributes` JSON NOT NULL,

  PRIMARY KEY (cluster_id, view_id, instance_id),
  INDEX (view_id),
  INDEX (instance_id),

  FOREIGN KEY (cluster_id, view_id)
      REFERENCES async_cluster_views (cluster_id, view_id)
) ENGINE = InnoDB CHARSET = utf8mb4 ROW_FORMAT = DYNAMIC;

/*
  This table contain a list of all router instances that are tracked by the
  cluster.
*/
CREATE TABLE IF NOT EXISTS routers (
  /*
    The ID of the router instance and is a unique identifier of the server
    instance.
  */
  `router_id` INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
  /*
    A user specified name for an instance of the router.
    Should default to address:port, where port is the RW port for classic
    protocol.
   */
  `router_name` VARCHAR(265) NOT NULL,
  /*
    The product name of the routing component, e.g. 'MySQL Router'
   */
  `product_name` VARCHAR(128) NOT NULL,
  /*
    network address of the host the Router is running on.
  */
  `address` VARCHAR(256) CHARACTER SET ascii COLLATE ascii_general_ci,
  /*
    The version of the router instance. Updated on bootstrap and each startup
    of the router instance. Format: x.y.z, 3 digits for each component.
    Managed by Router.
   */
  `version` VARCHAR(12) DEFAULT NULL,
  /*
    A timestamp updated by the router every hour with the current time. This
    timestamp is used to detect routers that are no longer used or stalled.
    Managed by Router.
   */
  `last_check_in` TIMESTAMP NULL DEFAULT NULL,
  /*
    Router specific custom attributes.
    Managed by Router.
   */
  `attributes` JSON,
  /*
     The ID of the cluster this router instance is routing to.
     (implicit foreign key to avoid trouble when deleting a cluster).
      For backwards compatibility, if NULL, assumes there's a single cluster
      in the metadata.
   */
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci
    DEFAULT NULL,
  /*
    Router instance specific configuration options.
    Managed by Shell.
   */
  `options` JSON DEFAULT NULL,

  UNIQUE KEY (address, router_name)
) ENGINE = InnoDB CHARSET = utf8mb4 ROW_FORMAT = DYNAMIC;

/*
  This table contains a list of all REST user accounts that are granted
  access to the routers REST interface. These are managed by the shell
  per cluster.
*/
CREATE TABLE IF NOT EXISTS router_rest_accounts (
  /*
     The ID of the cluster this router account applies to.
     (implicit foreign key to avoid trouble when deleting a cluster).
   */
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci
    NOT NULL,
  /*
    The name of the user account.
  */
  `user` VARCHAR(256) NOT NULL,
  /*
    The authentication method used.
  */
  `authentication_method` VARCHAR(64) NOT NULL DEFAULT 'modular_crypt_format',
  /*
    The authentication string of the user account. The password is stored
    hashed, salted, multiple-rounds.
  */
  `authentication_string` TEXT CHARACTER SET ascii COLLATE ascii_general_ci
    DEFAULT NULL,
  /*
    A short description of the user account.
  */
  `description` VARCHAR(255) DEFAULT NULL,
  /*
    Stores the users privileges in a JSON document. Example:
    {readPriv: "Y", updatePriv: "N"}
  */
  `privileges` JSON,
  /*
    Additional attributes for the user account
  */
  `attributes` JSON DEFAULT NULL,

  PRIMARY KEY (cluster_id, user)
) ENGINE = InnoDB CHARSET = utf8mb4 ROW_FORMAT = DYNAMIC;


--  Public Interface Views
--  ----------------------

/*
   These views will remain backwards compatible even if the internal schema
   change. No existing columns will have their types changed or removed,
   although new columns may be added in the future.
 */

DROP VIEW IF EXISTS v2_clusters;
CREATE SQL SECURITY INVOKER VIEW v2_clusters AS
    SELECT
          c.cluster_type,
          c.primary_mode,
          c.cluster_id,
          c.cluster_name,
          c.router_options
    FROM clusters c;

DROP VIEW IF EXISTS v2_gr_clusters;
CREATE SQL SECURITY INVOKER VIEW v2_gr_clusters AS
    SELECT
          c.cluster_type,
          c.primary_mode,
          c.cluster_id as cluster_id,
          c.cluster_name as cluster_name,
          c.attributes->>'$.group_replication_group_name' as group_name,
          c.attributes,
          c.options,
          c.router_options,
          c.description as description,
          NULL as replicated_cluster_id
    FROM clusters c
    WHERE c.cluster_type = 'gr';

DROP VIEW IF EXISTS v2_instances;
CREATE SQL SECURITY INVOKER VIEW v2_instances AS
  SELECT i.instance_id,
         i.cluster_id,
         i.instance_name as label,
         i.mysql_server_uuid,
         i.address,
         i.addresses->>'$.mysqlClassic' as endpoint,
         i.addresses->>'$.mysqlX' as xendpoint,
         i.attributes
  FROM instances i;

DROP VIEW IF EXISTS v2_ar_clusters;
CREATE SQL SECURITY INVOKER VIEW v2_ar_clusters AS
    SELECT
          acv.view_id,
          c.cluster_type,
          c.primary_mode,
          acv.topology_type as async_topology_type,
          c.cluster_id,
          c.cluster_name,
          c.attributes,
          c.options,
          c.router_options,
          c.description
    FROM clusters c
    JOIN async_cluster_views acv
      ON c.cluster_id = acv.cluster_id
    WHERE acv.view_id = (SELECT max(view_id)
        FROM async_cluster_views
        WHERE c.cluster_id = cluster_id)
      AND c.cluster_type = 'ar';

DROP VIEW IF EXISTS v2_ar_members;
CREATE SQL SECURITY INVOKER VIEW v2_ar_members AS
  SELECT
         acm.view_id,
         i.cluster_id,
         i.instance_id,
         i.instance_name as label,
         i.mysql_server_uuid as member_id,
         IF(acm.primary_master, 'PRIMARY', 'SECONDARY') as member_role,
         acm.master_instance_id as master_instance_id,
         mi.mysql_server_uuid as master_member_id
  FROM instances i
  LEFT JOIN async_cluster_members acm
    ON acm.cluster_id = i.cluster_id AND acm.instance_id = i.instance_id
  LEFT JOIN instances mi
    ON mi.instance_id = acm.master_instance_id
  WHERE acm.view_id = (SELECT max(view_id)
    FROM async_cluster_views WHERE i.cluster_id = cluster_id);

/*
  Returns information about the InnoDB cluster or replicaset the server
  being queried is part of.
 */
DROP VIEW IF EXISTS v2_this_instance;
CREATE SQL SECURITY INVOKER VIEW v2_this_instance AS
  SELECT i.cluster_id,
          i.instance_id,
          c.cluster_name,
          c.cluster_type
  FROM v2_instances i
  JOIN clusters c ON i.cluster_id = c.cluster_id
  WHERE i.mysql_server_uuid = (SELECT convert(variable_value using ascii)
      FROM performance_schema.global_variables
      WHERE variable_name = 'server_uuid');

/*
  List of registered router instances. New routers will do inserts into this
  VIEW during bootstrap. They will also update the version, last_check_in and
  attributes field during runtime.
 */
DROP VIEW IF EXISTS v2_routers;
CREATE SQL SECURITY INVOKER VIEW v2_routers AS
  SELECT r.router_id,
         r.cluster_id,
         r.router_name,
         r.product_name,
         r.address,
         r.version,
         r.last_check_in,
         r.attributes,
         r.options
  FROM routers r;

/*
  List of REST user accounts that are granted access to the routers REST
  interface. These are managed by the shell per cluster and consumed by the
  router.
*/
DROP VIEW IF EXISTS v2_router_rest_accounts;
CREATE SQL SECURITY INVOKER VIEW v2_router_rest_accounts AS
  SELECT a.cluster_id,
          a.user,
          a.authentication_method,
          a.authentication_string,
          a.description,
          a.privileges,
          a.attributes
  FROM router_rest_accounts a;
