/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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
 * A metadata schema upgrade file must contain the following structure:
 *
 * 1) Version Tags indicating the source and target versions this upgrade script
 *    handles, the format is:
 *
 *    -- Source: <major>.<minor>.<patch>
 *    -- Target: <major>.<minor>.<patch>
 *
 * 2) PRE SQL Statements: Statements to be executed before the actual migration
 *    takes place, examples of statements include (but not limited to):
 *
 * - Data Backup
 * - Removal of conflicting objects: views, stored procedures
 *
 * 3) Metadata Deploy Tag: Indicates the target metadata schema to be deployed
 *    once the PRE SQL statements are executed, the format is:
 *
 *    -- Deploy: METADATA_MODEL_<major>_<minor>_<patch>
 *
 *    This model will be taken from a sql file named:
 *
 *    metadata-model-<major>-<minor>-<patch>.sql
 *
 * 4) POST SQL statements: Statements to be executed after the target metadata
 *    model script has been executed, examples of statements include (but not
 *    limited to):
 *
 *    - Data Migration Statements
 *    - Data Transformation Statements
 *    - Cleanup
 */

-- Source: 1.0.1
-- Target: 2.0.0
-- PRE SQL Statements
USE mysql_innodb_cluster_metadata;

/*
 * The following tables are discarded in favor of views with the same name.
 *
 * They are already backed up by the upgrade initial backup at
 * mysql_innodb_cluster_metadata_previous
 */
DROP TABLE replicasets;
DROP TABLE hosts;
DROP TABLE clusters;
DROP TABLE instances;
DROP TABLE routers;

ALTER SCHEMA mysql_innodb_cluster_metadata DEFAULT CHARACTER SET utf8mb4;

SET NAMES utf8mb4;

-- Deploy: METADATA_MODEL_2_0_0

-- POST SQL Statements
/* Migrates the data from the mysql_innodb_cluster_metadata_previous taken by the
 * upgrade engine before the upgrade is started.
 */
DELIMITER ;

-- Deletes the foreign key from the instances table to actually allow the data migration
ALTER TABLE mysql_innodb_cluster_metadata_previous.instances DROP FOREIGN KEY instances_ibfk_2;

INSERT INTO clusters
    SELECT CAST(c.cluster_id AS CHAR(36)), c.cluster_name, c.description,
                c.options, JSON_MERGE_PATCH(JSON_REMOVE(c.attributes, '$.adminType'), JSON_REPLACE(r.attributes, '$.adopted',
                    CASE WHEN LOWER(r.attributes->>'$.adopted') = "true" THEN 1
                        WHEN LOWER(r.attributes->>'$.adopted') = "false" THEN 0
                        ELSE CONVERT(r.attributes->>'$.adopted', UNSIGNED INTEGER) END)),
                'gr', r.topology_type, NULL
    FROM mysql_innodb_cluster_metadata_previous.clusters AS c,
         mysql_innodb_cluster_metadata_previous.replicasets AS r
    WHERE c.cluster_id = r.cluster_id;

INSERT INTO instances
    SELECT instance_id, CAST(c.cluster_id AS CHAR(36)),
        i.addresses->>'$.mysqlClassic', i.mysql_server_uuid,
        i.instance_name, i.addresses, i.attributes, i.description
    FROM mysql_innodb_cluster_metadata_previous.instances AS i,
         mysql_innodb_cluster_metadata_previous.clusters AS c,
         mysql_innodb_cluster_metadata_previous.replicasets AS r
    WHERE i.replicaset_id = r.replicaset_id
    AND   c.cluster_id = r.cluster_id;

UPDATE instances
    SET addresses = JSON_REMOVE(addresses, '$.mysqlX')
    WHERE CAST(SUBSTRING_INDEX(addresses->>'$.mysqlX', ':', -1) AS UNSIGNED) = (CAST(SUBSTRING_INDEX(addresses->>'$.mysqlClassic', ':', -1) AS UNSIGNED)*10)
    AND   CAST(SUBSTRING_INDEX(addresses->>'$.mysqlX', ':', -1) AS UNSIGNED) > 65535;

INSERT INTO routers
    SELECT r.router_id, r.router_name, 'MySQL Router', h.host_name,
        r.attributes->>'$.version', NULL, r.attributes,
    CAST(c.cluster_id AS CHAR(36)), NULL
    FROM mysql_innodb_cluster_metadata_previous.routers AS r,
         mysql_innodb_cluster_metadata_previous.hosts AS h,
         mysql_innodb_cluster_metadata_previous.clusters AS c
    WHERE r.host_id = h.host_id;


