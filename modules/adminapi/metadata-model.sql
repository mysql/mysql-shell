/*
  All the tables that contain information about the topology of MySQL servers
  are stored in the metadata_schema database.
*/

DROP DATABASE metadata_schema; -- Drop if schema already exists
CREATE DATABASE metadata_schema;
USE metadata_schema;

SET FOREIGN_KEY_CHECKS = 0; -- Avoid foreign key dependencies while dropping tables
DROP TABLES IF EXISTS
  farm, groups, servers, protocol_defaults, protocols,
  shard_tables, shard_sets, shards;
SET FOREIGN_KEY_CHECKS = 1; -- Enable foreign key checks while creating the tables

/*
  This table contain information about the metadata and is used to identify
  basic information about the farm/cluster.
*/
CREATE TABLE farm (
  /* unique ID used to distinguish the farm from other farms */
  `farm_uuid` VARCHAR(40) NOT NULL,
  /* unique name for the farm */
  `farm_name` VARCHAR(40) NOT NULL,
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
  /* Brief description of the farm. */
  `description` VARCHAR(120),
  PRIMARY KEY(farm_uuid)
);

/*
  The high-availability groups are the containers of the application data. Each
  group is composed of a number of MySQL servers in a HA configuration.
*/
CREATE TABLE groups (
  /*
    The group UUID is generated on creation of the group and does not change
    over the lifetime of the group.
  */
  `group_uuid` VARCHAR(40),
  /*
    A group can be assigned a name and this can be used to refer to the group.
    The name of the group can change over time and can even be NULL.
  */
  `group_name` VARCHAR(40),
  /* Associates the group with a farm definition. */
  `farm_uuid` VARCHAR(40),
  /*
    State of the group. Either active, in which case traffic can be directed at
    the group, or inactive, in which case no traffic should be directed to the
    group.
  */
  `group_active` BIT(1) NOT NULL,
  /* Tells if this is the primary group or not. */
  `primary_group` BIT(1) NOT NULL,
  /*
    Specifies the type of a group, for example, Group Replication,
    Primary Backup, MySQL Cluster etc. A string is selected to allow
    custom types to be defined without having to change the schema.
  */
  `group_type` VARCHAR(64),
  /* Properties of a given group. */
  `group_attributes` JSON,
  /* An optional brief description of the group. */
  `description` VARCHAR(120),
  PRIMARY KEY (group_uuid),
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE CASCADE
);

/*
  This table contain a list of all server that are tracked by the farm,
  regardless of whether they are part of a group or not. The reason for tracking
  all servers is that some servers may be temporarily moved out of groups for
  different purposes but should still be tracked and possible to locate as part
  of administrative procedures.
*/
CREATE TABLE servers (
  /* The UUID of the server and is a unique identifer of the server. */
  `server_uuid` VARCHAR(40),
  /* Associates the group with a farm definition. */
  `farm_uuid` VARCHAR(40),
  /*
    Group UUID that the server belongs to. Can be NULL if it does not belong to
    a group.
  */
  `group_uuid` VARCHAR(40),
  /* Is the type of server. */
  `server_type` VARCHAR(30),
  /* The role of the server in the setup e.g. scale-out, master etc. */
  `server_role` VARCHAR(30) NOT NULL,
  /*
    The mode of the server: if it accept read queries, write queries, neither,
    or both.
  */
  `server_mode` VARCHAR(30) NOT NULL,
  /*
    The weight of the server for load balancing purposes. The relative
    proportion of traffic it should receive.
  */
  `server_weight` FLOAT,
  /* A string representing the location. */
  `server_location` VARCHAR(256) NOT NULL,
  /*
    A JSON blob with the addresses available for the server. The protocols and
    addresses are further described in the Protocol section below.
  */
  `server_addresses` JSON NOT NULL,
  /*
    Contain attributes assigned to the server and is a JSON data type with
    key-value pair. The attributes can be used to tag the servers with custom
    attributes.
  */
  `server_attributes` JSON,
  /*
    Server version token in effect for the server. The version token changes
    whenever there is a change of the role of the server and is used to force
    cache invalidation when topology changes.
  */
  `version_token` INTEGER UNSIGNED,
  /* An optional brief description of the group. */
  `description` VARCHAR(120),
  PRIMARY KEY (server_uuid),
  FOREIGN KEY (group_uuid) REFERENCES groups(group_uuid) ON DELETE SET NULL,
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE CASCADE
);

