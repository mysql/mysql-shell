#@<OUT> dba
NAME
      dba - InnoDB cluster and replicaset management functions.

DESCRIPTION
      Entry point for AdminAPI functions, including InnoDB clusters and replica
      sets.

      InnoDB clusters

      The dba.configure_instance() function can be used to configure a MySQL
      instance with the settings required to use it in an InnoDB cluster.

      InnoDB clusters can be created with the dba.create_cluster() function.

      Once created, InnoDB cluster management objects can be obtained with the
      dba.get_cluster() function.

      InnoDB ReplicaSets

      The dba.configure_replica_set_instance() function can be used to
      configure a MySQL instance with the settings required to use it in a
      replicaset.

      ReplicaSets can be created with the dba.create_replica_set() function.

      Once created, replicaset management objects can be obtained with the
      dba.get_replica_set() function.

      Sandboxes

      Utility functions are provided to create sandbox MySQL instances, which
      can be used to create test clusters and replicasets.

PROPERTIES
      session
            The session the dba object will use by default.

      verbose
            Controls debug message verbosity for sandbox related dba
            operations.

FUNCTIONS
      check_instance_configuration(instance[, options])
            Validates an instance for MySQL InnoDB Cluster usage.

      configure_instance([instance][, options])
            Validates and configures an instance for MySQL InnoDB Cluster
            usage.

      configure_local_instance(instance[, options])
            Validates and configures a local instance for MySQL InnoDB Cluster
            usage.

      configure_replica_set_instance([instance][, options])
            Validates and configures an instance for use in an InnoDB
            ReplicaSet.

      create_cluster(name[, options])
            Creates a MySQL InnoDB cluster.

      create_replica_set(name[, options])
            Creates a MySQL InnoDB ReplicaSet.

      delete_sandbox_instance(port[, options])
            Deletes an existing MySQL Server instance on localhost.

      deploy_sandbox_instance(port[, options])
            Creates a new MySQL Server instance on localhost.

      drop_metadata_schema(options)
            Drops the Metadata Schema.

      get_cluster([name][, options])
            Retrieves a cluster from the Metadata Store.

      get_cluster_set()
            Returns an object representing a ClusterSet.

      get_replica_set()
            Returns an object representing a ReplicaSet.

      help([member])
            Provides help about this object and it's members

      kill_sandbox_instance(port[, options])
            Kills a running MySQL Server instance on localhost.

      reboot_cluster_from_complete_outage([clusterName][, options])
            Brings a cluster back ONLINE when all members are OFFLINE.

      start_sandbox_instance(port[, options])
            Starts an existing MySQL Server instance on localhost.

      stop_sandbox_instance(port[, options])
            Stops a running MySQL Server instance on localhost.

      upgrade_metadata([options])
            Upgrades (or restores) the metadata to the version supported by the
            Shell.

      SEE ALSO

      - For general information about the AdminAPI use: \? AdminAPI
      - For help on a specific function use: \? dba.<functionName>

      e.g. \? dba.deploy_sandbox_instance

#@<OUT> dba.check_instance_configuration
NAME
      check_instance_configuration - Validates an instance for MySQL InnoDB
                                     Cluster usage.

SYNTAX
      dba.check_instance_configuration(instance[, options])

WHERE
      instance: An instance definition.
      options: Data for the operation.

RETURNS
      A descriptive text of the operation result.

DESCRIPTION
      This function reviews the instance configuration to identify if it is
      valid for usage with group replication. Use this to check for possible
      configuration issues on MySQL instances before creating a cluster with
      them or adding them to an existing cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following options:

      - mycnfPath: Optional path to the MySQL configuration file for the
        instance. Alias for verifyMyCnf
      - verifyMyCnf: Optional path to the MySQL configuration file for the
        instance. If this option is given, the configuration file will be
        verified for the expected option values, in addition to the global
        MySQL system variables.
      - password: The password to get connected to the instance.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      The returned descriptive text of the operation result indicates whether
      the instance is valid for InnoDB Cluster usage or not. If not, a table
      containing the following information is presented:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.
      - Note: the action to be taken.

      The note can be one of the following:

      - Update the config file and update or restart the server variable.
      - Update the config file and restart the server.
      - Update the config file.
      - Update the server variable.
      - Restart the server.

