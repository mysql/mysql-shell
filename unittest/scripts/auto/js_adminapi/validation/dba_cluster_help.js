//@<OUT> Object Help
NAME
      Cluster - Represents an InnoDB Cluster.

DESCRIPTION
      The cluster object is the entry point to manage and monitor a MySQL
      InnoDB Cluster.

      A cluster is a set of MySQLd Instances which holds the user's data.

      It provides high-availability and scalability for the user's data.

PROPERTIES
      name
            Retrieves the name of the cluster.

FUNCTIONS
      addInstance(instance[, options])
            Adds an Instance to the cluster.

      addReplicaInstance(instance[, options])
            Adds a Read Replica Instance to the Cluster.

      createClusterSet(domainName[, options])
            Creates a MySQL InnoDB ClusterSet from an existing standalone
            InnoDB Cluster.

      createRoutingGuideline(name[, json][, options])
            Creates a new Routing Guideline for the Cluster.

      describe()
            Describe the structure of the Cluster.

      disconnect()
            Disconnects all internal sessions used by the cluster object.

      dissolve([options])
            Dissolves the cluster.

      execute(cmd, instances, options)
            Executes a SQL statement at selected instances of the Cluster.

      fenceAllTraffic()
            Fences a Cluster from All Traffic.

      fenceWrites()
            Fences a Cluster from Write Traffic.

      forceQuorumUsingPartitionOf(instance)
            Restores the cluster from quorum loss.

      getClusterSet()
            Returns an object representing a ClusterSet.

      getName()
            Retrieves the name of the cluster.

      getRoutingGuideline([name])
            Returns the named Routing Guideline.

      help([member])
            Provides help about this class and it's members

      importRoutingGuideline(file[, options])
            Imports a Routing Guideline from a JSON file into the Cluster.

      listRouters([options])
            Lists the Router instances.

      options([options])
            Lists the cluster configuration options.

      rejoinInstance(instance[, options])
            Rejoins an Instance to the cluster.

      removeInstance(instance[, options])
            Removes an Instance from the cluster.

      removeRouterMetadata(routerDef)
            Removes metadata for a router instance.

      removeRoutingGuideline(name)
            Removes the named Routing Guideline.

      rescan([options])
            Rescans the Cluster.

      resetRecoveryAccountsPassword(options)
            Resets the password of the recovery and replication accounts of the
            Cluster.

      routerOptions(options)
            Lists the configuration options of the Cluster's Routers.

      routingGuidelines()
            Lists the Routing Guidelines defined for the Cluster.

      routingOptions([router])
            Lists the Cluster Routers configuration options.

            ATTENTION: This function is deprecated and will be removed in a
                       future release of MySQL Shell. Use
                       Cluster.routerOptions() instead.

      setInstanceOption(instance, option, value)
            Changes the value of an option in a Cluster member.

      setOption(option, value)
            Changes the value of an option for the whole Cluster.

      setPrimaryInstance(instance[, options])
            Elects a specific cluster member as the new primary.

      setRoutingOption([router], option, value)
            Changes the value of either a global Cluster Routing option or of a
            single Router instance.

      setupAdminAccount(user, options)
            Create or upgrade an InnoDB Cluster admin account.

      setupRouterAccount(user, options)
            Create or upgrade a MySQL account to use with MySQL Router.

      status([options])
            Describe the status of the Cluster.

      switchToMultiPrimaryMode()
            Switches the cluster to multi-primary mode.

      switchToSinglePrimaryMode([instance])
            Switches the cluster to single-primary mode.

      unfenceWrites()
            Unfences a Cluster.

      For more help on a specific function use: cluster.help('<functionName>')

      e.g. cluster.help('addInstance')

//@<OUT> Name
NAME
      name - Retrieves the name of the cluster.

SYNTAX
      <Cluster>.name

//@<OUT> Add Instance
NAME
      addInstance - Adds an Instance to the cluster.

SYNTAX
      <Cluster>.addInstance(instance[, options])

WHERE
      instance: Connection options for the target instance to be added.
      options: Dictionary with options for the operation.

RETURNS
      nothing

DESCRIPTION
      This function adds an Instance to a InnoDB cluster.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following attributes:

      - label: an identifier for the instance being added
      - recoveryMethod: Preferred method of state recovery. May be auto, clone
        or incremental. Default is auto.
      - recoveryProgress: Integer value to indicate the recovery process
        verbosity level.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication. Only valid if communicationStack=XCOM.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.
      - certSubject: instance's certificate subject to use when
        'memberAuthType' contains "CERT_SUBJECT" or "CERT_SUBJECT_PASSWORD".

      The label must be non-empty and no greater than 256 characters long. It
      must be unique within the Cluster and can only contain alphanumeric, _
      (underscore), . (period), - (hyphen), or : (colon) characters.

      The recoveryMethod option supports the following values:

      - incremental: uses distributed state recovery, which applies missing
        transactions copied from another cluster member. Clone will be
        disabled.
      - clone: clone: uses built-in MySQL clone support, which completely
        replaces the state of the target instance with a full snapshot of
        another cluster member before distributed recovery starts. Requires
        MySQL 8.0.17 or newer.
      - auto: let Group Replication choose whether or not a full snapshot has
        to be taken, based on what the target server supports and the
        group_replication_clone_threshold sysvar. This is the default value. A
        prompt will be shown if not possible to safely determine a safe way
        forward. If interaction is disabled, the operation will be canceled
        instead.

      The recoveryProgress option supports the following values:

      - 0: do not show any progress information.
      - 1: show detailed static progress information.
      - 2: show detailed dynamic progress information using progress bars.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally or exhausts its auto-rejoin attempts.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally or exhausts its
        auto-rejoin attempts.
      - OFFLINE_MODE: if used, the instance switches itself to offline mode if
        it leaves the cluster unintentionally or exhausts its auto-rejoin
        attempts. Requires MySQL 8.0.18 or newer.

      If exitStateAction is not specified, it defaults to OFFLINE_MODE for
      server versions 8.4.0 or newer, and READ_ONLY otherwise.

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

      This option is only used and allowed when communicationStack is set to
      XCOM.

      The localAddress and groupSeeds are advanced options and their usage is
      discouraged since incorrect values can lead to Group Replication errors.

      The value for localAddress is used to set the Group Replication system
      variable 'group_replication_local_address'. The localAddress option
      accepts values in the format: 'host:port' or 'host:' or ':port'. If the
      specified value does not include a colon (:) and it is numeric, then it
      is assumed to be the port, otherwise it is considered to be the host.
      When the host is not specified, the default value is the value of the
      system variable 'report_host' if defined (i.e., not 'NULL'), otherwise it
      is the hostname value. When the port is not specified, the default value
      is the port of the target instance if the communication stack in use by
      the Cluster is 'MYSQL', otherwise, port * 10 + 1  when the communication
      stack is 'XCOM'. In case the automatically determined  default port value
      is invalid (> 65535) then an error is thrown.

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

      The default value is OFFLINE_MODE for server versions 8.4.0 or newer, and
      READ_ONLY otherwise.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values and, since 8.0.21, defaults to 3.

