#@ __global__
||

#@<OUT> cluster
NAME
      Cluster - Represents an InnoDB cluster.

DESCRIPTION
      The cluster object is the entry point to manage and monitor a MySQL
      InnoDB cluster.

      A cluster is a set of MySQLd Instances which holds the user's data.

      It provides high-availability and scalability for the user's data.

PROPERTIES
      name
            Retrieves the name of the cluster.

FUNCTIONS
      add_instance(instance[, options])
            Adds an Instance to the cluster.

      check_instance_state(instance)
            Verifies the instance gtid state in relation to the cluster.

      describe()
            Describe the structure of the cluster.

      disconnect()
            Disconnects all internal sessions used by the cluster object.

      dissolve([options])
            Dissolves the cluster.

      force_quorum_using_partition_of(instance[, password])
            Restores the cluster from quorum loss.

      get_name()
            Retrieves the name of the cluster.

      help([member])
            Provides help about this class and it's members

      options([options])
            Lists the cluster configuration options.

      rejoin_instance(instance[, options])
            Rejoins an Instance to the cluster.

      remove_instance(instance[, options])
            Removes an Instance from the cluster.

      rescan([options])
            Rescans the cluster.

      set_instance_option(instance, option, value)
            Changes the value of a configuration option in a Cluster member.

      set_option(option, value)
            Changes the value of a configuration option for the whole cluster.

      set_primary_instance(instance)
            Elects a specific cluster member as the new primary.

      status([options])
            Describe the status of the cluster.

      switch_to_multi_primary_mode()
            Switches the cluster to multi-primary mode.

      switch_to_single_primary_mode([instance])
            Switches the cluster to single-primary mode.

      For more help on a specific function use: cluster.help('<functionName>')

      e.g. cluster.help('addInstance')

#@<OUT> cluster.add_instance
NAME
      add_instance - Adds an Instance to the cluster.

SYNTAX
      <Cluster>.add_instance(instance[, options])

WHERE
      instance: Connection options for the target instance to be added.
      options: Dictionary with options for the operation.

RETURNS
       nothing

DESCRIPTION
      This function adds an Instance to the default replica set of the cluster.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The options dictionary may contain the following attributes:

      - label: an identifier for the instance being added
      - password: the instance connection password
      - memberSslMode: SSL mode used on the instance
      - ipWhitelist: The list of hosts allowed to connect to the instance for
        group replication
      - localAddress: string value with the Group Replication local address to
        be used instead of the automatically generated one.
      - groupSeeds: string value with a comma-separated list of the Group
        Replication peer addresses to be used instead of the automatically
        generated one.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.

      The password may be contained on the instance definition, however, it can
      be overwritten if it is specified on the options.

      ATTENTION: The memberSslMode option will be removed in a future release.

      The memberSslMode option supports the following values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instance to
        communicate with other members of the cluster
      - DISABLED: if used, SSL (encryption) will be disabled
      - AUTO: if used, SSL (encryption) will be automatically enabled or
        disabled based on the cluster configuration

      If memberSslMode is not specified AUTO will be used by default.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally.

      If exitStateAction is not specified READ_ONLY will be used by default.

      The ipWhitelist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the whitelist.

      The localAddress and groupSeeds are advanced options and their usage is
      discouraged since incorrect values can lead to Group Replication errors.

      The value for localAddress is used to set the Group Replication system
      variable 'group_replication_local_address'. The localAddress option
      accepts values in the format: 'host:port' or 'host:' or ':port'. If the
      specified value does not include a colon (:) and it is numeric, then it
      is assumed to be the port, otherwise it is considered to be the host.
      When the host is not specified, the default value is the value of the
      system variable 'report_host' if defined (i.e., not 'NULL'), otherwise it
      is the hostname value. When the port is not specified, the default value
      is the port of the target instance * 10 + 1. In case the automatically
      determined default port value is invalid (> 65535) then a random value in
      the range [10000, 65535] is used.

      The value for groupSeeds is used to set the Group Replication system
      variable 'group_replication_group_seeds'. The groupSeeds option accepts a
      comma-separated list of addresses in the format:
      'host1:port1,...,hostN:portN'.

      The value for exitStateAction is used to configure how Group Replication
      behaves when a server instance leaves the group unintentionally, for
      example after encountering an applier error. When set to ABORT_SERVER,
      the instance shuts itself down, and when set to READ_ONLY the server
      switches itself to super-read-only mode. The exitStateAction option
      accepts case-insensitive string values, being the accepted values:
      ABORT_SERVER (or 1) and READ_ONLY (or 0). The default value is READ_ONLY.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.
      Group Replication uses a default value of 50 if no value is provided.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition is invalid.
      - If the instance definition is a connection dictionary but empty.
      - If the value for the memberSslMode option is not one of the allowed:
        "AUTO", "DISABLED", "REQUIRED".
      - If the value for the ipWhitelist, localAddress, groupSeeds, or
        exitStateAction options is empty.
      - If the instance definition cannot be used for Group Replication.

      RuntimeError in the following scenarios:

      - If the instance accounts are invalid.
      - If the instance is not in bootstrapped state.
      - If the SSL mode specified is not compatible with the one used in the
        cluster.
      - If the value for the localAddress, groupSeeds, exitStateAction, or
        memberWeight options is not valid for Group Replication.