#@<OUT> dba.configure_instance
NAME
      configure_instance - Validates and configures an instance for MySQL
                           InnoDB Cluster usage.

SYNTAX
      dba.configure_instance([instance][, options])

WHERE
      instance: An instance definition.
      options: Additional options for the operation.

RETURNS
      A descriptive text of the operation result.

DESCRIPTION
      This function auto-configures the instance for InnoDB Cluster usage. If
      the target instance already belongs to an InnoDB Cluster it errors out.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following options:

      - mycnfPath: The path to the MySQL configuration file of the instance.
      - outputMycnfPath: Alternative output path to write the MySQL
        configuration file of the instance.
      - password: The password to be used on the connection.
      - clusterAdmin: The name of the "cluster administrator" account.
      - clusterAdminPassword: The password for the "cluster administrator"
        account.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled.
      - restart: boolean value used to indicate that a remote restart of the
        target instance should be performed to finalize the operation.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - applierWorkerThreads: Number of threads used for applying replicated
        transactions. The default value is 4.

      If the outputMycnfPath option is used, only that file is updated and
      mycnfPath is treated as read-only.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      The clusterAdmin must be a standard MySQL account name. It could be
      either an existing account or an account to be created.

      The clusterAdminPassword must be specified only if the clusterAdmin
      account will be created.

      This function reviews the instance configuration to identify if it is
      valid for usage in group replication and cluster. An exception is thrown
      if not.

      If the instance was not valid for InnoDB Cluster and interaction is
      enabled, before configuring the instance a prompt to confirm the changes
      is presented and a table with the following information:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.

#@<OUT> dba.configure_local_instance
NAME
      configure_local_instance - Validates and configures a local instance for
                                 MySQL InnoDB Cluster usage.

SYNTAX
      dba.configure_local_instance(instance[, options])

WHERE
      instance: An instance definition.
      options: Additional options for the operation.

RETURNS
      Nothing

DESCRIPTION
      This function reviews the instance configuration to identify if it is
      valid for usage in an InnoDB cluster, making configuration changes if
      necessary.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following options:

      - mycnfPath: The path to the MySQL configuration file of the instance.
      - outputMycnfPath: Alternative output path to write the MySQL
        configuration file of the instance.
      - password: The password to be used on the connection.
      - clusterAdmin: The name of the "cluster administrator" account.
      - clusterAdminPassword: The password for the "cluster administrator"
        account.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled.
      - restart: boolean value used to indicate that a remote restart of the
        target instance should be performed to finalize the operation.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      If the outputMycnfPath option is used, only that file is updated and
      mycnfPath is treated as read-only.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      The clusterAdmin must be a standard MySQL account name. It could be
      either an existing account or an account to be created.

      The clusterAdminPassword must be specified only if the clusterAdmin
      account will be created.

      The returned descriptive text of the operation result indicates whether
      the instance was successfully configured for InnoDB Cluster usage or if
      it was already valid for InnoDB Cluster usage.

      If the instance was not valid for InnoDB Cluster and interaction is
      enabled, before configuring the instance a prompt to confirm the changes
      is presented and a table with the following information:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.

#@<OUT> dba.configure_replica_set_instance
NAME
      configure_replica_set_instance - Validates and configures an instance for
                                       use in an InnoDB ReplicaSet.

SYNTAX
      dba.configure_replica_set_instance([instance][, options])

WHERE
      instance: An instance definition. By default, the active shell session is
                used.
      options: Additional options for the operation.

RETURNS
      Nothing

