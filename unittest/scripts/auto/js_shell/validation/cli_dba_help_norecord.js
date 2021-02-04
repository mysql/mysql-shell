//@<OUT> CLI dba --help
The following operations are available at 'dba':

   check-instance-configuration
      Validates an instance for MySQL InnoDB Cluster usage.

   configure-instance
      Validates and configures an instance for MySQL InnoDB Cluster usage.

   configure-local-instance
      Validates and configures a local instance for MySQL InnoDB Cluster usage.

   configure-replica-set-instance
      Validates and configures an instance for use in an InnoDB ReplicaSet.

   create-cluster
      Creates a MySQL InnoDB cluster.

   create-replica-set
      Creates a MySQL InnoDB ReplicaSet.

   delete-sandbox-instance
      Deletes an existing MySQL Server instance on localhost.

   deploy-sandbox-instance
      Creates a new MySQL Server instance on localhost.

   drop-metadata-schema
      Drops the Metadata Schema.

   kill-sandbox-instance
      Kills a running MySQL Server instance on localhost.

   reboot-cluster-from-complete-outage
      Brings a cluster back ONLINE when all members are OFFLINE.

   start-sandbox-instance
      Starts an existing MySQL Server instance on localhost.

   stop-sandbox-instance
      Stops a running MySQL Server instance on localhost.

   upgrade-metadata
      Upgrades (or restores) the metadata to the version supported by the
      Shell.

//@<OUT> CLI dba check-instance-configuration --help
NAME
      check-instance-configuration - Validates an instance for MySQL InnoDB
                                     Cluster usage.

SYNTAX
      dba check-instance-configuration [<instance>] [<options>]

WHERE
      instance: An instance definition.

RETURNS
      A descriptive text of the operation result.

OPTIONS
--mycnfPath=<str>
            Optional path to the MySQL configuration file for the instance.
            Alias for verifyMyCnf

--verifyMyCnf=<str>
            Optional path to the MySQL configuration file for the instance. If
            this option is given, the configuration file will be verified for
            the expected option values, in addition to the global MySQL system
            variables.

--password=<str>
            The password to get connected to the instance.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

//@<OUT> CLI dba configure-instance --help
NAME
      configure-instance - Validates and configures an instance for MySQL
                           InnoDB Cluster usage.

SYNTAX
      dba configure-instance [<instance>] [<options>]

WHERE
      instance: An instance definition.

RETURNS
      A descriptive text of the operation result.

OPTIONS
--clusterAdmin=<str>
            The name of the "cluster administrator" account.

--clusterAdminPassword=<str>
            The password for the "cluster administrator" account.

--restart=<bool>
            Boolean value used to indicate that a remote restart of the target
            instance should be performed to finalize the operation.

--password=<str>
            The password to be used on the connection.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

--mycnfPath=<str>
            The path to the MySQL configuration file of the instance.

--outputMycnfPath=<str>
            Alternative output path to write the MySQL configuration file of
            the instance.

--clearReadOnly=<bool>
            Boolean value used to confirm that super_read_only must be
            disabled.

--applierWorkerThreads=<int>
            Number of threads used for applying replicated transactions. The
            default value is 4.

//@<OUT> CLI dba configure-local-instance --help
NAME
      configure-local-instance - Validates and configures a local instance for
                                 MySQL InnoDB Cluster usage.

SYNTAX
      dba configure-local-instance [<instance>] [<options>]

WHERE
      instance: An instance definition.

RETURNS
      Nothing

OPTIONS
--clusterAdmin=<str>
            The name of the "cluster administrator" account.

--clusterAdminPassword=<str>
            The password for the "cluster administrator" account.

--restart=<bool>
            Boolean value used to indicate that a remote restart of the target
            instance should be performed to finalize the operation.

--password=<str>
            The password to be used on the connection.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

--mycnfPath=<str>
            The path to the MySQL configuration file of the instance.

--outputMycnfPath=<str>
            Alternative output path to write the MySQL configuration file of
            the instance.

--clearReadOnly=<bool>
            Boolean value used to confirm that super_read_only must be
            disabled.

//@<OUT> CLI dba configure-replica-set-instance --help
NAME
      configure-replica-set-instance - Validates and configures an instance for
                                       use in an InnoDB ReplicaSet.

SYNTAX
      dba configure-replica-set-instance [<instance>] [<options>]

WHERE
      instance: An instance definition. By default, the active shell session is
                used.

RETURNS
      Nothing

OPTIONS
--clusterAdmin=<str>
            The name of a "cluster administrator" user to be created. The
            supported format is the standard MySQL account name format.

--clusterAdminPassword=<str>
            The password for the "cluster administrator" account.

--restart=<bool>
            Boolean value used to indicate that a remote restart of the target
            instance should be performed to finalize the operation.

--password=<str>
            The password to be used on the connection.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

--applierWorkerThreads=<int>
            Number of threads used for applying replicated transactions. The
            default value is 4.

//@<OUT> CLI dba create-cluster --help
NAME
      create-cluster - Creates a MySQL InnoDB cluster.

SYNTAX
      dba create-cluster <name> [<options>]

WHERE
      name: An identifier for the cluster to be created.

RETURNS
      The created cluster object.

OPTIONS
--memberSslMode=<str>
            SSL mode used to configure the members of the cluster.

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for group
            replication.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one.

--groupSeeds=<str>
            String value with a comma-separated list of the Group Replication
            peer addresses to be used instead of the automatically generated
            one.

--exitStateAction=<str>
            String value indicating the group replication exit state action.

--memberWeight=<int>
            Integer value with a percentage weight for automatic primary
            election on failover.

--autoRejoinTries=<int>
            Integer value to define the number of times an instance will
            attempt to rejoin the cluster after being expelled.

