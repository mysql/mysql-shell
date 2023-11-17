//@<OUT> CLI replicaset --help
The following operations are available at 'rs':

   add-instance
      Adds an instance to the replicaset.

   describe
      Describe the structure of the ReplicaSet.

   dissolve
      Dissolves the ReplicaSet.

   force-primary-instance
      Performs a failover in a replicaset with an unavailable PRIMARY.

   list-routers
      Lists the Router instances.

   options
      Lists the ReplicaSet configuration options.

   rejoin-instance
      Rejoins an instance to the replicaset.

   remove-instance
      Removes an Instance from the replicaset.

   remove-router-metadata
      Removes metadata for a router instance.

   rescan
      Rescans the ReplicaSet.

   routing-options
      Lists the ReplicaSet Routers configuration options.

   set-instance-option
      Changes the value of an option in a ReplicaSet member.

   set-option
      Changes the value of an option for the whole ReplicaSet.

   set-primary-instance
      Performs a safe PRIMARY switchover, promoting the given instance.

   setup-admin-account
      Create or upgrade an InnoDB ReplicaSet admin account.

   setup-router-account
      Create or upgrade a MySQL account to use with MySQL Router.

   status
      Describe the status of the ReplicaSet.

//@<OUT> CLI replicaset add-instance --help
NAME
      add-instance - Adds an instance to the replicaset.

SYNTAX
      rs add-instance <instance> [<options>]

WHERE
      instance: host:port of the target instance to be added.

RETURNS
      Nothing.

OPTIONS
--recoveryMethod=<str>
            Preferred method of state recovery. May be auto, clone or
            incremental. Default is auto.

--cloneDonor=<str>
            Host:port of an existing replicaSet member to clone from. IPv6
            addresses are not supported for this option.

--dryRun=<bool>
            If true, performs checks and logs changes that would be made, but
            does not execute them

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.

--timeout=<int>
            Timeout in seconds for transaction sync operations; 0 disables
            timeout and force the Shell to wait until the transaction sync
            finishes. Defaults to 0.

--label=<str>
            An identifier for the instance being added, used in the output of
            status()

--certSubject=<str>
            Instance's certificate subject to use when 'memberAuthType'
            contains "CERT_SUBJECT".

--replicationConnectRetry=<int>
            Integer that specifies the interval in seconds between the
            reconnection attempts that the replica makes after the connection
            to the source times out.

--replicationRetryCount=<int>
            Integer that sets the maximum number of reconnection attempts that
            the replica makes after the connection to the source times out.

--replicationHeartbeatPeriod=<float>
            Decimal that controls the heartbeat interval, which stops the
            connection timeout occurring in the absence of data if the
            connection is still good.

--replicationCompressionAlgorithms=<str>
            String that specifies the permitted compression algorithms for
            connections to the replication source.

--replicationZstdCompressionLevel=<int>
            Integer that specifies the compression level to use for connections
            to the replication source server that use the zstd compression
            algorithm.

--replicationBind=<str>
            String that determines which of the replica's network interfaces is
            chosen for connecting to the source.

--replicationNetworkNamespace=<str>
            String that specifies the network namespace to use for TCP/IP
            connections to the replication source server.

//@<OUT> CLI replicaset force-primary-instance --help
NAME
      force-primary-instance - Performs a failover in a replicaset with an
                               unavailable PRIMARY.

SYNTAX
      rs force-primary-instance [<instance>] [<options>]

WHERE
      instance: host:port of the target instance to be promoted. If blank, a
                suitable instance will be selected automatically.

RETURNS
      Nothing

OPTIONS
--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

--timeout=<int>
            Integer value with the maximum number of seconds to wait until the
            instance being promoted catches up to the current PRIMARY.

--invalidateErrorInstances=<bool>
            If false, aborts the failover if any instance other than the old
            master is unreachable or has errors. If true, such instances will
            not be failed over and be invalidated.

//@<OUT> CLI replicaset list-routers --help
NAME
      list-routers - Lists the Router instances.

SYNTAX
      rs list-routers [<options>]

RETURNS
      A JSON object listing the Router instances associated to the ReplicaSet.

OPTIONS
--onlyUpgradeRequired=<bool>
            Boolean, enables filtering so only router instances that support
            older version of the Metadata Schema and require upgrade are
            included.

