//@<OUT> Object Help
NAME
      ReplicaSet - Represents an InnoDB ReplicaSet.

DESCRIPTION
      The ReplicaSet object is used to manage MySQL server topologies that use
      asynchronous replication. It can be created using the
      dba.createReplicaSet() or dba.getReplicaSet() functions.

PROPERTIES
      name
            Returns the name of the replicaset.

FUNCTIONS
      addInstance(instance[, options])
            Adds an instance to the replicaset.

      describe()
            Describe the structure of the ReplicaSet.

      disconnect()
            Disconnects all internal sessions used by the replicaset object.

      dissolve(options)
            Dissolves the ReplicaSet.

      forcePrimaryInstance(instance, options)
            Performs a failover in a replicaset with an unavailable PRIMARY.

      getName()
            Returns the name of the replicaset.

      help([member])
            Provides help about this class and it's members

      listRouters([options])
            Lists the Router instances.

      options()
            Lists the ReplicaSet configuration options.

      rejoinInstance(instance[, options])
            Rejoins an instance to the replicaset.

      removeInstance(instance[, options])
            Removes an Instance from the replicaset.

      removeRouterMetadata(routerDef)
            Removes metadata for a router instance.

      rescan(options)
            Rescans the ReplicaSet.

      routingOptions([router])
            Lists the ReplicaSet Routers configuration options.

      setInstanceOption(instance, option, value)
            Changes the value of an option in a ReplicaSet member.

      setOption(option, value)
            Changes the value of an option for the whole ReplicaSet.

      setPrimaryInstance(instance, options)
            Performs a safe PRIMARY switchover, promoting the given instance.

      setRoutingOption([router], option, value)
            Changes the value of either a global ReplicaSet Routing option or
            of a single Router instance.

      setupAdminAccount(user, options)
            Create or upgrade an InnoDB ReplicaSet admin account.

      setupRouterAccount(user, options)
            Create or upgrade a MySQL account to use with MySQL Router.

      status([options])
            Describe the status of the ReplicaSet.

      For more help on a specific function, use the \help shell command, e.g.:
      \help ReplicaSet.addInstance

//@<OUT> Name
NAME
      name - Returns the name of the replicaset.

SYNTAX
      <ReplicaSet>.name

//@<OUT> Add Instance
NAME
      addInstance - Adds an instance to the replicaset.

SYNTAX
      <ReplicaSet>.addInstance(instance[, options])