//@<OUT> Describe
NAME
      describe - Describe the structure of the Cluster.

SYNTAX
      <Cluster>.describe()

RETURNS
      A JSON object describing the structure of the Cluster.

DESCRIPTION
      This function describes the structure of the Cluster including all its
      information and members.

      The returned JSON object contains the following attributes:

      - clusterName: the name of the Cluster
      - defaultReplicaSet: the default ReplicaSet object

      The defaultReplicaSet JSON object contains the following attributes:

      - name: the ReplicaSet name (default)
      - topology: a list of dictionaries describing each instance belonging to
        the Cluster.
      - topologyMode: the InnoDB Cluster topology mode.

      Each instance dictionary contains the following attributes:

      - address: the instance address in the form of host:port
      - label: the instance name identifier
      - role: the instance role

//@<OUT> Disconnect
NAME
      disconnect - Disconnects all internal sessions used by the cluster
                   object.

SYNTAX
      <Cluster>.disconnect()

RETURNS
      Nothing.

DESCRIPTION
      Disconnects the internal MySQL sessions used by the cluster to query for
      metadata and replication information.

//@<OUT> Dissolve
NAME
      dissolve - Dissolves the cluster.

SYNTAX
      <Cluster>.dissolve([options])

WHERE
      options: Parameters as described below.

RETURNS
      Nothing.

DESCRIPTION
      This function stops group replication and unregisters all members from
      the cluster metadata.

      It keeps all the user's data intact.

      The options dictionary may contain the following attributes:

      - force: boolean value used to confirm that the dissolve operation must
        be executed, even if some members of the cluster cannot be reached or
        the timeout was reached when waiting for members to catch up with
        replication changes. By default, set to false.

      The force option (set to true) must only be used to dissolve a cluster
      with instances that are permanently not available (no longer reachable)
      or never to be reused again in a cluster. This allows to dissolve a
      cluster and remove it from the metadata, including instances than can no
      longer be recovered. Otherwise, the instances must be brought back ONLINE
      and the cluster dissolved without the force option to avoid errors trying
      to reuse the instances and add them back to a cluster.

//@<OUT> Execute
NAME
      execute - Executes a SQL statement at selected instances of the Cluster.

SYNTAX
      <Cluster>.execute(cmd, instances, options)

WHERE
      cmd: The SQL statement to execute.
      instances: The instances where cmd should be executed.
      options: Dictionary with options for the operation.

RETURNS
      A JSON object with a list of results / information regarding the
      executing of the SQL statement on each of the target instances.

DESCRIPTION
      This function allows a single MySQL SQL statement to be executed on
      multiple instances of the Cluster.

      The 'instances' parameter can be either a string (keyword) or a list of
      instance addresses where cmd should be executed. If a string, the allowed
      keywords are:

      - "all" / "a": all reachable instances.
      - "primary" / "p": the primary instance on a single-primary Cluster or
        all primaries on a multi-primary Cluster.
      - "secondaries" / "s": the secondary instances.
      - "read-replicas" / "rr": the read-replicas instances.

      The options dictionary may contain the following attributes:

      - exclude: similar to the instances parameter, it can be either a string
        (keyword) or a list of instance addresses to exclude from the instances
        specified in instances. It accepts the same keywords, except "all".
      - timeout: integer value with the maximum number of seconds to wait for
        cmd to execute in each target instance. Default value is 0 meaning it
        doesn't timeout.
      - dryRun: boolean if true, all validations and steps for executing cmd
        are performed, but no cmd is actually executed on any instance.

      The keyword "secondaries" / "s" is not permitted on a multi-primary
      Cluster.

      To calculate the final list of instances where cmd should be executed,
      the function starts by parsing the instances parameter and then subtract
      from that list the ones specified in the exclude option. For example, if
      instances is "all" and exclude is "read-replicas", then all (primary and
      secondaries) instances are targeted, except the read-replicas.

//@<OUT> Force Quorum Using Partition Of
NAME
      forceQuorumUsingPartitionOf - Restores the cluster from quorum loss.

SYNTAX
      <Cluster>.forceQuorumUsingPartitionOf(instance)

WHERE
      instance: An instance definition to derive the forced group from.

RETURNS
      Nothing.

DESCRIPTION
      This function restores the cluster back into operational status from a
      loss of quorum scenario. Such a scenario can occur if a group is
      partitioned or more crashes than tolerable occur.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Note that this operation is DANGEROUS as it can create a split-brain if
      incorrectly used and should be considered a last resort. Make absolutely
      sure that there are no partitions of this group that are still operating
      somewhere in the network, but not accessible from your location.

      When this function is used, all the members that are ONLINE from the
      point of view of the given instance definition will be added to the
      group.

//@<OUT> Get Name
NAME
      getName - Retrieves the name of the cluster.

SYNTAX
      <Cluster>.getName()

RETURNS
      The name of the cluster.

//@<OUT> Help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Cluster>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> listRouters
NAME
      listRouters - Lists the Router instances.

SYNTAX
      <Cluster>.listRouters([options])

WHERE
      options: Dictionary with options for the operation.

RETURNS
      A JSON object listing the Router instances associated to the Cluster.

DESCRIPTION
      This function lists and provides information about all Router instances
      registered for the Cluster.

      For each router, the following information is provided, when available:

      - hostname: Hostname.
      - lastCheckIn: Timestamp of the last statistics update (check-in).
      - roPort: Read-only port (Classic protocol).
      - roXPort: Read-only port (X protocol).
      - rwPort: Read-write port (Classic protocol).
      - rwSplitPort: Read-write split port (Classic protocol).
      - rwXPort: Read-write port (X protocol).
      - upgradeRequired: If true, it indicates Router is incompatible with the
        Cluster's metadata version and must be upgraded.
      - version: Version.

      Whenever a Metadata Schema upgrade is necessary, the recommended process
      is to upgrade MySQL Router instances to the latest version before
      upgrading the Metadata itself, in order to minimize service disruption.

      The options dictionary may contain the following attributes:

      - onlyUpgradeRequired: boolean, enables filtering so only router
        instances that support older version of the Metadata Schema and require
        upgrade are included.

//@<OUT> removeRouterMetadata
NAME
      removeRouterMetadata - Removes metadata for a router instance.

SYNTAX
      <Cluster>.removeRouterMetadata(routerDef)

WHERE
      routerDef: identifier of the router instance to be removed (e.g.
                 192.168.45.70::system)

RETURNS
      Nothing

DESCRIPTION
      MySQL Router automatically registers itself within the InnoDB cluster
      metadata when bootstrapped. However, that metadata may be left behind
      when instances are uninstalled or moved over to a different host. This
      function may be used to clean up such instances that no longer exist.

      The Cluster.listRouters() function may be used to list registered router
      instances, including their identifier.

//@<OUT> Options
NAME
      options - Lists the cluster configuration options.

