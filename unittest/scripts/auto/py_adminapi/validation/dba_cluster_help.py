#@ __global__
||

#@<OUT> cluster
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
      add_instance(instance[, options])
            Adds an Instance to the cluster.

      check_instance_state(instance)
            Verifies the instance gtid state in relation to the cluster.

      create_cluster_set(domainName[, options])
            Creates a MySQL InnoDB ClusterSet from an existing standalone
            InnoDB Cluster.

      describe()
            Describe the structure of the cluster.

      disconnect()
            Disconnects all internal sessions used by the cluster object.

      dissolve([options])
            Dissolves the cluster.

      fence_all_traffic()
            Fences a Cluster from All Traffic.

      fence_writes()
            Fences a Cluster from Write Traffic.

      force_quorum_using_partition_of(instance[, password])
            Restores the cluster from quorum loss.

      get_cluster_set()
            Returns an object representing a ClusterSet.

      get_name()
            Retrieves the name of the cluster.

      help([member])
            Provides help about this class and it's members

      list_routers([options])
            Lists the Router instances.

      options([options])
            Lists the cluster configuration options.

      rejoin_instance(instance[, options])
            Rejoins an Instance to the cluster.

      remove_instance(instance[, options])
            Removes an Instance from the cluster.

      remove_router_metadata(routerDef)
            Removes metadata for a router instance.

      rescan([options])
            Rescans the cluster.

      reset_recovery_accounts_password(options)
            Reset the password of the recovery accounts of the cluster.

      set_instance_option(instance, option, value)
            Changes the value of an option in a Cluster member.

      set_option(option, value)
            Changes the value of an option for the whole Cluster.

      set_primary_instance(instance[, options])
            Elects a specific cluster member as the new primary.

      setup_admin_account(user, options)
            Create or upgrade an InnoDB Cluster admin account.

      setup_router_account(user, options)
            Create or upgrade a MySQL account to use with MySQL Router.

      status([options])
            Describe the status of the cluster.

      switch_to_multi_primary_mode()
            Switches the cluster to multi-primary mode.

      switch_to_single_primary_mode([instance])
            Switches the cluster to single-primary mode.

      unfence_writes()
            Unfences a Cluster.

      For more help on a specific function use: cluster.help('<functionName>')

      e.g. cluster.help('addInstance')

#@<OUT> cluster.add_instance
NAME
      add_instance - Adds an Instance to the cluster.

SYNTAX
      <Cluster>.add_instance(instance[, options])

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
      - waitRecovery: Integer value to indicate if the command shall wait for
        the recovery process to finish and its verbosity level.
      - password: the instance connection password
      - memberSslMode: SSL mode used on the instance
      - ipWhitelist: The list of hosts allowed to connect to the instance for
        group replication. Deprecated.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
      - groupSeeds: string value with a comma-separated list of the Group
        Replication peer addresses to be used instead of the automatically
        generated one. Deprecated and ignored.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.

      The password may be contained on the instance definition, however, it can
      be overwritten if it is specified on the options.

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

      ATTENTION: The memberSslMode option will be removed in a future release.

      The memberSslMode option supports the following values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instance to
        communicate with other members of the cluster
      - VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
        certificate against the configured Certificate Authority (CA)
        certificates.
      - VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the
        server certificate matches the host to which the connection is
        attempted.
      - DISABLED: if used, SSL (encryption) will be disabled
      - AUTO: if used, SSL (encryption) will be automatically enabled or
        disabled based on the cluster configuration

      If memberSslMode is not specified AUTO will be used by default.

      The waitRecovery option supports the following values:

      - 0: do not wait and let the recovery process to finish in the
        background.
      - 1: block until the recovery process to finishes.
      - 2: block until the recovery process finishes and show progress
        information.
      - 3: block until the recovery process finishes and show progress using
        progress bars.

      By default, if the standard output on which the Shell is running refers
      to a terminal, the waitRecovery option has the value of 3. Otherwise, it
      has the value of 2.

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

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

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

      The groupSeeds option is deprecated as of MySQL Shell 8.0.28 and is
      ignored. 'group_replication_group_seeds' is automatically set based on
      the current topology.

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

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values and, since 8.0.21, defaults to 3.

      ATTENTION: The ipWhitelist option will be removed in a future release.
                 Please use the ipAllowlist option instead.

      ATTENTION: The groupSeeds option will be removed in a future release.

#@<OUT> cluster.check_instance_state
NAME
      check_instance_state - Verifies the instance gtid state in relation to
                             the cluster.

