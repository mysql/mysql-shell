//@<OUT> CLI clusterset --help
The following operations are available at 'clusterset':

   create-replica-cluster
      Creates a new InnoDB Cluster that is a Replica of the Primary Cluster.

   create-routing-guideline
      Creates a new Routing Guideline for the ClusterSet.

   describe
      Describe the structure of the ClusterSet.

   dissolve
      Dissolves the ClusterSet.

   execute
      Executes a SQL statement at selected instances of the ClusterSet.

   force-primary-cluster
      Performs a failover of the PRIMARY Cluster of the ClusterSet.

   get-routing-guideline
      Returns the named Routing Guideline.

   import-routing-guideline
      Imports a Routing Guideline from a JSON file into the ClusterSet.

   list-routers
      Lists the Router instances of the ClusterSet, or a single Router
      instance.

   options
      Lists the ClusterSet configuration options.

   rejoin-cluster
      Rejoin an invalidated Cluster back to the ClusterSet and update
      replication.

   remove-cluster
      Removes a Replica cluster from a ClusterSet.

   remove-routing-guideline
      Removes the named Routing Guideline.

   router-options
      Lists the configuration options of the ClusterSet's Routers.

   routing-guidelines
      Lists the Routing Guidelines defined for the ClusterSet.

   routing-options
      Lists the ClusterSet Routers configuration options.

      ATTENTION: This function is deprecated and will be removed in a future
                 release of MySQL Shell. Use ClusterSet.routerOptions()
                 instead.

   set-option
      Changes the value of an option for the whole ClusterSet.

   set-primary-cluster
      Performs a safe switchover of the PRIMARY Cluster of the ClusterSet.

   setup-admin-account
      Create or upgrade an InnoDB ClusterSet admin account.

   setup-router-account
      Create or upgrade a MySQL account to use with MySQL Router.

   status
      Describe the status of the ClusterSet.

//@<OUT> CLI clusterset create-replica-cluster --help
NAME
      create-replica-cluster - Creates a new InnoDB Cluster that is a Replica
                               of the Primary Cluster.

SYNTAX
      clusterset create-replica-cluster <instanceDef> <clusterName> [<options>]

WHERE
      instance: host:port of the target instance to be used to create the
                Replica Cluster
      clusterName: An identifier for the REPLICA cluster to be created.

RETURNS
      The created Replica Cluster object.

OPTIONS
--autoRejoinTries=<int>
            Integer value to define the number of times an instance will
            attempt to rejoin the cluster after being expelled.

--certSubject=<str>
            Instance's certificate subject to use when 'memberAuthType'
            contains "CERT_SUBJECT" or "CERT_SUBJECT_PASSWORD".

--cloneDonor=<str>
            Host:port of an existing member of the PRIMARY cluster to clone
            from. IPv6 addresses are not supported for this option.

--clusterSetReplicationBind=<str>
            String that determines which of the replica's network interfaces is
            chosen for connecting to the source.

--clusterSetReplicationCompressionAlgorithms=<str>
            String that specifies the permitted compression algorithms for
            connections to the replication source.

--clusterSetReplicationConnectRetry=<int>
            Integer that specifies the interval in seconds between the
            reconnection attempts that the replica makes after the connection
            to the source times out.

--clusterSetReplicationHeartbeatPeriod=<float>
            Decimal that controls the heartbeat interval, which stops the
            connection timeout occurring in the absence of data if the
            connection is still good.

--clusterSetReplicationNetworkNamespace=<str>
            String that specifies the network namespace to use for TCP/IP
            connections to the replication source server or, if the MySQL
            communication stack is in use, for Group Replication’s group
            communication connections.

--clusterSetReplicationRetryCount=<int>
            Integer that sets the maximum number of reconnection attempts that
            the replica makes after the connection to the source times out.

--clusterSetReplicationZstdCompressionLevel=<int>
            Integer that specifies the compression level to use for connections
            to the replication source server that use the zstd compression
            algorithm.

--communicationStack=<str>
            The Group Replication communication stack to be used in the
            Cluster: XCom (legacy) or MySQL.

--consistency=<str>
            String value indicating the consistency guarantees that the cluster
            provides.

--dryRun=<bool>
            Boolean if true, all validations and steps for creating a Replica
            Cluster are executed, but no changes are actually made. An
            exception will be thrown when finished.