DESCRIPTION
      This function will verify and automatically configure the target instance
      for use in an InnoDB ReplicaSet.

      The function can optionally create a "cluster administrator" account, if
      the "clusterAdmin" and "clusterAdminPassword" options are given. The
      account is created with the minimal set of privileges required to manage
      InnoDB clusters or ReplicaSets. The "cluster administrator" account must
      have matching username and password across all instances of the same
      cluster or replicaset.

      Options

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following options:

      - password: The password to be used on the connection.
      - clusterAdmin: The name of a "cluster administrator" user to be created.
        The supported format is the standard MySQL account name format.
      - clusterAdminPassword: The password for the "cluster administrator"
        account.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - restart: boolean value used to indicate that a remote restart of the
        target instance should be performed to finalize the operation.
      - applierWorkerThreads: Number of threads used for applying replicated
        transactions. The default value is 4.

      The connection password may be contained on the instance definition,
      however, it can be overwritten if it is specified on the options.

      This function reviews the instance configuration to identify if it is
      valid for usage in replicasets. An exception is thrown if not.

      If the instance was not valid for InnoDB ReplicaSet and interaction is
      enabled, before configuring the instance a prompt to confirm the changes
      is presented and a table with the following information:

      - Variable: the invalid configuration variable.
      - Current Value: the current value for the invalid configuration
        variable.
      - Required Value: the required value for the configuration variable.

#@<OUT> dba.create_cluster
NAME
      create_cluster - Creates a MySQL InnoDB cluster.

SYNTAX
      dba.create_cluster(name[, options])

WHERE
      name: An identifier for the cluster to be created.
      options: Dictionary with additional parameters described below.

RETURNS
      The created cluster object.

