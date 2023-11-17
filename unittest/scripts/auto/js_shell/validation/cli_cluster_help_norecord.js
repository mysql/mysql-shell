//@<OUT> CLI cluster --help
The following operations are available at 'cluster':

   add-instance
      Adds an Instance to the cluster.

   add-replica-instance
      Adds a Read Replica Instance to the Cluster.

   check-instance-state
      Verifies the instance gtid state in relation to the cluster.

      ATTENTION: This function is deprecated and will be removed in a future
                 release of MySQL Shell.

   create-cluster-set
      Creates a MySQL InnoDB ClusterSet from an existing standalone InnoDB
      Cluster.

   describe
      Describe the structure of the Cluster.

   dissolve
      Dissolves the cluster.

   fence-all-traffic
      Fences a Cluster from All Traffic.

   fence-writes
      Fences a Cluster from Write Traffic.

   force-quorum-using-partition-of
      Restores the cluster from quorum loss.

   list-routers
      Lists the Router instances.

   options
      Lists the cluster configuration options.

   rejoin-instance
      Rejoins an Instance to the cluster.

   remove-instance
      Removes an Instance from the cluster.

   remove-router-metadata
      Removes metadata for a router instance.

   rescan
      Rescans the cluster.

   reset-recovery-accounts-password
      Resets the password of the recovery and replication accounts of the
      Cluster.

   routing-options
      Lists the Cluster Routers configuration options.

   set-instance-option
      Changes the value of an option in a Cluster member.

   set-option
      Changes the value of an option for the whole Cluster.

   set-primary-instance
      Elects a specific cluster member as the new primary.

   setup-admin-account
      Create or upgrade an InnoDB Cluster admin account.

   setup-router-account
      Create or upgrade a MySQL account to use with MySQL Router.

   status
      Describe the status of the Cluster.

   switch-to-single-primary-mode
      Switches the cluster to single-primary mode.

   unfence-writes
      Unfences a Cluster.

//@<OUT> CLI cluster add-instance --help
NAME
      add-instance - Adds an Instance to the cluster.

SYNTAX
      cluster add-instance <instance> [<options>]

WHERE
      instance: Connection options for the target instance to be added.

RETURNS
      nothing

OPTIONS
--memberSslMode=<str>
            SSL mode used on the instance

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for group
            replication. Only valid if communicationStack=XCOM.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one.

--groupSeeds=<str>
            String value with a comma-separated list of the Group Replication
            peer addresses to be used instead of the automatically generated
            one. Deprecated and ignored.

--exitStateAction=<str>
            String value indicating the group replication exit state action.

--memberWeight=<int>
            Integer value with a percentage weight for automatic primary
            election on failover.

--autoRejoinTries=<int>
            Integer value to define the number of times an instance will
            attempt to rejoin the cluster after being expelled.

--recoveryMethod=<str>
            Preferred method of state recovery. May be auto, clone or
            incremental. Default is auto.

--label=<str>
            An identifier for the instance being added

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.

--certSubject=<str>
            Instance's certificate subject to use when 'memberAuthType'
            contains "CERT_SUBJECT".

//@<OUT> CLI cluster check-instance-state --help
NAME
      check-instance-state - Verifies the instance gtid state in relation to
                             the cluster.

SYNTAX
      cluster check-instance-state <instance>

WHERE
      instance: An instance definition.

RETURNS
      resultset A JSON object with the status.

//@<OUT> CLI cluster describe --help
NAME
      describe - Describe the structure of the Cluster.

SYNTAX
      cluster describe

RETURNS
      A JSON object describing the structure of the Cluster.

//@<OUT> CLI cluster dissolve --help
NAME
      dissolve - Dissolves the cluster.

SYNTAX
      cluster dissolve [<options>]

RETURNS
      Nothing.

OPTIONS
--force=<bool>
            Boolean value used to confirm that the dissolve operation must be
            executed, even if some members of the cluster cannot be reached or
            the timeout was reached when waiting for members to catch up with
            replication changes. By default, set to false.

//@<OUT> CLI cluster force-quorum-using-partition-of --help
NAME
      force-quorum-using-partition-of - Restores the cluster from quorum loss.

SYNTAX
      cluster force-quorum-using-partition-of <instance> [<password>]

WHERE
      instance: An instance definition to derive the forced group from.
      password: String with the password for the connection. Deprecated.

RETURNS
      Nothing.

//@<OUT> CLI cluster list-routers --help
NAME
      list-routers - Lists the Router instances.

SYNTAX
      cluster list-routers [<options>]

RETURNS
      A JSON object listing the Router instances associated to the Cluster.

OPTIONS
--onlyUpgradeRequired=<bool>
            Boolean, enables filtering so only router instances that support
            older version of the Metadata Schema and require upgrade are
            included.