--exitStateAction=<str>
            String value indicating the group replication exit state action.

--expelTimeout=<int>
            Integer value to define the time period in seconds that cluster
            members should wait for a non-responding member before evicting it
            from the cluster.

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for group
            replication. Only valid if communicationStack=XCOM.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one.

--manualStartOnBoot=<bool>
            Boolean (default false). If false, Group Replication in cluster
            instances will automatically start and rejoin when MySQL starts,
            otherwise it must be started manually.

--memberSslMode=<str>
            SSL mode for communication channels opened by Group Replication
            from one server to another.

--memberWeight=<int>
            Integer value with a percentage weight for automatic primary
            election on failover.

--paxosSingleLeader=<bool>
            Boolean value used to enable/disable the Group Communication engine
            to operate with a single consensus leader.

--recoveryMethod=<str>
            Preferred method for state recovery/provisioning. May be auto,
            clone or incremental. Default is auto.

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.

--replicationAllowedHost=<str>
            String value to use as the host name part of internal replication
            accounts (i.e. 'mysql_innodb_cluster_###'@'hostname'). Default is
            %. It must be possible for any member of the Cluster to connect to
            any other member using accounts with this hostname value.

--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up with
            the PRIMARY Cluster. Default is 0 and it means no timeout.

//@<OUT> CLI clusterset remove-cluster --help
NAME
      remove-cluster - Removes a Replica cluster from a ClusterSet.

SYNTAX
      clusterset remove-cluster <clusterName> [<options>]

WHERE
      clusterName: The name identifier of the Replica cluster to be removed.

RETURNS
      Nothing.

OPTIONS
--dissolve=<bool>
            Whether to dissolve the cluster after removing it from the
            ClusterSet. Set to false if you intend to use it as a standalone
            cluster. Default is true.

--dryRun=<bool>
            Boolean if true, all validations and steps for removing a
            the Cluster from the ClusterSet are executed, but no changes are
            actually made. An exception will be thrown when finished.

--force=<bool>
            Boolean, indicating if the cluster must be removed (even if
            only from metadata) in case the PRIMARY cannot be reached, or
            the ClusterSet replication channel cannot be found or is stopped.
            By default, set to false.

--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up
            with the PRIMARY Cluster. Default is 0 and it means no timeout.

//@<OUT> CLI clusterset describe --help
NAME
      describe - Describe the structure of the ClusterSet.

SYNTAX
      clusterset describe

RETURNS
      A JSON object describing the structure of the ClusterSet.

//@<OUT> CLI clusterset dissolve --help
NAME
      dissolve - Dissolves the ClusterSet.

SYNTAX
      clusterset dissolve [<options>]

RETURNS
      Nothing

OPTIONS
--dryRun=<bool>
            If true, all validations and steps for dissolving the ClusterSet
            are executed, but no changes are actually made.

--force=<bool>
            Set to true to confirm that the dissolve operation must be
            executed, even if some members of the ClusterSet cannot be reached
            or the timeout was reached when waiting for members to catch up
            with replication changes. By default, set to false.

--timeout=<int>
            Maximum number of seconds to wait for pending transactions to be
            applied in each reachable instance of the ClusterSet (default value
            is retrieved from the 'dba.gtidWaitTimeout' shell option).

//@<OUT> CLI clusterset rejoin-cluster --help
NAME
      rejoin-cluster - Rejoin an invalidated Cluster back to the ClusterSet and
                       update replication.

SYNTAX
      clusterset rejoin-cluster <clusterName> [<options>]

WHERE
      clusterName: Name of the Cluster to be rejoined.

RETURNS
      Nothing

OPTIONS
--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

//@<OUT> CLI clusterset set-primary-cluster --help
NAME
      set-primary-cluster - Performs a safe switchover of the PRIMARY Cluster
                            of the ClusterSet.

SYNTAX
      clusterset set-primary-cluster <clusterName> [<options>]

WHERE
      clusterName: Name of the REPLICA cluster to be promoted.

RETURNS
      Nothing

OPTIONS
--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

--invalidateReplicaClusters[:<type>]=<value>
            List of names of REPLICA Clusters that are unreachable or
            unavailable that are to be invalidated during the switchover.