SYNTAX
      <Cluster>.check_instance_state(instance)

WHERE
      instance: An instance definition.

RETURNS
      resultset A JSON object with the status.

DESCRIPTION
      Analyzes the instance executed GTIDs with the executed/purged GTIDs on
      the cluster to determine if the instance is valid for the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      The returned JSON object contains the following attributes:

      - state: the state of the instance
      - reason: the reason for the state reported

      The state of the instance can be one of the following:

      - ok: if the instance transaction state is valid for the cluster
      - error: if the instance transaction state is not valid for the cluster

      The reason for the state reported can be one of the following:

      - new: if the instance doesn't have any transactions
      - recoverable:  if the instance executed GTIDs are not conflicting with
        the executed GTIDs of the cluster instances
      - diverged: if the instance executed GTIDs diverged with the executed
        GTIDs of the cluster instances
      - lost_transactions: if the instance has more executed GTIDs than the
        executed GTIDs of the cluster instances

#@<OUT> cluster.describe
NAME
      describe - Describe the structure of the cluster.

SYNTAX
      <Cluster>.describe()

RETURNS
      A JSON object describing the structure of the cluster.

DESCRIPTION
      This function describes the structure of the cluster including all its
      information, ReplicaSets and Instances.

      The returned JSON object contains the following attributes:

      - clusterName: the cluster name
      - defaultReplicaSet: the default ReplicaSet object

      The defaultReplicaSet JSON object contains the following attributes:

      - name: the ReplicaSet name
      - topology: a list of dictionaries describing each instance belonging to
        the ReplicaSet.
      - topologyMode: the InnoDB Cluster topology mode.

      Each instance dictionary contains the following attributes:

      - address: the instance address in the form of host:port
      - label: the instance name identifier
      - role: the instance role
      - version: the instance version (only available for instances >= 8.0.11)

#@<OUT> cluster.disconnect
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

#@<OUT> cluster.dissolve
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
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      The force option (set to true) must only be used to dissolve a cluster
      with instances that are permanently not available (no longer reachable)
      or never to be reused again in a cluster. This allows to dissolve a
      cluster and remove it from the metadata, including instances than can no
      longer be recovered. Otherwise, the instances must be brought back ONLINE
      and the cluster dissolved without the force option to avoid errors trying
      to reuse the instances and add them back to a cluster.

#@<OUT> cluster.force_quorum_using_partition_of
NAME
      force_quorum_using_partition_of - Restores the cluster from quorum loss.

SYNTAX
      <Cluster>.force_quorum_using_partition_of(instance[, password])

WHERE
      instance: An instance definition to derive the forced group from.
      password: String with the password for the connection.

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

#@<OUT> cluster.get_name
NAME
      get_name - Retrieves the name of the cluster.

SYNTAX
      <Cluster>.get_name()

RETURNS
      The name of the cluster.

#@<OUT> cluster.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Cluster>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> cluster.options
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

#@<OUT> cluster.name
NAME
      name - Retrieves the name of the cluster.

SYNTAX
      <Cluster>.name

#@<OUT> cluster.rejoin_instance
NAME
      rejoin_instance - Rejoins an Instance to the cluster.

SYNTAX
      <Cluster>.rejoin_instance(instance[, options])

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

      - password: the instance connection password
      - memberSslMode: SSL mode used on the instance
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - ipWhitelist: The list of hosts allowed to connect to the instance for
        group replication. Deprecated.
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.

      The password may be contained on the instance definition, however, it can
      be overwritten if it is specified on the options.

      ATTENTION: The memberSslMode option will be removed in a future release.

      The memberSslMode option supports these values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instance to
        communicate with other members of the cluster
      - VERIFY_CA: Like REQUIRED, but additionally verify the server TLS
        certificate against the configured Certificate Authority (CA)
        certificates.
      - VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the
        server certificate matches the host to which the connection is
        attempted.
      - DISABLED: if used, SSL (encryption) will be disabled
      - AUTO: if used, SSL (encryption) will be automatically enabled or
        disabled based on the cluster configuration

      If memberSslMode is not specified AUTO will be used by default.

      The ipAllowlist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the allowlist.

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

      ATTENTION: The ipWhitelist option will be removed in a future release.
                 Please use the ipAllowlist option instead.

#@<OUT> cluster.remove_instance
NAME
      remove_instance - Removes an Instance from the cluster.