SYNTAX
      <Cluster>.options([options])

WHERE
      options: Dictionary with options.

RETURNS
      A JSON object describing the configuration options of the cluster.

DESCRIPTION
      This function lists the cluster configuration options for its ReplicaSets
      and Instances. The following options may be given to control the amount
      of information gathered and returned.

      - all: if true, includes information about all group_replication system
        variables.

//@<OUT> Rejoin Instance
NAME
      rejoinInstance - Rejoins an Instance to the cluster.

SYNTAX
      <Cluster>.rejoinInstance(instance[, options])

WHERE
      instance: An instance definition.
      options: Dictionary with options for the operation.

RETURNS
      A JSON object with the result of the operation.

DESCRIPTION
      This function rejoins an Instance to the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following attributes:

      - recoveryMethod: Preferred method of state recovery. May be auto, clone
        or incremental. Default is auto.
      - recoveryProgress: Integer value to indicate the recovery process
        verbosity level. recovery process to finish and its verbosity level.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication. Only valid if communicationStack=XCOM.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
      - dryRun: boolean if true, all validations and steps for rejoining the
        instance are executed, but no changes are actually made.
      - cloneDonor: The Cluster member to be used as donor when performing
        clone-based recovery. Available only for Read Replicas.
      - timeout: maximum number of seconds to wait for the instance to sync up
        with the PRIMARY after it's provisioned and the replication channel is
        established. If reached, the operation is rolled-back. Default is 0 (no
        timeout). Available only for Read Replicas.

      The recoveryMethod option supports the following values:

      - incremental: uses distributed state recovery, which applies missing
        transactions copied from another cluster member. Clone will be
        disabled.
      - clone: uses built-in MySQL clone support, which completely replaces the
        state of the target instance with a full snapshot of another cluster
        member before distributed recovery starts. Requires MySQL 8.0.17 or
        newer.
      - auto: let Group Replication choose whether or not a full snapshot has
        to be taken, based on what the target server supports and the
        group_replication_clone_threshold sysvar. This is the default value. A
        prompt will be shown if not possible to safely determine a safe way
        forward. If interaction is disabled, the operation will be canceled
        instead.

      If recoveryMethod is not specified 'auto' will be used by default.

      The recoveryProgress option supports the following values:

      - 0: do not show any progress information.
      - 1: show detailed static progress information.
      - 2: show detailed dynamic progress information using progress bars.

      By default, if the standard output on which the Shell is running refers
      to a terminal, the recoveryProgress option has the value of 2. Otherwise,
      it has the value of 1.

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

      This option is only used and allowed when communicationStack is set to
      XCOM.

      The localAddress is an advanced option and its usage is discouraged since
      incorrect values can lead to Group Replication errors.

      The value for localAddress is used to set the Group Replication system
      variable 'group_replication_local_address'. The localAddress option
      accepts values in the format: 'host:port' or 'host:' or ':port'. If the
      specified value does not include a colon (:) and it is numeric, then it
      is assumed to be the port, otherwise it is considered to be the host.
      When the host is not specified, the default value is the value of the
      system variable 'report_host' if defined (i.e., not 'NULL'), otherwise it
      is the hostname value. When the port is not specified, the default value
      is the port of the target instance if the communication stack in use by
      the Cluster is 'MYSQL', otherwise, port * 10 + 1  when the communication
      stack is 'XCOM'. In case the automatically determined  default port value
      is invalid (> 65535) then an error is thrown.

//@<OUT> Remove Instance
NAME
      removeInstance - Removes an Instance from the cluster.

SYNTAX
      <Cluster>.removeInstance(instance[, options])

WHERE
      instance: An instance definition.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function removes an Instance from the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The options dictionary may contain the following attributes:

      - force: boolean, indicating if the instance must be removed (even if
        only from metadata) in case it cannot be reached. By default, set to
        false.
      - dryRun: boolean if true, all validations and steps for removing the
        instance are executed, but no changes are actually made. An exception
        will be thrown when finished.
      - timeout: maximum number of seconds to wait for the instance to sync up
        with the PRIMARY. If reached, the operation is rolled-back. Default is
        0 (no timeout).

      The force option (set to true) must only be used to remove instances that
      are permanently not available (no longer reachable) or never to be reused
      again in a cluster. This allows to remove from the metadata an instance
      than can no longer be recovered. Otherwise, the instance must be brought
      back ONLINE and removed without the force option to avoid errors trying
      to add it back to a cluster.

//@<OUT> SetInstanceOption
NAME
      setInstanceOption - Changes the value of an option in a Cluster member.

SYNTAX
      <Cluster>.setInstanceOption(instance, option, value)

WHERE
      instance: An instance definition.
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

DESCRIPTION
      This function changes an option for a member of the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The accepted options are:

      - tag:<option>: built-in and user-defined tags to be associated to the
        Cluster.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication. Only valid if communicationStack=XCOM.
      - label: a string identifier of the instance.
      - replicationSources: The list of sources for a Read Replica Instance.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally or exhausts its auto-rejoin attempts.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally or exhausts its
        auto-rejoin attempts.
      - OFFLINE_MODE: if used, the instance switches itself to offline mode if
        it leaves the cluster unintentionally or exhausts its auto-rejoin
        attempts. Requires MySQL 8.0.18 or newer.

      If exitStateAction is not specified, it defaults to OFFLINE_MODE for
      server versions 8.4.0 or newer, and READ_ONLY otherwise.

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

      The default value is OFFLINE_MODE for server versions 8.4.0 or newer, and
      READ_ONLY otherwise.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values and, since 8.0.21, defaults to 3.

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

      This option is only used and allowed when communicationStack is set to
      XCOM.

      Tags

      Tags make it possible to associate custom key/value pairs to a Cluster,
      storing them in its metadata. Custom tag names can be any string starting
      with letters and followed by letters, numbers and _. Tag values may be
      any JSON value. If the value is null, the tag is deleted.

      The following pre-defined tags are available:

      - _hidden: bool - instructs the router to exclude the instance from the
        list of possible destinations for client applications.
      - _disconnect_existing_sessions_when_hidden: bool - instructs the router
        to disconnect existing connections from instances that are marked to be
        hidden.

      NOTE: '_hidden' and '_disconnect_existing_sessions_when_hidden' can be
            useful to shut down the instance and perform maintenance on it
            without disrupting incoming application traffic.

      The label must be non-empty and no greater than 256 characters long. It
      must be unique within the Cluster and can only contain alphanumeric, _
      (underscore), . (period), - (hyphen), or : (colon) characters.

      The replicationSources is a comma separated list of instances (host:port)
      to act as sources of the replication channel, i.e. to provide source
      failover of the channel. The first member of the list is configured with
      the highest priority among all members so when the channel activates it
      will be chosen for the first connection attempt. By default, the source
      list is automatically managed by Group Replication according to the
      current group membership and the primary member of the Cluster is the
      current source for the replication channel, this is the same as setting
      to "primary". Alternatively, it's possible to set to "secondary" to
      instruct Group Replication to automatically manage the list but use a
      secondary member of the Cluster as source.

      For the change to be effective, the Read-Replica must be reconfigured
      after using Cluster.rejoinInstance().