DESCRIPTION
      Creates a MySQL InnoDB cluster taking as seed instance the server the
      shell is currently connected to.

      The options dictionary can contain the following values:

      - disableClone: boolean value used to disable the clone usage on the
        cluster.
      - gtidSetIsComplete: boolean value which indicates whether the GTID set
        of the seed instance corresponds to all transactions executed. Default
        is false.
      - multiPrimary: boolean value used to define an InnoDB cluster with
        multiple writable instances.
      - force: boolean, confirms that the multiPrimary option must be applied
        and/or the operation must proceed even if unmanaged replication
        channels were detected.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - adoptFromGR: boolean value used to create the InnoDB cluster based on
        existing replication group.
      - memberSslMode: SSL mode used to configure the members of the cluster.
      - ipWhitelist: The list of hosts allowed to connect to the instance for
        group replication. Deprecated.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication.
      - groupName: string value with the Group Replication group name UUID to
        be used instead of the automatically generated one.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
      - groupSeeds: string value with a comma-separated list of the Group
        Replication peer addresses to be used instead of the automatically
        generated one.
      - manualStartOnBoot: boolean (default false). If false, Group Replication
        in cluster instances will automatically start and rejoin when MySQL
        starts, otherwise it must be started manually.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - consistency: string value indicating the consistency guarantees that
        the cluster provides.
      - failoverConsistency: string value indicating the consistency guarantees
        that the cluster provides.
      - expelTimeout: integer value to define the time period in seconds that
        cluster members should wait for a non-responding member before evicting
        it from the cluster.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled. Deprecated.
      - multiMaster: boolean value used to define an InnoDB cluster with
        multiple writable instances. Deprecated.

      An InnoDB cluster may be setup in two ways:

      - Single Primary: One member of the cluster allows write operations while
        the rest are read-only secondaries.
      - Multi Primary: All the members in the cluster allow both read and write
        operations.

      Note that Multi-Primary mode has limitations about what can be safely
      executed. Make sure to read the MySQL documentation for Group Replication
      and be aware of what is and is not safely executable in such setups.

      By default this function creates a Single Primary cluster. Use the
      multiPrimary option set to true if a Multi Primary cluster is required.

      Options

      interactive controls whether prompts are shown for MySQL passwords,
      confirmations and handling of cases where user feedback may be required.
      Defaults to true, unless the Shell is started with the --no-wizards
      option.

      disableClone should be set to true if built-in clone support should be
      completely disabled, even in instances where that is supported. Built-in
      clone support is available starting with MySQL 8.0.17 and allows
      automatically provisioning new cluster members by copying state from an
      existing cluster member. Note that clone will completely delete all data
      in the instance being added to the cluster.

      gtidSetIsComplete is used to indicate that GTIDs have been always enabled
      at the cluster seed instance and that GTID_EXECUTED contains all
      transactions ever executed. It must be left as false if data was inserted
      or modified while GTIDs were disabled or if RESET MASTER was executed.
      This flag affects how cluster.add_instance() decides which recovery
      methods are safe to use. Distributed recovery based on replaying the
      transaction history is only assumed to be safe if the transaction history
      is known to be complete, otherwise cluster members could end up with
      incomplete data sets.

      adoptFromGR allows creating an InnoDB cluster from an existing unmanaged
      Group Replication setup, enabling use of MySQL Router and the shell
      AdminAPI for managing it.

      The memberSslMode option supports the following values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instances
        to communicate with other members of the cluster
      - VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
        certificate against the configured Certificate Authority (CA)
        certificates.
      - VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the
        server certificate matches the host to which the connection is
        attempted.
      - DISABLED: if used, SSL (encryption) will be disabled
      - AUTO: if used, SSL (encryption) will be enabled if supported by the
        instance, otherwise disabled

      If memberSslMode is not specified AUTO will be used by default.

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

      The groupName, localAddress, and groupSeeds are advanced options and
      their usage is discouraged since incorrect values can lead to Group
      Replication errors.

      The value for groupName is used to set the Group Replication system
      variable 'group_replication_group_name'.

      The value for localAddress is used to set the Group Replication system
      variable 'group_replication_local_address'. The localAddress option
      accepts values in the format: 'host:port' or 'host:' or ':port'. If the
      specified value does not include a colon (:) and it is numeric, then it
      is assumed to be the port, otherwise it is considered to be the host.
      When the host is not specified, the default value is the value of the
      system variable 'report_host' if defined (i.e., not 'NULL'), otherwise it
      is the hostname value. When the port is not specified, the default value
      is the port of the current active connection (session) * 10 + 1. In case
      the automatically determined default port value is invalid (> 65535) then
      an error is thrown.

      The value for groupSeeds is used to set the Group Replication system
      variable 'group_replication_group_seeds'. The groupSeeds option accepts a
      comma-separated list of addresses in the format:
      'host1:port1,...,hostN:portN'.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally or exhausts its auto-rejoin attempts.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally or exhausts its
        auto-rejoin attempts.
      - OFFLINE_MODE: if used, the instance switches itself to offline mode if
        it leaves the cluster unintentionally or exhausts its auto-rejoin
        attempts. Requires MySQL 8.0.18 or newer.

      If exitStateAction is not specified READ_ONLY will be used by default.

      The consistency option supports the following values:

      - BEFORE_ON_PRIMARY_FAILOVER: if used, new queries (read or write) to the
        new primary will be put on hold until after the backlog from the old
        primary is applied.
      - EVENTUAL: if used, read queries to the new primary are allowed even if
        the backlog isn't applied.

      If consistency is not specified, EVENTUAL will be used by default.

      The value for exitStateAction is used to configure how Group Replication
      behaves when a server instance leaves the group unintentionally (for
      example after encountering an applier error) or exhausts its auto-rejoin
      attempts. When set to ABORT_SERVER, the instance shuts itself down. When
      set to READ_ONLY the server switches itself to super-read-only mode. When
      set to OFFLINE_MODE it switches itself to offline mode. In this mode,
      connected client users are disconnected on their next request and
      connections are no longer accepted, with the exception of client users
      that have the CONNECTION_ADMIN or SUPER privilege. The exitStateAction
      option accepts case-insensitive string values, being the accepted values:
      OFFLINE_MODE (or 2), ABORT_SERVER (or 1) and READ_ONLY (or 0).

      The default value is READ_ONLY.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

      The value for consistency is used to set the Group Replication system
      variable 'group_replication_consistency' and configure the transaction
      consistency guarantee which a cluster provides.

      When set to to BEFORE_ON_PRIMARY_FAILOVER, whenever a primary failover
      happens in single-primary mode (default), new queries (read or write) to
      the newly elected primary that is applying backlog from the old primary,
      will be hold before execution until the backlog is applied. When set to
      EVENTUAL, read queries to the new primary are allowed even if the backlog
      isn't applied but writes will fail (if the backlog isn't applied) due to
      super-read-only mode being enabled. The client may return old values.

      When set to BEFORE, each transaction (RW or RO) waits until all preceding
      transactions are complete before starting its execution. This ensures
      that each transaction is executed on the most up-to-date snapshot of the
      data, regardless of which member it is executed on. The latency of the
      transaction is affected but the overhead of synchronization on RW
      transactions is reduced since synchronization is used only on RO
      transactions.

      When set to AFTER, each RW transaction waits until its changes have been
      applied on all of the other members. This ensures that once this
      transaction completes, all following transactions read a database state
      that includes its changes, regardless of which member they are executed
      on. This mode shall only be used on a group that is used for
      predominantly RO operations to  to ensure that subsequent reads fetch the
      latest data which includes the latest writes. The overhead of
      synchronization on every RO transaction is reduced since synchronization
      is used only on RW transactions.

      When set to BEFORE_AND_AFTER, each RW transaction waits for all preceding
      transactions to complete before being applied and until its changes have
      been applied on other members. A RO transaction waits for all preceding
      transactions to complete before execution takes place. This ensures the
      guarantees given by BEFORE and by AFTER. The overhead of synchronization
      is higher.

      The consistency option accepts case-insensitive string values, being the
      accepted values: EVENTUAL (or 0), BEFORE_ON_PRIMARY_FAILOVER (or 1),
      BEFORE (or 2), AFTER (or 3), and BEFORE_AND_AFTER (or 4).

      The default value is EVENTUAL.

      The value for expelTimeout is used to set the Group Replication system
      variable 'group_replication_member_expel_timeout' and configure how long
      Group Replication will wait before expelling from the group any members
      suspected of having failed. On slow networks, or when there are expected
      machine slowdowns, increase the value of this option. The expelTimeout
      option accepts positive integer values in the range [0, 3600].

      The default value is 0.

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values in the range [0, 2016].

      The default value is 0.

      ATTENTION: The clearReadOnly option will be removed in a future release.

      ATTENTION: The multiMaster option will be removed in a future release.
                 Please use the multiPrimary option instead.

      ATTENTION: The failoverConsistency option will be removed in a future
                 release. Please use the consistency option instead.

      ATTENTION: The ipWhitelist option will be removed in a future release.
                 Please use the ipAllowlist option instead.