SYNTAX
      <Cluster>.remove_instance(instance[, options])

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

      - password: the instance connection password
      - force: boolean, indicating if the instance must be removed (even if
        only from metadata) in case it cannot be reached. By default, set to
        false.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      The password may be contained in the instance definition, however, it can
      be overwritten if it is specified on the options.

      The force option (set to true) must only be used to remove instances that
      are permanently not available (no longer reachable) or never to be reused
      again in a cluster. This allows to remove from the metadata an instance
      than can no longer be recovered. Otherwise, the instance must be brought
      back ONLINE and removed without the force option to avoid errors trying
      to add it back to a cluster.

#@<OUT> cluster.set_instance_option
NAME
      set_instance_option - Changes the value of an option in a Cluster member.

SYNTAX
      <Cluster>.set_instance_option(instance, option, value)

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
      - label a string identifier of the instance.

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

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values and, since 8.0.21, defaults to 3.

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

#@<OUT> cluster.set_option
NAME
      set_option - Changes the value of an option for the whole Cluster.

SYNTAX
      <Cluster>.set_option(option, value)

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
      - failoverConsistency: string value indicating the consistency guarantees
        that the cluster provides.
      - consistency: string value indicating the consistency guarantees that
        the cluster provides.
      - expelTimeout: integer value to define the time period in seconds that
        cluster members should wait for a non-responding member before evicting
        it from the cluster.
      - autoRejoinTries: integer value to define the number of times an
        instance will attempt to rejoin the cluster after being expelled.
      - disableClone: boolean value used to disable the clone usage on the
        cluster.
      - replicationAllowedHost string value to use as the host name part of
        internal replication accounts. Existing accounts will be re-created
        with the new value.

      ATTENTION: The failoverConsistency option will be removed in a future
                 release. Please use the consistency option instead.

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
      option accepts positive integer values and, since 8.0.21, defaults to 5
      seconds.

      The value for autoRejoinTries is used to set the Group Replication system
      variable 'group_replication_autorejoin_tries' and configure how many
      times an instance will try to rejoin a Group Replication group after
      being expelled. In scenarios where network glitches happen but recover
      quickly, setting this option prevents users from having to manually add
      the expelled node back to the group. The autoRejoinTries option accepts
      positive integer values and, since 8.0.21, defaults to 3.

      Tags

      Tags make it possible to associate custom key/value pairs to a Cluster,
      storing them in its metadata. Custom tag names can be any string starting
      with letters and followed by letters, numbers and _. Tag values may be
      any JSON value. If the value is null, the tag is deleted.

#@<OUT> cluster.setup_admin_account
NAME
      setup_admin_account - Create or upgrade an InnoDB Cluster admin account.

SYNTAX
      <Cluster>.setup_admin_account(user, options)

WHERE
      user: Name of the InnoDB cluster administrator account.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function creates/upgrades a MySQL user account with the necessary
      privileges to administer an InnoDB cluster.

      This function also allows a user to upgrade an existing admin account
      with the necessary privileges before a dba.upgrade_metadata() call.

      The mandatory argument user is the name of the MySQL account we want to
      create or upgrade to be used as Administrator account. The accepted
      format is username[@host] where the host part is optional and if not
      provided defaults to '%'.

      The options dictionary may contain the following attributes:

      - password: The password for the InnoDB cluster administrator account.
      - dryRun: boolean value used to enable a dry run of the account setup
        process. Default value is False.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - update: boolean value that must be enabled to allow updating the
        privileges and/or password of existing accounts. Default value is
        False.

      If the user account does not exist, the password is mandatory.

      If the user account exists, the update option must be enabled.

      If dryRun is used, the function will display information about the
      permissions to be granted to `user` account without actually creating
      and/or performing any changes on it.

      The interactive option can be used to explicitly enable or disable the
      interactive prompts that help the user through the account setup process.

      The update option must be enabled to allow updating an existing account's
      privileges and/or password.

#@<OUT> cluster.setup_router_account
NAME
      setup_router_account - Create or upgrade a MySQL account to use with
                             MySQL Router.

SYNTAX
      <Cluster>.setup_router_account(user, options)