--timeout=<int>
            Integer value to set the maximum number of seconds to wait for the
            synchronization of the Cluster and also for the instance being
            promoted to catch up with the current PRIMARY (in the case of the
            latter, the default value is retrieved from the
            'dba.gtidWaitTimeout' shell option).

//@<OUT> CLI clusterset force-primary-cluster --help
NAME
      force-primary-cluster - Performs a failover of the PRIMARY Cluster of the
                              ClusterSet.

SYNTAX
      clusterset force-primary-cluster <clusterName> [<options>]

WHERE
      clusterName: Name of the REPLICA cluster to be promoted.

RETURNS
      Nothing

OPTIONS
--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

--invalidateReplicaClusters[:<type>]=<value>
            List of names of REPLICA Clusters that are unreachable or
            unavailable that are to be invalidated during the failover.

--timeout=<uint>
            Integer value with the maximum number of seconds to wait for
            pending transactions to be applied in each instance of the cluster
            (default value is retrieved from the 'dba.gtidWaitTimeout' shell
            option).

//@<OUT> CLI clusterset status --help
NAME
      status - Describe the status of the ClusterSet.

SYNTAX
      clusterset status [<options>]

RETURNS
      A JSON object describing the status of the ClusterSet and its members.

OPTIONS
--extended=<uint>
            Verbosity level of the command output. Default is 0.

//@<OUT> CLI clusterset router-options --help
NAME
      router-options - Lists the configuration options of the ClusterSet's
                       Routers.

SYNTAX
      clusterset router-options [<options>]

RETURNS
      A JSON object with the list of Router configuration options.

OPTIONS
--extended=<uint>
            Verbosity level of the command output.

--router=<str>
            Identifier of the Router instance to be displayed.

//@<OUT> CLI clusterset execute --help
NAME
      execute - Executes a SQL statement at selected instances of the
                ClusterSet.

SYNTAX
      clusterset execute <cmd> <instances> [<options>]

WHERE
      cmd: The SQL statement to execute.
      instances: The instances where cmd should be executed.

RETURNS
      A JSON object with a list of results / information regarding the
      executing of the SQL statement on each of the target instances.

OPTIONS
--dryRun=<bool>
            Boolean if true, all validations and steps for executing cmd are
            performed, but no cmd is actually executed on any instance.

--exclude[:<type>]=<value>
            Similar to the instances parameter, it can be either a string
            (keyword) or a list of instance addresses to exclude from the
            instances specified in instances. It accepts the same keywords,
            except "all".

--timeout=<int>
            Integer value with the maximum number of seconds to wait for cmd to
            execute in each target instance. Default value is 0 meaning it
            doesn't timeout.

//@<OUT> CLI clusterset create-routing-guideline --help
NAME
      create-routing-guideline - Creates a new Routing Guideline for the
                                 ClusterSet.

SYNTAX
      clusterset create-routing-guideline <name> [<json>] [<options>]

WHERE
      name: The identifier name for the new Routing Guideline.

RETURNS
      A RoutingGuideline object representing the newly created Routing
      Guideline.

OPTIONS
--force=<bool>

//@<OUT> CLI clusterset get-routing-guideline --help
NAME
      get-routing-guideline - Returns the named Routing Guideline.

SYNTAX
      clusterset get-routing-guideline [<name>]

WHERE
      name: Name of the Guideline to be returned.

RETURNS
      The Routing Guideline object.

//@<OUT> CLI clusterset remove-routing-guideline --help
NAME
      remove-routing-guideline - Removes the named Routing Guideline.

SYNTAX
      clusterset remove-routing-guideline <name>

WHERE
      name: Name of the Guideline to be removed.

RETURNS
      Nothing.

//@<OUT> CLI clusterset routing-guidelines --help
NAME
      routing-guidelines - Lists the Routing Guidelines defined for the
                           ClusterSet.

SYNTAX
      clusterset routing-guidelines

RETURNS
      The list of Routing Guidelines of the ClusterSet.

//@<OUT> CLI clusterset import-routing-guideline --help
NAME
      import-routing-guideline - Imports a Routing Guideline from a JSON file
                                 into the ClusterSet.

SYNTAX
      clusterset import-routing-guideline <file> [<options>]

WHERE
      file: The file path to the JSON file containing the Routing Guideline.

RETURNS
      A RoutingGuideline object representing the newly imported Routing
      Guideline.

OPTIONS
--force=<bool>

--rename=<str>