#@<OUT> dba.create_replica_set
NAME
      create_replica_set - Creates a MySQL InnoDB ReplicaSet.

SYNTAX
      dba.create_replica_set(name[, options])

WHERE
      name: An identifier for the replicaset to be created.
      options: Dictionary with additional parameters described below.

RETURNS
      The created replicaset object.

DESCRIPTION
      This function will create a managed replicaset using MySQL master/slave
      replication, as opposed to Group Replication. The MySQL instance the
      shell is connected to will be the initial PRIMARY of the replica set.

      The function will perform several checks to ensure the instance state and
      configuration are compatible with a managed replicaset and if so, a
      metadata schema will be initialized there.

      New replica instances can be added through the add_instance() function of
      the returned replicaset object. Status of the instances and their
      replication channels can be inspected with status().

      InnoDB ReplicaSets

      A replicaset allows managing a GTID-based MySQL replication setup, in a
      single PRIMARY/multiple SECONDARY topology.

      A replicaset has several limitations compared to a InnoDB cluster and
      thus, it is recommended that InnoDB clusters be preferred unless not
      possible. Generally, a ReplicaSet on its own does not provide High
      Availability. Among its limitations are:

      - No automatic failover
      - No protection against inconsistencies or partial data loss in a crash

      Pre-Requisites

      The following is a non-exhaustive list of requirements for managed
      replicasets. The dba.configure_instance() command can be used to make
      necessary configuration changes automatically.

      - MySQL 8.0 or newer required
      - Statement Based Replication (SBR) is unsupported, only Row Based
        Replication
      - GTIDs required
      - Replication filters are not allowed
      - All instances in the replicaset must be managed
      - Unmanaged replication channels are not allowed in any instance

      Adopting an Existing Topology

      Existing asynchronous setups can be managed by calling this function with
      the adoptFromAR option. The topology will be automatically scanned and
      validated, starting from the instance the shell is connected to, and all
      instances that are part of the topology will be automatically added to
      the replicaset.

      The only changes made by this function to an adopted replicaset are the
      creation of the metadata schema. Existing replication channels will not
      be changed during adoption, although they will be changed during PRIMARY
      switch operations.

      However, it is only possible to manage setups that use supported
      configurations and topology. Configuration of all instances will be
      checked during adoption, to ensure they are compatible. All replication
      channels must be active and their transaction sets as verified through
      GTID sets must be consistent. The data set of all instances are expected
      to be identical, but is not verified.

      Options

      The options dictionary can contain the following values:

      - adoptFromAR: boolean value used to create the replicaset based on an
        existing asynchronous replication setup.
      - instanceLabel: string a name to identify the target instance. Defaults
        to hostname:port
      - dryRun: boolean if true, all validations and steps for creating a
        replica set are executed, but no changes are actually made. An
        exception will be thrown when finished.
      - gtidSetIsComplete: boolean value which indicates whether the GTID set
        of the seed instance corresponds to all transactions executed. Default
        is false.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