//@<OUT> SetOption
NAME
      setOption - Changes the value of an option for the whole Cluster.

SYNTAX
      <Cluster>.setOption(option, value)

WHERE
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

DESCRIPTION
      This function changes an option for the Cluster.

      The accepted options are:

      - tag:<option>: built-in and user-defined tags to be associated to the
        Cluster.
      - clusterName: string value to define the cluster name.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - consistency: string value indicating the consistency guarantees that
        the cluster provides.
      - expelTimeout: integer value to define the time period in seconds that
        cluster members should wait for a non-responding member before evicting
        it from the cluster.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication. Only valid if communicationStack=XCOM.
      - disableClone: boolean value used to disable the clone usage on the
        cluster.
      - replicationAllowedHost string value to use as the host name part of
        internal replication accounts. Existing accounts will be re-created
        with the new value.
      - transactionSizeLimit: integer value to configure the maximum
        transaction size in bytes which the Cluster accepts
      - clusterSetReplicationConnectRetry: integer that specifies the interval
        in seconds between the reconnection attempts that the replica makes
        after the connection to the source times out.
      - clusterSetReplicationRetryCount: integer that sets the maximum number
        of reconnection attempts that the replica makes after the connection to
        the source times out.
      - clusterSetReplicationHeartbeatPeriod: decimal that controls the
        heartbeat interval, which stops the connection timeout occurring in the
        absence of data if the connection is still good.
      - clusterSetReplicationCompressionAlgorithms: string that specifies the
        permitted compression algorithms for connections to the replication
        source.
      - clusterSetReplicationZstdCompressionLevel: integer that specifies the
        compression level to use for connections to the replication source
        server that use the zstd compression algorithm.
      - clusterSetReplicationBind: string that determines which of the
        replica's network interfaces is chosen for connecting to the source.
      - clusterSetReplicationNetworkNamespace: string that specifies the
        network namespace to use for TCP/IP connections to the replication
        source server or, if the MySQL communication stack is in use, for Group
        Replication’s group communication connections.

      The clusterName must be non-empty and no greater than 63 characters long.
      It can only start with an alphanumeric character or with _ (underscore),
      and can only contain alphanumeric, _ ( underscore), . (period), or -
      (hyphen) characters.

      ATTENTION: The transactionSizeLimit option is not supported on Replica
                 Clusters of InnoDB ClusterSets.

      NOTE: Changing any of the "clusterSetReplication*" options won't
            immediately update the replication channel.
            ClusterSet.rejoinCluster() must be used to to reconfigure and
            restart the replication channel of that Cluster.

      NOTE: Any of the "clusterSetReplication*" options accepts 'null', which
            resets the corresponding option to its default value on the next
            call to ClusterSet.rejoinCluster().

      The value for the configuration option is used to set the Group
      Replication system variable that corresponds to it.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally or exhausts its auto-rejoin attempts.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally or exhausts its
        auto-rejoin attempts.
      - OFFLINE_MODE: if used, the instance switches itself to offline mode if
        it leaves the cluster unintentionally or exhausts its auto-rejoin
        attempts. Requires MySQL 8.0.18 or newer.

      If exitStateAction is not specified, it defaults to OFFLINE_MODE for
      server versions 8.4.0 or newer, and READ_ONLY otherwise.

      The consistency option supports the following values:

      - BEFORE_ON_PRIMARY_FAILOVER: if used, new queries (read or write) to the
        new primary will be put on hold until after the backlog from the old
        primary is applied.
      - EVENTUAL: if used, read queries to the new primary are allowed even if
        the backlog isn't applied.

      If consistency is not specified, it defaults to
      BEFORE_ON_PRIMARY_FAILOVER for server versions 8.4.0 or newer, and
      EVENTUAL otherwise.

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

      The default value is OFFLINE_MODE for server versions 8.4.0 or newer, and
      READ_ONLY otherwise.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

      The value for consistency is used to set the Group Replication system
      variable 'group_replication_consistency' and configure the transaction
      consistency guarantee which a cluster provides.

      When set to BEFORE_ON_PRIMARY_FAILOVER, whenever a primary failover
      happens in single-primary mode (default), new queries (read or write) to
      the newly elected primary that is applying backlog from the old primary,
      will be hold before execution until the backlog is applied. Special care
      is needed if LOCK / UNLOCK queries are used in a secondary, because the
      UNLOCK statement can be held. This means that, if the secondary is
      promoted, and transactions from the primary are being applied that target
      locked tables, they won't be applied and an explicit UNLOCK will also
      hold because the secondary needs to finish applying the transactions from
      the previous primary. See
      https://dev.mysql.com/doc/refman/en/group-replication-system-variables.html#sysvar_group_replication_consistency
      for more details.

      When set to EVENTUAL, read queries to the new primary are allowed even if
      the backlog isn't applied but writes will fail (if the backlog isn't
      applied) due to super-read-only mode being enabled. The client may return
      old values.

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
      predominantly RO operations to ensure that subsequent reads fetch the
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

      It defaults to BEFORE_ON_PRIMARY_FAILOVER for server versions 8.4.0 or
      newer, and EVENTUAL otherwise.

      The value for expelTimeout is used to set the Group Replication system
      variable 'group_replication_member_expel_timeout' and configure how long
      Group Replication will wait before expelling from the group any members
      suspected of having failed. On slow networks, or when there are expected
      machine slowdowns, increase the value of this option. The expelTimeout
      option accepts positive integer values and, since 8.0.21, defaults to 5
      seconds.

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values and, since 8.0.21, defaults to 3.

      The value for transactionSizeLimit is used to set the Group Replication
      system variable 'group_replication_transaction_size_limit' and configures
      the maximum transaction size in bytes which the Cluster accepts.
      Transactions larger than this size are rolled back by the receiving
      member and are not broadcast to the Cluster.

      The transactionSizeLimit option accepts positive integer values and, if
      set to zero, there is no limit to the size of transactions the Cluster
      accepts

      All members added or rejoined to the Cluster will use the same value.

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

      This option is only used and allowed when communicationStack is set to
      XCOM.

      Tags

      Tags make it possible to associate custom key/value pairs to a Cluster,
      storing them in its metadata. Custom tag names can be any string starting
      with letters and followed by letters, numbers and _. Tag values may be
      any JSON value. If the value is null, the tag is deleted.

//@<OUT> setupAdminAccount
NAME
      setupAdminAccount - Create or upgrade an InnoDB Cluster admin account.

SYNTAX
      <Cluster>.setupAdminAccount(user, options)

