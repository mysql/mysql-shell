#@ Initialization
||

#@<OUT> create cluster
{
    "clusterName": "dev",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "<<<__ssl_mode>>>",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<localhost>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<localhost>>>:<<<__mysql_sandbox_port1>>>",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE"
            }
        }
    },
    "groupInformationSourceMember": "mysql://root@<<<localhost>>>:<<<__mysql_sandbox_port1>>>"
}

#@<OUT> Object Help
The cluster object is the entry point to manage and monitor a MySQL InnoDB
cluster.

A cluster is a set of MySQLd Instances which holds the user's data.

It provides high-availability and scalability for the user's data.

The following properties are currently supported.

 - name Cluster name.


The following functions are currently supported.

 - add_instance                    Adds an Instance to the cluster.
 - check_instance_state            Verifies the instance gtid state in relation
                                   with the cluster.
 - describe                        Describe the structure of the cluster.
 - disconnect                      Disconnects all internal sessions used by
                                   the cluster object.
 - dissolve                        Dissolves the cluster.
 - force_quorum_using_partition_of Restores the cluster from quorum loss.
 - get_name                        Retrieves the name of the cluster.
 - help                            Provides help about this class and it's
                                   members
 - rejoin_instance                 Rejoins an Instance to the cluster.
 - remove_instance                 Removes an Instance from the cluster.
 - rescan                          Rescans the cluster.
 - status                          Describe the status of the cluster.

For more help on a specific function use: cluster.help('<functionName>')

e.g. cluster.help('addInstance')

#@<OUT> Add Instance
Adds an Instance to the cluster.

SYNTAX

  <Cluster>.add_instance(instance[, options])

WHERE

  instance: An instance definition.
  options: Dictionary with options for the operation.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  ArgumentError: if the instance parameter is empty.
  ArgumentError: if the instance definition is invalid.
  ArgumentError: if the instance definition is a connection dictionary but
                 empty.
  RuntimeError: if the instance accounts are invalid.
  RuntimeError: if the instance is not in bootstrapped state.
  ArgumentError: if the value for the memberSslMode option is not one of the
                 allowed: "AUTO", "DISABLED", "REQUIRED".
  RuntimeError: if the SSL mode specified is not compatible with the one used
                in the cluster.
  ArgumentError: if the value for the ipWhitelist, localAddress, or groupSeeds
                 options is empty.
  RuntimeError: if the value for the localAddress or groupSeeds options is not
                valid for Group Replication.

RETURNS

 nothing

DESCRIPTION

This function adds an Instance to the default replica set of the cluster.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The options dictionary may contain the following attributes:

 - label: an identifier for the instance being added
 - password: the instance connection password
 - memberSslMode: SSL mode used on the instance
 - ipWhitelist: The list of hosts allowed to connect to the instance for group
   replication
 - localAddress: string value with the Group Replication local address to be
   used instead of the automatically generated one.
 - groupSeeds: string value with a comma-separated list of the Group
   Replication peer addresses to be used instead of the automatically generated
   one.

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

The memberSslMode option supports these values:

 - REQUIRED: if used, SSL (encryption) will be enabled for the instance to
   communicate with other members of the cluster
 - DISABLED: if used, SSL (encryption) will be disabled
 - AUTO: if used, SSL (encryption) will be automatically enabled or disabled
   based on the cluster configuration

If memberSslMode is not specified AUTO will be used by default.

The ipWhitelist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the whitelist.

The localAddress and groupSeeds are advanced options and their usage is
discouraged since incorrect values can lead to Group Replication errors.

The value for localAddress is used to set the Group Replication system variable
'group_replication_local_address'. The localAddress option accepts values in
the format: 'host:port' or 'host:' or ':port'. If the specified value does not
include a colon (:) and it is numeric, then it is assumed to be the port,
otherwise it is considered to be the host. When the host is not specified, the
default value is the host of the target instance specified as argument. When
the port is not specified, the default value is the port of the target instance
* 10 + 1. In case the automatically determined default port value is invalid (>
65535) then a random value in the range [1000, 65535] is used.

The value for groupSeeds is used to set the Group Replication system variable
'group_replication_group_seeds'. The groupSeeds option accepts a
comma-separated list of addresses in the format: 'host1:port1,...,hostN:portN'.