#@<OUT> dba.delete_sandbox_instance
NAME
      delete_sandbox_instance - Deletes an existing MySQL Server instance on
                                localhost.

SYNTAX
      dba.delete_sandbox_instance(port[, options])

WHERE
      port: The port of the instance to be deleted.
      options: Dictionary with options that modify the way this function is
               executed.

RETURNS
      Nothing.

DESCRIPTION
      This function will delete an existing MySQL Server instance on the local
      host. The following options affect the result:

      - sandboxDir: path where the instance is located.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

#@<OUT> dba.deploy_sandbox_instance
NAME
      deploy_sandbox_instance - Creates a new MySQL Server instance on
                                localhost.

SYNTAX
      dba.deploy_sandbox_instance(port[, options])

WHERE
      port: The port where the new instance will listen for connections.
      options: Dictionary with options affecting the new deployed instance.

RETURNS
      Nothing.

DESCRIPTION
      This function will deploy a new MySQL Server instance, the result may be
      affected by the provided options:

      - portx: port where the new instance will listen for X Protocol
        connections.
      - sandboxDir: path where the new instance will be deployed.
      - password: password for the MySQL root user on the new instance.
      - allowRootFrom: create remote root account, restricted to the given
        address pattern (default: %).
      - ignoreSslError: Ignore errors when adding SSL support for the new
        instance, by default: true.
      - mysqldOptions: List of MySQL configuration options to write to the
        my.cnf file, as option=value strings.

      If the portx option is not specified, it will be automatically calculated
      as 10 times the value of the provided MySQL port.

      The password option specifies the MySQL root password on the new
      instance.

      The sandboxDir must be an existing folder where the new instance will be
      deployed. If not specified the new instance will be deployed at:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      SSL support is added by default if not already available for the new
      instance, but if it fails to be added then the error is ignored. Set the
      ignoreSslError option to false to ensure the new instance is deployed
      with SSL support.