#@<OUT> cluster.check_instance_state
NAME
      check_instance_state - Verifies the instance gtid state in relation to
                             the cluster.

SYNTAX
      <Cluster>.check_instance_state(instance)

WHERE
      instance: An instance definition.

RETURNS
       resultset A JSON object with the status.

DESCRIPTION
      Analyzes the instance executed GTIDs with the executed/purged GTIDs on
      the cluster to determine if the instance is valid for the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The returned JSON object contains the following attributes:

      - state: the state of the instance
      - reason: the reason for the state reported

      The state of the instance can be one of the following:

      - ok: if the instance transaction state is valid for the cluster
      - error: if the instance transaction state is not valid for the cluster

      The reason for the state reported can be one of the following:

      - new: if the instance doesn't have any transactions
      - recoverable:  if the instance executed GTIDs are not conflicting with
        the executed GTIDs of the cluster instances
      - diverged: if the instance executed GTIDs diverged with the executed
        GTIDs of the cluster instances
      - lost_transactions: if the instance has more executed GTIDs than the
        executed GTIDs of the cluster instances

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the 'instance' parameter is empty.
      - If the 'instance' parameter is invalid.
      - If the 'instance' definition is a connection dictionary but empty.

      RuntimeError in the following scenarios:

      - If the 'instance' is unreachable/offline.
      - If the 'instance' is a cluster member.
      - If the 'instance' belongs to a Group Replication group that is not
        managed as an InnoDB cluster.
      - If the 'instance' is a standalone instance but is part of a different
        InnoDB Cluster.
      - If the 'instance' has an unknown state.

#@<OUT> cluster.describe
NAME
      describe - Describe the structure of the cluster.

SYNTAX
      <Cluster>.describe()

RETURNS
       A JSON object describing the structure of the cluster.

DESCRIPTION
      This function describes the structure of the cluster including all its
      information, ReplicaSets and Instances.

      The returned JSON object contains the following attributes:

      - clusterName: the cluster name
      - defaultReplicaSet: the default ReplicaSet object

      The defaultReplicaSet JSON object contains the following attributes:

      - name: the ReplicaSet name
      - topology: a list of dictionaries describing each instance belonging to
        the ReplicaSet.
      - topologyMode: the InnoDB Cluster topology mode.

      Each instance dictionary contains the following attributes:

      - address: the instance address in the form of host:port
      - label: the instance name identifier
      - role: the instance role
      - version: the instance version (only available for instances >= 8.0.11)

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.

      RuntimeError in the following scenarios:

      - If the InnoDB Cluster topology mode does not match the current Group
        Replication configuration.
      - If the InnoDB Cluster name is not registered in the Metadata.

#@<OUT> cluster.disconnect
NAME
      disconnect - Disconnects all internal sessions used by the cluster
                   object.

SYNTAX
      <Cluster>.disconnect()

RETURNS
       Nothing.

DESCRIPTION
      Disconnects the internal MySQL sessions used by the cluster to query for
      metadata and replication information.

#@<OUT> cluster.dissolve
NAME
      dissolve - Dissolves the cluster.

SYNTAX
      <Cluster>.dissolve([options])

WHERE
      options: Parameter to specify if it should deactivate replication and
               unregister the ReplicaSets from the cluster.