WHERE
      user: Name of the account to create/upgrade for MySQL Router.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function creates/upgrades a MySQL user account with the necessary
      privileges to be used by MySQL Router.

      This function also allows a user to upgrade existing MySQL router
      accounts with the necessary privileges after a dba.upgrade_metadata()
      call.

      The mandatory argument user is the name of the MySQL account we want to
      create or upgrade to be used by MySQL Router. The accepted format is
      username[@host] where the host part is optional and if not provided
      defaults to '%'.

      The options dictionary may contain the following attributes:

      - password: The password for the MySQL Router account.
      - dryRun: boolean value used to enable a dry run of the account setup
        process. Default value is False.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - update: boolean value that must be enabled to allow updating the
        privileges and/or password of existing accounts. Default value is
        False.

      If the user account does not exist, the password is mandatory.

      If the user account exists, the update option must be enabled.

      If dryRun is used, the function will display information about the
      permissions to be granted to `user` account without actually creating
      and/or performing any changes on it.

      The interactive option can be used to explicitly enable or disable the
      interactive prompts that help the user through the account setup process.

      The update option must be enabled to allow updating an existing account's
      privileges and/or password.

#@<OUT> cluster.rescan
NAME
      rescan - Rescans the cluster.

SYNTAX
      <Cluster>.rescan([options])

WHERE
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function rescans the cluster for new and obsolete Group Replication
      members/instances, as well as changes in the used topology mode (i.e.,
      single-primary and multi-primary).

      The options dictionary may contain the following attributes:

      - addInstances: List with the connection data of the new active instances
        to add to the metadata, or "auto" to automatically add missing
        instances to the metadata.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - removeInstances: List with the connection data of the obsolete
        instances to remove from the metadata, or "auto" to automatically
        remove obsolete instances from the metadata.
      - updateTopologyMode: boolean value used to indicate if the topology mode
        (single-primary or multi-primary) in the metadata should be updated
        (true) or not (false) to match the one being used by the cluster. By
        default, the metadata is not updated (false). Deprecated.
      - upgradeCommProtocol: boolean. Set to true to upgrade the Group
        Replication communication protocol to the highest version possible.
      - updateViewChangeUuid: boolean value used to indicate if the command
        should generate and set a value for Group Replication View Change UUID
        in the whole Cluster. Required for InnoDB ClusterSet usage.

      The value for addInstances and removeInstances is used to specify which
      instances to add or remove from the metadata, respectively. Both options
      accept list connection data. In addition, the "auto" value can be used
      for both options in order to automatically add or remove the instances in
      the metadata, without having to explicitly specify them.

      ATTENTION: The updateTopologyMode option will be removed in a future
                 release.

#@<OUT> cluster.status
NAME
      status - Describe the status of the cluster.

SYNTAX
      <Cluster>.status([options])

WHERE
      options: Dictionary with options.

RETURNS
      A JSON object describing the status of the cluster.

DESCRIPTION
      This function describes the status of the cluster including its
      ReplicaSets and Instances. The following options may be given to control
      the amount of information gathered and returned.

      - extended: verbosity level of the command output.
      - queryMembers: if true, connect to each Instance of the ReplicaSets to
        query for more detailed stats about the replication machinery.

      ATTENTION: The queryMembers option will be removed in a future release.
                 Please use the extended option instead.

      The extended option supports Integer or Boolean values:

      - 0: disables the command verbosity (default);
      - 1: includes information about the Metadata Version, Group Protocol
        Version, Group name, cluster member UUIDs, cluster member roles and
        states as reported byGroup Replication and the list of fenced system
        variables;
      - 2: includes information about transactions processed by connection and
        applier;
      - 3: includes more detailed stats about the replication machinery of each
        cluster member;
      - Boolean: equivalent to assign either 0 (false) or 1 (true).

#@<OUT> cluster.list_routers
NAME
      list_routers - Lists the Router instances.

SYNTAX
      <Cluster>.list_routers([options])

WHERE
      options: Dictionary with options for the operation.

RETURNS
      A JSON object listing the Router instances associated to the cluster.

DESCRIPTION
      This function lists and provides information about all Router instances
      registered for the cluster.

      Whenever a Metadata Schema upgrade is necessary, the recommended process
      is to upgrade MySQL Router instances to the latest version before
      upgrading the Metadata itself, in order to minimize service disruption.

      ATTENTION: The lastCheckIn property reflects the Routers' startup
                 timestamp.

      The options dictionary may contain the following attributes:

      - onlyUpgradeRequired: boolean, enables filtering so only router
        instances that support older version of the Metadata Schema and require
        upgrade are included.

#@<OUT> cluster.remove_router_metadata
NAME
      remove_router_metadata - Removes metadata for a router instance.

SYNTAX
      <Cluster>.remove_router_metadata(routerDef)

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

      The Cluster.list_routers() function may be used to list registered router
      instances, including their identifier.

#@<OUT> cluster.set_primary_instance
NAME
      set_primary_instance - Elects a specific cluster member as the new
                             primary.