#@<OUT> dba.drop_metadata_schema
NAME
      drop_metadata_schema - Drops the Metadata Schema.

SYNTAX
      dba.drop_metadata_schema(options)

WHERE
      options: Dictionary containing an option to confirm the drop operation.

RETURNS
      Nothing.

DESCRIPTION
      The options dictionary may contain the following options:

      - force: boolean, confirms that the drop operation must be executed.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled

#@<OUT> dba.get_cluster
NAME
      get_cluster - Retrieves a cluster from the Metadata Store.

SYNTAX
      dba.get_cluster([name][, options])

WHERE
      name: Parameter to specify the name of the cluster to be returned.
      options: Dictionary with additional options.

RETURNS
      The cluster object identified by the given name or the default cluster.

DESCRIPTION
      If name is not specified or is null, the default cluster will be
      returned.

      If name is specified, and no cluster with the indicated name is found, an
      error will be raised.

      The options dictionary accepts the connectToPrimary option, which
      defaults to true and indicates the shell to automatically connect to the
      primary member of the cluster.

#@<OUT> dba.get_replica_set
NAME
      get_replica_set - Returns an object representing a ReplicaSet.

SYNTAX
      dba.get_replica_set()

DESCRIPTION
      The returned object is identical to the one returned by
      create_replica_set() and can be used to manage the replicaset.

      The function will work regardless of whether the target instance is a
      PRIMARY or a SECONDARY, but its copy of the metadata is expected to be
      up-to-date. This function will also work if the PRIMARY is unreachable or
      unavailable, although replicaset change operations will not be possible,
      except for force_primary_instance().

#@<OUT> dba.get_cluster_set
NAME
      get_cluster_set - Returns an object representing a ClusterSet.

SYNTAX
      dba.get_cluster_set()

RETURNS
      The ClusterSet object to which the current session belongs to.

DESCRIPTION
      The returned object is identical to the one returned by
      create_cluster_set() and can be used to manage the ClusterSet.

      The function will work regardless of whether the active session is
      established to an instance that belongs to a PRIMARY or a REPLICA
      Cluster, but its copy of the metadata is expected to be up-to-date.

      This function will also work if the PRIMARY Cluster is unreachable or
      unavailable, although ClusterSet change operations will not be possible,
      except for failover with force_primary_cluster().


#@<OUT> dba.help
NAME
      help - Provides help about this object and it's members

SYNTAX
      dba.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> dba.kill_sandbox_instance
NAME
      kill_sandbox_instance - Kills a running MySQL Server instance on
                              localhost.

SYNTAX
      dba.kill_sandbox_instance(port[, options])

WHERE
      port: The port of the instance to be killed.
      options: Dictionary with options affecting the result.

RETURNS
      Nothing.

DESCRIPTION
      This function will kill the process of a running MySQL Server instance on
      the local host. The following options affect the result:

      - sandboxDir: path where the instance is located.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

#@<OUT> dba.reboot_cluster_from_complete_outage
NAME
      reboot_cluster_from_complete_outage - Brings a cluster back ONLINE when
                                            all members are OFFLINE.

SYNTAX
      dba.reboot_cluster_from_complete_outage([clusterName][, options])

WHERE
      clusterName: The name of the cluster to be rebooted.
      options: Dictionary with options that modify the behavior of this
               function.

RETURNS
      The rebooted cluster object.

