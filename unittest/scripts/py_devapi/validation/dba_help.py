#@<OUT> Object Help
The global variable 'dba' is used to access the AdminAPI functionality and
perform DBA operations. It is used for managing MySQL InnoDB clusters.

The following properties are currently supported.

 - verbose Enables verbose mode on the Dba operations.


The following functions are currently supported.

 - check_instance_configuration        Validates an instance for cluster usage.
 - configure_local_instance            Validates and configures an instance for
                                       cluster usage.
 - create_cluster                      Creates a MySQL InnoDB cluster.
 - delete_sandbox_instance             Deletes an existing MySQL Server
                                       instance on localhost.
 - deploy_sandbox_instance             Creates a new MySQL Server instance on
                                       localhost.
 - drop_metadata_schema                Drops the Metadata Schema.
 - get_cluster                         Retrieves a cluster from the Metadata
                                       Store.
 - help                                Provides help about this class and it's
                                       members
 - kill_sandbox_instance               Kills a running MySQL Server instance on
                                       localhost.
 - reboot_cluster_from_complete_outage Brings a cluster back ONLINE when all
                                       members are OFFLINE.
 - reset_session                       Sets the session object to be used on
                                       the Dba operations.
 - start_sandbox_instance              Starts an existing MySQL Server instance
                                       on localhost.
 - stop_sandbox_instance               Stops a running MySQL Server instance on
                                       localhost.

For more help on a specific function use: dba.help('<functionName>')

e.g. dba.help('deploySandboxInstance')



#@<OUT> Create Cluster
Creates a MySQL InnoDB cluster.

SYNTAX

  <Dba>.create_cluster(name[, options])

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


#@<OUT> Delete Sandbox
Deletes an existing MySQL Server instance on localhost.

SYNTAX

  <Dba>.delete_sandbox_instance(port[, options])

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


#@<OUT> Deploy Sandbox
Creates a new MySQL Server instance on localhost.

SYNTAX

  <Dba>.deploy_sandbox_instance(port[, options])

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


#@<OUT> Drop Metadata
Drops the Metadata Schema.

SYNTAX

  <Dba>.drop_metadata_schema(options)

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


#@<OUT> Get Cluster
Retrieves a cluster from the Metadata Store.

SYNTAX

  <Dba>.get_cluster([name])

WHERE

  name: Parameter to specify the name of the cluster to be returned.

EXCEPTIONS

  MetadataError: if the Metadata is inaccessible.
  MetadataError: if the Metadata update operation failed.
  ArgumentError: if the Cluster name is empty.
  ArgumentError: if the Cluster name is invalid.
  ArgumentError: if the Cluster does not exist.

RETURNS

 The cluster object identified  by the given name or the default  cluster.

DESCRIPTION

If name is not specified, the default cluster will be returned.

If name is specified, and no cluster with the indicated name is found, an error
will be raised.


#@<OUT> Kill Sandbox
Kills a running MySQL Server instance on localhost.

SYNTAX

  <Dba>.kill_sandbox_instance(port[, options])

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


#@<OUT> Reset Session
Sets the session object to be used on the Dba operations.

SYNTAX

  <Dba>.reset_session(session)

WHERE

  session: Session object to be used on the Dba operations.

DESCRIPTION

Many of the Dba operations require an active session to the Metadata Store, use
this function to define the session to be used.

At the moment only a Classic session type is supported.

If the session type is not defined, the global dba object will use the active
session.

#@<OUT> Start Sandbox
Starts an existing MySQL Server instance on localhost.

SYNTAX

  <Dba>.start_sandbox_instance(port[, options])

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

#@<OUT> Check Instance Configuration
Validates an instance for cluster usage.

SYNTAX

  <Dba>.check_instance_configuration(instance[, options])

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

The connection data may be specified in the following formats:

 - A URI string
 - A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]host[:port][/schema][?option=value&option=value...]

The following options are valid for use either in a URI or in a dictionary:

 - ssl-mode: the SSL mode to be used in the connection.
 - ssl-ca: the path to the X509 certificate authority in PEM format.
 - ssl-capath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - ssl-cert: The path to the X509 certificate in PEM format.
 - ssl-key: The path to the X509 key in PEM format.
 - ssl-crl: The path to file that contains certificate revocation lists.
 - ssl-crlpath: The path of directory that contains certificate revocation list
   files.
 - ssl-ciphers: List of permitted ciphers to use for connection encryption.
 - tls-version: List of protocols permitted for secure connections
 - auth-method: Authentication method

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

 - scheme: the protocol to be used on the connection.
 - user: the MySQL user name to be used on the connection.
 - dbUser: alias for user.
 - password: the password to be used on the connection.
 - dbPassword: same as password.
 - host: the hostname or IP address to be used on a TCP connection.
 - port: the port to be used in a TCP connection.
 - socket: the socket file name to be used on a connection through unix
   sockets.
 - schema: the schema to be selected once the connection is done.

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

For additional information on connection data use \? connection.

The options dictionary may contain the following options:

 - mycnfPath: The path of the MySQL configuration file for the instance.
 - password: The password to get connected to the instance.
 - clusterAdmin: The name of the InnoDB cluster administrator user.
 - clusterAdminPassword: The password for the InnoDB cluster administrator
   account.

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



#@<OUT> Stop Sandbox
Stops a running MySQL Server instance on localhost.

SYNTAX

  <Dba>.stop_sandbox_instance(port[, options])

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


#@<OUT> Configure Local Instance
Validates and configures an instance for cluster usage.

SYNTAX

  <Dba>.configure_local_instance(instance[, options])

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

The connection data may be specified in the following formats:

 - A URI string
 - A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]host[:port][/schema][?option=value&option=value...]

The following options are valid for use either in a URI or in a dictionary:

 - ssl-mode: the SSL mode to be used in the connection.
 - ssl-ca: the path to the X509 certificate authority in PEM format.
 - ssl-capath: the path to the directory that contains the X509 certificates
   authorities in PEM format.
 - ssl-cert: The path to the X509 certificate in PEM format.
 - ssl-key: The path to the X509 key in PEM format.
 - ssl-crl: The path to file that contains certificate revocation lists.
 - ssl-crlpath: The path of directory that contains certificate revocation list
   files.
 - ssl-ciphers: List of permitted ciphers to use for connection encryption.
 - tls-version: List of protocols permitted for secure connections
 - auth-method: Authentication method

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

 - scheme: the protocol to be used on the connection.
 - user: the MySQL user name to be used on the connection.
 - dbUser: alias for user.
 - password: the password to be used on the connection.
 - dbPassword: same as password.
 - host: the hostname or IP address to be used on a TCP connection.
 - port: the port to be used in a TCP connection.
 - socket: the socket file name to be used on a connection through unix
   sockets.
 - schema: the schema to be selected once the connection is done.

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

For additional information on connection data use \? connection.

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

#@<OUT> Verbose
Enables verbose mode on the Dba operations.

DESCRIPTION

The assigned value can be either boolean or integer, the result depends on the
assigned value:

 - 0: disables mysqlprovision verbosity
 - 1: enables mysqlprovision verbosity
 - >1: enables mysqlprovision debug verbosity
 - Boolean: equivalent to assign either 0 or 1


#@<OUT> Reboot Cluster
Brings a cluster back ONLINE when all members are OFFLINE.

SYNTAX

  <Dba>.reboot_cluster_from_complete_outage([clusterName][, options])

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