//@<OUT> CLI cluster options --help
NAME
      options - Lists the cluster configuration options.

SYNTAX
      cluster options [<options>]

RETURNS
      A JSON object describing the configuration options of the cluster.

OPTIONS
--all=<bool>
            If true, includes information about all group_replication system
            variables.

//@<OUT> CLI cluster rejoin-instance --help
NAME
      rejoin-instance - Rejoins an Instance to the cluster.

SYNTAX
      cluster rejoin-instance <instance> [<options>]

WHERE
      instance: An instance definition.

RETURNS
      A JSON object with the result of the operation.

OPTIONS
--memberSslMode=<str>
            SSL mode used on the instance

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for group
            replication. Only valid if communicationStack=XCOM.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one.

--recoveryMethod=<str>
            Preferred method of state recovery. May be auto, clone or
            incremental. Default is auto.

--cloneDonor=<str>
            The Cluster member to be used as donor when performing clone-based
            recovery. Available only for Read Replicas.

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.
            recovery process to finish and its verbosity level.

--dryRun=<bool>
            Boolean if true, all validations and steps for rejoining the
            instance are executed, but no changes are actually made.

--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up with
            the PRIMARY after it's provisioned and the replication channel is
            established. If reached, the operation is rolled-back. Default is 0
            (no timeout). Available only for Read Replicas.

//@<OUT> CLI cluster remove-instance --help
NAME
      remove-instance - Removes an Instance from the cluster.

SYNTAX
      cluster remove-instance <instance> [<options>]

WHERE
      instance: An instance definition.

RETURNS
      Nothing.

OPTIONS
--force=<bool>
            Boolean, indicating if the instance must be removed (even if only
            from metadata) in case it cannot be reached. By default, set to
            false.

--dryRun=<bool>
            Boolean if true, all validations and steps for removing the
            instance are executed, but no changes are actually made. An
            exception will be thrown when finished.

--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up with
            the PRIMARY. If reached, the operation is rolled-back. Default is 0
            (no timeout).

//@<OUT> CLI cluster remove-router-metadata --help
NAME
      remove-router-metadata - Removes metadata for a router instance.

SYNTAX
      cluster remove-router-metadata <router>

WHERE
      routerDef: identifier of the router instance to be removed (e.g.
                 192.168.45.70::system)

RETURNS
      Nothing

//@<OUT> CLI cluster rescan --help
NAME
      rescan - Rescans the cluster.

SYNTAX
      cluster rescan [<options>]

RETURNS
      Nothing.

OPTIONS
--updateTopologyMode=<bool>
            Boolean value used to indicate if the topology mode (single-primary
            or multi-primary) in the metadata should be updated (true) or not
            (false) to match the one being used by the cluster. By default, the
            metadata is not updated (false). Deprecated.

--addInstances[:<type>]=<value>
            List with the connection data of the new active instances to add to
            the metadata, or "auto" to automatically add missing instances to
            the metadata. Deprecated.

--removeInstances[:<type>]=<value>
            List with the connection data of the obsolete instances to remove
            from the metadata, or "auto" to automatically remove obsolete
            instances from the metadata. Deprecated.

--addUnmanaged=<bool>
            Set to true to automatically add newly discovered instances, i.e.
            already part of the replication topology but not managed in the
            Cluster, to the metadata. Defaults to false.

--removeObsolete=<bool>
            Set to true to automatically remove all obsolete instances, i.e. no
            longer part of the replication topology, from the metadata.
            Defaults to false.

--upgradeCommProtocol=<bool>
            Boolean. Set to true to upgrade the Group Replication communication
            protocol to the highest version possible.

//@<OUT> CLI cluster reset-recovery-accounts-password --help
NAME
      reset-recovery-accounts-password - Resets the password of the recovery
                                         and replication accounts of the
                                         Cluster.

SYNTAX
      cluster reset-recovery-accounts-password [<options>]

RETURNS
      Nothing.

OPTIONS
--force=<bool>
            Boolean, indicating if the operation will continue in case an error
            occurs when trying to reset the passwords on any of the instances,
            for example if any of them is not online. By default, set to false.

//@<OUT> CLI cluster set-instance-option --help
NAME
      set-instance-option - Changes the value of an option in a Cluster member.

SYNTAX
      cluster set-instance-option <instance> <option> <value>

WHERE
      instance: An instance definition.
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

//@<OUT> CLI cluster set-option --help
NAME
      set-option - Changes the value of an option for the whole Cluster.

SYNTAX
      cluster set-option <option> <value>

WHERE
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

//@<OUT> CLI cluster set-primary-instance --help
NAME
      set-primary-instance - Elects a specific cluster member as the new
                             primary.