--groupName=<str>
            String value with the Group Replication group name UUID to be used
            instead of the automatically generated one.

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

--disableClone=<bool>
            Boolean value used to disable the clone usage on the cluster.

--gtidSetIsComplete=<bool>
            Boolean value which indicates whether the GTID set of the seed
            instance corresponds to all transactions executed. Default is
            false.

--multiPrimary=<bool>
            Boolean value used to define an InnoDB cluster with multiple
            writable instances.

--adoptFromGR=<bool>
            Boolean value used to create the InnoDB cluster based on existing
            replication group.

--clearReadOnly=<bool>
            Boolean value used to confirm that super_read_only must be
            disabled. Deprecated.

--force=<bool>
            Boolean, confirms that the multiPrimary option must be applied
            and/or the operation must proceed even if unmanaged replication
            channels were detected.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

//@<OUT> CLI dba create-replica-set --help
NAME
      create-replica-set - Creates a MySQL InnoDB ReplicaSet.

SYNTAX
      dba create-replica-set <name> [<options>]

WHERE
      name: An identifier for the replicaset to be created.

RETURNS
      The created replicaset object.

OPTIONS
--adoptFromAR=<bool>
            Boolean value used to create the replicaset based on an existing
            asynchronous replication setup.

--dryRun=<bool>
            Boolean if true, all validations and steps for creating a replica
            set are executed, but no changes are actually made. An exception
            will be thrown when finished.

--instanceLabel=<str>
            String a name to identify the target instance. Defaults to
            hostname:port

--gtidSetIsComplete=<bool>
            Boolean value which indicates whether the GTID set of the seed
            instance corresponds to all transactions executed. Default is
            false.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

//@<OUT> CLI dba delete-sandbox-instance --help
NAME
      delete-sandbox-instance - Deletes an existing MySQL Server instance on
                                localhost.

SYNTAX
      dba delete-sandbox-instance <port> [<options>]

WHERE
      port: The port of the instance to be deleted.

RETURNS
      Nothing.

OPTIONS
--sandboxDir=<str>
            Path where the instance is located.

//@<OUT> CLI dba drop-metadata-schema --help
NAME
      drop-metadata-schema - Drops the Metadata Schema.

SYNTAX
      dba drop-metadata-schema [<options>]

RETURNS
      Nothing.

OPTIONS
--force=<bool>
            Boolean, confirms that the drop operation must be executed.

--clearReadOnly=<bool>
            Boolean value used to confirm that super_read_only must be disabled

//@<OUT> CLI dba deploy-sandbox-instance --help
NAME
      deploy-sandbox-instance - Creates a new MySQL Server instance on
                                localhost.

SYNTAX
      dba deploy-sandbox-instance <port> [<options>]

WHERE
      port: The port where the new instance will listen for connections.

RETURNS
      Nothing.

OPTIONS
--sandboxDir=<str>
            Path where the new instance will be deployed.

--password=<str>
            Password for the MySQL root user on the new instance.

--portx=<int>
            Port where the new instance will listen for X Protocol connections.

--allowRootFrom=<str>
            Create remote root account, restricted to the given address pattern
            (default: %).

--ignoreSslError=<bool>
            Ignore errors when adding SSL support for the new instance, by
            default: true.

--mysqldOptions[:<type>]=<value>
            List of MySQL configuration options to write to the my.cnf file, as
            option=value strings.

//@<OUT> CLI dba kill-sandbox-instance --help
NAME
      kill-sandbox-instance - Kills a running MySQL Server instance on
                              localhost.

SYNTAX
      dba kill-sandbox-instance <port> [<options>]

WHERE
      port: The port of the instance to be killed.

RETURNS
      Nothing.

OPTIONS
--sandboxDir=<str>
            Path where the instance is located.

//@<OUT> CLI dba reboot-cluster-from-complete-outage --help
NAME
      reboot-cluster-from-complete-outage - Brings a cluster back ONLINE when
                                            all members are OFFLINE.

SYNTAX
      dba reboot-cluster-from-complete-outage [<clusterName>] [<options>]

WHERE
      clusterName: The name of the cluster to be rebooted.

RETURNS
      The rebooted cluster object.

OPTIONS
--removeInstances=<str list>
            The list of instances to be removed from the cluster.

--rejoinInstances=<str list>
            The list of instances to be rejoined to the cluster.

--clearReadOnly=<bool>
            Boolean value used to confirm that super_read_only must be disabled

--user=<str>
            The user used for the instances sessions required operations.

--password=<str>
            The password used for the instances sessions required operations.

//@<OUT> CLI dba start-sandbox-instance --help
NAME
      start-sandbox-instance - Starts an existing MySQL Server instance on
                               localhost.

SYNTAX
      dba start-sandbox-instance <port> [<options>]

WHERE
      port: The port where the instance listens for MySQL connections.

RETURNS
      Nothing.

OPTIONS
--sandboxDir=<str>
            Path where the instance is located.

//@<OUT> CLI dba stop-sandbox-instance --help
NAME
      stop-sandbox-instance - Stops a running MySQL Server instance on
                              localhost.

SYNTAX
      dba stop-sandbox-instance <port> [<options>]

WHERE
      port: The port of the instance to be stopped.

RETURNS
      Nothing.

OPTIONS
--sandboxDir=<str>
            Path where the instance is located.

--password=<str>
            Password for the MySQL root user on the instance.

//@<OUT> CLI dba upgrade-metadata --help
NAME
      upgrade-metadata - Upgrades (or restores) the metadata to the version
                         supported by the Shell.

SYNTAX
      dba upgrade-metadata [<options>]

OPTIONS
--dryRun=<bool>
            Boolean value used to enable a dry run of the upgrade process.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