RETURNS
       Nothing.

DESCRIPTION
      This function disables replication on the ReplicaSets, unregisters them
      and the the cluster from the metadata.

      It keeps all the user's data intact.

      The options dictionary may contain the following attributes:

      - force: boolean value used to confirm that the dissolve operation must
        be executed, even if some members of the cluster cannot be reached or
        the timeout was reached when waiting for members to catch up with
        replication changes. By default, set to false.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      The force option (set to true) must only be used to dissolve a cluster
      with instances that are permanently not available (no longer reachable)
      or never to be reused again in a cluster. This allows to dissolve a
      cluster and remove it from the metadata, including instances than can no
      longer be recovered. Otherwise, the instances must be brought back ONLINE
      and the cluster dissolved without the force option to avoid errors trying
      to reuse the instances and add them back to a cluster.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

#@<OUT> cluster.force_quorum_using_partition_of
NAME
      force_quorum_using_partition_of - Restores the cluster from quorum loss.

SYNTAX
      <Cluster>.force_quorum_using_partition_of(instance[, password])

WHERE
      instance: An instance definition to derive the forced group from.
      password: String with the password for the connection.

RETURNS
       Nothing.

DESCRIPTION
      This function restores the cluster's default replicaset back into
      operational status from a loss of quorum scenario. Such a scenario can
      occur if a group is partitioned or more crashes than tolerable occur.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      Note that this operation is DANGEROUS as it can create a split-brain if
      incorrectly used and should be considered a last resort. Make absolutely
      sure that there are no partitions of this group that are still operating
      somewhere in the network, but not accessible from your location.

      When this function is used, all the members that are ONLINE from the
      point of view of the given instance definition will be added to the
      group.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition cannot be used for Group Replication.

      RuntimeError in the following scenarios:

      - If the instance does not exist on the Metadata.
      - If the instance is not on the ONLINE state.
      - If the instance does is not an active member of a replication group.
      - If there are no ONLINE instances visible from the given one.

      LogicError in the following scenarios:

      - If the cluster does not exist.

#@<OUT> cluster.get_name
NAME
      get_name - Retrieves the name of the cluster.

SYNTAX
      <Cluster>.get_name()

RETURNS
       The name of the cluster.

#@<OUT> cluster.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Cluster>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> cluster.options
NAME
      options - Lists the cluster configuration options.

SYNTAX
      <Cluster>.options([options])

WHERE
      options: Dictionary with options.

RETURNS
       A JSON object describing the configuration options of the cluster.

DESCRIPTION
      This function lists the cluster configuration options for its ReplicaSets
      and Instances. The following options may be given to controlthe amount of
      information gathered and returned.

      - all: if true, includes information about all group_replication system
        variables.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.

#@<OUT> cluster.name
NAME
      name - Retrieves the name of the cluster.

SYNTAX
      <Cluster>.name

#@<OUT> cluster.rejoin_instance
NAME
      rejoin_instance - Rejoins an Instance to the cluster.

SYNTAX
      <Cluster>.rejoin_instance(instance[, options])

WHERE
      instance: An instance definition.
      options: Dictionary with options for the operation.

RETURNS
       Nothing.

DESCRIPTION
      This function rejoins an Instance to the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The options dictionary may contain the following attributes:

      - label: an identifier for the instance being added
      - password: the instance connection password
      - memberSslMode: SSL mode used on the instance
      - ipWhitelist: The list of hosts allowed to connect to the instance for
        group replication

      The password may be contained on the instance definition, however, it can
      be overwritten if it is specified on the options.

      ATTENTION: The memberSslMode option will be removed in a future release.

      The memberSslMode option supports these values:

      - REQUIRED: if used, SSL (encryption) will be enabled for the instance to
        communicate with other members of the cluster
      - DISABLED: if used, SSL (encryption) will be disabled
      - AUTO: if used, SSL (encryption) will be automatically enabled or
        disabled based on the cluster configuration

      If memberSslMode is not specified AUTO will be used by default.

      The ipWhitelist format is a comma separated list of IP addresses or
      subnet CIDR notation, for example: 192.168.1.0/24,10.0.0.1. By default
      the value is set to AUTOMATIC, allowing addresses from the instance
      private network to be automatically set for the whitelist.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

      ArgumentError in the following scenarios:

      - If the value for the memberSslMode option is not one of the allowed:
        "AUTO", "DISABLED", "REQUIRED".
      - If the instance definition cannot be used for Group Replication.

      RuntimeError in the following scenarios:

      - If the instance does not exist.
      - If the instance accounts are invalid.
      - If the instance is not in bootstrapped state.
      - If the SSL mode specified is not compatible with the one used in the
        cluster.
      - If the instance is an active member of the ReplicaSet.

