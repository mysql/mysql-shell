//@<OUT> Object Help
The global variable 'dba' is used to access the AdminAPI functionality and
perform DBA operations. It is used for managing MySQL InnoDB clusters.

The following properties are currently supported.

 - verbose Enables verbose mode on the Dba operations.


The following functions are currently supported.

 - checkInstanceConfiguration      Validates an instance for cluster usage.
 - configureLocalInstance          Validates and configures an instance for
                                   cluster usage.
 - createCluster                   Creates a MySQL InnoDB cluster.
 - deleteSandboxInstance           Deletes an existing MySQL Server instance on
                                   localhost.
 - deploySandboxInstance           Creates a new MySQL Server instance on
                                   localhost.
 - dropMetadataSchema              Drops the Metadata Schema.
 - getCluster                      Retrieves a cluster from the Metadata Store.
 - help                            Provides help about this class and it's
                                   members
 - killSandboxInstance             Kills a running MySQL Server instance on
                                   localhost.
 - rebootClusterFromCompleteOutage Brings a cluster back ONLINE when all
                                   members are OFFLINE.
 - startSandboxInstance            Starts an existing MySQL Server instance on
                                   localhost.
 - stopSandboxInstance             Stops a running MySQL Server instance on
                                   localhost.

For more help on a specific function use: dba.help('<functionName>')

e.g. dba.help('deploySandboxInstance')


//@<OUT> Create Cluster
Creates a MySQL InnoDB cluster.

SYNTAX

  <Dba>.createCluster(name[, options])

WHERE

  name: The name of the cluster object to be created.
  options: Dictionary with options that modify the behavior of this function.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  ArgumentError: if the Cluster name is empty.
  ArgumentError: if the Cluster name is not valid.
  ArgumentError: if the options contain an invalid attribute.
  ArgumentError: if adoptFromGR is true and the memberSslMode option is used.
  ArgumentError: if the value for the memberSslMode option is not one of the
                 allowed.
  ArgumentError: if adoptFromGR is true and the multiMaster option is used.
  ArgumentError: if the value for the ipWhitelist, groupName, localAddress, or
                 groupSeeds options is empty.
  RuntimeError: if the value for the groupName, localAddress, or groupSeeds
                options is not valid for Group Replication.

RETURNS

 The created cluster object.

DESCRIPTION

Creates a MySQL InnoDB cluster taking as seed instance the active global
session.

The options dictionary can contain the next values:

 - multiMaster: boolean value used to define an InnoDB cluster with multiple
   writable instances.
 - force: boolean, confirms that the multiMaster option must be applied.
 - adoptFromGR: boolean value used to create the InnoDB cluster based on
   existing replication group.
 - memberSslMode: SSL mode used to configure the members of the cluster.
 - ipWhitelist: The list of hosts allowed to connect to the instance for group
   replication.
 - clearReadOnly: boolean value used to confirm that super_read_only must be
   disabled.
 - groupName: string value with the Group Replication group name UUID to be
   used instead of the automatically generated one.
 - localAddress: string value with the Group Replication local address to be
   used instead of the automatically generated one.
 - groupSeeds: string value with a comma-separated list of the Group
   Replication peer addresses to be used instead of the automatically generated
   one.

A InnoDB cluster may be setup in two ways:

 - Single Master: One member of the cluster allows write operations while the
   rest are in read only mode.
 - Multi Master: All the members in the cluster support both read and write
   operations.

By default this function create a Single Master cluster, use the multiMaster
option set to true if a Multi Master cluster is required.

The memberSslMode option supports these values:

 - REQUIRED: if used, SSL (encryption) will be enabled for the instances to
   communicate with other members of the cluster
 - DISABLED: if used, SSL (encryption) will be disabled
 - AUTO: if used, SSL (encryption) will be enabled if supported by the
   instance, otherwise disabled

If memberSslMode is not specified AUTO will be used by default.

The ipWhitelist format is a comma separated list of IP addresses or subnet CIDR
notation, for example: 192.168.1.0/24,10.0.0.1. By default the value is set to
AUTOMATIC, allowing addresses from the instance private network to be
automatically set for the whitelist.

