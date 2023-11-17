//@<OUT> Object Help
NAME
      ClusterSet - Represents an InnoDB ClusterSet.

DESCRIPTION
      The clusterset object is the entry point to manage and monitor a MySQL
      InnoDB ClusterSet.

      ClusterSets allow InnoDB Cluster deployments to achieve fault-tolerance
      at a whole Data Center / region or geographic location, by creating
      REPLICA clusters in different locations (Data Centers), ensuring Disaster
      Recovery is possible.

      For more help on a specific function, use the \help shell command, e.g.:
      \help ClusterSet.createReplicaCluster

PROPERTIES
      name
            Returns the domain name of the clusterset.

FUNCTIONS
      createReplicaCluster(instance, clusterName[, options])
            Creates a new InnoDB Cluster that is a Replica of the Primary
            Cluster.

      describe()
            Describe the structure of the ClusterSet.

      disconnect()
            Disconnects all internal sessions used by the ClusterSet object.

      forcePrimaryCluster(clusterName[, options])
            Performs a failover of the PRIMARY Cluster of the ClusterSet.

      getName()
            Returns the domain name of the clusterset.

      help([member])
            Provides help about this class and it's members

      listRouters([router])
            Lists the Router instances of the ClusterSet, or a single Router
            instance.

      options()
            Lists the ClusterSet configuration options.

      rejoinCluster(clusterName[, options])
            Rejoin an invalidated Cluster back to the ClusterSet and update
            replication.

      removeCluster(clusterName[, options])
            Removes a Replica cluster from a ClusterSet.

      routingOptions([router])
            Lists the ClusterSet Routers configuration options.

      setOption(option, value)
            Changes the value of an option for the whole ClusterSet.

      setPrimaryCluster(clusterName[, options])
            Performs a safe switchover of the PRIMARY Cluster of the
            ClusterSet.

      setRoutingOption([router], option, value)
            Changes the value of either a global Routing option or of a single
            Router instance.

      setupAdminAccount(user, options)
            Create or upgrade an InnoDB ClusterSet admin account.

      setupRouterAccount(user, options)
            Create or upgrade a MySQL account to use with MySQL Router.

      status([options])
            Describe the status of the ClusterSet.

//@<OUT> Name
NAME
      name - Returns the domain name of the clusterset.

SYNTAX
      <ClusterSet>.name

//@<OUT> Disconnect
NAME
      disconnect - Disconnects all internal sessions used by the ClusterSet
                   object.

SYNTAX
      <ClusterSet>.disconnect()

RETURNS
      Nothing.

DESCRIPTION
      Disconnects the internal MySQL sessions used by the ClusterSet to query
      for metadata and replication information.

//@<OUT> CreateReplicaCluster
NAME
      createReplicaCluster - Creates a new InnoDB Cluster that is a Replica of
                             the Primary Cluster.

SYNTAX
      <ClusterSet>.createReplicaCluster(instance, clusterName[, options])

WHERE
      instance: host:port of the target instance to be used to create the
                Replica Cluster
      clusterName: An identifier for the REPLICA cluster to be created.
      options: Dictionary with additional parameters described below.

RETURNS
      The created Replica Cluster object.