#@<OUT> cluster.remove_instance
NAME
      remove_instance - Removes an Instance from the cluster.

SYNTAX
      <Cluster>.remove_instance(instance[, options])

WHERE
      instance: An instance definition.
      options: Dictionary with options for the operation.

RETURNS
       Nothing.

DESCRIPTION
      This function removes an Instance from the default replicaSet of the
      cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The options dictionary may contain the following attributes:

      - password: the instance connection password
      - force: boolean, indicating if the instance must be removed (even if
        only from metadata) in case it cannot be reached. By default, set to
        false.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.

      The password may be contained in the instance definition, however, it can
      be overwritten if it is specified on the options.

      The force option (set to true) must only be used to remove instances that
      are permanently not available (no longer reachable) or never to be reused
      again in a cluster. This allows to remove from the metadata an instance
      than can no longer be recovered. Otherwise, the instance must be brought
      back ONLINE and removed without the force option to avoid errors trying
      to add it back to a cluster.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition is invalid.
      - If the instance definition is a connection dictionary but empty.
      - If the instance definition cannot be used for Group Replication.

      RuntimeError in the following scenarios:

      - If the instance accounts are invalid.
      - If an error occurs when trying to remove the instance (e.g., instance
        is not reachable).

#@<OUT> cluster.set_instance_option
NAME
      set_instance_option - Changes the value of a configuration option in a
                            Cluster member.

SYNTAX
      <Cluster>.set_instance_option(instance, option, value)

WHERE
      instance: An instance definition.
      option: The configuration option to be changed.
      value: The value that the configuration option shall get.

RETURNS
       Nothing.

DESCRIPTION
      This function changes an InnoDB Cluster configuration option in a member
      of the cluster.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The option parameter is the name of the configuration option to be
      changed

      The value parameter is the value that the configuration option shall get.

      The accepted values for the configuration option are:

      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - label a string identifier of the instance.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally.

      If exitStateAction is not specified READ_ONLY will be used by default.

      The value for exitStateAction is used to configure how Group Replication
      behaves when a server instance leaves the group unintentionally, for
      example after encountering an applier error. When set to ABORT_SERVER,
      the instance shuts itself down, and when set to READ_ONLY the server
      switches itself to super-read-only mode. The exitStateAction option
      accepts case-insensitive string values, being the accepted values:
      ABORT_SERVER (or 1) and READ_ONLY (or 0).

      The default value is READ_ONLY.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the 'instance' parameter is empty.
      - If the 'instance' parameter is invalid.
      - If the 'instance' definition is a connection dictionary but empty.
      - If the 'option' parameter is empty.
      - If the 'value' parameter is empty.
      - If the 'option' parameter is invalid.

      RuntimeError in the following scenarios:

      - If 'instance' does not refer to a cluster member.
      - If the cluster has no visible quorum.
      - If 'instance' is not ONLINE.
      - If 'instance' does not support the configuration option passed in
        'option'.
      - If the value passed in 'option' is not valid for Group Replication.

#@<OUT> cluster.set_option
NAME
      set_option - Changes the value of a configuration option for the whole
                   cluster.

SYNTAX
      <Cluster>.set_option(option, value)

WHERE
      option: The configuration option to be changed.
      value: The value that the configuration option shall get.

RETURNS
       Nothing.

