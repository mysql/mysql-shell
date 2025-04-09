//@<OUT> clusterset.routerOptions() with all defaults
{
    "configuration": {
        "routing_rules": {
            "invalidated_cluster_policy": "drop_all",
            "read_only_targets": "secondaries",
            "tags": {},
            "target_cluster": "primary",
            "use_replica_primary_as_rw": false
        }
    },
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "configuration": {}
        },
        "routerhost2::": {
            "configuration": {}
        },
        "routerhost2::another": {
            "configuration": {}
        },
        "routerhost2::system": {
            "configuration": {}
        }
    }
}


//@<OUT> clusterset.listRouters (CLI)
{
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost1",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwSplitPort": null,
            "rwXPort": "6482",
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwSplitPort": null,
            "rwXPort": null,
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": null,
            "version": "8.0.27"
        },
        "routerhost2::another": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwSplitPort": null,
            "rwXPort": null,
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": null,
            "version": "8.0.27"
        },
        "routerhost2::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "rwPort": "mysql.sock",
            "rwSplitPort": null,
            "rwXPort": "mysqlx.sock",
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        }
    }
}

//@<OUT> clusterset.listRouters (CLI-1)
{
    "routerhost1::system": {
        "currentRoutingGuideline": null,
        "hostname": "routerhost1",
        "lastCheckIn": "2021-01-01 11:22:33",
        "localCluster": null,
        "roPort": "6481",
        "roXPort": "6483",
        "rwPort": "6480",
        "rwSplitPort": null,
        "rwXPort": "6482",
        "supportedRoutingGuidelinesVersion": null,
        "targetCluster": "cluster",
        "version": "8.0.27"
    }
}

//@<OUT> clusterset.listRouters (CLI-2)
{
    "routerhost2::system": {
        "currentRoutingGuideline": null,
        "hostname": "routerhost2",
        "lastCheckIn": "2021-01-01 11:22:33",
        "localCluster": null,
        "roPort": "mysqlro.sock",
        "roXPort": "mysqlxro.sock",
        "rwPort": "mysql.sock",
        "rwSplitPort": null,
        "rwXPort": "mysqlx.sock",
        "supportedRoutingGuidelinesVersion": null,
        "targetCluster": "replicacluster",
        "version": "8.0.27"
    }
}

//@<OUT> clusterset.listRouters() warning re-bootstrap
{
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost1",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwSplitPort": null,
            "rwXPort": "6482",
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwSplitPort": null,
            "rwXPort": null,
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        },
        "routerhost2::another": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": null,
            "roXPort": null,
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized."
            ],
            "rwPort": null,
            "rwSplitPort": null,
            "rwXPort": null,
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::system": {
            "currentRoutingGuideline": null,
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "localCluster": null,
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized."
            ],
            "rwPort": "mysql.sock",
            "rwSplitPort": null,
            "rwXPort": "mysqlx.sock",
            "supportedRoutingGuidelinesVersion": null,
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        }
    }
}