DESCRIPTION
      Creates a REPLICA InnoDB cluster of the current PRIMARY cluster with the
      given cluster name and options, at the target instance.

      If the target instance meets the requirements for InnoDB Cluster a new
      cluster is created on it, replicating from the PRIMARY instance of the
      primary cluster of the ClusterSet.

      Pre-requisites

      The following is a list of requirements to create a REPLICA cluster:

      - The target instance must comply with the requirements for InnoDB
        Cluster.
      - The target instance must be a standalone instance.
      - The target instance must running MySQL 8.0.27 or newer.
      - Unmanaged replication channels are not allowed.
      - The target instance 'server_id' and 'server_uuid' must be unique in the
        ClusterSet
      - The target instance must have the same credentials used to manage the
        ClusterSet.

      The Replica Cluster's name must be non-empty and no greater than 63
      characters long. It can only start with an alphanumeric character or with
      _ (underscore), and can only contain alphanumeric, _ ( underscore), .
      (period), or - ( hyphen) characters.

      For the detailed list of requirements to create an InnoDB Cluster, please
      use \? createCluster

      Options

      The options dictionary can contain the following values:

      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode. Deprecated.
      - dryRun: boolean if true, all validations and steps for creating a
        Replica Cluster are executed, but no changes are actually made. An
        exception will be thrown when finished.
      - recoveryMethod: Preferred method for state recovery/provisioning. May
        be auto, clone or incremental. Default is auto.
      - recoveryProgress: Integer value to indicate the recovery process
        verbosity level.
      - cloneDonor: host:port of an existing member of the PRIMARY cluster to
        clone from. IPv6 addresses are not supported for this option.
      - manualStartOnBoot: boolean (default false). If false, Group Replication
        in cluster instances will automatically start and rejoin when MySQL
        starts, otherwise it must be started manually.
      - memberSslMode: SSL mode for communication channels opened by Group
        Replication from one server to another.
      - certSubject: instance's certificate subject to use when
        'memberAuthType' contains "CERT_SUBJECT".
      - ipAllowlist: The list of hosts allowed to connect to the instance for
        group replication. Only valid if communicationStack=XCOM.
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
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
      - timeout: maximum number of seconds to wait for the instance to sync up
        with the PRIMARY Cluster. Default is 0 and it means no timeout.
      - replicationAllowedHost: string value to use as the host name part of
        internal replication accounts (i.e.
        'mysql_innodb_cluster_###'@'hostname'). Default is %. It must be
        possible for any member of the Cluster to connect to any other member
        using accounts with this hostname value.
      - communicationStack: The Group Replication communication stack to be
        used in the Cluster: XCom (legacy) or MySQL.
      - transactionSizeLimit: integer value to configure the maximum
        transaction size in bytes which the Cluster accepts
      - paxosSingleLeader: boolean value used to enable/disable the Group
        Communication engine to operate with a single consensus leader.
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

      The recoveryMethod option supports the following values:

      - incremental: waits until the new instance has applied missing
        transactions from the PRIMARY
      - clone: uses MySQL clone to provision the instance, which completely
        replaces the state of the target instance with a full snapshot of
        another ClusterSet member.
      - auto: compares the transaction set of the instance with that of the
        PRIMARY to determine if incremental recovery is safe to be
        automatically chosen as the most appropriate recovery method. A prompt
        will be shown if not possible to safely determine a safe way forward.
        If interaction is disabled, the operation will be canceled instead.

      If recoveryMethod is not specified 'auto' will be used by default.

      The recoveryProgress option supports the following values:

      - 0: do not show any progress information.
      - 1: show detailed static progress information.
      - 2: show detailed dynamic progress information using progress bars.

      By default, if the standard output on which the Shell is running refers
      to a terminal, the recoveryProgress option has the value of 2. Otherwise,
      it has the value of 1.

      The cloneDonor option is used to override the automatic selection of a
      donor to be used when clone is selected as the recovery method. By
      default, a SECONDARY member will be chosen as donor. If no SECONDARY
      members are available the PRIMARY will be selected. The option accepts
      values in the format: 'host:port'. IPv6 addresses are not supported.

      The memberSslMode option controls whether TLS is to be used for
      connections opened by Group Replication from one server to another (both
      recovery and group communication, in either communication stack). It also
      controls what kind of verification the client end of connections perform
      on the SSL certificate presented by the server end.

      The memberSslMode option supports the following values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instances
        to communicate with other members of the cluster
      - VERIFY_CA: Like REQUIRED, but additionally verify the peer server TLS
        certificate against the configured Certificate Authority (CA)
        certificates.
      - VERIFY_IDENTITY: Like VERIFY_CA, but additionally verify that the peer
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

      This option is only used and allowed when communicationStack is set to
      XCOM.

      The groupName and localAddress are advanced options and their usage is
      discouraged since incorrect values can lead to Group Replication errors.

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
      is the port of the target instance if the communication stack in use by
      the Cluster is 'MYSQL', otherwise, port * 10 + 1  when the communication
      stack is 'XCOM'. In case the automatically determined  default port value
      is invalid (> 65535) then an error is thrown.

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

      When set to BEFORE_ON_PRIMARY_FAILOVER, whenever a primary failover
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

      The value for communicationStack is used to choose which Group
      Replication communication stack must be used in the Cluster. It's used to
      set the value of the Group Replication system variable
      'group_replication_communication_stack'.

      When set to legacy 'XCom', all internal GCS network traffic (PAXOS and
      communication infrastructure) flows through a separate network address:
      the localAddress.

      When set to 'MySQL', such traffic re-uses the existing MySQL Server
      facilities to establish connections among Cluster members. It allows a
      simpler and safer setup as it obsoletes the usage of IP allowlists
      (ipAllowlist), removes the explicit need to have a separate network
      address (localAddress), and introduces user-based authentication.

      The default value for Clusters running 8.0.27+ is 'MySQL'.

      The value for transactionSizeLimit is used to set the Group Replication
      system variable 'group_replication_transaction_size_limit' and configures
      the maximum transaction size in bytes which the Cluster accepts.
      Transactions larger than this size are rolled back by the receiving
      member and are not broadcast to the Cluster.

      The transactionSizeLimit option accepts positive integer values and, if
      set to zero, there is no limit to the size of transactions the Cluster
      accepts

      All members added or rejoined to the Cluster will use the same value.

      The value for paxosSingleLeader is used to enable or disable the Group
      Communication engine to operate with a single consensus leader when the
      Cluster is in single-primary more. When enabled, the Cluster uses a
      single leader to drive consensus which improves performance and
      resilience in single-primary mode, particularly when some of the
      Cluster's members are unreachable.

      The option is available on MySQL 8.0.31 or newer and the default value is
      'OFF'.

      ATTENTION: The interactive option will be removed in a future release.