The groupName, localAddress, and groupSeeds are advanced options and their
usage is discouraged since incorrect values can lead to Group Replication
errors.

The value for groupName is used to set the Group Replication system variable
'group_replication_group_name'.

The value for localAddress is used to set the Group Replication system variable
'group_replication_local_address'. The localAddress option accepts values in
the format: 'host:port' or 'host:' or ':port'. If the specified value does not
include a colon (:) and it is numeric, then it is assumed to be the port,
otherwise it is considered to be the host. When the host is not specified, the
default value is the host of the current active connection (session). When the
port is not specified, the default value is the port of the current active
connection (session) + 10000. In case the automatically determined default port
value is invalid (> 65535) then a random value in the range [1000, 65535] is
used.

The value for groupSeeds is used to set the Group Replication system variable
'group_replication_group_seeds'. The groupSeeds option accepts a
comma-separated list of addresses in the format: 'host1:port1,...,hostN:portN'.


//@<OUT> Delete Sandbox
Deletes an existing MySQL Server instance on localhost.

SYNTAX

  <Dba>.deleteSandboxInstance(port[, options])

WHERE

  port: The port of the instance to be deleted.
  options: Dictionary with options that modify the way this function is
           executed.

EXCEPTIONS

  ArgumentError: if the options contain an invalid attribute.
  ArgumentError: if the port value is < 1024 or > 65535.

RETURNS

 nothing.

DESCRIPTION

This function will delete an existing MySQL Server instance on the local host.
The following options affect the result:

 - sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems or %userprofile%\MySQL\mysql-sandboxes
on Windows systems.

If the instance is not located on the used path an error will occur.


//@<OUT> Deploy Sandbox
Creates a new MySQL Server instance on localhost.

SYNTAX

  <Dba>.deploySandboxInstance(port[, options])

WHERE

  port: The port where the new instance will listen for connections.
  options: Dictionary with options affecting the new deployed instance.

EXCEPTIONS

  ArgumentError: if the options contain an invalid attribute.
  ArgumentError: if the root password is missing on the options.
  ArgumentError: if the port value is < 1024 or > 65535.
  RuntimeError: if SSL support can be provided and ignoreSslError: false.

RETURNS

 nothing.

DESCRIPTION

This function will deploy a new MySQL Server instance, the result may be
affected by the provided options:

 - portx: port where the new instance will listen for X Protocol connections.
 - sandboxDir: path where the new instance will be deployed.
 - password: password for the MySQL root user on the new instance.
 - allowRootFrom: create remote root account, restricted to the given address
   pattern (eg %).
 - ignoreSslError: Ignore errors when adding SSL support for the new instance,
   by default: true.

If the portx option is not specified, it will be automatically calculated as 10
times the value of the provided MySQL port.

The password or dbPassword options specify the MySQL root password on the new
instance.

The sandboxDir must be an existing folder where the new instance will be
deployed. If not specified the new instance will be deployed at:

  ~/mysql-sandboxes on Unix-like systems or %userprofile%\MySQL\mysql-sandboxes
on Windows systems.

SSL support is added by default if not already available for the new instance,
but if it fails to be added then the error is ignored. Set the ignoreSslError
option to false to ensure the new instance is deployed with SSL support.


//@<OUT> Drop Metadata
Drops the Metadata Schema.

SYNTAX

  <Dba>.dropMetadataSchema(options)

WHERE

  options: Dictionary containing an option to confirm the drop operation.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.

RETURNS

 nothing.

DESCRIPTION

The options dictionary may contain the following options:

 - force: boolean, confirms that the drop operation must be executed.
 - clearReadOnly: boolean value used to confirm that super_read_only must be
   disabled


//@<OUT> Get Cluster
Retrieves a cluster from the Metadata Store.

SYNTAX

  <Dba>.getCluster([name][, options])

WHERE

  name: Parameter to specify the name of the cluster to be returned.
  options: Dictionary with additional options.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  ArgumentError: if the Cluster name is empty.
  ArgumentError: if the Cluster name is invalid.
  ArgumentError: if the Cluster does not exist.

RETURNS

 The cluster object identified  by the given name or the default  cluster.

DESCRIPTION