#@<OUT> Check Instance State
Verifies the instance gtid state in relation with the cluster.

SYNTAX

  <Cluster>.check_instance_state(instance[, password])

WHERE

  instance: An instance definition.
  password: String with the password for the connection.

EXCEPTIONS

  ArgumentError: if the instance parameter is empty.
  ArgumentError: if the instance definition is invalid.
  ArgumentError: if the instance definition is a connection dictionary but
                 empty.
  RuntimeError: if the instance accounts are invalid.
  RuntimeError: if the instance is offline.

DESCRIPTION

Analyzes the instance executed GTIDs with the executed/purged GTIDs on the
cluster to determine if the instance is valid for the cluster.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The password may be contained on the instance definition, however, it can be
overwritten if it is specified as a second parameter.

The returned JSON object contains the following attributes:

 - state: the state of the instance
 - reason: the reason for the state reported

The state of the instance can be one of the following:

 - ok: if the instance transaction state is valid for the cluster
 - error: if the instance transaction state is not valid for the cluster

The reason for the state reported can be one of the following:

 - new: if the instance doesn’t have any transactions
 - recoverable:  if the instance executed GTIDs are not conflicting with the
   executed GTIDs of the cluster instances
 - diverged: if the instance executed GTIDs diverged with the executed GTIDs of
   the cluster instances
 - lost_transactions: if the instance has more executed GTIDs than the executed
   GTIDs of the cluster instances



#@<OUT> Describe

Describe the structure of the cluster.

SYNTAX

  <Cluster>.describe()

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.

RETURNS

 A JSON object describing the structure of the cluster.

DESCRIPTION

This function describes the structure of the cluster including all its
information, ReplicaSets and Instances.

The returned JSON object contains the following attributes:

 - clusterName: the cluster name
 - defaultReplicaSet: the default replicaSet object

The defaultReplicaSet JSON object contains the following attributes:

 - name: the default replicaSet name
 - instances: a List of dictionaries describing each instance belonging to the
   Default ReplicaSet.

Each instance dictionary contains the following attributes:

 - label: the instance name identifier
 - host: the instance hostname and IP address in the form of host:port
 - role: the instance role

#@<OUT> Dissolve

Dissolves the cluster.

SYNTAX

  <Cluster>.dissolve([options])

WHERE

  options: Parameter to specify if it should deactivate replication and
           unregister the ReplicaSets from the cluster.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.

RETURNS

 nothing.

DESCRIPTION

This function disables replication on the ReplicaSets, unregisters them and the
the cluster from the metadata.

It keeps all the user's data intact.

The following is the only option supported:

 - force: boolean, confirms that the dissolve operation must be executed.

#@<OUT> Force Quorum Using Partition Of
Restores the cluster from quorum loss.

SYNTAX

  <Cluster>.force_quorum_using_partition_of(instance[, password])

WHERE

  instance: An instance definition to derive the forced group from.
  password: String with the password for the connection.

EXCEPTIONS

  MetadataError: if the instance parameter is empty.
  ArgumentError: if the instance parameter is empty.
  RuntimeError: if the instance does not exist on the Metadata.
  RuntimeError: if the instance is not on the ONLINE state.
  RuntimeError: if the instance does is not an active member of a replication
                group.
  RuntimeError: if there are no ONLINE instances visible from the given one.
  LogicError: if the cluster does not exist.

RETURNS

 nothing.

DESCRIPTION

This function restores the cluster's default replicaset back into operational
status from a loss of quorum scenario. Such a scenario can occur if a group is
partitioned or more crashes than tolerable occur.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The options dictionary may contain the following options:

 - mycnfPath: The path of the MySQL configuration file for the instance.
 - password: The password to get connected to the instance.
 - clusterAdmin: The name of the InnoDB cluster administrator user.
 - clusterAdminPassword: The password for the InnoDB cluster administrator
   account.

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

Note that this operation is DANGEROUS as it can create a split-brain if
incorrectly used and should be considered a last resort. Make absolutely sure
that there are no partitions of this group that are still operating somewhere
in the network, but not accessible from your location.

When this function is used, all the members that are ONLINE from the point of
view of the given instance definition will be added to the group.