//@<OUT> RemoveCluster
NAME
      removeCluster - Removes a Replica cluster from a ClusterSet.

SYNTAX
      <ClusterSet>.removeCluster(clusterName[, options])

WHERE
      clusterName: The name identifier of the Replica cluster to be removed.
      options: Dictionary with additional parameters described below.

RETURNS
      Nothing.

DESCRIPTION
      Removes a MySQL InnoDB Replica Cluster from the target ClusterSet.

      The Cluster is removed from the ClusterSet and implicitly dissolved, i.e.
      each member of it becomes a standalone instance.

      For the Cluster to be successfully removed from the ClusterSet the
      PRIMARY Cluster must be available and the ClusterSet replication channel
      healthy. If those conditions aren't met the Cluster can still be
      forcefully removed using the 'force' option, however, its Metadata won't
      be updated compromising the effortless usage of its members to create new
      Clusters and/or add to existing Clusters. To re-use those instances, the
      Metadata schema must be dropped using dba.dropMetadataSchema(), or the
      Cluster rebooted from complete outage using
      dba.rebootClusterFromCompleteOutage().

      Options

      The options dictionary can contain the following values:

      - force: boolean, indicating if the cluster must be removed (even if
        only from metadata) in case the PRIMARY cannot be reached, or
        the ClusterSet replication channel cannot be found or is stopped.
        By default, set to false.
      - timeout: maximum number of seconds to wait for the instance to sync up
        with the PRIMARY Cluster. Default is 0 and it means no timeout.
      - dryRun: boolean if true, all validations and steps for removing a
        the Cluster from the ClusterSet are executed, but no changes are
        actually made. An exception will be thrown when finished.

//@<OUT> ClusterSet.setPrimaryCluster
NAME
      setPrimaryCluster - Performs a safe switchover of the PRIMARY Cluster of
                          the ClusterSet.

SYNTAX
      <ClusterSet>.setPrimaryCluster(clusterName[, options])

WHERE
      clusterName: Name of the REPLICA cluster to be promoted.
      options: Dictionary with additional parameters described below.

RETURNS
      Nothing