/*
  This table provide the default port to use for a protocol given by a symbol.
  In the schema, we are using a string for the protocol name since this will
  allow custom definitions of protocols.
*/
CREATE TABLE protocol_defaults (
  /* Associates the group with a farm definition */
  `farm_uuid` VARCHAR(40),
  /* String describing the protocol e.g. classic.mysql */
  `protocol` VARCHAR(40) NOT NULL,
  /*
    Default address JSON object for the protocol without the address field
    filled in.
  */
  `default_address` JSON NOT NULL,
  PRIMARY KEY (protocol),
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE CASCADE
);

/*
  The table shard_sets identifies the set of application tables that are
  sharded using a specific sharding key.
*/
CREATE TABLE shard_sets(
  /* This column is a unique integer denoting the sharding set. */
  `shard_set_id` INT,
  /* Associates the group with a farm definition */
  `farm_uuid` VARCHAR(40),
  /*
    The sharding type is stored as a string to be possible to use when sharding
    on a string, integer, or arbitrary column.
  */
  `sharding_type` VARCHAR(40) NOT NULL,
  `master_schema_name` VARCHAR(64),
  `master_table_name` VARCHAR(64),
  PRIMARY KEY (shard_set_id),
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE CASCADE
);

/*
  The table shard_tables identifies tables that are sharded and whether there
  should be mechanisms in place (e.g. triggers) to check whether data is being
  inserted into the right shard.
*/
CREATE TABLE shard_tables(
  /* This column is a unique integer denoting the sharding set. */
  `shard_set_id` INT NOT NULL,
  /* Associates the group with a farm definition */
  `farm_uuid` VARCHAR(40),
  /* The schema in which the table being sharded is present */
  `schema_name` VARCHAR(64) NOT NULL,
  /* The table being sharded */
  `table_name` VARCHAR(64) NOT NULL,
  /*
    Name of the column that contains the sharding key. If the column is a field
    in a JSON blob, a generated column need to be constructed.
  */
  `field_ref` VARCHAR(256) NOT NULL,
  PRIMARY KEY (`schema_name`, `table_name`),
  FOREIGN KEY (shard_set_id) REFERENCES shard_sets(shard_set_id) ON DELETE CASCADE,
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE CASCADE
);

/*
  The shards table table identifies the data stored in each shard and the
  high-availability group of the shards. Note that with this definition of the
  shards table, it is possible to co-locate different shards in the same
  high-availability group.
*/
CREATE TABLE shards(
  /*
    This is the identifier for the shard and uniquely identify the partition of
    the data within the shard set.
  */
  `shard_id` INT,
  /* Associates the group with a farm definition */
  `farm_uuid` VARCHAR(40),
  /* The shard set the shard belong to. */
  `shard_set_id` INT,
  /*
    The group the shard belongs to. In the first iteration we only have one
    shard per group, but this scheme allow several shards per group.
  */
  `group_uuid` VARCHAR(40) NOT NULL,
  /*
    Lower bound for the shard, or NULL if there is no lower bound. The value
    have to be converted to the correct type to be used.
  */
  `lower_bound` VARBINARY(256) NOT NULL,
  /* An optional brief description of the group. */
  `description` VARCHAR(120),
  PRIMARY KEY (shard_set_id, shard_id),
  FOREIGN KEY (shard_set_id) REFERENCES shard_sets(shard_set_id),
  FOREIGN KEY (group_uuid) REFERENCES groups(group_uuid) ON DELETE CASCADE,
  FOREIGN KEY (farm_uuid) REFERENCES farm(farm_uuid) ON DELETE CASCADE
);
