//@ Initialization
||

//@ Create cluster errors using localAddress option
||ERROR: Error starting cluster: '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'
||Invalid value for localAddress. If ':' is specified then at least a non-empty host or port must be specified: '<host>:<port>' or '<host>:' or ':<port>'. (ArgumentError)
||Invalid value for localAddress, string value cannot be empty. (ArgumentError)
||Invalid port '123456' for localAddress option. The port must be an integer between 1 and 65535. (RuntimeError)
||The port '<<<__mysql_sandbox_port1>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__mysql_sandbox_port1>>>'. (RuntimeError)

//@ Create cluster errors using groupSeeds option
||Invalid value for groupSeeds, string value cannot be empty. (ArgumentError)
||ERROR: Error starting cluster: '<<<localhost>>>:<<<__mysql_sandbox_port1>>>'

//@ Create cluster errors using groupName option
||Invalid value for groupName, string value cannot be empty. (ArgumentError)
||ERROR: Error starting cluster: Invalid value 'abc' for groupName, it must be a valid UUID. (RuntimeError)

//@ Create cluster specifying :<valid_port> for localAddress (FR1-TS-1-2)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-1-2)
<<<__result_local_address_2>>>

//@ Dissolve cluster (FR1-TS-1-2)
||

//@ Create cluster specifying <valid_host>: for localAddress (FR1-TS-1-3)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-1-3)
<<<__result_local_address_3>>>

//@ Dissolve cluster (FR1-TS-1-3)
||

//@ Create cluster specifying <valid_port> for localAddress (FR1-TS-1-4)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-1-4)
<<<__result_local_address_4>>>

//@ Dissolve cluster (FR1-TS-1-4)
||

//@ Create cluster specifying <valid_host> for localAddress (FR1-TS-1-9)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-1-9)
<<<__result_local_address_9>>>

//@ Dissolve cluster (FR1-TS-1-9)
||

//@ Create cluster specifying <valid_host>:<valid_port> for localAddress (FR1-TS-1-10)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-1-10)
<<<__result_local_address_10>>>

//@ Dissolve cluster (FR1-TS-1-10)
||

//@ Create cluster specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-1-1)
||

//@<OUT> Confirm group seeds is set correctly (FR2-TS-1-1)
<<<__result_group_seeds_1>>>

//@ Dissolve cluster (FR2-TS-1-1)
||

//@ Create cluster specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-1-2)
||

//@<OUT> Confirm group seeds is set correctly (FR2-TS-1-2)
<<<__result_group_seeds_2>>>

//@ Dissolve cluster (FR2-TS-1-2)
||

//@ Create cluster
||

//@ Create cluster specifying aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa for groupName (FR3-TS-1-1)
||

//@<OUT> Confirm group name is set correctly (FR3-TS-1-1)
<<<__result_group_name_1>>>

//@ Dissolve cluster (FR3-TS-1-1)
||

//@ Create cluster
||

//@ Add instance errors using localAddress option
||ERROR: Error joining instance to cluster: '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'
||Invalid value for localAddress. If ':' is specified then at least a non-empty host or port must be specified: '<host>:<port>' or '<host>:' or ':<port>'. (ArgumentError)
||Invalid value for localAddress, string value cannot be empty. (ArgumentError)
||Invalid port '123456' for localAddress option. The port must be an integer between 1 and 65535. (RuntimeError)
||The port '<<<__mysql_sandbox_port1>>>' for localAddress option is already in use. Specify an available port to be used with localAddress option or free port '<<<__mysql_sandbox_port1>>>'. (RuntimeError)

//@ Add instance errors using groupSeeds option
||Invalid value for groupSeeds, string value cannot be empty. (ArgumentError)
||ERROR: Error joining instance to cluster: '<<<localhost>>>:<<<__mysql_sandbox_port2>>>'

//@ Add instance error using groupName (not a valid option)
||Invalid values in  options: groupName (ArgumentError)

//@ Add instance specifying :<valid_port> for localAddress (FR1-TS-2-2)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-2-2)
<<<__result_local_address_add_2>>>

//@ Remove instance (FR1-TS-2-2)
||

//@ Add instance specifying <valid_host>: for localAddress (FR1-TS-2-3)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-2-3)
<<<__result_local_address_add_3>>>

//@ Remove instance (FR1-TS-2-3)
||

//@ Add instance specifying <valid_port> for localAddress (FR1-TS-2-4)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-2-4)
<<<__result_local_address_add_4>>>

//@ Remove instance (FR1-TS-2-4)
||

//@ Add instance specifying <valid_host> for localAddress (FR1-TS-2-9)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-2-9)
<<<__result_local_address_add_9>>>

//@ Remove instance (FR1-TS-2-9)
||

//@ Add instance specifying <valid_host>:<valid_port> for localAddress (FR1-TS-2-10)
||

//@<OUT> Confirm local address is set correctly (FR1-TS-2-10)
<<<__result_local_address_add_10>>>

//@ Remove instance (FR1-TS-2-10)
||

//@ Add instance specifying 127.0.0.1:<valid_port> for groupSeeds (FR2-TS-2-1)
||

//@<OUT> Confirm group seeds is set correctly (FR2-TS-2-1)
<<<__result_group_seeds_1>>>

//@ Remove instance (FR2-TS-2-1)
||

//@ Add instance specifying 127.0.0.1:<valid_port>,127.0.0.1:<valid_port2> for groupSeeds (FR2-TS-2-2)
||

//@<OUT> Confirm group seeds is set correctly (FR2-TS-2-2)
<<<__result_group_seeds_2>>>

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

//@<OUT> Confirm localAddress, groupSeeds, and groupName values were persisted for added instance (FR1-TS-4)
<<<__cfg_local_address2>>>
<<<__cfg_group_seeds>>>
<<<__cfg_group_name>>>

//@ Restart seed instance (FR1-TS-4)
||

//@<OUT> Confirm localAddress, groupSeeds, and groupName values were persisted for seed instance (FR1-TS-4)
<<<__cfg_local_address1>>>
<<<__cfg_group_seeds>>>
<<<__cfg_group_name>>>

//@ Dissolve cluster (FR1-TS-4)
||

//@ Finalization
||
