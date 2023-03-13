//@<OUT> clusterset.routingOptions() with all defaults
{
    "domainName": "clusterset",
    "global": {
        "invalidated_cluster_policy": "drop_all",
        "read_only_targets": "secondaries",
        "stats_updates_frequency": 0,
        "tags": {},
        "target_cluster": "primary",
        "use_replica_primary_as_rw": false
    },
    "routers": {
        "routerhost1::system": {},
        "routerhost2::": {},
        "routerhost2::another": {},
        "routerhost2::system": {}
    }
}

//@<OUT> clusterset.listRouters
{
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": null,
            "version": "8.0.27"
        },
        "routerhost2::another": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": null,
            "version": "8.0.27"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "rwPort": "mysql.sock",
            "rwXPort": "mysqlx.sock",
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        }
    }
}
{
    "routerhost1::system": {
        "hostname": "routerhost1",
        "lastCheckIn": "2021-01-01 11:22:33",
        "roPort": "6481",
        "roXPort": "6483",
        "rwPort": "6480",
        "rwXPort": "6482",
        "targetCluster": "cluster",
        "version": "8.0.27"
    }
}
{
    "routerhost2::system": {
        "hostname": "routerhost2",
        "lastCheckIn": "2021-01-01 11:22:33",
        "roPort": "mysqlro.sock",
        "roXPort": "mysqlxro.sock",
        "rwPort": "mysql.sock",
        "rwXPort": "mysqlx.sock",
        "targetCluster": "replicacluster",
        "version": "8.0.27"
    }
}

//@<OUT> clusterset.listRouters() warning re-bootstrap
{
    "domainName": "clusterset",
    "routers": {
        "routerhost1::system": {
            "hostname": "routerhost1",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "6481",
            "roXPort": "6483",
            "rwPort": "6480",
            "rwXPort": "6482",
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        },
        "routerhost2::another": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": null,
            "roXPort": null,
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized."
            ],
            "rwPort": null,
            "rwXPort": null,
            "targetCluster": "cluster",
            "version": "8.0.27"
        },
        "routerhost2::system": {
            "hostname": "routerhost2",
            "lastCheckIn": "2021-01-01 11:22:33",
            "roPort": "mysqlro.sock",
            "roXPort": "mysqlxro.sock",
            "routerErrors": [
                "WARNING: Router must be bootstrapped again for the ClusterSet to be recognized."
            ],
            "rwPort": "mysql.sock",
            "rwXPort": "mysqlx.sock",
            "targetCluster": "replicacluster",
            "version": "8.0.27"
        }
    }
}