SYNTAX
      cluster set-primary-instance <instance> [<options>]

WHERE
      instance: An instance definition.

RETURNS
      Nothing.

//@<OUT> CLI cluster setup-admin-account --help
NAME
      setup-admin-account - Create or upgrade an InnoDB Cluster admin account.

SYNTAX
      cluster setup-admin-account <user> [<options>]

WHERE
      user: Name of the InnoDB Cluster administrator account.

RETURNS
      Nothing.

OPTIONS
--dryRun=<bool>
            Boolean value used to enable a dry run of the account setup
            process. Default value is False.

--update=<bool>
            Boolean value that must be enabled to allow updating the privileges
            and/or password of existing accounts. Default value is False.

--requireCertIssuer=<str>
            Optional SSL certificate issuer for the account.

--requireCertSubject=<str>
            Optional SSL certificate subject for the account.

--passwordExpiration[:<type>]=<value>
            Password expiration setting for the account. May be set to the
            number of days for expiration, 'NEVER' to disable expiration and
            'DEFAULT' to use the system default.

//@<OUT> CLI cluster setup-router-account --help
NAME
      setup-router-account - Create or upgrade a MySQL account to use with
                             MySQL Router.

SYNTAX
      cluster setup-router-account <user> [<options>]

WHERE
      user: Name of the account to create/upgrade for MySQL Router.

RETURNS
      Nothing.

OPTIONS
--dryRun=<bool>
            Boolean value used to enable a dry run of the account setup
            process. Default value is False.

--update=<bool>
            Boolean value that must be enabled to allow updating the privileges
            and/or password of existing accounts. Default value is False.

--requireCertIssuer=<str>
            Optional SSL certificate issuer for the account.

--requireCertSubject=<str>
            Optional SSL certificate subject for the account.

--passwordExpiration[:<type>]=<value>
            Password expiration setting for the account. May be set to the
            number of days for expiration, 'NEVER' to disable expiration and
            'DEFAULT' to use the system default.

//@<OUT> CLI cluster status --help
NAME
      status - Describe the status of the Cluster.

SYNTAX
      cluster status [<options>]

RETURNS
      A JSON object describing the status of the Cluster.

OPTIONS
--extended=<uint>
            Verbosity level of the command output.

--queryMembers=<bool>
            If true, connect to each Instance of the Cluster to query for more
            detailed stats about the replication machinery.

//@<OUT> CLI cluster switch-to-single-primary-mode --help
NAME
      switch-to-single-primary-mode - Switches the cluster to single-primary
                                      mode.

SYNTAX
      cluster switch-to-single-primary-mode [<instance>]

WHERE
      instance: An instance definition.

RETURNS
      Nothing.

//@<OUT> CLI cluster fence-all-traffic --help
NAME
      fence-all-traffic - Fences a Cluster from All Traffic.

SYNTAX
      cluster fence-all-traffic

RETURNS
      Nothing

//@<OUT> CLI cluster fence-writes --help
NAME
      fence-writes - Fences a Cluster from Write Traffic.

SYNTAX
      cluster fence-writes

RETURNS
      Nothing

//@<OUT> CLI cluster unfence-writes --help
NAME
      unfence-writes - Unfences a Cluster.

SYNTAX
      cluster unfence-writes

RETURNS
      Nothing

//@<OUT> CLI cluster add-replica-instance --help
NAME
      add-replica-instance - Adds a Read Replica Instance to the Cluster.

SYNTAX
      cluster add-replica-instance <instance> [<options>]

WHERE
      instance: host:port of the target instance to be added as a Read Replica.

RETURNS
      nothing

OPTIONS
--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up with
            the PRIMARY after it's provisioned and the replication channel is
            established. If reached, the operation is rolled-back. Default is 0
            (no timeout).

--dryRun=<bool>
            Boolean if true, all validations and steps for creating a Read
            Replica Instance are executed, but no changes are actually made. An
            exception will be thrown when finished.

--label=<str>
            An identifier for the Read Replica Instance being added, used in
            the output of status() and describe().

--replicationSources[:<type>]=<value>
            The list of sources for the Read Replica Instance. By default, the
            list is automatically managed by Group Replication and the primary
            member is used as source.

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.

--recoveryMethod=<str>
            Preferred method for state recovery/provisioning. May be auto,
            clone or incremental. Default is auto.

--cloneDonor=<str>
            The Cluster member to be used as donor when performing clone-based
            recovery.

//@<OUT> CLI cluster routing-options --help
NAME
      routing-options - Lists the Cluster Routers configuration options.

SYNTAX
      cluster routing-options [<router>]

WHERE
      router: Identifier of the router instance to query for the options.

RETURNS
      A JSON object describing the configuration options of all router
      instances of the Cluster and its global options or just the given Router.