DESCRIPTION
      The options dictionary can contain the next values:

      - user: The user used for the instances sessions required operations.
      - password: The password used for the instances sessions required
        operations.
      - removeInstances: The list of instances to be removed from the cluster.
      - rejoinInstances: The list of instances to be rejoined to the cluster.
      - clearReadOnly: boolean value used to confirm that super_read_only must
        be disabled

      ATTENTION: The clearReadOnly option will be removed in a future release.

      This function reboots a cluster from complete outage. It picks the
      instance the MySQL Shell is connected to as new seed instance and
      recovers the cluster. Optionally it also updates the cluster
      configuration based on user provided options.

      NOTE: When used with a metadata version lower than the one supported by
            this version of the Shell, the removeInstances option is NOT
            allowed, as in such scenario, no changes to the metadata are
            allowed.

      On success, the restored cluster object is returned by the function.

      The current session must be connected to a former instance of the
      cluster.

      If name is not specified, the default cluster will be returned.

#@<OUT> dba.start_sandbox_instance
NAME
      start_sandbox_instance - Starts an existing MySQL Server instance on
                               localhost.

SYNTAX
      dba.start_sandbox_instance(port[, options])

WHERE
      port: The port where the instance listens for MySQL connections.
      options: Dictionary with options affecting the result.

RETURNS
      Nothing.

DESCRIPTION
      This function will start an existing MySQL Server instance on the local
      host. The following options affect the result:

      - sandboxDir: path where the instance is located.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

#@<OUT> dba.stop_sandbox_instance
NAME
      stop_sandbox_instance - Stops a running MySQL Server instance on
                              localhost.

SYNTAX
      dba.stop_sandbox_instance(port[, options])

WHERE
      port: The port of the instance to be stopped.
      options: Dictionary with options affecting the result.

RETURNS
      Nothing.

DESCRIPTION
      This function will gracefully stop a running MySQL Server instance on the
      local host. The following options affect the result:

      - sandboxDir: path where the instance is located.
      - password: password for the MySQL root user on the instance.

      The sandboxDir must be the one where the MySQL instance was deployed. If
      not specified it will use:

        ~/mysql-sandboxes on Unix-like systems or
      %userprofile%\MySQL\mysql-sandboxes on Windows systems.

      If the instance is not located on the used path an error will occur.

#@<OUT> dba.verbose
NAME
      verbose - Controls debug message verbosity for sandbox related dba
                operations.

SYNTAX
      dba.verbose

DESCRIPTION
      The assigned value can be either boolean or integer, the result depends
      on the assigned value:

      - 0: disables mysqlprovision verbosity
      - 1: enables mysqlprovision verbosity
      - >1: enables mysqlprovision debug verbosity
      - Boolean: equivalent to assign either 0 or 1

#@<OUT> Upgrade Metadata
NAME
      upgrade_metadata - Upgrades (or restores) the metadata to the version
                         supported by the Shell.

SYNTAX
      dba.upgrade_metadata([options])

WHERE
      options: Dictionary with option for the operation.

DESCRIPTION
      This function will compare the version of the installed metadata schema
      with the version of the metadata schema supported by this Shell. If the
      installed metadata version is lower, an upgrade process will be started.

      The options dictionary accepts the following attributes:

      - dryRun: boolean value used to enable a dry run of the upgrade process.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      If dryRun is used, the function will determine whether a metadata upgrade
      or restore is required and inform the user without actually executing the
      operation.

      The interactive option can be used to explicitly enable or disable the
      interactive prompts that help the user through te upgrade process. The
      default value is equal to MySQL Shell wizard mode.

      The Upgrade Process

      When upgrading the metadata schema of clusters deployed by Shell versions
      before 8.0.19, a rolling upgrade of existing MySQL Router instances is
      required. This process allows minimizing disruption to applications
      during the upgrade.

      The rolling upgrade process must be performed in the following order:

      1. Execute dba.upgrade_metadata() using the latest Shell version. The
         upgrade function will stop if outdated Router instances are detected,
         at which point you can stop the upgrade to resume later.
      2. Upgrade MySQL Router to the latest version (same version number as the
         Shell)
      3. Continue or re-execute dba.upgrade_metadata()

      Failed Upgrades

      If the installed metadata is not available because a previous call to
      this function ended unexpectedly, this function will restore the metadata
      to the state it was before the failed upgrade operation.

