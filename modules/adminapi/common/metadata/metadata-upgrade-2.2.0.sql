/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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

-- Source: 2.1.0
-- Target: 2.2.0
-- PRE SQL Statements
USE mysql_innodb_cluster_metadata;

ALTER TABLE
  `mysql_innodb_cluster_metadata`.`instances`
ADD
  COLUMN `instance_type` ENUM('group-member', 'async-member', 'read-replica') NOT NULL
AFTER
  `description`;

-- Populate the new column with the right data: instance is either part of a Cluster or a ReplicaSet
UPDATE `mysql_innodb_cluster_metadata`.`instances` AS i
JOIN `mysql_innodb_cluster_metadata`.`clusters` AS c
  ON i.cluster_id = c.cluster_id
SET i.instance_type =
  IF(c.cluster_type = 'gr', 'group-member', 'async-member');

ALTER TABLE
  `mysql_innodb_cluster_metadata`.`routers`
MODIFY `address` VARCHAR(256) CHARACTER SET ascii COLLATE ascii_general_ci NOT NULL;

-- Deploy: METADATA_MODEL_2_2_0

-- POST SQL Statements
/* Migrates the data from the mysql_innodb_cluster_metadata_previous taken by
 * the upgrade engine before the upgrade is started.
 */
DELIMITER ;