DESCRIPTION
      This command will perform a safe switchover of the PRIMARY Cluster of a
      ClusterSet. The current PRIMARY will be demoted to a REPLICA Cluster,
      while the promoted Cluster will be made the PRIMARY Cluster. All other
      REPLICA Clusters will be updated to replicate from the new PRIMARY.

      During the switchover, the promoted Cluster will be synchronized with the
      old PRIMARY, ensuring that all transactions present in the PRIMARY are
      applied before the topology change is committed. The current PRIMARY
      instance is also locked with 'FLUSH TABLES WITH READ LOCK' in order to
      prevent changes during the switch. If either of these operations take too
      long or fails, the switch will be aborted.

      For a switchover to be possible, all instances of the target Cluster must
      be reachable from the shell and have consistent transaction sets with the
      current PRIMARY Cluster. If the PRIMARY Cluster is not available and
      cannot be restored, a failover must be performed instead, using
      ClusterSet.forcePrimaryCluster().

      The switchover will be canceled if there are REPLICA Clusters that are
      unreachable or unavailable. To continue, they must either be restored or
      invalidated by including their name in the 'invalidateReplicaClusters'
      option. Invalidated REPLICA Clusters must be either removed from the
      Cluster or restored and rejoined, using removeCluster() or
      rejoinCluster().

      Additionally, if any available REPLICA Cluster has members that are not
      ONLINE and/or reachable, these members will not be in a properly
      configured state even after being restored and rejoined. To ensure
      failover works correctly, rejoinCluster() must be called on the Cluster
      once these members are rejoined.

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.
      - timeout: integer value to set the maximum number of seconds to wait for
        the synchronization of the Cluster and also for the instance being
        promoted to catch up with the current PRIMARY (in the case of the
        latter, the default value is retrieved from the 'dba.gtidWaitTimeout'
        shell option).
      - invalidateReplicaClusters: list of names of REPLICA Clusters that are
        unreachable or unavailable that are to be invalidated during the
        switchover.

//@<OUT> ClusterSet.forcePrimaryCluster
NAME
      forcePrimaryCluster - Performs a failover of the PRIMARY Cluster of the
                            ClusterSet.

SYNTAX
      <ClusterSet>.forcePrimaryCluster(clusterName[, options])

WHERE
      clusterName: Name of the REPLICA cluster to be promoted.
      options: Dictionary with additional parameters described below.

RETURNS
      Nothing

DESCRIPTION
      This command will perform a failover of the PRIMARY Cluster of a
      ClusterSet. The target cluster is promoted to the new PRIMARY Cluster
      while the previous PRIMARY Cluster is invalidated. The previous PRIMARY
      Cluster is presumed unavailable by the Shell, but if that is not the
      case, it is recommended that instances of that Cluster are taken down to
      avoid or minimize inconsistencies.

      The failover will be canceled if there are REPLICA Clusters that are
      unreachable or unavailable. To continue, they must either be restored or
      invalidated by including their name in the 'invalidateReplicaClusters'
      option.

      Additionally, if any available REPLICA Cluster has members that are not
      ONLINE and/or reachable, these members will not be in a properly
      configured state even after being restored and rejoined. To ensure
      failover works correctly, rejoinCluster() must be called on the Cluster
      once these members are rejoined.

      Note that because a failover may result in loss of transactions, it is
      always preferrable that the PRIMARY Cluster is restored.

      Aftermath of a Failover

      If a failover is the only viable option to recover from an outage, the
      following must be considered:

      - The topology of the ClusterSet, including a record of "who is the
        primary", is stored in the ClusterSet itself. In a failover, the
        metadata is updated to reflect the new topology and the invalidation of
        the old primary. However, if the invalidated primary is still ONLINE
        somewhere, the copy of the metadata held there will will remain
        outdated and inconsistent with the actual topology. MySQL Router
        instances that can connect to the new PRIMARY Cluster will be able to
        tell which topology is the correct one, but those instances that can
        only connect to the invalid Cluster will behave as if nothing changed.
        If applications can still update the database through such Router
        instances, there will be a "Split-Brain" and the database will become
        inconsistent. To avoid such scenario, fence the old primary from all
        traffic using Cluster.fenceAllTraffic(), or from write traffic only
        using Cluster.fenceWrites().
      - An invalidated PRIMARY Cluster that is later restored can only be
        rejoined if its GTID set has not diverged relative to the rest of the
        ClusterSet.
      - A diverged invalidated Cluster can only be removed from the ClusterSet.
        Recovery and reconciliation of transactions that only exist in that
        Cluster can only be done manually.
      - Because regular Asynchronous Replication is used between PRIMARY and
        REPLICA Clusters, any transactions at the PRIMARY that were not yet
        replicated at the time of the failover will be lost. Even if the
        original PRIMARY Cluster is restored at some point, these transactions
        would have to be recovered and reconciled manually.

      Thus, the recommended course of action in event of an outage is to always
      restore the PRIMARY Cluster if at all possible, even if a failover may be
      faster and easier in the short term.

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.
      - invalidateReplicaClusters: list of names of REPLICA Clusters that are
        unreachable or unavailable that are to be invalidated during the
        failover.
      - timeout: integer value with the maximum number of seconds to wait for
        pending transactions to be applied in each instance of the cluster
        (default value is retrieved from the 'dba.gtidWaitTimeout' shell
        option).

