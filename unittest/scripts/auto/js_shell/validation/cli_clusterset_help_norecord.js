//@<OUT> CLI clusterset --help
The following operations are available at 'clusterset':

   create-replica-cluster
      Creates a new InnoDB Cluster that is a Replica of the Primary Cluster.

   describe
      Describe the structure of the ClusterSet.

   dissolve
      Dissolves the ClusterSet.
      
   execute
      Executes a SQL statement at selected instances of the ClusterSet.

   force-primary-cluster
      Performs a failover of the PRIMARY Cluster of the ClusterSet.

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

   router-options
      Lists the configuration options of the ClusterSet's Routers.

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
--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up with
            the PRIMARY Cluster. Default is 0 and it means no timeout.

--dryRun=<bool>
            Boolean if true, all validations and steps for creating a Replica
            Cluster are executed, but no changes are actually made. An
            exception will be thrown when finished.

--recoveryProgress=<int>
            Integer value to indicate the recovery process verbosity level.

--memberSslMode=<str>
            SSL mode for communication channels opened by Group Replication
            from one server to another.

--replicationAllowedHost=<str>
            String value to use as the host name part of internal replication
            accounts (i.e. 'mysql_innodb_cluster_###'@'hostname'). Default is
            %. It must be possible for any member of the Cluster to connect to
            any other member using accounts with this hostname value.

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for group
            replication. Only valid if communicationStack=XCOM.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one.

--exitStateAction=<str>
            String value indicating the group replication exit state action.

--memberWeight=<int>
            Integer value with a percentage weight for automatic primary
            election on failover.

--manualStartOnBoot=<bool>
            Boolean (default false). If false, Group Replication in cluster
            instances will automatically start and rejoin when MySQL starts,
            otherwise it must be started manually.

--consistency=<str>
            String value indicating the consistency guarantees that the cluster
            provides.

--expelTimeout=<int>
            Integer value to define the time period in seconds that cluster
            members should wait for a non-responding member before evicting it
            from the cluster.

--autoRejoinTries=<int>
            Integer value to define the number of times an instance will
            attempt to rejoin the cluster after being expelled.

--communicationStack=<str>
            The Group Replication communication stack to be used in the
            Cluster: XCom (legacy) or MySQL.

--paxosSingleLeader=<bool>
            Boolean value used to enable/disable the Group Communication engine
            to operate with a single consensus leader.

--recoveryMethod=<str>
            Preferred method for state recovery/provisioning. May be auto,
            clone or incremental. Default is auto.

--cloneDonor=<str>
            Host:port of an existing member of the PRIMARY cluster to clone
            from. IPv6 addresses are not supported for this option.

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
--timeout=<int>
            Maximum number of seconds to wait for the instance to sync up
            with the PRIMARY Cluster. Default is 0 and it means no timeout.

--dryRun=<bool>
            Boolean if true, all validations and steps for removing a
            the Cluster from the ClusterSet are executed, but no changes are
            actually made. An exception will be thrown when finished.

--force=<bool>
            Boolean, indicating if the cluster must be removed (even if
            only from metadata) in case the PRIMARY cannot be reached, or
            the ClusterSet replication channel cannot be found or is stopped.
            By default, set to false.

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
--timeout=<int>
            Maximum number of seconds to wait for pending transactions to be
            applied in each reachable instance of the ClusterSet (default value
            is retrieved from the 'dba.gtidWaitTimeout' shell option).

--dryRun=<bool>
            If true, all validations and steps for dissolving the ClusterSet
            are executed, but no changes are actually made.

--force=<bool>
            Set to true to confirm that the dissolve operation must be
            executed, even if some members of the ClusterSet cannot be reached
            or the timeout was reached when waiting for members to catch up
            with replication changes. By default, set to false.

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
--invalidateReplicaClusters[:<type>]=<value>
            List of names of REPLICA Clusters that are unreachable or
            unavailable that are to be invalidated during the switchover.

--timeout=<int>
            Integer value to set the maximum number of seconds to wait for the
            synchronization of the Cluster and also for the instance being
            promoted to catch up with the current PRIMARY (in the case of the
            latter, the default value is retrieved from the
            'dba.gtidWaitTimeout' shell option).

--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

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
--invalidateReplicaClusters[:<type>]=<value>
            List of names of REPLICA Clusters that are unreachable or
            unavailable that are to be invalidated during the failover.

--dryRun=<bool>
            If true, will perform checks and log operations that would be
            performed, but will not execute them. The operations that would be
            performed can be viewed by enabling verbose output in the shell.

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
--router=<str>
            Identifier of the Router instance to be displayed.

--extended=<uint>
            Verbosity level of the command output.
