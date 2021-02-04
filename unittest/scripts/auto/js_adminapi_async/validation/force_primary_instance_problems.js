//@ INCLUDE async_utils.inc
||

//@#promoted has stopped replication, should fail
|Replication applier is OFF at instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|
|ERROR: Replication errors found for one or more SECONDARY instances. Use the 'invalidateErrorInstances' option to perform the failover anyway by skipping and invalidating instances with errors.|
||One or more instances have replication applier errors. (MYSQLSH 51145)

//@ promoted has stopped replication, still fail with invalidateErrorInstances
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> will be invalidated (replication applier is OFF) and must be fixed or removed from the replicaset|
||<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> cannot be promoted (ArgumentError)

//@# a secondary has stopped replication, should fail
|Replication applier is OFF at instance <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|
|ERROR: Replication errors found for one or more SECONDARY instances. Use the 'invalidateErrorInstances' option to perform the failover anyway by skipping and invalidating instances with errors.|
||One or more instances have replication applier errors. (MYSQLSH 51145)

//@ a secondary has stopped replication, pass with invalidateErrorInstances
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> will be invalidated (replication applier is OFF) and must be fixed or removed from the replicaset|
|NOTE: 1 instances will be skipped and invalidated during the failover|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was force-promoted to PRIMARY.|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|Failover finished successfully.|
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|        "status": "AVAILABLE_PARTIAL",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>", |
|"instanceRole": "PRIMARY", |
|"status": "ONLINE"|

//@# promoted is down (should fail)
||Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port3)>>>' ([[*]])

//@# a secondary is down (should fail and suggest invalidateErrorInstances)
|ERROR: Could not connect to one or more SECONDARY instances. Use the 'invalidateErrorInstances' option to perform the failover anyway by skipping and invalidating unreachable instances.|
||One or more instances are unreachable

//@# a different slave is more up-to-date (should fail)
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is more up-to-date than the selected instance and should be used for promotion instead.|
||Target instance is behind others

//@# but promoting sb3 should be fine
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was force-promoted to PRIMARY.|

//@# retry the same but let the primary be picked automatically
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was force-promoted to PRIMARY.|

//@# a secondary has errant GTIDs (should fail)
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is more up-to-date than the selected instance and should be used for promotion instead.|
||Target instance is behind others

//@# Replication conflict error (should fail)
|ERROR: Replication applier error at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: [[*]]|
||One or more instances have replication applier errors. (MYSQLSH 51145)

//@ Replication conflict error (pass with invalidateErrorInstances)
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> will be invalidated (replication applier error) and must be fixed or removed from the replicaset.|
|NOTE: 1 instances will be skipped and invalidated during the failover|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was force-promoted to PRIMARY.|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|Failover finished successfully.|
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|        "status": "AVAILABLE_PARTIAL",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>", |
|"instanceRole": "PRIMARY", |
|"status": "ONLINE"|