//@<OUT> ClusterSet.rejoinCluster
NAME
      rejoinCluster - Rejoin an invalidated Cluster back to the ClusterSet and
                      update replication.

SYNTAX
      <ClusterSet>.rejoinCluster(clusterName[, options])

WHERE
      clusterName: Name of the Cluster to be rejoined.
      options: Dictionary with additional parameters described below.

RETURNS
      Nothing

DESCRIPTION
      Rejoins a Cluster that was invalidated as part of a failover or
      switchover, if possible. This can also be used to update replication
      channel in REPLICA Clusters, if it does not have the expected state,
      source and settings.

      The PRIMARY Cluster of the ClusterSet must be reachable and available
      during the operation.

      Pre-Requisites

      The following pre-requisites are expected for Clusters rejoined to a
      ClusterSet. They will be automatically checked by rejoinCluster(), which
      will stop if any issues are found.

      - The target Cluster must belong to the ClusterSet metadata and be
        reachable.
      - The target Cluster must not be an active member of the ClusterSet.
      - The target Cluster must not be holding any Metadata or InnoDB
        transaction lock.
      - The target Cluster's transaction set must not contain transactions
        that don't exist in the PRIMARY Cluster.
      - The target Cluster's transaction set must not be missing transactions
        that have been purged from the PRIMARY Cluster.
      - The target Cluster's executed transaction set (GTID_EXECUTED) must not
        be empty.

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.

//@<OUT> Status
NAME
      status - Describe the status of the ClusterSet.

SYNTAX
      <ClusterSet>.status([options])

WHERE
      options: Dictionary with additional parameters described below.

RETURNS
      A JSON object describing the status of the ClusterSet and its members.

DESCRIPTION
      This function describes the status of the ClusterSet including its
      members (Clusters).

      The function will gather state information from each member of the
      ClusterSet and the replication channel of it to produce a status report
      of the ClusterSet as a whole.

      Options

      The following options may be given to control the amount of information
      gathered and returned.

      - extended: verbosity level of the command output. Default is 0.

      Option 'extended' may have the following values:

      - 0: regular level of details. Only basic information about the status of
        the ClusterSet and Cluster members.
      - 1: includes basic information about the status of each cluster,
        information about each cluster member role and state as reported by
        Group Replication, and information about the ClusterSet Replication
        channel.
      - 2: includes the list of the fenced system variables, applier worker
        threads, member ID, etc. The information about the ClusterSet
        Replication channel is extended to include information about the
        applier queue size, applier queue GTID set, coordinator state, etc.
      - 3: includes important replication related configuration settings, such
        as replication delay, heartbeat delay, retry count and connection retry
        for the ClusterSet replication channel.

//@<OUT> Describe
NAME
      describe - Describe the structure of the ClusterSet.

