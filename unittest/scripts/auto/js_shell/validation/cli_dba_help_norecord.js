//@<OUT> CLI dba --help
The following operations are available at 'dba':

   check-instance-configuration
      Validates an instance for MySQL InnoDB Cluster usage.

   configure-instance
      Validates and configures an instance for MySQL InnoDB Cluster usage.

   configure-replica-set-instance
      Validates and configures an instance for use in an InnoDB ReplicaSet.

   create-cluster
      Creates a MySQL InnoDB Cluster.

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
--applierWorkerThreads=<int>
            Number of threads used for applying replicated transactions. The
            default value is 4.

--clusterAdmin=<str>
            The name of the "cluster administrator" account.

--clusterAdminCertIssuer=<str>
            Optional SSL certificate issuer for the account.

--clusterAdminCertSubject=<str>
            Optional SSL certificate subject for the account.

--clusterAdminPassword=<str>
            The password for the "cluster administrator" account.

--clusterAdminPasswordExpiration[:<type>]=<value>
            Password expiration setting for the account. May be set to the
            number of days for expiration, 'NEVER' to disable expiration and
            'DEFAULT' to use the system default.

--mycnfPath=<str>
            The path to the MySQL configuration file of the instance.

--outputMycnfPath=<str>
            Alternative output path to write the MySQL configuration file of
            the instance.

--restart=<bool>
            Boolean value used to indicate that a remote restart of the target
            instance should be performed to finalize the operation.

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
--applierWorkerThreads=<int>
            Number of threads used for applying replicated transactions. The
            default value is 4.

--clusterAdmin=<str>
            The name of a "cluster administrator" user to be created. The
            supported format is the standard MySQL account name format.

--clusterAdminCertIssuer=<str>
            Optional SSL certificate issuer for the account.

--clusterAdminCertSubject=<str>
            Optional SSL certificate subject for the account.

--clusterAdminPassword=<str>
            The password for the "cluster administrator" account.

--clusterAdminPasswordExpiration[:<type>]=<value>
            Password expiration setting for the account. May be set to the
            number of days for expiration, 'NEVER' to disable expiration and
            'DEFAULT' to use the system default.

--restart=<bool>
            Boolean value used to indicate that a remote restart of the target
            instance should be performed to finalize the operation.

//@<OUT> CLI dba create-cluster --help
NAME
      create-cluster - Creates a MySQL InnoDB Cluster.

SYNTAX
      dba create-cluster <name> [<options>]

WHERE
      name: An identifier for the Cluster to be created.

RETURNS
      The created Cluster object.

OPTIONS
--adoptFromGR=<bool>
            Boolean value used to create the InnoDB Cluster based on existing
            replication group.

--autoRejoinTries=<int>
            Integer value to define the number of times an instance will
            attempt to rejoin the cluster after being expelled.

--certIssuer=<str>
            Common certificate issuer to use when 'memberAuthType' contains one
            of the four "CERT_*" values

--certSubject=<str>
            Instance's certificate subject to use when 'memberAuthType'
            contains "CERT_SUBJECT" or "CERT_SUBJECT_PASSWORD".

--communicationStack=<str>
            The Group Replication communication stack to be used in the
            Cluster: XCom (legacy) or MySQL.

--consistency=<str>
            String value indicating the consistency guarantees that the cluster
            provides.

--disableClone=<bool>
            Boolean value used to disable the clone usage on the Cluster.

--exitStateAction=<str>
            String value indicating the group replication exit state action.

--expelTimeout=<int>
            Integer value to define the time period in seconds that cluster
            members should wait for a non-responding member before evicting it
            from the cluster.

--force=<bool>
            Boolean, confirms that the multiPrimary option must be applied
            and/or the operation must proceed even if unmanaged replication
            channels were detected.

--groupName=<str>
            String value with the Group Replication group name UUID to be used
            instead of the automatically generated one.

--gtidSetIsComplete=<bool>
            Boolean value which indicates whether the GTID set of the seed
            instance corresponds to all transactions executed. Default is
            false.

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for group
            replication. Only valid if communicationStack=XCOM.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one.

--manualStartOnBoot=<bool>
            Boolean (default false). If false, Group Replication in Cluster
            instances will automatically start and rejoin when MySQL starts,
            otherwise it must be started manually.

--memberAuthType=<str>
            Controls the authentication type to use for the internal
            replication accounts.

--memberSslMode=<str>
            SSL mode for communication channels opened by Group Replication
            from one server to another.

--memberWeight=<int>
            Integer value with a percentage weight for automatic primary
            election on failover.