WHERE
      user: Name of the InnoDB Cluster administrator account.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function creates/upgrades a MySQL user account with the necessary
      privileges to administer an InnoDB Cluster.

      This function also allows a user to upgrade an existing admin account
      with the necessary privileges before a dba.upgradeMetadata() call.

      The mandatory argument user is the name of the MySQL account we want to
      create or upgrade to be used as Administrator account. The accepted
      format is username[@host] where the host part is optional and if not
      provided defaults to '%'.

      The options dictionary may contain the following attributes:

      - password: The password for the InnoDB Cluster administrator account.
      - passwordExpiration: Password expiration setting for the account. May be
        set to the number of days for expiration, 'NEVER' to disable expiration
        and 'DEFAULT' to use the system default.
      - requireCertIssuer: Optional SSL certificate issuer for the account.
      - requireCertSubject: Optional SSL certificate subject for the account.
      - dryRun: boolean value used to enable a dry run of the account setup
        process. Default value is False.
      - update: boolean value that must be enabled to allow updating the
        privileges and/or password of existing accounts. Default value is
        False.

      If the user account does not exist, either the password,
      requireCertIssuer or requireCertSubject are mandatory.

      If the user account exists, the update option must be enabled.

      If dryRun is used, the function will display information about the
      permissions to be granted to `user` account without actually creating
      and/or performing any changes to it.

      To change authentication options for an existing account, set `update` to
      `true`. It is possible to change password without affecting certificate
      options or vice-versa but certificate options can only be changed
      together.

//@<OUT> setupRouterAccount
NAME
      setupRouterAccount - Create or upgrade a MySQL account to use with MySQL
                           Router.

SYNTAX
      <Cluster>.setupRouterAccount(user, options)

WHERE
      user: Name of the account to create/upgrade for MySQL Router.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function creates/upgrades a MySQL user account with the necessary
      privileges to be used by MySQL Router.

      This function also allows a user to upgrade existing MySQL router
      accounts with the necessary privileges after a dba.upgradeMetadata()
      call.

      The mandatory argument user is the name of the MySQL account we want to
      create or upgrade to be used by MySQL Router. The accepted format is
      username[@host] where the host part is optional and if not provided
      defaults to '%'.

      The options dictionary may contain the following attributes:

      - password: The password for the MySQL Router account.
      - passwordExpiration: Password expiration setting for the account. May be
        set to the number of days for expiration, 'NEVER' to disable expiration
        and 'DEFAULT' to use the system default.
      - requireCertIssuer: Optional SSL certificate issuer for the account.
      - requireCertSubject: Optional SSL certificate subject for the account.
      - dryRun: boolean value used to enable a dry run of the account setup
        process. Default value is False.
      - update: boolean value that must be enabled to allow updating the
        privileges and/or password of existing accounts. Default value is
        False.

      If the user account does not exist, either the password,
      requireCertIssuer or requireCertSubject are mandatory.

      If the user account exists, the update option must be enabled.

      If dryRun is used, the function will display information about the
      permissions to be granted to `user` account without actually creating
      and/or performing any changes to it.

      To change authentication options for an existing account, set `update` to
      `true`. It is possible to change password without affecting certificate
      options or vice-versa but certificate options can only be changed
      together.

//@<OUT> Rescan
NAME
      rescan - Rescans the Cluster.

SYNTAX
      <Cluster>.rescan([options])

WHERE
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This command rescans the Cluster for any changes in the Group Replication
      topology, ensuring proper management of instances, validating metadata
      consistency, and confirming that the correct settings are in place for
      all instances.

      It performs the following tasks:

      - Detects new instances that are part of the Group Replication topology
        but not yet managed by the Cluster.
      - Identifies obsolete instances that are no longer part of the topology.
      - Validates and updates the Metadata, ensuring it accurately reflects the
        current state of the Cluster, including topology mode changes, recovery
        accounts, server IDs, and other relevant configuration data.
      - Verifies if 'group_replication_view_change_uuid' is configured for
        MySQL versions from 8.0.27 up to 8.3.0.
      - Ensures that 'group_replication_transaction_size_limit' is consistent
        across all Cluster members and stored in the Metadata.
      - Automatically upgrades the MySQL communication protocol version, if
        necessary.
      - Detects and corrects instances using the wrong account for the recovery
        channel, resetting and updating credentials to use the proper account.
      - Detects inconsistencies in the metadata, such as invalid
        Cluster/ClusterSet members or other potential issues.

      The options dictionary may contain the following attributes:

      - addInstances: List with the connection data of the new active instances
        to add to the metadata, or "auto" to automatically add missing
        instances to the metadata. Deprecated.
      - removeInstances: List with the connection data of the obsolete
        instances to remove from the metadata, or "auto" to automatically
        remove obsolete instances from the metadata. Deprecated.
      - upgradeCommProtocol: Boolean. Set to true to upgrade the Group
        Replication communication protocol to the highest version possible.
      - updateViewChangeUuid: Boolean. Indicates if the command should generate
        and set a value for the Group Replication View Change UUID in the
        entire Cluster. Required for InnoDB ClusterSet usage (if running MySQL
        version lower than 8.3.0).
      - addUnmanaged: Boolean. Set to true to automatically add newly
        discovered instances, i.e. already part of the replication topology but
        not managed in the Cluster, to the metadata. Defaults to false.
      - removeObsolete: Boolean. Set to true to automatically remove all
        obsolete instances, i.e. no longer part of the replication topology,
        from the metadata. Defaults to false.
      - repairMetadata: Boolean. Set to true to repair the Metadata if detected
        to be inconsistent.

      The value for 'addInstances' and 'removeInstances' is used to specify
      which instances to add or remove from the metadata, respectively. Both
      options accept list connection data. In addition, the "auto" value can be
      used for both options in order to automatically add or remove the
      instances in the metadata, without having to explicitly specify them.

      'repairMetadata' is used to eliminate any inconsistencies detected in the
      Metadata. These inconsistencies may arise from a few scenarios, such as
      the failure of one or more commands. Clusters detected in the ClusterSet
      Metadata that do not qualify as valid members will be removed.

      ATTENTION: The addInstances and removeInstances options will be removed
                 in a future release. Use addUnmanaged and removeObsolete
                 instead.

//@<OUT> Status
NAME
      status - Describe the status of the Cluster.

SYNTAX
      <Cluster>.status([options])

WHERE
      options: Dictionary with options.

RETURNS
      A JSON object describing the status of the Cluster.

DESCRIPTION
      This function describes the status of the Cluster and its members. The
      following options may be given to control the amount of information
      gathered and returned.

      - extended: verbosity level of the command output.

      The extended option supports Integer or Boolean values:

      - 0: disables the command verbosity (default);
      - 1: includes information about the Metadata Version, Group Protocol
        Version, Group name, cluster member UUIDs, cluster member roles and
        states as reported by Group Replication and the list of fenced system
        variables;
      - 2: includes information about transactions processed by connection and
        applier;
      - 3: includes more detailed stats about the replication machinery of each
        cluster member;
      - Boolean: equivalent to assign either 0 (false) or 1 (true).

//@<OUT> setPrimaryInstance
NAME
      setPrimaryInstance - Elects a specific cluster member as the new primary.

SYNTAX
      <Cluster>.setPrimaryInstance(instance[, options])