SYNTAX
      <ClusterSet>.describe()

RETURNS
      A JSON object describing the structure of the ClusterSet.

DESCRIPTION
      This function describes the status of the ClusterSet including its
      members (Clusters).

      This function describes the structure of the ClusterSet including all its
      information and Clusters belonging to it.

      The returned JSON object contains the following attributes:

      - domainName: The ClusterSet domain name
      - primaryCluster: The current primary Cluster of the ClusterSet
      - clusters: the list of members of the ClusterSet

      The clusters JSON object contains the following attributes:

      - clusterRole: The role of the Cluster
      - a list of dictionaries describing each instance belonging to the
        Cluster.

      Each instance dictionary contains the following attributes:

      - address: the instance address in the form of host:port
      - label: the instance name identifier

//@<OUT> listRouters
NAME
      listRouters - Lists the Router instances of the ClusterSet, or a single
                    Router instance.

SYNTAX
      <ClusterSet>.listRouters([router])

WHERE
      router: Identifier of the target router instance (e.g.
              192.168.45.70::system)

RETURNS
      A JSON object listing the Router instances registered in the ClusterSet.

DESCRIPTION
      This function lists and provides information about all Router instances
      registered on the Clusters members of the ClusterSet.

      For each router, the following information is provided, when available:

      - hostname: Hostname.
      - lastCheckIn: Timestamp of the last statistics update (check-in).
      - roPort: Read-only port (Classic protocol).
      - roXPort: Read-only port (X protocol).
      - rwPort: Read-write port (Classic protocol).
      - rwSplitPort: Read-write split port (Classic protocol).
      - rwXPort: Read-write port (X protocol).
      - targetCluster: Target Cluster for Router routing operations.
      - version: Version.

//@<OUT> routingOptions
NAME
      routingOptions - Lists the ClusterSet Routers configuration options.

SYNTAX
      <ClusterSet>.routingOptions([router])

WHERE
      router: Identifier of the router instance to query for the options.

RETURNS
      A JSON object describing the configuration options of all router
      instances of the ClusterSet and its global options or just the given
      Router.

DESCRIPTION
      This function lists the Router configuration options of all Routers of
      the ClusterSet or the target Router.

//@<OUT> setRoutingOption
NAME
      setRoutingOption - Changes the value of either a global Routing option or
                         of a single Router instance.

SYNTAX
      <ClusterSet>.setRoutingOption([router], option, value)

WHERE
      router: Identifier of the target router instance (e.g.
              192.168.45.70::system).
      option: The Router option to be changed.
      value: The value that the option shall get (or null to unset).

RETURNS
      Nothing.

DESCRIPTION
      The accepted options are:

      - target_cluster: Target Cluster for Router routing operations. Default
        value is 'primary'.
      - invalidated_cluster_policy: Routing policy to be taken when the target
        cluster is detected as being invalidated. Default value is 'drop_all'.
      - stats_updates_frequency: Number of seconds between updates that the
        Router is to make to its statistics in the InnoDB Cluster metadata.
      - use_replica_primary_as_rw: Enable/Disable the RW Port in Replica
        Clusters. Disabled by default.
      - tags: Associates an arbitrary JSON object with custom key/value pairs
        with the ClusterSet metadata.
      - read_only_targets: Routing policy to define Router's usage of Read
        Replicas. Default is 'append'.

      The target_cluster option supports the following values:

      - primary: Follow the Primary Cluster whenever it changes in runtime (
        default).
      - <clusterName>: Use the Cluster named <clusterName> as target.

      The invalidated_cluster_policy option supports the following values:

      - accept_ro: all the RW connections are be dropped and no new RW
        connections are be accepted. RO connections keep being accepted and
        handled.
      - drop_all: all connections to the target Cluster are closed and no new
        connections will be accepted (default).

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

      The use_replica_primary_as_rw option accepts a boolean value to configure
      whether the Router should enable or disable the RW Port for the target
      Cluster.

      When enabled, forces the RW port of Routers targeting a specific Cluster
      (target_cluster != 'primary') to always route to the PRIMARY of that
      Cluster, even when it is in a REPLICA cluster and thus, read-only. By
      default, the option is false and Router blocks connections to the RW port
      in this scenario.

      The read_only_targets option supports the following values:

      - all: All Read Replicas of the target Cluster should be used along the
        other SECONDARY Cluster members for R/O traffic.
      - read_replicas: Only Read Replicas of the target Cluster should be used
        for R/O traffic.
      - secondaries: Only Secondary members of the target Cluster should be
        used for R/O traffic (default).