#@<OUT> Get Name

Retrieves the name of the cluster.

SYNTAX

  <Cluster>.get_name()

RETURNS

 The name of the cluster.

#@<OUT> Help

SYNTAX

  <Cluster>.help()

#@<OUT> Rejoin Instance
Rejoins an Instance to the cluster.

SYNTAX

  <Cluster>.rejoin_instance(instance[, options])

WHERE

  instance: An instance definition.
  options: Dictionary with options for the operation.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  RuntimeError: if the instance does not exist.
  RuntimeError: if the instance accounts are invalid.
  RuntimeError: if the instance is not in bootstrapped state.
  ArgumentError: if the value for the memberSslMode option is not one of the
                 allowed: "AUTO", "DISABLED", "REQUIRED".
  RuntimeError: if the SSL mode specified is not compatible with the one used
                in the cluster.
  RuntimeError: if the instance is an active member of the ReplicaSet.

RETURNS

 nothing.

DESCRIPTION

This function rejoins an Instance to the cluster.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The options dictionary may contain the following attributes:

 - label: an identifier for the instance being added
 - password: the instance connection password
 - memberSslMode: SSL mode used to be used on the instance
 - ipWhitelist: The list of hosts allowed to connect to the instance for group
   replication

The password may be contained on the instance definition, however, it can be
overwritten if it is specified on the options.

The memberSslMode option supports these values:

 - REQUIRED: if used, SSL (encryption) will be enabled for the instance to
   communicate with other members of the cluster
 - DISABLED: if used, SSL (encryption) will be disabled
 - AUTO: if used, SSL (encryption) will be automatically enabled or disabled
   based on the cluster configuration

If memberSslMode is not specified AUTO will be used by default.

The ipWhitelist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the whitelist.


#@<OUT> Remove Instance
Removes an Instance from the cluster.

SYNTAX

  <Cluster>.remove_instance(instance[, options])

WHERE

  instance: An instance definition.
  options: Dictionary with options for the operation.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  ArgumentError: if the instance parameter is empty.
  ArgumentError: if the instance definition is invalid.
  ArgumentError: if the instance definition is a connection dictionary but
                 empty.
  RuntimeError: if the instance accounts are invalid.
  RuntimeError: if an error occurs when trying to remove the instance (e.g.,
                instance is not reachable).

RETURNS

 nothing.

DESCRIPTION

This function removes an Instance from the default replicaSet of the cluster.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The options dictionary may contain the following attributes:

 - password/dbPassword: the instance connection password
 - force: boolean, indicating if the instance must be removed (even if only
   from metadata) in case it cannot be reached. By default, set to false.

The password may be contained in the instance definition, however, it can be
overwritten if it is specified on the options.

The force option (set to true) must only be used to remove instances that are
permanently not available (no longer reachable) or never to be reused again in
a cluster. This allows to remove from the metadata an instance than can no
longer be recovered. Otherwise, the instance must be brought back ONLINE and
removed without the force option to avoid errors trying to add it back to a
cluster.


#@<OUT> Rescan

Rescans the cluster.

SYNTAX

  <Cluster>.rescan()

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  LogicError: if the cluster does not exist.
  RuntimeError: if all the ReplicaSet instances of any ReplicaSet are offline.

RETURNS

 nothing.

DESCRIPTION

This function rescans the cluster for new Group Replication members/instances.

#@<OUT> Status

Describe the status of the cluster.

SYNTAX

  <Cluster>.status()

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.

RETURNS

 A JSON object describing the status of the cluster.

DESCRIPTION

This function describes the status of the cluster including its ReplicaSets and
Instances.

The returned JSON object contains the following attributes:

 - clusterName: the cluster name
 - defaultReplicaSet: the default replicaSet object

The defaultReplicaSet JSON object contains the following attributes:

 - name: the default replicaSet name
 - primary: the Default ReplicaSet single-master primary instance
 - status: the Default ReplicaSet status
 - statusText: the Default ReplicaSet status descriptive text
 - topology: a List of instances belonging to the Default ReplicaSet.

Each instance is dictionary containing the following attributes:

 - label: the instance name identifier
 - address: the instance hostname and IP address in the form of host:port

#@ Finalization
||