WHERE
      instance: An instance definition.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function forces the election of a new primary, overriding any
      election process.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The instance definition is mandatory and is the identifier of the cluster
      member that shall become the new primary.

      The options dictionary may contain the following attributes:

      - runningTransactionsTimeout: integer value to define the time period, in
        seconds, that the primary election waits for ongoing transactions to
        complete. After the timeout is reached, any ongoing transaction is
        rolled back allowing the operation to complete.

//@<OUT> switchToMultiPrimaryMode
NAME
      switchToMultiPrimaryMode - Switches the cluster to multi-primary mode.

SYNTAX
      <Cluster>.switchToMultiPrimaryMode()

RETURNS
      Nothing.

DESCRIPTION
      This function changes a cluster running in single-primary mode to
      multi-primary mode.

//@<OUT> switchToSinglePrimaryMode
NAME
      switchToSinglePrimaryMode - Switches the cluster to single-primary mode.

SYNTAX
      <Cluster>.switchToSinglePrimaryMode([instance])

WHERE
      instance: An instance definition.

RETURNS
      Nothing.

DESCRIPTION
      This function changes a cluster running in multi-primary mode to
      single-primary mode.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The instance definition is optional and is the identifier of the cluster
      member that shall become the new primary.

      If the instance definition is not provided, the new primary will be the
      instance with the highest member weight (and the lowest UUID in case of a
      tie on member weight).

//@<OUT> resetRecoveryAccountsPassword
NAME
      resetRecoveryAccountsPassword - Resets the password of the recovery and
                                      replication accounts of the Cluster.

SYNTAX
      <Cluster>.resetRecoveryAccountsPassword(options)

WHERE
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function resets the passwords for all internal recovery user
      accounts used by the Cluster as well as for all replication accounts used
      by any Read Replica instance. It can be used to reset the passwords of
      the recovery or replication user accounts when needed for any security
      reasons. For example: periodically to follow some custom password
      lifetime policy, or after some security breach event.

      The options dictionary may contain the following attributes:

      - force: boolean, indicating if the operation will continue in case an
        error occurs when trying to reset the passwords on any of the
        instances, for example if any of them is not online. By default, set to
        false.

      The use of the force option (set to true) is not recommended. Use it only
      if really needed when instances are permanently not available (no longer
      reachable) or never going to be reused again in a cluster. Prefer to
      bring the non available instances back ONLINE or remove them from the
      cluster if they will no longer be used.

//@<OUT> createClusterSet
NAME
      createClusterSet - Creates a MySQL InnoDB ClusterSet from an existing
                         standalone InnoDB Cluster.

SYNTAX
      <Cluster>.createClusterSet(domainName[, options])

WHERE
      domainName: An identifier for the ClusterSet's logical dataset.
      options: Dictionary with additional parameters described below.

RETURNS
      The created ClusterSet object.

DESCRIPTION
      Creates a ClusterSet object from an existing cluster, with the given data
      domain name.

      Several checks and validations are performed to ensure that the target
      Cluster complies with the requirements for ClusterSets and if so, the
      Metadata schema will be updated to create the new ClusterSet and the
      target Cluster becomes the PRIMARY cluster of the ClusterSet.

      InnoDB ClusterSet

      A ClusterSet is composed of a single PRIMARY InnoDB Cluster that can have
      one or more replica InnoDB Clusters that replicate from the PRIMARY using
      asynchronous replication.

      ClusterSets allow InnoDB Cluster deployments to achieve fault-tolerance
      at a whole Data Center / region or geographic location, by creating
      REPLICA clusters in different locations (Data Centers), ensuring Disaster
      Recovery is possible.

      If the PRIMARY InnoDB Cluster becomes completely unavailable, it's
      possible to promote a REPLICA of that cluster to take over its duties
      with minimal downtime or data loss.

      All Cluster operations are available at each individual member (cluster)
      of the ClusterSet. The AdminAPI ensures all updates are performed at the
      PRIMARY and controls the command availability depending on the individual
      status of each Cluster.

      Please note that InnoDB ClusterSets don't have the same consistency and
      data loss guarantees as InnoDB Clusters. To read more about ClusterSets,
      see \? ClusterSet or refer to the MySQL manual.

      Pre-requisites

      The following is a non-exhaustive list of requirements to create a
      ClusterSet:

      - The target cluster must not already be part of a ClusterSet.
      - MySQL 8.0.27 or newer.
      - The target cluster's Metadata schema version is 2.1.0 or newer.
      - Unmanaged replication channels are not allowed.

      The domainName must be non-empty and no greater than 63 characters long.
      It can only start with an alphanumeric character or with _ (underscore),
      and can only contain alphanumeric, _ ( underscore), . (period), or -
      (hyphen) characters.

      Options

      The options dictionary can contain the following values:

      - dryRun: boolean if true, all validations and steps for creating a
        ClusterSet are executed, but no changes are actually made. An exception
        will be thrown when finished.
      - clusterSetReplicationSslMode: SSL mode for the ClusterSet replication
        channels.
      - replicationAllowedHost: string value to use as the host name part of
        internal replication accounts (i.e. 'mysql_innodb_cs_###'@'hostname').
        Default is %. It must be possible for any member of the ClusterSet to
        connect to any other member using accounts with this hostname value.

      The clusterSetReplicationSslMode option supports the following values:

      - DISABLED: TLS encryption is disabled for the ClusterSet replication
        channels.
      - REQUIRED: TLS encryption is enabled for the ClusterSet replication
        channels.
      - VERIFY_CA: like REQUIRED, but additionally verify the peer server TLS
        certificate against the configured Certificate Authority (CA)
        certificates.
      - VERIFY_IDENTITY: like VERIFY_CA, but additionally verify that the peer
        server certificate matches the host to which the connection is
        attempted.
      - AUTO: TLS encryption will be enabled if supported by the instance,
        otherwise disabled.

      If clusterSetReplicationSslMode is not specified, it defaults to the
      value of the cluster's memberSslMode option.

//@<OUT> getClusterSet
NAME
      getClusterSet - Returns an object representing a ClusterSet.

SYNTAX
      <Cluster>.getClusterSet()

RETURNS
      The ClusterSet object to which the current cluster belongs to.

DESCRIPTION
      The returned object is identical to the one returned by
      createClusterSet() and can be used to manage the ClusterSet.

      The function will work regardless of whether the target cluster is a
      PRIMARY or a REPLICA Cluster, but its copy of the metadata is expected to
      be up-to-date.

      This function will also work if the PRIMARY Cluster is unreachable or
      unavailable, although ClusterSet change operations will not be possible,
      except for failover with forcePrimaryCluster().

//@<OUT> fenceAllTraffic
NAME
      fenceAllTraffic - Fences a Cluster from All Traffic.

SYNTAX
      <Cluster>.fenceAllTraffic()

RETURNS
      Nothing