WHERE
      instance: host:port of the target instance to be added.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function adds the given MySQL instance as a read-only SECONDARY
      replica of the current PRIMARY of the replicaset. Replication will be
      configured between the added instance and the PRIMARY. Replication will
      use an automatically created MySQL account with a random password.

      Once the instance is added to the replicaset, the function will wait for
      it to apply all pending transactions. The transaction sync may timeout if
      the transaction backlog is large. The timeout option may be used to
      increase that time.

      The Shell connects to the target instance using the same username and
      password used to obtain the replicaset handle object. All instances of
      the replicaset are expected to have this same admin account with the same
      grants and passwords. A custom admin account with the required grants can
      be created while an instance is configured with
      dba.configureReplicaSetInstance(), but the root user may be used too.

      The PRIMARY of the replicaset must be reachable and available during this
      operation.

      Pre-Requisites

      The following pre-requisites are expected for instances added to a
      replicaset. They will be automatically checked by addInstance(), which
      will stop if any issues are found.

      - binary log and replication related options must have been validated
        and/or configured by dba.configureReplicaSetInstance()

      If the selected recovery method is incremental:

      - transaction set in the instance being added must not contain
        transactions that don't exist in the PRIMARY
      - transaction set in the instance being added must not be missing
        transactions that have been purged from the binary log of the PRIMARY

      If clone is available, the pre-requisites listed above can be overcome by
      using clone as the recovery method.

      Options

      The options dictionary may contain the following values:

      - dryRun: if true, performs checks and logs changes that would be made,
        but does not execute them
      - label: an identifier for the instance being added, used in the output
        of status()
      - recoveryMethod: Preferred method of state recovery. May be auto, clone
        or incremental. Default is auto.
      - waitRecovery: Integer value to indicate the recovery process verbosity
        level.
      - recoveryProgress: Integer value to indicate the recovery process
        verbosity level.
      - cloneDonor: host:port of an existing replicaSet member to clone from.
        IPv6 addresses are not supported for this option.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode. Deprecated.
      - timeout: timeout in seconds for transaction sync operations; 0 disables
        timeout and force the Shell to wait until the transaction sync
        finishes. Defaults to 0.
      - certSubject: instance's certificate subject to use when
        'memberAuthType' contains "CERT_SUBJECT".
      - replicationConnectRetry: integer that specifies the interval in seconds
        between the reconnection attempts that the replica makes after the
        connection to the source times out.
      - replicationRetryCount: integer that sets the maximum number of
        reconnection attempts that the replica makes after the connection to
        the source times out.
      - replicationHeartbeatPeriod: decimal that controls the heartbeat
        interval, which stops the connection timeout occurring in the absence
        of data if the connection is still good.
      - replicationCompressionAlgorithms: string that specifies the permitted
        compression algorithms for connections to the replication source.
      - replicationZstdCompressionLevel: integer that specifies the compression
        level to use for connections to the replication source server that use
        the zstd compression algorithm.
      - replicationBind: string that determines which of the replica's network
        interfaces is chosen for connecting to the source.
      - replicationNetworkNamespace: string that specifies the network
        namespace to use for TCP/IP connections to the replication source
        server.

      The recoveryMethod option supports the following values:

      - incremental: waits until the new instance has applied missing
        transactions from the PRIMARY
      - clone: uses MySQL clone to provision the instance, which completely
        replaces the state of the target instance with a full snapshot of
        another ReplicaSet member. Requires MySQL 8.0.17 or newer.
      - auto: compares the transaction set of the instance with that of the
        PRIMARY to determine if incremental recovery is safe to be
        automatically chosen as the most appropriate recovery method. A prompt
        will be shown if not possible to safely determine a safe way forward.
        If interaction is disabled, the operation will be canceled instead.

      If recoveryMethod is not specified 'auto' will be used by default.

      The waitRecovery option supports the following values:

      - 0: not supported.
      - 1: do not show any progress information.
      - 2: show detailed static progress information.
      - 3: show detailed dynamic progress information using progress bars.

      The recoveryProgress option supports the following values:

      - 0: do not show any progress information.
      - 1: show detailed static progress information.
      - 2: show detailed dynamic progress information using progress bars.

      By default, if the standard output on which the Shell is running refers
      to a terminal, the waitRecovery option has the value of 3. Otherwise, it
      has the value of 2.

      The cloneDonor option is used to override the automatic selection of a
      donor to be used when clone is selected as the recovery method. By
      default, a SECONDARY member will be chosen as donor. If no SECONDARY
      members are available the PRIMARY will be selected. The option accepts
      values in the format: 'host:port'. IPv6 addresses are not supported.

      ATTENTION: The waitRecovery option will be removed in a future release.
                 Please use the recoveryProgress option instead.

      ATTENTION: The interactive option will be removed in a future release.

//@<OUT> Disconnect
NAME
      disconnect - Disconnects all internal sessions used by the replicaset
                   object.

SYNTAX
      <ReplicaSet>.disconnect()

RETURNS
      Nothing.

DESCRIPTION
      Disconnects the internal MySQL sessions used by the replicaset to query
      for metadata and replication information.

//@<OUT> Dissolve
NAME
      dissolve - Dissolves the ReplicaSet.

SYNTAX
      <ReplicaSet>.dissolve(options)

WHERE
      options: Dictionary with options for the operation.

RETURNS
      Nothing

DESCRIPTION
      This function stops and deletes asynchronous replication channels and
      unregisters all members from the ReplicaSet metadata.

      It keeps all the user's data intact.

      Options

      The options dictionary may contain the following attributes:

      - force: set to true to confirm that the dissolve operation must be
        executed, even if some members of the ReplicaSet cannot be reached or
        the timeout was reached when waiting for members to catch up with
        replication changes. By default, set to false.
      - timeout: maximum number of seconds to wait for pending transactions to
        be applied in each reachable instance of the ReplicaSet (default value
        is retrieved from the 'dba.gtidWaitTimeout' shell option).

      The force option (set to true) must only be used to dissolve a ReplicaSet
      with instances that are permanently not available (no longer reachable)
      or never to be reused again in a ReplicaSet. This allows a ReplicaSet to
      be dissolved and remove it from the metadata, including instances than
      can no longer be recovered. Otherwise, the instances must be brought back
      ONLINE and the ReplicaSet dissolved without the force option to avoid
      errors trying to reuse the instances and add them back to a ReplicaSet.