//@<OUT> setOption
NAME
      setOption - Changes the value of an option for the whole ClusterSet.

SYNTAX
      <ClusterSet>.setOption(option, value)

WHERE
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

DESCRIPTION
      This function changes an option for the ClusterSet.

      The accepted options are:

      - replicationAllowedHost string value to use as the host name part of
        internal replication accounts. Existing accounts will be re-created
        with the new value.

//@<OUT> options
NAME
      options - Lists the ClusterSet configuration options.

SYNTAX
      <ClusterSet>.options()

RETURNS
      A JSON object describing the configuration options of the ClusterSet.

DESCRIPTION
      This function lists the configuration options for the ClusterSet.



//@<OUT> setupAdminAccount
NAME
      setupAdminAccount - Create or upgrade an InnoDB ClusterSet admin account.

SYNTAX
      <ClusterSet>.setupAdminAccount(user, options)

WHERE
      user: Name of the InnoDB ClusterSet administrator account.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function creates/upgrades a MySQL user account with the necessary
      privileges to administer an InnoDB ClusterSet.

      This function also allows a user to upgrade an existing admin account
      with the necessary privileges before a dba.upgradeMetadata() call.

      The mandatory argument user is the name of the MySQL account we want to
      create or upgrade to be used as Administrator account. The accepted
      format is username[@host] where the host part is optional and if not
      provided defaults to '%'.

      The options dictionary may contain the following attributes:

      - password: The password for the InnoDB ClusterSet administrator account.
      - passwordExpiration: Password expiration setting for the account. May be
        set to the number of days for expiration, 'NEVER' to disable expiration
        and 'DEFAULT' to use the system default.
      - requireCertIssuer: Optional SSL certificate issuer for the account.
      - requireCertSubject: Optional SSL certificate subject for the account.
      - dryRun: boolean value used to enable a dry run of the account setup
        process. Default value is False.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode. Deprecated.
      - update: boolean value that must be enabled to allow updating the
        privileges and/or password of existing accounts. Default value is
        False.

      If the user account does not exist, either the password,
      requireCertIssuer or requireCertSubject are mandatory.

      If the user account exists, the update option must be enabled.

      If dryRun is used, the function will display information about the
      permissions to be granted to `user` account without actually creating
      and/or performing any changes to it.

      The interactive option can be used to explicitly enable or disable the
      interactive prompts that help the user through the account setup process.

      To change authentication options for an existing account, set `update` to
      `true`. It is possible to change password without affecting certificate
      options or vice-versa but certificate options can only be changed
      together.

      ATTENTION: The interactive option will be removed in a future release.

//@<OUT> setupRouterAccount
NAME
      setupRouterAccount - Create or upgrade a MySQL account to use with MySQL
                           Router.

SYNTAX
      <ClusterSet>.setupRouterAccount(user, options)

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
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode. Deprecated.
      - update: boolean value that must be enabled to allow updating the
        privileges and/or password of existing accounts. Default value is
        False.

      If the user account does not exist, either the password,
      requireCertIssuer or requireCertSubject are mandatory.

      If the user account exists, the update option must be enabled.

      If dryRun is used, the function will display information about the
      permissions to be granted to `user` account without actually creating
      and/or performing any changes to it.

      The interactive option can be used to explicitly enable or disable the
      interactive prompts that help the user through the account setup process.

      To change authentication options for an existing account, set `update` to
      `true`. It is possible to change password without affecting certificate
      options or vice-versa but certificate options can only be changed
      together.

      ATTENTION: The interactive option will be removed in a future release.
