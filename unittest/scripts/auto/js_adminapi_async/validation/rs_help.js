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

      disconnect()
            Disconnects all internal sessions used by the replicaset object.

      forcePrimaryInstance(instance, options)
            Performs a failover in a replicaset with an unavailable PRIMARY.

      getName()
            Returns the name of the replicaset.

      help([member])
            Provides help about this class and it's members

      listRouters([options])
            Lists the Router instances.

      rejoinInstance(instance)
            Rejoins an instance to the replicaset.

      removeInstance(instance[, options])
            Removes an Instance from the replicaset.

      removeRouterMetadata(routerDef)
            Removes metadata for a router instance.

      setPrimaryInstance(instance, options)
            Performs a safe PRIMARY switchover, promoting the given instance.

      status([options])
            Describe the status of the replicaset.

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
      - transaction set in the instance being added must not contain
        transactions that don't exist in the PRIMARY
      - transaction set in the instance being added must not be missing
        transactions that have been purged from the binary log of the PRIMARY

      Options

      The options dictionary may contain the following values:

      - dryRun: if true, performs checks and logs changes that would be made,
        but does not execute them
      - label: an identifier for the instance being added, used in the output
        of status()
      - recoveryMethod: Preferred method of state recovery. May be auto or
        incremental. Default is auto.
      - interactive: if true, enables interactive password and confirmation
        prompts. Defaults to the value of the useWizards shell option.
      - timeout: timeout in seconds for transaction sync operations; 0 disables
        timeout and force the Shell to wait until the transaction sync
        finishes. Defaults to 0.

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
      This command will perform a forced failover of the PRIMARY of a
      replicaset in disaster scenarios where the current PRIMARY is unavailable
      and cannot be restored. If given, the target instance will be promoted to
      a PRIMARY, while other reachable SECONDARY instances will be switched to
      the new PRIMARY. The target instance must have the most up-to-date
      GTID_EXECUTED set among reachable instances, otherwise the operation will
      fail. If a target instance is not given (or is null), the most up-to-date
      instance will be automatically selected and promoted.

      After a forced failover, the old PRIMARY will be considered invalid by
      the new PRIMARY and can no longer be part of the replicaset. If the
      instance is still usable, it must be removed from the replicaset and
      re-added as a new instance. If there were any SECONDARY instances that
      could not be switched to the new PRIMARY during the failover, they will
      also be considered invalid.

      ATTENTION: a forced failover is a potentially destructive action and
                 should only be used as a last resort measure.

      Data loss is possible after a failover, because the old PRIMARY may have
      had transactions that were not yet replicated to the SECONDARY being
      promoted. Moreover, if the instance that was presumed to have failed is
      still able to process updates, for example because the network where it
      is located is still functioning but unreachable from the shell, it will
      continue diverging from the promoted clusters. Recovering or
      re-conciliating diverged transaction sets requires manual intervention
      and may some times not be possible, even if the failed MySQL servers can
      be recovered. In many cases, the fastest and simplest way to recover from
      a disaster that required a forced failover is by discarding such diverged
      transactions and re-provisioning a new instance from the newly promoted
      PRIMARY.

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.
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
      <ReplicaSet>.rejoinInstance(instance)

WHERE
      instance: host:port of the target instance to be rejoined.

RETURNS
      Nothing.

DESCRIPTION
      This function rejoins the given MySQL instance to the replicaset, making
      it a read-only SECONDARY replica of the current PRIMARY. Only instances
      previously added to the replicaset can be rejoined, otherwise the
      <ReplicaSet>.addInstance() function must be used to add a new instance.

      The PRIMARY of the replicaset must be reachable and available during this
      operation.

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
      - interactive: boolean value used to disable/enable the wizards in the

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
      are applied before the topology change is commited. If that
      synchronization step takes too long or is not possible in any of the
      SECONDARY instances, the switch will be aborted. These problematic
      SECONDARY instances must be either repaired or removed from the
      replicaset for the fail over to be possible.

      For a safe switchover to be possible, all replicaset instances must be
      reachable from the shell and have consistent transaction sets. If the
      PRIMARY is not available, a forced failover must be performed instead,
      using forcePrimaryInstance().

      Options

      The following options may be given:

      - dryRun: if true, will perform checks and log operations that would be
        performed, but will not execute them. The operations that would be
        performed can be viewed by enabling verbose output in the shell.
      - timeout: integer value with the maximum number of seconds to wait until
        the instance being promoted catches up to the current PRIMARY.

//@<OUT> Status
NAME
      status - Describe the status of the replicaset.

SYNTAX
      <ReplicaSet>.status([options])

WHERE
      options: Dictionary with options.

RETURNS
      A JSON object describing the status of the members of the replicaset.

DESCRIPTION
      This function will connect to each member of the replicaset and query
      their state, producing a status report of the replicaset as a whole, as
      well as of their individual members.

      Options

      The following options may be given to control the amount of information
      gathered and returned.

      - extended: Can be 0, 1 or 2. Default is 0.

      Option 'extended' may have the following values:

      - 0 - regular level of details. Only basic information about the status
        of the instance and replication is included, in addition to non-default
        or unexpected replication settings and status.
      - 1 - includes server UUID and the raw information used to derive the
        status of the instance, size of the applier queue, value of system
        variables that protect against unexpected writes etc.
      - 2 - includes important replication related configuration settings, such
        as SSL, worker threads, replication delay and heartbeat delay.