//@<OUT> Force Primary Instance
NAME
      forcePrimaryInstance - Performs a failover in a replicaset with an
                             unavailable PRIMARY.

SYNTAX
      <ReplicaSet>.forcePrimaryInstance(instance, options)

WHERE
      instance: host:port of the target instance to be promoted. If blank, a
                suitable instance will be selected automatically.
      options: Dictionary with additional options.

RETURNS
      Nothing

DESCRIPTION
      This command will perform a failover of the PRIMARY of a replicaset in
      disaster scenarios where the current PRIMARY is unavailable and cannot be
      restored. If given, the target instance will be promoted to a PRIMARY,
      while other reachable SECONDARY instances will be switched to the new
      PRIMARY. The target instance must have the most up-to-date GTID_EXECUTED
      set among reachable instances, otherwise the operation will fail. If a
      target instance is not given (or is null), the most up-to-date instance
      will be automatically selected and promoted.

      After a failover, the old PRIMARY will be considered invalid by the new
      PRIMARY and can no longer be part of the replicaset. If the instance is
      still usable, it must be removed from the replicaset and re-added as a
      new instance. If there were any SECONDARY instances that could not be
      switched to the new PRIMARY during the failover, they will also be
      considered invalid.

      ATTENTION: a failover is a potentially destructive action and should only
                 be used as a last resort measure.

      Data loss is possible after a failover, because the old PRIMARY may have
      had transactions that were not yet replicated to the SECONDARY being
      promoted. Moreover, if the instance that was presumed to have failed is
      still able to process updates, for example because the network where it
      is located is still functioning but unreachable from the shell, it will
      continue diverging from the promoted clusters. Recovering or
      re-conciliating diverged transaction sets requires manual intervention
      and may some times not be possible, even if the failed MySQL servers can
      be recovered. In many cases, the fastest and simplest way to recover from
      a disaster that required a failover is by discarding such diverged
      transactions and re-provisioning a new instance from the newly promoted
      PRIMARY.

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.
      - timeout: integer value with the maximum number of seconds to wait until
        the instance being promoted catches up to the current PRIMARY.
      - invalidateErrorInstances: if false, aborts the failover if any instance
        other than the old master is unreachable or has errors. If true, such
        instances will not be failed over and be invalidated.

//@<OUT> Get Name
NAME
      getName - Returns the name of the replicaset.

SYNTAX
      <ReplicaSet>.getName()

RETURNS
      name of the replicaset.

//@<OUT> Help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <ReplicaSet>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Rejoin Instance
NAME
      rejoinInstance - Rejoins an instance to the replicaset.

SYNTAX
      <ReplicaSet>.rejoinInstance(instance[, options])