If name is not specified or is null, the default cluster will be returned.

If name is specified, and no cluster with the indicated name is found, an error
will be raised.

The options dictionary accepts the connectToPrimary option,which defaults to
true and indicates the shell to automatically connect to the primary member of
the cluster.

//@<OUT> Kill Sandbox
Kills a running MySQL Server instance on localhost.

SYNTAX

  <Dba>.killSandboxInstance(port[, options])

WHERE

  port: The port of the instance to be killed.
  options: Dictionary with options affecting the result.

EXCEPTIONS

  ArgumentError: if the options contain an invalid attribute.
  ArgumentError: if the port value is < 1024 or > 65535.

RETURNS

 nothing.

DESCRIPTION

This function will kill the process of a running MySQL Server instance on the
local host. The following options affect the result:

 - sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems or %userprofile%\MySQL\mysql-sandboxes
on Windows systems.

If the instance is not located on the used path an error will occur.


//@<OUT> Start Sandbox
Starts an existing MySQL Server instance on localhost.

SYNTAX

  <Dba>.startSandboxInstance(port[, options])

WHERE

  port: The port where the instance listens for MySQL connections.
  options: Dictionary with options affecting the result.

EXCEPTIONS

  ArgumentError: if the options contain an invalid attribute.
  ArgumentError: if the port value is < 1024 or > 65535.

RETURNS

 nothing.

DESCRIPTION

This function will start an existing MySQL Server instance on the local host.
The following options affect the result:

 - sandboxDir: path where the instance is located.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems or %userprofile%\MySQL\mysql-sandboxes
on Windows systems.

If the instance is not located on the used path an error will occur.

//@<OUT> Check Instance Configuration
Validates an instance for cluster usage.

SYNTAX

  <Dba>.checkInstanceConfiguration(instance[, options])

WHERE

  instance: An instance definition.
  options: Data for the operation.

EXCEPTIONS

  ArgumentError: if the instance parameter is empty.
  ArgumentError: if the instance definition is invalid.
  ArgumentError: if the instance definition is a connection dictionary but
                 empty.
  RuntimeError: if the instance accounts are invalid.
  RuntimeError: if the instance is offline.
  RuntimeError: if the instance is already part of a Replication Group.
  RuntimeError: if the instance is already part of an InnoDB Cluster.

RETURNS

 A JSON object with the status.

DESCRIPTION

This function reviews the instance configuration to identify if it is valid for
usage in group replication.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The options dictionary may contain the following options:

 - mycnfPath: The path of the MySQL configuration file for the instance.
 - password: The password to get connected to the instance.

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

The returned JSON object contains the following attributes:

 - status: the final status of the command, either "ok" or "error".
 - config_errors: a list of dictionaries containing the failed requirements
 - errors: a list of errors of the operation
 - restart_required: a boolean value indicating whether a restart is required

Each dictionary of the list of config_errors includes the following attributes:

 - option: The configuration option for which the requirement wasn't met
 - current: The current value of the configuration option
 - required: The configuration option required value
 - action: The action to be taken in order to meet the requirement

The action can be one of the following:

 - server_update+config_update: Both the server and the configuration need to
   be updated
 - config_update+restart: The configuration needs to be updated and the server
   restarted
 - config_update: The configuration needs to be updated
 - server_update: The server needs to be updated
 - restart: The server needs to be restarted


//@<OUT> Stop Sandbox
Stops a running MySQL Server instance on localhost.

SYNTAX

  <Dba>.stopSandboxInstance(port[, options])

WHERE

  port: The port of the instance to be stopped.
  options: Dictionary with options affecting the result.

EXCEPTIONS

  ArgumentError: if the options contain an invalid attribute.
  ArgumentError: if the root password is missing on the options.
  ArgumentError: if the port value is < 1024 or > 65535.

RETURNS

 nothing.

DESCRIPTION

This function will gracefully stop a running MySQL Server instance on the local
host. The following options affect the result:

 - sandboxDir: path where the instance is located.
 - password: password for the MySQL root user on the instance.

The sandboxDir must be the one where the MySQL instance was deployed. If not
specified it will use:

  ~/mysql-sandboxes on Unix-like systems or %userprofile%\MySQL\mysql-sandboxes
