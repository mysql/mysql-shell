//@# rebootCluster on an auto-rejoining instance {VER(>=8.0.0)}
|Cancelling active GR auto-initialization at 127.0.0.1:<<<__mysql_sandbox_port1>>>|

//@# status while the target is autorejoining (should pass)
|WARNING: Error connecting to Cluster: MYSQLSH 51004: Unable to connect to the primary member of the Cluster: 'Group PRIMARY is not ONLINE'|
|Retrying getCluster() using a secondary member|

