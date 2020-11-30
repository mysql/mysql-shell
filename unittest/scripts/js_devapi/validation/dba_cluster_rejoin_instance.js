//@ Initialization
||

//@ Connect to instance 1
||

//@ create cluster
||

//@ Adding instance 2 using the root account
||

//@ Adding instance 3
||

//@ ipWhitelist deprecation error {VER(>=8.0.22)}
||Cluster.rejoinInstance: Cannot use the ipWhitelist and ipAllowlist options simultaneously. The ipWhitelist option is deprecated, please use the ipAllowlist option instead. (ArgumentError)

//@<OUT> Rejoin instance 2
WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.

//@<OUT> Cannot rejoin an instance that is already in the group (not missing) Bug#26870329
NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is already an active (ONLINE) member of cluster 'dev'.

//@ Dissolve cluster
||

//@ Finalization
||