SYNTAX
      <Cluster>.set_primary_instance(instance[, options])

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

#@<OUT> cluster.switch_to_multi_primary_mode
NAME
      switch_to_multi_primary_mode - Switches the cluster to multi-primary
                                     mode.

SYNTAX
      <Cluster>.switch_to_multi_primary_mode()

RETURNS
      Nothing.

DESCRIPTION
      This function changes a cluster running in single-primary mode to
      multi-primary mode.

#@<OUT> cluster.switch_to_single_primary_mode
NAME
      switch_to_single_primary_mode - Switches the cluster to single-primary
                                      mode.

SYNTAX
      <Cluster>.switch_to_single_primary_mode([instance])

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

#@<OUT> cluster.reset_recovery_accounts_password
NAME
      reset_recovery_accounts_password - Reset the password of the recovery
                                         accounts of the cluster.

SYNTAX
      <Cluster>.reset_recovery_accounts_password(options)

WHERE
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function resets the passwords for all internal recovery user
      accounts used by the Cluster. It can be used to reset the passwords of
      the recovery user accounts when needed for any security reasons. For
      example: periodically to follow some custom password lifetime policy, or
      after some security breach event.

      The options dictionary may contain the following attributes:

      - force: boolean, indicating if the operation will continue in case an
        error occurs when trying to reset the passwords on any of the
        instances, for example if any of them is not online. By default, set to
        false.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      The use of the force option (set to true) is not recommended. Use it only
      if really needed when instances are permanently not available (no longer
      reachable) or never going to be reused again in a cluster. Prefer to
      bring the non available instances back ONLINE or remove them from the
      cluster if they will no longer be used.

#@<OUT> cluster.create_cluster_set
NAME
      create_cluster_set - Creates a MySQL InnoDB ClusterSet from an existing
                           standalone InnoDB Cluster.

SYNTAX
      <Cluster>.create_cluster_set(domainName[, options])

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

      - REQUIRED: if used, SSL (encryption) will be enabled for the ClusterSet
        replication channels.
      - DISABLED: if used, SSL (encryption) will be disabled for the ClusterSet
        replication channels.
      - AUTO: if used, SSL (encryption) will be enabled if supported by the
        instance, otherwise disabled.

      If clusterSetReplicationSslMode is not specified AUTO will be used by
      default.

#@<OUT> cluster.get_cluster_set
NAME
      get_cluster_set - Returns an object representing a ClusterSet.

SYNTAX
      <Cluster>.get_cluster_set()

RETURNS
      The ClusterSet object to which the current cluster belongs to.

DESCRIPTION
      The returned object is identical to the one returned by
      create_cluster_set() and can be used to manage the ClusterSet.

      The function will work regardless of whether the target cluster is a
      PRIMARY or a REPLICA Cluster, but its copy of the metadata is expected to
      be up-to-date.

      This function will also work if the PRIMARY Cluster is unreachable or
      unavailable, although ClusterSet change operations will not be possible,
      except for failover with force_primary_cluster().

#@<OUT> cluster.fence_all_traffic
NAME
      fence_all_traffic - Fences a Cluster from All Traffic.

SYNTAX
      <Cluster>.fence_all_traffic()

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
                 dba.reboot_cluster_from_complete_outage().

#@<OUT> cluster.fence_writes
NAME
      fence_writes - Fences a Cluster from Write Traffic.

SYNTAX
      <Cluster>.fence_writes()

RETURNS
      Nothing

DESCRIPTION
      This function fences a PRIMARY Cluster from all Write Traffic by ensuring
      all of its members are Read-Only regardless of any topology change on it.
      The Cluster will be put into READ ONLY mode and all members will remain
      available for reads. To unfence the Cluster so it restores its normal
      functioning and can accept all traffic use Cluster.unfence().

      Use this function when performing a PRIMARY Cluster failover in a
      ClusterSet to allow only read traffic in the previous Primary Cluster in
      the event of a split-brain.

      The function is not permitted on REPLICA Clusters.

#@<OUT> cluster.unfence_writes
NAME
      unfence_writes - Unfences a Cluster.

SYNTAX
      <Cluster>.unfence_writes()

RETURNS
      Nothing

DESCRIPTION
      This function unfences a Cluster that was previously fenced to Write
      traffic with Cluster.fence_writes().

      ATTENTION: This function does not unfence Clusters that have been fenced
                 to ALL traffic. Those Cluster are completely shut down and can
                 only be restored using
                 dba.reboot_cluster_from_complete_outage().