//@<OUT> CLI replicaset options --help
NAME
      options - Lists the ReplicaSet configuration options.

SYNTAX
      rs options

RETURNS
      A JSON object describing the configuration options of the ReplicaSet.

//@<OUT> CLI replicaset rejoin-instance --help
NAME
      rejoin-instance - Rejoins an instance to the replicaset.

SYNTAX
      rs rejoin-instance <instance> [<options>]

WHERE
      instance: host:port of the target instance to be rejoined.

RETURNS
      Nothing.

OPTIONS
--recoveryMethod=<str>
            Preferred method of state recovery. May be auto, clone or
            incremental. Default is auto.

--cloneDonor=<str>
            Host:port of an existing replicaSet member to clone from. IPv6
            addresses are not supported for this option.

--dryRun=<bool>
            If true, performs checks and logs changes that would be made, but
            does not execute them

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.

--timeout=<int>
            Timeout in seconds for transaction sync operations; 0 disables
            timeout and force the Shell to wait until the transaction sync
            finishes. Defaults to 0.

//@<OUT> CLI replicaset remove-instance --help
NAME
      remove-instance - Removes an Instance from the replicaset.

SYNTAX
      rs remove-instance <instance> [<options>]

WHERE
      instance: host:port of the target instance to be removed

RETURNS
      Nothing.

OPTIONS
--force=<bool>
            Boolean, indicating if the instance must be removed (even if only
            from metadata) in case it cannot be reached. By default, set to
            false.

--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up with
            the PRIMARY. 0 means no timeout and <0 will skip sync.

//@<OUT> CLI replicaset remove-router-metadata --help
NAME
      remove-router-metadata - Removes metadata for a router instance.

SYNTAX
      rs remove-router-metadata <routerDef>

WHERE
      routerDef: identifier of the router instance to be removed (e.g.
                 192.168.45.70::system)

RETURNS
      Nothing

//@<OUT> CLI replicaset routing-options --help
NAME
      routing-options - Lists the ReplicaSet Routers configuration options.

SYNTAX
      rs routing-options [<router>]

WHERE
      router: Identifier of the router instance to query for the options.

RETURNS
      A JSON object describing the configuration options of all router
      instances of the ReplicaSet and its global options or just the given
      Router.

//@<OUT> CLI replicaset set-instance-option --help
NAME
      set-instance-option - Changes the value of an option in a ReplicaSet
                            member.

SYNTAX
      rs set-instance-option <instance> <option> <value>

WHERE
      instance: host:port of the target instance.
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

//@<OUT> CLI replicaset set-option --help
NAME
      set-option - Changes the value of an option for the whole ReplicaSet.

SYNTAX
      rs set-option <option> <value>

WHERE
      option: The option to be changed.
      value: The value that the option shall get.

RETURNS
      Nothing.

//@<OUT> CLI replicaset set-primary-instance --help
NAME
      set-primary-instance - Performs a safe PRIMARY switchover, promoting the
                             given instance.

SYNTAX
      rs set-primary-instance <instance> [<options>]

WHERE
      instance: host:port of the target instance to be promoted.

RETURNS
      Nothing

OPTIONS
--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

--timeout=<int>
            Integer value with the maximum number of seconds to wait until the
            instance being promoted catches up to the current PRIMARY (default
            value is retrieved from the 'dba.gtidWaitTimeout' shell option).

//@<OUT> CLI replicaset setup-admin-account --help
NAME
      setup-admin-account - Create or upgrade an InnoDB ReplicaSet admin
                            account.

SYNTAX
      rs setup-admin-account <user> [<options>]

WHERE
      user: Name of the InnoDB ReplicaSet administrator account.

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

//@<OUT> CLI replicaset setup-router-account --help
NAME
      setup-router-account - Create or upgrade a MySQL account to use with
                             MySQL Router.

SYNTAX
      rs setup-router-account <user> [<options>]

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

//@<OUT> CLI replicaset status --help
NAME
      status - Describe the status of the ReplicaSet.

SYNTAX
      rs status [<options>]

RETURNS
      A JSON object describing the status of the ReplicaSet.

OPTIONS
--extended=<int>
            Can be 0, 1 or 2. Default is 0.