WHERE
      instance: host:port of the target instance to be rejoined.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function rejoins the given MySQL instance to the replicaset, making
      it a read-only SECONDARY replica of the current PRIMARY. Only instances
      previously added to the replicaset can be rejoined, otherwise the
      <ReplicaSet>.addInstance() function must be used to add a new instance.
      This can also be used to update the replication channel of instances
      (even if they are already ONLINE) if it does not have the expected state,
      source and settings.

      The PRIMARY of the replicaset must be reachable and available during this
      operation.

      Pre-Requisites

      The following pre-requisites are expected for instances rejoined to a
      replicaset. They will be automatically checked by rejoinInstance(), which
      will stop if any issues are found.

      If the selected recovery method is incremental:

      - transaction set in the instance being rejoined must not contain
        transactions that don't exist in the PRIMARY
      - transaction set in the instance being rejoined must not be missing
        transactions that have been purged from the binary log of the PRIMARY
      - executed transactions set (GTID_EXECUTED) in the instance being
        rejoined must not be empty.

      If clone is available, the pre-requisites listed above can be overcome by
      using clone as the recovery method.

      Options

      The options dictionary may contain the following values:

      - dryRun: if true, performs checks and logs changes that would be made,
        but does not execute them
      - recoveryMethod: Preferred method of state recovery. May be auto, clone
        or incremental. Default is auto.
      - waitRecovery: Integer value to indicate the recovery process verbosity
        level.
      - recoveryProgress: Integer value to indicate the recovery process
        verbosity level.
      - cloneDonor: host:port of an existing replicaSet member to clone from.
        IPv6 addresses are not supported for this option.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode. Deprecated.
      - timeout: timeout in seconds for transaction sync operations; 0 disables
        timeout and force the Shell to wait until the transaction sync
        finishes. Defaults to 0.

      The recoveryMethod option supports the following values:

      - incremental: waits until the rejoining instance has applied missing
        transactions from the PRIMARY
      - clone: uses MySQL clone to provision the instance, which completely
        replaces the state of the target instance with a full snapshot of
        another ReplicaSet member. Requires MySQL 8.0.17 or newer.
      - auto: compares the transaction set of the instance with that of the
        PRIMARY to determine if incremental recovery is safe to be
        automatically chosen as the most appropriate recovery method. A prompt
        will be shown if not possible to safely determine a safe way forward.
        If interaction is disabled, the operation will be canceled instead.

      If recoveryMethod is not specified 'auto' will be used by default.

      The waitRecovery option supports the following values:

      - 0: not supported.
      - 1: do not show any progress information.
      - 2: show detailed static progress information.
      - 3: show detailed dynamic progress information using progress bars.

      The recoveryProgress option supports the following values:

      - 0: do not show any progress information.
      - 1: show detailed static progress information.
      - 2: show detailed dynamic progress information using progress bars.

      By default, if the standard output on which the Shell is running refers
      to a terminal, the waitRecovery option has the value of 3. Otherwise, it
      has the value of 2.

      The cloneDonor option is used to override the automatic selection of a
      donor to be used when clone is selected as the recovery method. By
      default, a SECONDARY member will be chosen as donor. If no SECONDARY
      members are available the PRIMARY will be selected. The option accepts
      values in the format: 'host:port'. IPv6 addresses are not supported.

      ATTENTION: The waitRecovery option will be removed in a future release.
                 Please use the recoveryProgress option instead.

      ATTENTION: The interactive option will be removed in a future release.

//@<OUT> Remove Instance
NAME
      removeInstance - Removes an Instance from the replicaset.

SYNTAX
      <ReplicaSet>.removeInstance(instance[, options])

WHERE
      instance: host:port of the target instance to be removed
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function removes an Instance from the replicaset and stops it from
      replicating. The instance must not be the PRIMARY and it is expected to
      be reachable.

      The PRIMARY of the replicaset must be reachable and available during this
      operation.

      Options

      The instance definition is the connection data for the instance.

      The options dictionary may contain the following attributes:

      - force: boolean, indicating if the instance must be removed (even if
        only from metadata) in case it cannot be reached. By default, set to
        false.
      - timeout: maximum number of seconds to wait for the instance to sync up
        with the PRIMARY. 0 means no timeout and <0 will skip sync.

      The force option (set to true) is required to remove instances that are
      unreachable. Removed instances are normally synchronized with the rest of
      the replicaset, so that their own copy of the metadata will be consistent
      and the instance will be suitable for being added back to the replicaset.
      When the force option is set during removal, that step will be skipped.

//@<OUT> Set Primary Instance
NAME
      setPrimaryInstance - Performs a safe PRIMARY switchover, promoting the
                           given instance.

SYNTAX
      <ReplicaSet>.setPrimaryInstance(instance, options)

WHERE
      instance: host:port of the target instance to be promoted.
      options: Dictionary with additional options.

RETURNS
      Nothing

DESCRIPTION
      This command will perform a safe switchover of the PRIMARY of a
      replicaset. The current PRIMARY will be demoted to a SECONDARY and made
      read-only, while the promoted instance will be made a read-write master.
      All other SECONDARY instances will be updated to replicate from the new
      PRIMARY.

      During the switchover, the promoted instance will be synchronized with
      the old PRIMARY, ensuring that all transactions present in the PRIMARY
      are applied before the topology change is committed. If that
      synchronization step takes too long or is not possible in any of the
      SECONDARY instances, the switch will be aborted. These problematic
      SECONDARY instances must be either repaired or removed from the
      replicaset for the fail over to be possible.

      For a safe switchover to be possible, all replicaset instances must be
      reachable from the shell and have consistent transaction sets. If the
      PRIMARY is not available, a failover must be performed instead, using
      forcePrimaryInstance().

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.
      - timeout: integer value with the maximum number of seconds to wait until
        the instance being promoted catches up to the current PRIMARY (default
        value is retrieved from the 'dba.gtidWaitTimeout' shell option).

