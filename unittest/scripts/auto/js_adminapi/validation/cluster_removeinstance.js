//@# removeInstance() while instance is up through hostname (same as in MD and report_host)
||

//@# removeInstance() while instance is up through hostname_ip
||

//@# removeInstance() while instance is up through localhost
||

//@# removeInstance() while the instance is down - force and wrong address (should fail)
|NOTE: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port2)>>>' ([[*]])|
|ERROR: The instance localhost:<<<__mysql_sandbox_port2>>> is not reachable and does not belong to the cluster either. Please ensure the member is either connectable or remove it through the exact address as shown in the cluster status output.|
||Metadata for instance localhost:<<<__mysql_sandbox_port2>>> not found (MYSQLSH 51104)

//@# removeInstance() while the instance is down - no force and correct address (should fail)
|WARNING: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])|
|ERROR: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and cannot be safely removed from the cluster.|
|To safely remove the instance from the cluster, make sure the instance is back ONLINE and try again. If you are sure the instance is permanently unable to rejoin the group and no longer connectable, use the 'force' option to remove it from the metadata.|
||Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]]) (MySQL Error 2003)

//@# removeInstance() while the instance is down - force and correct address
|NOTE: MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])|
|NOTE: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is not reachable and it will only be removed from the metadata. Please take any necessary actions to ensure the instance will not rejoin the cluster if brought back online.|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@# removeInstance() while the instance is up but OFFLINE - no force and wrong address (should fail)
|ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
|To safely remove it from the cluster, it must be brought back ONLINE. If not possible, use the 'force' option to remove it anyway.|
||Instance is not ONLINE and cannot be safely removed (MYSQLSH 51004)

//@# removeInstance() while the instance is up but OFFLINE - no force and correct address (should fail)
|ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
|To safely remove it from the cluster, it must be brought back ONLINE. If not possible, use the 'force' option to remove it anyway.|

||Instance is not ONLINE and cannot be safely removed (MYSQLSH 51004)

//@# removeInstance() while the instance is up but OFFLINE - force and wrong address
|NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
|The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@# removeInstance() while the instance is up but OFFLINE - force and correct address
|NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@# removeInstance() - OFFLINE, no force, interactive
|ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
||Instance is not ONLINE and cannot be safely removed (MYSQLSH 51004)

//@# removeInstance() - OFFLINE, force:false, interactive
|ERROR: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
||Instance is not ONLINE and cannot be safely removed (MYSQLSH 51004)

//@# removeInstance() - OFFLINE, force:true, interactive
|NOTE: <<<hostname>>>:<<<__mysql_sandbox_port2>>> is reachable but has state OFFLINE|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@<> removeInstance() using hostname - hostname is not in MD, but uuid is
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@<> removeInstance() using localhost - localhost is not in MD, but uuid is
|The instance 'localhost:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@<> removeInstance() using hostname_ip - MD has hostname_ip (and UUID), although report_host is different
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was successfully removed from the cluster.|

//@<OUT> WL#13208: Cluster status {VER(>=8.0.17)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
}


//@<OUT> WL#13208: Cluster status 2 {VER(>=8.0.17)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
}