DESCRIPTION
      This function changes an InnoDB Cluster configuration option in all
      members of the cluster.

      The 'option' parameter is the name of the configuration option to be
      changed.

      The value parameter is the value that the configuration option shall get.

      The accepted values for the configuration option are:

      - clusterName: string value to define the cluster name.
      - exitStateAction: string value indicating the group replication exit
        state action.
      - memberWeight: integer value with a percentage weight for automatic
        primary election on failover.
      - failoverConsistency: string value indicating the consistency guarantees
        for primary failover in single primary mode.
      - expelTimeout: integer value to define the time period in seconds that
        cluster members should wait for a non-responding member before evicting
        it from the cluster.

      The value for the configuration option is used to set the Group
      Replication system variable that corresponds to it.

      The exitStateAction option supports the following values:

      - ABORT_SERVER: if used, the instance shuts itself down if it leaves the
        cluster unintentionally.
      - READ_ONLY: if used, the instance switches itself to super-read-only
        mode if it leaves the cluster unintentionally.

      If exitStateAction is not specified READ_ONLY will be used by default.

      The failoverConsistency option supports the following values:

      - BEFORE_ON_PRIMARY_FAILOVER: if used, new queries (read or write) to the
        new primary will be put on hold until after the backlog from the old
        primary is applied.
      - EVENTUAL: if used, read queries to the new primary are allowed even if
        the backlog isn't applied.

      If failoverConsistency is not specified, EVENTUAL will be used by
      default.

      The value for exitStateAction is used to configure how Group Replication
      behaves when a server instance leaves the group unintentionally, for
      example after encountering an applier error. When set to ABORT_SERVER,
      the instance shuts itself down, and when set to READ_ONLY the server
      switches itself to super-read-only mode. The exitStateAction option
      accepts case-insensitive string values, being the accepted values:
      ABORT_SERVER (or 1) and READ_ONLY (or 0).

      The default value is READ_ONLY.

      The value for memberWeight is used to set the Group Replication system
      variable 'group_replication_member_weight'. The memberWeight option
      accepts integer values. Group Replication limits the value range from 0
      to 100, automatically adjusting it if a lower/bigger value is provided.

      Group Replication uses a default value of 50 if no value is provided.

      The value for failoverConsistency is used to set the Group Replication
      system variable 'group_replication_failover_consistency' and configure
      how Group Replication behaves when a new primary instance is elected.
      When set to BEFORE_ON_PRIMARY_FAILOVER, new queries (read or write) to
      the newly elected primary that is applying backlog from the old primary,
      will be hold be hold before execution until the backlog is applied. When
      set to EVENTUAL, read queries to the new primary are allowed even if the
      backlog isn't applied but writes will fail (if the backlog isn't applied)
      due to super-read-only mode being enabled. The client may return old
      valued. The failoverConsistency option accepts case-insensitive string
      values, being the accepted values: BEFORE_ON_PRIMARY_FAILOVER (or 1) and
      EVENTUAL (or 0).

      The default value is EVENTUAL.

      The value for expelTimeout is used to set the Group Replication system
      variable 'group_replication_member_expel_timeout' and configure how long
      Group Replication will wait before expelling from the group any members
      suspected of having failed. On slow networks, or when there are expected
      machine slowdowns, increase the value of this option. The expelTimeout
      option accepts positive integer values in the range [0, 3600].

      The default value is 0.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the 'option' parameter is empty.
      - If the 'value' parameter is empty.
      - If the 'option' parameter is invalid.

      RuntimeError in the following scenarios:

      - If any of the cluster members do not support the configuration option
        passed in 'option'.
      - If the value passed in 'option' is not valid for Group Replication.
      - If the cluster has no visible quorum.
      - If any of the cluster members is not ONLINE.

#@<OUT> cluster.rescan
NAME
      rescan - Rescans the cluster.

SYNTAX
      <Cluster>.rescan([options])

WHERE
      options: Dictionary with options for the operation.

RETURNS
       Nothing.