//@<OUT> setupAdminAccount
NAME
      setupAdminAccount - Create or upgrade an InnoDB ReplicaSet admin account.

SYNTAX
      <ReplicaSet>.setupAdminAccount(user, options)

WHERE
      user: Name of the InnoDB ReplicaSet administrator account.
      options: Dictionary with options for the operation.

RETURNS
      Nothing.

DESCRIPTION
      This function creates/upgrades a MySQL user account with the necessary
      privileges to administer an InnoDB ReplicaSet.

      This function also allows a user to upgrade an existing admin account
      with the necessary privileges before a dba.upgradeMetadata() call.

      The mandatory argument user is the name of the MySQL account we want to
      create or upgrade to be used as Administrator account. The accepted
      format is username[@host] where the host part is optional and if not
      provided defaults to '%'.

      The options dictionary may contain the following attributes:

      - password: The password for the InnoDB ReplicaSet administrator account.
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
      <ReplicaSet>.setupRouterAccount(user, options)

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

//@<OUT> Rescan
NAME
      rescan - Rescans the ReplicaSet.

SYNTAX
      <ReplicaSet>.rescan(options)

WHERE
      options: Dictionary with options for the operation.

RETURNS
      Nothing

DESCRIPTION
      This function re-scans the ReplicaSet for new (already part of the
      replication topology but not managed in the ReplicaSet) and obsolete (no
      longer part of the topology) replication members/instances, as well as
      change to the instances configurations.

      Options

      The options dictionary may contain the following attributes:

      - addUnmanaged: if true, all newly discovered instances will be
        automatically added to the metadata. Defaults to false.
      - removeObsolete: if true, all obsolete instances will be automatically
        removed from the metadata. Defaults to false.

//@<OUT> routingOptions
NAME
      routingOptions - Lists the ReplicaSet Routers configuration options.

SYNTAX
      <ReplicaSet>.routingOptions([router])

WHERE
      router: Identifier of the router instance to query for the options.

RETURNS
      A JSON object describing the configuration options of all router
      instances of the ReplicaSet and its global options or just the given
      Router.

DESCRIPTION
      This function lists the Router configuration options of all Routers of
      the ReplicaSet or the target Router.

//@<OUT> setRoutingOption
NAME
      setRoutingOption - Changes the value of either a global ReplicaSet
                         Routing option or of a single Router instance.

SYNTAX
      <ReplicaSet>.setRoutingOption([router], option, value)

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
        with the ReplicaSet metadata.
      - stats_updates_frequency: Number of seconds between updates that the
        Router is to make to its statistics in the InnoDB Cluster metadata.

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

//@<OUT> Describe
NAME
      describe - Describe the structure of the ReplicaSet.

SYNTAX
      <ReplicaSet>.describe()

RETURNS
      A JSON object describing the structure of the ReplicaSet.

DESCRIPTION
      This function describes the structure of the ReplicaSet including all its
      Instances.

      The returned JSON object contains the following attributes:

      - name: the ReplicaSet name
      - topology: a list of dictionaries describing each instance belonging to
        the ReplicaSet.

      Each instance dictionary contains the following attributes:

      - address: the instance address in the form of host:port
      - label: the instance name identifier
      - instanceRole: the instance role (either "PRIMARY" or "REPLICA")

//@<OUT> Status
NAME
      status - Describe the status of the ReplicaSet.

SYNTAX
      <ReplicaSet>.status([options])

WHERE
      options: Dictionary with options.

RETURNS
      A JSON object describing the status of the ReplicaSet.

