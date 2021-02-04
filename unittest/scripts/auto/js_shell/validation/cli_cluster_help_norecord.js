//@<OUT> CLI cluster --help
The following operations are available at 'cluster':

   add-instance
      Adds an Instance to the cluster.

   check-instance-state
      Verifies the instance gtid state in relation to the cluster.

   describe
      Describe the structure of the cluster.

   dissolve
      Dissolves the cluster.

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
      Reset the password of the recovery accounts of the cluster.

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
      Describe the status of the cluster.

   switch-to-single-primary-mode
      Switches the cluster to single-primary mode.

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

--recoveryMethod=<str>
            Preferred method of state recovery. May be auto, clone or
            incremental. Default is auto.

--label=<str>
            An identifier for the instance being added

--waitRecovery=<int>
            Integer value to indicate if the command shall wait for the
            recovery process to finish and its verbosity level.

--password=<str>
            The instance connection password

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

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
      describe - Describe the structure of the cluster.

SYNTAX
      cluster describe

RETURNS
      A JSON object describing the structure of the cluster.

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

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

//@<OUT> CLI cluster force-quorum-using-partition-of --help
NAME
      force-quorum-using-partition-of - Restores the cluster from quorum loss.

SYNTAX
      cluster force-quorum-using-partition-of <instance> [<password>]

WHERE
      instance: An instance definition to derive the forced group from.
      password: String with the password for the connection.

RETURNS
      Nothing.

//@<OUT> CLI cluster list-routers --help
NAME
      list-routers - Lists the Router instances.

SYNTAX
      cluster list-routers [<options>]

RETURNS
      A JSON object listing the Router instances associated to the cluster.

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
            replication.

--password=<str>
            The instance connection password

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

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

--password=<str>
            The instance connection password

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

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
--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

--updateTopologyMode=<bool>
            Boolean value used to indicate if the topology mode (single-primary
            or multi-primary) in the metadata should be updated (true) or not
            (false) to match the one being used by the cluster. By default, the
            metadata is not updated (false). Deprecated.

--addInstances[:<type>]=<value>
            List with the connection data of the new active instances to add to
            the metadata, or "auto" to automatically add missing instances to
            the metadata.

--removeInstances[:<type>]=<value>
            List with the connection data of the obsolete instances to remove
            from the metadata, or "auto" to automatically remove obsolete
            instances from the metadata.

//@<OUT> CLI cluster reset-recovery-accounts-password --help
NAME
      reset-recovery-accounts-password - Reset the password of the recovery
                                         accounts of the cluster.

SYNTAX
      cluster reset-recovery-accounts-password [<options>]

RETURNS
      Nothing.

OPTIONS
--force=<bool>
            Boolean, indicating if the operation will continue in case an error
            occurs when trying to reset the passwords on any of the instances,
            for example if any of them is not online. By default, set to false.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

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
      cluster set-primary-instance <instance>

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
      user: Name of the InnoDB cluster administrator account.

RETURNS
      Nothing.

OPTIONS
--dryRun=<bool>
            Boolean value used to enable a dry run of the account setup
            process. Default value is False.

--update=<bool>
            Boolean value that must be enabled to allow updating the privileges
            and/or password of existing accounts. Default value is False.

--password=<str>
            The password for the InnoDB cluster administrator account.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

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

--password=<str>
            The password for the MySQL Router account.

--interactive=<bool>
            Boolean value used to disable/enable the wizards in the command
            execution, i.e. prompts and confirmations will be provided or not
            according to the value set. The default value is equal to MySQL
            Shell wizard mode.

//@<OUT> CLI cluster status --help
NAME
      status - Describe the status of the cluster.

SYNTAX
      cluster status [<options>]

RETURNS
      A JSON object describing the status of the cluster.

OPTIONS
--extended=<uint>
            Verbosity level of the command output.

--queryMembers=<bool>
            If true, connect to each Instance of the ReplicaSets to query for
            more detailed stats about the replication machinery.

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

