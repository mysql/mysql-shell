//@# INCLUDE async_utils.inc
||

//@<OUT> Check the status of replicaset B.
{
    "replicaSet": {
        "name": "B",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Check the status of replicaset A.
{
    "replicaSet": {
        "name": "A",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}
