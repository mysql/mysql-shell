//@ WL#12011: Initialization
||

//@<OUT> WL#12011: FR2-03 - no interactive option (default: interactive).
A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB Cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB Cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.

Confirm [y/N]:

//@<ERR> WL#12011: FR2-03 - no interactive option (default: interactive).
Dba.createCluster: Cancelled (RuntimeError)

//@<OUT> WL#12011: FR2-02 - interactive = false.
A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80027)?"NOTE: The 'localAddress' \"" + hostname + "\" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.\n":""\>>>
<<<(__version_num<80027)?"See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.\n":""\>>>
<<<(__version_num<80027)?"NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.\n":""\>>>
* Checking connectivity and SSL configuration...

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB Cluster 'test' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

The MySQL InnoDB Cluster is going to be setup in advanced Multi-Primary Mode. Consult its requirements and limitations in https://dev.mysql.com/doc/refman/en/group-replication-limitations.html
Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ WL#12011: Finalization.
||

//@ WL#12049: Initialization
||

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not supported on target server version: '<<<__version>>>'

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Unable to set value ':' for 'exitStateAction': Variable 'group_replication_exit_state_action' can't be set to the value of ':'
||Unable to set value 'AB' for 'exitStateAction': Variable 'group_replication_exit_state_action' can't be set to the value of 'AB'
||Unable to set value '10' for 'exitStateAction': Variable 'group_replication_exit_state_action' can't be set to the value of '10'

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
||Option 'memberWeight' not available for target server version.

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Float (TypeError)


//@ WL#11032: Create cluster specifying a valid value for memberWeight (integer) {VER(>=5.7.20)}
||

//@ WL#11032: Finalization
||

//@ WL#12067: Initialization
||

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
||Option 'consistency' not supported on target server version: '<<<__version>>>'

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
||Invalid value for 'consistency', string value cannot be empty.
||Invalid value for 'consistency', string value cannot be empty.
||Unable to set value ':' for 'consistency': Variable 'group_replication_consistency' can't be set to the value of ':'
||Unable to set value 'AB' for 'consistency': Variable 'group_replication_consistency' can't be set to the value of 'AB'
||Unable to set value '10' for 'consistency': Variable 'group_replication_consistency' can't be set to the value of '10'
||Option 'consistency' is expected to be of type String, but is Integer (TypeError)

//@ WL#12067: TSF1_1 Create cluster using a valid as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Finalization
||

//@ WL#12050: Initialization
||

//@ WL#12050: TSF1_5 Unsupported server version {VER(<8.0.13)}
||Option 'expelTimeout' not supported on target server version: '<<<__version>>>'

//@ WL#12050: TSF1_1 Create cluster using a valid value for expelTimeout {VER(>=8.0.13)}
||

//@ WL#12050: Finalization
||

//@ BUG#29361352: Initialization.
||

//@<OUT> BUG#29361352: no warning or prompt for multi-primary (multiPrimary: false).
A new InnoDB Cluster will be created on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80027)?"NOTE: The 'localAddress' \"" + hostname + "\" is [[*]], which is compatible with the Group Replication automatically generated list of IPs.\n":""\>>>
<<<(__version_num<80027)?"See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.\n":""\>>>
<<<(__version_num<80027)?"NOTE: When adding more instances to the Cluster, be aware that the subnet masks dictate whether the instance's address is automatically added to the allowlist or not. Please specify the 'ipAllowlist' accordingly if needed.\n":""\>>>
* Checking connectivity and SSL configuration...

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB Cluster 'test' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ BUG#29361352: Finalization.
||