DESCRIPTION
      This function fences a Cluster from all Traffic by ensuring the Group
      Replication is completely shut down and all members are Read-Only and in
      Offline mode, preventing regular client connections from connecting to
      it.

      Use this function when performing a PRIMARY Cluster failover in a
      ClusterSet to prevent a split-brain.

      ATTENTION: The Cluster will be put OFFLINE but Cluster members will not
                 be shut down. This is the equivalent of a graceful shutdown of
                 Group Replication. To restore the Cluster use
                 dba.rebootClusterFromCompleteOutage().

//@<OUT> fenceWrites
NAME
      fenceWrites - Fences a Cluster from Write Traffic.

SYNTAX
      <Cluster>.fenceWrites()

RETURNS
      Nothing

DESCRIPTION
      This function fences a Cluster member of a ClusterSet from all Write
      Traffic by ensuring all of its members are Read-Only regardless of any
      topology change on it. The Cluster will be put into READ ONLY mode and
      all members will remain available for reads. To unfence the Cluster so it
      restores its normal functioning and can accept all traffic use
      Cluster.unfenceWrites().

      Use this function when performing a PRIMARY Cluster failover in a
      ClusterSet to allow only read traffic in the previous Primary Cluster in
      the event of a split-brain.

      The function is not permitted on standalone Clusters.

//@<OUT> unfenceWrites
NAME
      unfenceWrites - Unfences a Cluster.

SYNTAX
      <Cluster>.unfenceWrites()

RETURNS
      Nothing

DESCRIPTION
      This function unfences a Cluster that was previously fenced to Write
      traffic with Cluster.fenceWrites().

      ATTENTION: This function does not unfence Clusters that have been fenced
                 to ALL traffic. Those Cluster are completely shut down and can
                 only be restored using dba.rebootClusterFromCompleteOutage().

//@<OUT> addReplicaInstance
NAME
      addReplicaInstance - Adds a Read Replica Instance to the Cluster.

SYNTAX
      <Cluster>.addReplicaInstance(instance[, options])

WHERE
      instance: host:port of the target instance to be added as a Read Replica.
      options: Dictionary with additional parameters described below.

RETURNS
      nothing

DESCRIPTION
      This function adds an Instance acting as Read-Replica of an InnoDB
      Cluster.

      Pre-requisites

      The following is a list of requirements to create a REPLICA cluster:

      - The target instance must be a standalone instance.
      - The target instance and Cluster must running MySQL 8.0.23 or newer.
      - Unmanaged replication channels are not allowed.
      - The target instance server_id and server_uuid must be unique in the
        topology, including among OFFLINE or unreachable members

      Options

      The options dictionary may contain the following values:

      - dryRun: boolean if true, all validations and steps for creating a Read
        Replica Instance are executed, but no changes are actually made. An
        exception will be thrown when finished.
      - label: an identifier for the Read Replica Instance being added, used in
        the output of status() and describe().
      - replicationSources: The list of sources for the Read Replica Instance.
        By default, the list is automatically managed by Group Replication and
        the primary member is used as source.
      - recoveryMethod: Preferred method for state recovery/provisioning. May
        be auto, clone or incremental. Default is auto.
      - recoveryProgress: Integer value to indicate the recovery process
        verbosity level.
      - timeout: maximum number of seconds to wait for the instance to sync up
        with the PRIMARY after it's provisioned and the replication channel is
        established. If reached, the operation is rolled-back. Default is 0 (no
        timeout).
      - cloneDonor: The Cluster member to be used as donor when performing
        clone-based recovery.
      - certSubject: instance's certificate subject to use when
        'memberAuthType' contains "CERT_SUBJECT" or "CERT_SUBJECT_PASSWORD".

      The label must be non-empty and no greater than 256 characters long. It
      must be unique within the Cluster and can only contain alphanumeric, _
      (underscore), . (period), - (hyphen), or : (colon) characters.

      The replicationSources is a comma separated list of instances (host:port)
      to act as sources of the replication channel, i.e. to provide source
      failover of the channel. The first member of the list is configured with
      the highest priority among all members so when the channel activates it
      will be chosen for the first connection attempt. By default, the source
      list is automatically managed by Group Replication according to the
      current group membership and the primary member of the Cluster is the
      current source for the replication channel, this is the same as setting
      to "primary". Alternatively, it's possible to set to "secondary" to
      instruct Group Replication to automatically manage the list too but use a
      secondary member of the Cluster as source.

      The recoveryMethod option supports the following values:

      - incremental: waits until the new instance has applied missing
        transactions from the source.
      - clone: uses MySQL clone to provision the instance, which completely
        replaces the state of the target instance with a full snapshot of the
        instance's source.
      - auto: compares the transaction set of the instance with that of the
        source to determine if incremental recovery is safe to be automatically
        chosen as the most appropriate recovery method. A prompt will be shown
        if not possible to safely determine a safe way forward. If interaction
        is disabled, the operation will be canceled instead.

      If recoveryMethod is not specified 'auto' will be used by default.

      The recoveryProgress option supports the following values:

      - 0: do not show any progress information.
      - 1: show detailed static progress information.
      - 2: show detailed dynamic progress information using progress bars.

      By default, if the standard output on which the Shell is running refers
      to a terminal, the recoveryProgress option has the value of 2. Otherwise,
      it has the value of 1.

//@<OUT> routingOptions
NAME
      routingOptions - Lists the Cluster Routers configuration options.

SYNTAX
      <Cluster>.routingOptions([router])

WHERE
      router: Identifier of the router instance to query for the options.

RETURNS
      A JSON object describing the configuration options of all router
      instances of the Cluster and its global options or just the given Router.

DESCRIPTION
      ATTENTION: This function is deprecated and will be removed in a future
                 release of MySQL Shell. Use Cluster.routerOptions() instead.

      This function lists the Router configuration options of all Routers of
      the Cluster or the target Router.

//@<OUT> routerOptions
NAME
      routerOptions - Lists the configuration options of the Cluster's Routers.

SYNTAX
      <Cluster>.routerOptions(options)

WHERE
      options: Dictionary with options for the operation.

RETURNS
      A JSON object with the list of Router configuration options.

DESCRIPTION
      This function lists the Router configuration options of the Cluster
      (global), and the Router instances. By default, only the options that can
      be changed from Shell (dynamic options) are displayed. Router instances
      with different configurations than the global ones will include the
      differences under their dedicated description.

      The options dictionary may contain the following attributes:

      - extended: Verbosity level of the command output.
      - router: Identifier of the Router instance to be displayed.

      The extended option supports Integer or Boolean values:

      - 0: Includes only options that can be changed from Shell (default);
      - 1: Includes all Cluster global options and, per Router, only the
        options that have a different value than the corresponding global one.
      - 2: Includes all Cluster and Router options.
      - Boolean: equivalent to assign either 0 (false) or 1 (true).

//@<OUT> setRoutingOption
NAME
      setRoutingOption - Changes the value of either a global Cluster Routing
                         option or of a single Router instance.

SYNTAX
      <Cluster>.setRoutingOption([router], option, value)

