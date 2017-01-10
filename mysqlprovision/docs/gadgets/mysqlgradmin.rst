***************************************************
*mysql-gr-admin* - Group Replication Administration
***************************************************
.. index:: mysqlgradmin

This section describe the architecture of the *mysql-gr-admin* gadget that
implements the needed procedures to administrate MySQL Group Replication.

..
    Configuring ``mysqld`` instances
    ================================

    Before adding a server to a group managed using group replication, it
    is necessary to ensure that it is properly configured.

    Group replication is relying on normal replication for propagating
    changes between the servers and it only support row-based replication,
    so it is necessary to enable the binary log and turn on row-based
    replication.

    It is also necessary to have a transactional engine since group
    replication do not work correctly with non-transactional engines.

    It therefore is necessary to ensure that the following settings are in
    the configuration file and the server is restarted::

        log-bin
        binlog-format=row
        gtid-mode=on
        enforce-gtid-consistency
        log-slave-updates
        default-storage-engine=innodb
        master-info-repository=table
        relay-log-info-repository=table
        transaction-write-set-extraction=murmur32
        binlog-checksum=none
        plugin-load=group_replication.so

    The configuration of servers might be done different ways. Some use
    Puppet, others Chef, but it is assumed to be performed by the user and not
    by this gadget.


    Bootstrapping a group
    =====================

    Before there are any servers in a group it is necessary to add a
    single server to the group. This is called *bootstrapping* the
    group. A group containing a single server is not highly available, but
    it is a stepping stone to the real group.

    To get a group that is highly available in the presence of network
    partitions it is necessary to have an odd number of servers, which
    means that the minimum for a highly available group using group
    replication is three servers.

    As an intermediary step a second server need to be added to the single
    server group constructed when bootstrapping the group. This
    configuration is not resistant to network partitions and should
    therefore only be considered an intermediary step.