DESCRIPTION
      This function rescans the cluster for new and obsolete Group Replication
      members/instances, as well as changes in the used topology mode (i.e.,
      single-primary and multi-primary).

      The options dictionary may contain the following attributes:

      - addInstances: List with the connection data of the new active instances
        to add to the metadata, or "auto" to automatically add missing
        instances to the metadata.
      - interactive: boolean value used to disable/enable the wizards in the
        command execution, i.e. prompts and confirmations will be provided or
        not according to the value set. The default value is equal to MySQL
        Shell wizard mode.
      - removeInstances: List with the connection data of the obsolete
        instances to remove from the metadata, or "auto" to automatically
        remove obsolete instances from the metadata.
      - updateTopologyMode: boolean value used to indicate if the topology mode
        (single-primary or multi-primary) in the metadata should be updated
        (true) or not (false) to match the one being used by the cluster. By
        default, the metadata is not updated (false).

      The value for addInstances and removeInstances is used to specify which
      instances to add or remove from the metadata, respectively. Both options
      accept list connection data. In addition, the "auto" value can be used
      for both options in order to automatically add or remove the instances in
      the metadata, without having to explicitly specify them.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the value for `addInstances` or `removeInstance` is empty.
      - If the value for `addInstances` or `removeInstance` is invalid.

      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.
      - If the Metadata update operation failed.

      LogicError in the following scenarios:

      - If the cluster does not exist.

      RuntimeError in the following scenarios:

      - If all the ReplicaSet instances of any ReplicaSet are offline.
      - If an instance specified for `addInstances` is not an active member of
        the replication group.
      - If an instance specified for `removeInstances` is an active member of
        the replication group.

#@<OUT> cluster.status
NAME
      status - Describe the status of the cluster.

SYNTAX
      <Cluster>.status([options])

WHERE
      options: Dictionary with options.

RETURNS
       A JSON object describing the status of the cluster.

DESCRIPTION
      This function describes the status of the cluster including its
      ReplicaSets and Instances. The following options may be given to control
      the amount of information gathered and returned.

      - extended: if true, includes information about transactions processed by
        connection and applier, as well as groupName and memberId values.
      - queryMembers: if true, connect to each Instance of the ReplicaSets to
        query for more detailed stats about the replication machinery.

EXCEPTIONS
      MetadataError in the following scenarios:

      - If the Metadata is inaccessible.

      RuntimeError in the following scenarios:

      - If the InnoDB Cluster topology mode does not match the current Group
        Replication configuration.
      - If the InnoDB Cluster name is not registered in the Metadata.

#@<OUT> cluster.set_primary_instance
NAME
      set_primary_instance - Elects a specific cluster member as the new
                             primary.

SYNTAX
      <Cluster>.set_primary_instance(instance)

WHERE
      instance: An instance definition.

RETURNS
       Nothing.

DESCRIPTION
      This function forces the election of a new primary, overriding any
      election process.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The instance definition is mandatory and is the identifier of the cluster
      member that shall become the new primary.

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition is invalid.

      RuntimeError in the following scenarios:

      - If the cluster is in multi-primary mode.
      - If 'instance' does not refer to a cluster member.
      - If any of the cluster members has a version < 8.0.13.
      - If the cluster has no visible quorum.
      - If any of the cluster members is not ONLINE.

#@<OUT> cluster.switch_to_multi_primary_mode
NAME
      switch_to_multi_primary_mode - Switches the cluster to multi-primary
                                     mode.

SYNTAX
      <Cluster>.switch_to_multi_primary_mode()

RETURNS
       Nothing.

DESCRIPTION
      This function changes a cluster running in single-primary mode to
      multi-primary mode.

EXCEPTIONS
      RuntimeError in the following scenarios:

      - If any of the cluster members has a version < 8.0.13.
      - If the cluster has no visible quorum.
      - If any of the cluster members is not ONLINE.

#@<OUT> cluster.switch_to_single_primary_mode
NAME
      switch_to_single_primary_mode - Switches the cluster to single-primary
                                      mode.

SYNTAX
      <Cluster>.switch_to_single_primary_mode([instance])

WHERE
      instance: An instance definition.

RETURNS
       Nothing.

DESCRIPTION
      This function changes a cluster running in multi-primary mode to
      single-primary mode.

      The instance definition is the connection data for the instance.

      For additional information on connection data use \? connection.

      Only TCP/IP connections are allowed for this function.

      The instance definition is optional and is the identifier of the cluster
      member that shall become the new primary.

      If the instance definition is not provided, the new primary will be the
      instance with the highest member weight (and the lowest UUID in case of a
      tie on member weight).

EXCEPTIONS
      ArgumentError in the following scenarios:

      - If the instance parameter is empty.
      - If the instance definition is invalid.

      RuntimeError in the following scenarios:

      - If 'instance' does not refer to a cluster member.
      - If any of the cluster members has a version < 8.0.13.
      - If the cluster has no visible quorum.
      - If any of the cluster members is not ONLINE.

