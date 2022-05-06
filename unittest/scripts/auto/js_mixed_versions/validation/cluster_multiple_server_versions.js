//@# get cluster status
|{|
|    "clusterName": "testCluster", |
|    "defaultReplicaSet": {|
|        "clusterErrors": [|
|            "Group communication protocol in use is version 5.7.14 but it is possible to upgrade to 8.0.27. Message fragmentation for large transactions and Single Consensus Leader can only be enabled after upgrade. Use Cluster.rescan({upgradeCommProtocol:true}) to upgrade."|
|        ],|
|        "name": "default", |
|        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>", |
|        "ssl": "REQUIRED", |
|        "status": "OK_NO_TOLERANCE", |
|        "topology": {|
|            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|                "memberRole": "PRIMARY", |
|                "status": "ONLINE",|
|            }|
|        },|
|    }, |
|}|

//@<OUT> extended status should have the protocol version
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "GRProtocolVersion": "5.7.14",
        "clusterErrors": [
            "Group communication protocol in use is version 5.7.14 but it is possible to upgrade to 8.0.27. Message fragmentation for large transactions and Single Consensus Leader can only be enabled after upgrade. Use Cluster.rescan({upgradeCommProtocol:true}) to upgrade."
        ],

//@<OUT> upgraded status
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "GRProtocolVersion": "8.0.27",
