//@ IPv6 local_address of cluster members not supported by target instance {VER(>= 8.0.14) && DEF(MYSQLD8013_PATH)}
|ERROR: Cannot join instance 'localhost:<<<__mysql_sandbox_port3>>>' to cluster: unsupported localAddress value on the cluster.|

//@ IPv6 local_address of target instance not supported by cluster members {VER(>= 8.0.14) && DEF(MYSQLD57_PATH)}
|ERROR: Cannot join instance 'localhost:<<<__mysql_sandbox_port2>>>' to cluster: localAddress value not supported by the cluster.|