DESCRIPTION
      This function will connect to each member of the ReplicaSet and query
      their state, producing a status report of the ReplicaSet as a whole, as
      well as of its individual members.

      Options

      The following options may be given to control the amount of information
      gathered and returned.

      - extended: Can be 0, 1 or 2. Default is 0.

      Option 'extended' may have the following values:

      - 0 - regular level of details. Only basic information about the status
        of the instance and replication is included, in addition to non-default
        or unexpected replication settings and status.
      - 1 - includes Metadata Version, server UUID and the raw information used
        to derive the status of the instance, size of the applier queue, value
        of system variables that protect against unexpected writes etc.
      - 2 - includes important replication related configuration settings, such
        as SSL, worker threads, replication delay and heartbeat delay.

//@<OUT> options
NAME
      options - Lists the ReplicaSet configuration options.

SYNTAX
      <ReplicaSet>.options()

RETURNS
      A JSON object describing the configuration options of the ReplicaSet.

DESCRIPTION
      This function lists the configuration options for the ReplicaSet and its
      instances.

//@<OUT> setOption
NAME
      setOption - Changes the value of an option for the whole ReplicaSet.

SYNTAX
      <ReplicaSet>.setOption(option, value)

WHERE
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

DESCRIPTION
      This function changes an option for the ReplicaSet.

      The accepted options are:

      - replicationAllowedHost string value to use as the host name part of
        internal replication accounts. Existing accounts will be re-created
        with the new value.
      - tag:<option>: built-in and user-defined tags to be associated to the
        Cluster.

      Tags

      Tags make it possible to associate custom key/value pairs to a
      ReplicaSet, storing them in its metadata. Custom tag names can be any
      string starting with letters and followed by letters, numbers and _. Tag
      values may be any JSON value. If the value is null, the tag is deleted.

//@<OUT> setInstanceOption
NAME
      setInstanceOption - Changes the value of an option in a ReplicaSet
                          member.

SYNTAX
      <ReplicaSet>.setInstanceOption(instance, option, value)

WHERE
      instance: host:port of the target instance.
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

DESCRIPTION
      This function changes an option for a member of the ReplicaSet.

      The accepted options are:

      - tag:<option>: built-in and user-defined tags to be associated to the
        Cluster.
      - replicationConnectRetry: integer that specifies the interval in seconds
        between the reconnection attempts that the replica makes after the
        connection to the source times out.
      - replicationRetryCount: integer that sets the maximum number of
        reconnection attempts that the replica makes after the connection to
        the source times out.
      - replicationHeartbeatPeriod: decimal that controls the heartbeat
        interval, which stops the connection timeout occurring in the absence
        of data if the connection is still good.
      - replicationCompressionAlgorithms: string that specifies the permitted
        compression algorithms for connections to the replication source.
      - replicationZstdCompressionLevel: integer that specifies the compression
        level to use for connections to the replication source server that use
        the zstd compression algorithm.
      - replicationBind: string that determines which of the replica's network
        interfaces is chosen for connecting to the source.
      - replicationNetworkNamespace: string that specifies the network
        namespace to use for TCP/IP connections to the replication source
        server.

      NOTE: Changing any of the "replication*" options won't immediately update
            the replication channel. Only on the next call to rejoinInstance()
            will the updated options take effect.

      NOTE: Any of the "replication*" options accepts 'null', which sets the
            corresponding option to its default value on the next call to
            rejoinInstance().

      Tags

      Tags make it possible to associate custom key/value pairs to a
      ReplicaSet, storing them in its metadata. Custom tag names can be any
      string starting with letters and followed by letters, numbers and _. Tag
      values may be any JSON value. If the value is null, the tag is deleted.

      The following pre-defined tags are available:

      - _hidden: bool - instructs the router to exclude the instance from the
        list of possible destinations for client applications.
      - _disconnect_existing_sessions_when_hidden: bool - instructs the router
        to disconnect existing connections from instances that are marked to be
        hidden.

      NOTE: '_hidden' and '_disconnect_existing_sessions_when_hidden' can be
            useful to shut down the instance and perform maintenance on it
            without disrupting incoming application traffic.

//@<OUT> listRouters
NAME
      listRouters - Lists the Router instances.

SYNTAX
      <ReplicaSet>.listRouters([options])

WHERE
      options: Dictionary with options for the operation.

RETURNS
      A JSON object listing the Router instances associated to the ReplicaSet.

DESCRIPTION
      This function lists and provides information about all Router instances
      registered for the ReplicaSet.

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