WHERE
      router: Identifier of the target router instance (e.g.
              192.168.45.70::system).
      option: The Router option to be changed.
      value: The value that the option shall get (or null to unset).

RETURNS
      Nothing.

DESCRIPTION
      The accepted options are:

      - tags: Associates an arbitrary JSON object with custom key/value pairs
        with the Cluster metadata.
      - read_only_targets: Routing policy to define Router's usage of Read Only
        instance. Default is 'secondaries'.
      - stats_updates_frequency: Number of seconds between updates that the
        Router is to make to its statistics in the InnoDB Cluster metadata.
      - unreachable_quorum_allowed_traffic: Routing policy to define Router's
        behavior regarding traffic destinations (ports) when it loses access to
        the Cluster's quorum.
      - guideline: Name of the Routing Guideline to be set as active in the
        Cluster.

      The read_only_targets option supports the following values:

      - all: All Read Replicas of the target Cluster should be used along the
        other SECONDARY Cluster members for R/O traffic.
      - read_replicas: Only Read Replicas of the target Cluster should be used
        for R/O traffic.
      - secondaries: Only Secondary members of the target Cluster should be
        used for R/O traffic (default).

      The stats_updates_frequency option accepts positive integers and sets the
      frequency of updates of Router stats (timestamp, version, etc.), in
      seconds, in the Metadata. If set to 0 (default), no periodic updates are
      done. Router will round up the value to be a multiple of Router's TTL,
      i.e.:

      - If lower than TTL its gets rounded up to TTL, e.g. TTL=30, and
        stats_updates_frequency=1, effective frequency is 30 seconds.
      - If not a multiple of TTL it will be rounded up and adjusted according
        to the TTL, e.g. TTL=5, stats_updates_frequency=11, effective frequency
        is 15 seconds; TTL=5, stats_updates_frequency=13, effective frequency
        is 15 seconds.

      If the value is null, the option value is cleared and the default value
      (0) takes effect.

      The unreachable_quorum_allowed_traffic option allows configuring Router's
      behavior in the event of a loss of quorum on the only reachable Cluster
      partition. By default, Router will disconnect all existing connections
      and refuse new ones, but that can be configured using the following
      options:

      - read: Router will keep using ONLINE members as Read-Only destination,
        leaving the RO and RW-split ports open.
      - all: Router will keep ONLINE members as Read/Write destinations,
        leaving all ports (RW, RO, and RW-split), open.
      - none: All current connections are disconnected and new ones are refused
        (default behavior).

      ATTENTION: Setting this option to a different value other than the
                 default may have unwanted consequences: the consistency
                 guarantees provided by InnoDB Cluster are broken since the
                 data read can be stale; different Routers may be accessing
                 different partitions, thus return different data; and
                 different Routers may also have different behavior (i.e. some
                 provide only read traffic while others read and write
                 traffic). Note that writes on a partition with no quorum will
                 block until quorum is restored.

      This option has no practical effect if
      group_replication_unreachable_majority_timeout is set to a positive value
      and group_replication_exit_state_action is either OFFLINE_MODE or
      ABORT_SERVER.

      ATTENTION: When the Cluster has an active Routing Guideline, the option
                 'read_only_targets' is ignored since the Guideline has
                 precedence over it.

//@<OUT> createRoutingGuideline
NAME
      createRoutingGuideline - Creates a new Routing Guideline for the Cluster.

SYNTAX
      <Cluster>.createRoutingGuideline(name[, json][, options])

WHERE
      name: The identifier name for the new Routing Guideline.
      json: JSON document defining the Routing Guideline content.
      options: Dictionary with options for the operation.

RETURNS
      A RoutingGuideline object representing the newly created Routing
      Guideline.

DESCRIPTION
      This command creates a new Routing Guideline that defines MySQL Router's
      routing behavior using rules that specify potential destination MySQL
      servers for incoming client sessions.

      You can optionally pass a JSON document defining the Routing Guideline
      via the 'json' parameter. This must be a valid Routing Guideline
      definition, with the exception of the "name" field, which is overridden
      by the provided 'name' parameter.

      If the 'json' parameter is not provided, a default Routing Guideline is
      created based on the parent topology type. The guideline is automatically
      populated with default values tailored to the topology, ensuring the
      Router's default behavior for that topology is represented.

      The newly created guideline won't be set as the active guideline for the
      topology. That needs to be explicitly done with
      Cluster.setRoutingOption() using the option 'guideline'.

      The following options are supported:

      - force (boolean): Allows overwriting an existing Routing Guideline with
        the specified name. Disabled by default.

      Behavior

      - If 'json' is not provided, a default Routing Guideline is created
        according to the parent topology type.
      - If 'json' is provided, the content from the JSON is used, except for
        the "name" field, which is overridden by the 'name' parameter.

      For more information on Routing Guidelines, see \? RoutingGuideline.


//@<OUT> getRoutingGuideline
NAME
      getRoutingGuideline - Returns the named Routing Guideline.

SYNTAX
      <Cluster>.getRoutingGuideline([name])

WHERE
      name: Name of the Guideline to be returned.

RETURNS
      The Routing Guideline object.

DESCRIPTION
      Returns the named Routing Guideline object associated to the Cluster. If
      no name is given, the guideline currently active for the Cluster is
      returned. If there is none, then an exception is thrown.

      For more information about Routing Guidelines, see \? RoutingGuideline

//@<OUT> removeRoutingGuideline
NAME
      removeRoutingGuideline - Removes the named Routing Guideline.

SYNTAX
      <Cluster>.removeRoutingGuideline(name)

WHERE
      name: Name of the Guideline to be removed.

RETURNS
      Nothing.

DESCRIPTION
      Removes the named Routing Guideline object associated to the Cluster.

      For more information about Routing Guidelines, see \? RoutingGuideline

//@<OUT> routingGuidelines
NAME
      routingGuidelines - Lists the Routing Guidelines defined for the Cluster.

SYNTAX
      <Cluster>.routingGuidelines()

RETURNS
      The list of Routing Guidelines of the Cluster.

DESCRIPTION
      For more information about Routing Guidelines, see \? RoutingGuideline

//@<OUT> importRoutingGuideline
NAME
      importRoutingGuideline - Imports a Routing Guideline from a JSON file
                               into the Cluster.

SYNTAX
      <Cluster>.importRoutingGuideline(file[, options])

WHERE
      file: The file path to the JSON file containing the Routing Guideline.
      options: Dictionary with options for the operation.

RETURNS
      A RoutingGuideline object representing the newly imported Routing
      Guideline.

DESCRIPTION
      This command imports a Routing Guideline from a JSON file into the target
      topology. The imported guideline will be validated before it is saved in
      the topology's metadata.

      The imported guideline won't be set as the active guideline for the
      topology. That needs to be explicitly done with
      Cluster.setRoutingOption() using the option 'guideline'.

      The following options are supported:

      - force (boolean): Allows overwriting an existing Routing Guideline with
        the same name. Disabled by default.

      For more information on Routing Guidelines, see \? RoutingGuideline.