--multiPrimary=<bool>
            Boolean value used to define an InnoDB Cluster with multiple
            writable instances.

--paxosSingleLeader=<bool>
            Boolean value used to enable/disable the Group Communication engine
            to operate with a single consensus leader.

--replicationAllowedHost=<str>
            String value to use as the host name part of internal replication
            accounts (i.e. 'mysql_innodb_cluster_###'@'hostname'). Default is
            %. It must be possible for any member of the Cluster to connect to
            any other member using accounts with this hostname value.

--transactionSizeLimit=<int>
            Integer value to configure the maximum transaction size in bytes
            which the Cluster accepts

//@<OUT> CLI dba create-replica-set --help
NAME
      create-replica-set - Creates a MySQL InnoDB ReplicaSet.

SYNTAX
      dba create-replica-set <name> [<options>]

WHERE
      name: An identifier for the ReplicaSet to be created.

RETURNS
      The created ReplicaSet object.

OPTIONS
--adoptFromAR=<bool>
            Boolean value used to create the ReplicaSet based on an existing
            asynchronous replication setup.

--certIssuer=<str>
            Common certificate issuer to use when 'memberAuthType' contains one
            of the four "CERT_*" values

--certSubject=<str>
            Instance's certificate subject to use when 'memberAuthType'
            contains "CERT_SUBJECT" or "CERT_SUBJECT_PASSWORD".

--dryRun=<bool>
            Boolean if true, all validations and steps for creating a replica
            set are executed, but no changes are actually made. An exception
            will be thrown when finished.

--gtidSetIsComplete=<bool>
            Boolean value which indicates whether the GTID set of the seed
            instance corresponds to all transactions executed. Default is
            false.

--instanceLabel=<str>
            String a name to identify the target instance. Defaults to
            hostname:port

--memberAuthType=<str>
            Controls the authentication type to use for the internal
            replication accounts.

--replicationAllowedHost=<str>
            String value to use as the host name part of internal replication
            accounts (i.e. 'mysql_innodb_rs_###'@'hostname'). Default is %. It
            must be possible for any member of the ReplicaSet to connect to any
            other member using accounts with this hostname value.

--replicationSslMode=<str>
            SSL mode to use to configure the asynchronous replication channels
            of the ReplicaSet.

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
--allowRootFrom=<str>
            Create remote root account, restricted to the given address pattern
            (default: %).

--ignoreSslError=<bool>
            Ignore errors when adding SSL support for the new instance, by
            default: true.

--mysqldOptions[:<type>]=<value>
            List of MySQL configuration options to write to the my.cnf file, as
            option=value strings.

--mysqldPath=<str>
            Specify the path to the mysqld binary to be used in the sandbox
            deployment.

--password=<str>
            Password for the MySQL root user on the new instance.

--portx=<int>
            Port where the new instance will listen for X Protocol connections.

--sandboxDir=<str>
            Path where the new instance will be deployed.

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
--dryRun=<bool>
            Boolean value that if true, all validations and steps of the
            command are executed, but no changes are actually made. An
            exception will be thrown when finished.

--force=<bool>
            Boolean value to indicate that the operation must be executed even
            if some members of the Cluster cannot be reached, or the primary
            instance selected has a diverging or lower GTID_SET.

--ipAllowlist=<str>
            The list of hosts allowed to connect to the instance for Group
            Replication traffic when using the 'XCOM' communication stack.

--localAddress=<str>
            String value with the Group Replication local address to be used
            instead of the automatically generated one when using the 'XCOM'
            communication stack.

--paxosSingleLeader=<bool>
            Boolean value used to enable/disable the Group Communication engine
            to operate with a single consensus leader.

--primary=<str>
            Instance definition representing the instance that must be selected
            as the primary.

--switchCommunicationStack=<str>
            The Group Replication communication stack to be used by the Cluster
            after the reboot.

--timeout=<uint>
            Integer value with the maximum number of seconds to wait for
            pending transactions to be applied in each instance of the cluster
            (default value is retrieved from the 'dba.gtidWaitTimeout' shell
            option).

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
--password=<str>
            Password for the MySQL root user on the instance.

--sandboxDir=<str>
            Path where the instance is located.

//@<OUT> CLI dba upgrade-metadata --help
NAME
      upgrade-metadata - Upgrades (or restores) the metadata to the version
                         supported by the Shell.

SYNTAX
      dba upgrade-metadata [<options>]

OPTIONS
--dryRun=<bool>
            Boolean, if true all validations and steps to run the upgrade
            process are executed but no changes are actually made.

