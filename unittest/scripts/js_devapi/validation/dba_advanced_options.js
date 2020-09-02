//@ Initialization
||

//@ Create cluster fails because port default GR local address port is already in use. {!__replaying && !__recording}
||Dba.createCluster: The port '<<<__busy_port>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__busy_port>>>'. (RuntimeError)

//@ Create cluster errors using localAddress option
||Dba.createCluster: Group Replication failed to start
||Invalid value for localAddress. If ':' is specified then at least a non-empty host or port must be specified: '<host>:<port>' or '<host>:' or ':<port>'. (ArgumentError)
||Invalid value for localAddress, string value cannot be empty. (ArgumentError)
||Invalid port '123456' for localAddress option. The port must be an integer between 1 and 65535. (ArgumentError)

//@ Create cluster errors using localAddress option on busy port {!__replaying && !__recording}
||The port '<<<__mysql_port>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__mysql_port>>>'. (RuntimeError)

//@ Create cluster errors using groupSeeds option
||Invalid value for groupSeeds, string value cannot be empty. (ArgumentError)
||Dba.createCluster: Invalid address format: 'abc'

//@ Create cluster specifying :<valid_port> for localAddress (FR1-TS-1-2)
||

//@ Dissolve cluster (FR1-TS-1-2)
||

//@ Create cluster specifying <valid_host>: for localAddress (FR1-TS-1-3)
||

//@ Dissolve cluster (FR1-TS-1-3)
||

//@ Create cluster specifying <valid_port> for localAddress (FR1-TS-1-4)
||

//@ Dissolve cluster (FR1-TS-1-4)
||

//@ Create cluster specifying <valid_host> for localAddress (FR1-TS-1-9)
||

//@ Dissolve cluster (FR1-TS-1-9)
||

//@ Create cluster specifying <valid_host>:<valid_port> for localAddress (FR1-TS-1-10)
||

//@ Dissolve cluster (FR1-TS-1-10)
||

//@ Create cluster specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-1-1)
||

//@ Dissolve cluster (FR2-TS-1-1)
||

//@ Create cluster specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-1-2)
||

//@ Dissolve cluster (FR2-TS-1-2)
||

//@ Create cluster specifying aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa for groupName (FR3-TS-1-1)
||

//@ Dissolve cluster (FR3-TS-1-1)
||

//@ Create cluster
||

//@ Add instance errors using localAddress option
|ERROR: Unable to start Group Replication for instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. Please check the MySQL server error log for more information.|The START GROUP_REPLICATION command failed as there was an error when initializing the group communication layer.
||Invalid value for localAddress. If ':' is specified then at least a non-empty host or port must be specified: '<host>:<port>' or '<host>:' or ':<port>'. (ArgumentError)
||Invalid value for localAddress, string value cannot be empty. (ArgumentError)
||Invalid port '123456' for localAddress option. The port must be an integer between 1 and 65535. (ArgumentError)

//@ Add instance errors using localAddress option on busy port {!__replaying && !__recording}
||The port '<<<__mysql_port>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__mysql_port>>>'. (RuntimeError)

//@ Add instance errors using groupSeeds option
||Invalid value for groupSeeds, string value cannot be empty. (ArgumentError)
||Cluster.addInstance: Invalid address format: 'abc'

//@ Add instance error using groupName (not a valid option)
||Invalid options: groupName (ArgumentError)

//@ Add instance specifying :<valid_port> for localAddress (FR1-TS-2-2)
||

//@ Remove instance (FR1-TS-2-2)
||

//@ Add instance specifying <valid_host>: for localAddress (FR1-TS-2-3)
||

//@ Remove instance (FR1-TS-2-3)
||

//@ Add instance specifying <valid_port> for localAddress (FR1-TS-2-4)
||

//@ Remove instance (FR1-TS-2-4)
||

//@ Add instance specifying <valid_host> for localAddress (FR1-TS-2-9)
||

//@ Remove instance (FR1-TS-2-9)
||

//@ Add instance specifying <valid_host>:<valid_port> for localAddress (FR1-TS-2-10)
||

//@ Remove instance (FR1-TS-2-10)
||

//@ Add instance specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-2-1)
||

//@ Remove instance (FR2-TS-2-1)
||

//@ Add instance specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-2-2)
||

//@ Remove instance (FR2-TS-2-2)
||

//@ Dissolve cluster
||

//@ Create cluster with a specific localAddress, groupSeeds and groupName (FR1-TS-4)
||

//@ Add instance with a specific localAddress and groupSeeds (FR1-TS-4)
||

//@ Add a 3rd instance to ensure it will not affect the persisted group seed values specified on others (FR1-TS-4)
||

//@ Configure seed instance (FR1-TS-4)
||

//@ Configure added instance (FR1-TS-4)
||

//@ Stop seed and added instance with specific options (FR1-TS-4)
||

//@ Restart added instance (FR1-TS-4)
||

//@ Restart seed instance (FR1-TS-4)
||

//@ Dissolve cluster (FR1-TS-4)
||

//@ Finalization
||