on Windows systems.

If the instance is not located on the used path an error will occur.


//@<OUT> Configure Local Instance
Validates and configures an instance for cluster usage.

SYNTAX

  <Dba>.configureLocalInstance(instance[, options])

WHERE

  instance: An instance definition.
  options: Additional options for the operation.

EXCEPTIONS

  ArgumentError: if the instance parameter is empty.
  ArgumentError: if the instance definition is invalid.
  ArgumentError: if the instance definition is a connection dictionary but
                 empty.
  RuntimeError: if the instance accounts are invalid.
  RuntimeError: if the instance is offline.
  RuntimeError: if the instance is already part of a Replication Group.
  RuntimeError: if the instance is already part of an InnoDB Cluster.

RETURNS

 resultset A JSON object with the status.

DESCRIPTION

This function reviews the instance configuration to identify if it is valid for
usage in group replication and cluster. A JSON object is returned containing
the result of the operation.

The instance definition is the connection data for the instance.

For additional information on connection data use \? connection.

Only TCP/IP connections are allowed for this function.

The options dictionary may contain the following options:

 - mycnfPath: The path to the MySQL configuration file of the instance.
 - password: The password to be used on the connection.
 - clusterAdmin: The name of the InnoDB cluster administrator user to be
   created. The supported format is the standard MySQL account name format.
 - clusterAdminPassword: The password for the InnoDB cluster administrator
   account.
 - clearReadOnly: boolean value used to confirm that super_read_only must be
   disabled.

The connection password may be contained on the instance definition, however,
it can be overwritten if it is specified on the options.

The returned JSON object contains the following attributes:

 - status: the final status of the command, either "ok" or "error".
 - config_errors: a list of dictionaries containing the failed requirements
 - errors: a list of errors of the operation
 - restart_required: a boolean value indicating whether a restart is required

Each dictionary of the list of config_errors includes the following attributes:

 - option: The configuration option for which the requirement wasn't met
 - current: The current value of the configuration option
 - required: The configuration option required value
 - action: The action to be taken in order to meet the requirement

The action can be one of the following:

 - server_update+config_update: Both the server and the configuration need to
   be updated
 - config_update+restart: The configuration needs to be updated and the server
   restarted
 - config_update: The configuration needs to be updated
 - server_update: The server needs to be updated
 - restart: The server needs to be restarted


//@<OUT> Verbose
Enables verbose mode on the Dba operations.

DESCRIPTION

The assigned value can be either boolean or integer, the result depends on the
assigned value:

 - 0: disables mysqlprovision verbosity
 - 1: enables mysqlprovision verbosity
 - >1: enables mysqlprovision debug verbosity
 - Boolean: equivalent to assign either 0 or 1


//@<OUT> Reboot Cluster
Brings a cluster back ONLINE when all members are OFFLINE.

SYNTAX

  <Dba>.rebootClusterFromCompleteOutage([clusterName][, options])

WHERE

  clusterName: The name of the cluster to be rebooted.
  options: Dictionary with options that modify the behavior of this function.

EXCEPTIONS

  MetadataError:  if the Metadata is inaccessible.
  ArgumentError: if the Cluster name is empty.
  ArgumentError: if the Cluster name is not valid.
  ArgumentError: if the options contain an invalid attribute.
  RuntimeError: if the Cluster does not exist on the Metadata.
  RuntimeError: if some instance of the Cluster belongs to a Replication Group.

RETURNS

 The rebooted cluster object.

DESCRIPTION

The options dictionary can contain the next values:

 - password: The password used for the instances sessions required operations.
 - removeInstances: The list of instances to be removed from the cluster.
 - rejoinInstances: The list of instances to be rejoined on the cluster.
 - clearReadOnly: boolean value used to confirm that super_read_only must be
   disabled

This function reboots a cluster from complete outage. It picks the instance the
MySQL Shell is connected to as new seed instance and recovers the cluster.
Optionally it also updates the cluster configuration based on user provided
options.

On success, the restored cluster object is returned by the function.

The current session must be connected to a former instance of the cluster.

If name is not specified, the default cluster will be returned.
